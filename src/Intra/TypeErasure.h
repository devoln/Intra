#pragma once

#include <Intra/Concepts.h>
#include <Intra/Container/Compound.h>
#include <Intra/Container/Ownership.h>
#include <Intra/Numeric/Traits.h>
#include <Intra/Range.h>
#include <Intra/Platform/Error.h>

//// <Common interfaces>
namespace Intra { INTRA_BEGIN
// Adds a virtual destructor to IParent interface, may be in the middle of an inheritance chain for interfaces usable without a virtual destructor.
template<class IParent> INTRA_INTERFACE IGenericDestructible
{
	constexpr virtual ~IGenericDestructible() = default;
};

// Usually used first in interface inheritance chains.
using IDestructible = IGenericDestructible<TEmpty>;

template<class IParent> INTRA_INTERFACE IGenericCloneable: IParent
{
	// Copy the object. If optStorage is null allocate the copy via new, otherwise construct in place if optStorageMaxSize is large enough.
	constexpr virtual IGenericCloneable* Clone(char* optStorage = nullptr, size_t optStorageSize = 0) const = 0;
};
using ICloneable = IGenericCloneable<IDestructible>;

template<typename TFor, class TParent> class INTRA_NOVTABLE ImplCloneable: public TParent
{
public:
	constexpr TFor* Clone(char* optStorage, size_t optStorageSize) const final
	{
		if(!optStorage) return new TFor(static_cast<const TFor&>(*this));
		if(sizeof(TFor) < optStorageSize) return nullptr;
		new(Construct, optStorage) TFor(static_cast<const TFor&>(*this));
	}
};
} INTRA_END

//// <Callable interfaces>
namespace Intra { INTRA_BEGIN
template<typename FuncSignature, class IParent> INTRA_INTERFACE IGenericCallable;
template<class IParent, typename R, typename... Args> INTRA_INTERFACE IGenericCallable<R(Args...), IParent>: IParent
{
	constexpr virtual R operator()(Args... args) = 0;
};

// The most basic callback. Unlike ICallable it has no virtual destructor.
// Must only be used without passing ownership, so doesn't support smart pointers.
template<typename FuncSignature> using ICallback = IGenericCallable<FuncSignature, TEmpty>;

// Basic callback with virtual destructor that supports passing ownership and smart pointers.
template<typename FuncSignature> using ICallable = IGenericDestructible<ICallback<FuncSignature>>;

template<typename FuncSignature, class IParent> INTRA_INTERFACE IGenericConstCallable;
template<class IParent, typename R, typename... Args> INTRA_INTERFACE IGenericConstCallable<R(Args...), IParent>: public IParent
{
public:
	constexpr virtual R operator()(Args... args) const = 0;
};
template<typename FuncSignature, class IParent = IDestructible>
using IConstCallable = IGenericConstCallable<FuncSignature, ICallable<FuncSignature>>; // IConstCallable* is convertible to ICallable* 

template<CFunction FuncSignature, class T, class TParent> class GenericFunctor;
template<class TParent, typename R, typename... Args, CCallable<Args...> T>
class GenericFunctor<R(Args...), T, TParent>: public TParent
{
public:
	INTRA_NO_UNIQUE_ADDRESS T Obj;

	template<typename... Ts> constexpr GenericFunctor(Ts&&... constructArgs):
		Obj(INTRA_FWD(constructArgs)...) {}
	constexpr R operator()(Args... args) const {return static_cast<R>(Obj(INTRA_FWD(args)...));}
	constexpr R operator()(Args... args) {return static_cast<R>(Obj(INTRA_FWD(args)...));}
};

template<CFunction FuncSignature, class T> class Functor final: public GenericFunctor<FuncSignature, T,
	ICallable<FuncSignature>
> {using GenericFunctor::GenericFunctor;};
template<CFunction FuncSignature, class T> class ConstFunctor final: public GenericFunctor<FuncSignature, T,
	IConstCallable<FuncSignature, ICallable<FuncSignature>>
> {using GenericFunctor::GenericFunctor;};
template<CFunction FuncSignature, class T> class CopyableFunctor final: public GenericFunctor<FuncSignature, T,
	ImplCloneable<CopyableFunctor<FuncSignature, T>, ICallable<FuncSignature>>
> {using GenericFunctor::GenericFunctor;};
template<CFunction FuncSignature, class T> class CopyableConstFunctor final: public GenericFunctor<FuncSignature, T,
	ImplCloneable<CopyableConstFunctor<FuncSignature, T>, IConstCallable<FuncSignature>>
> {using GenericFunctor::GenericFunctor;};

template<CCallable TGetFunc> class GenericHolder
{
	INTRA_NO_UNIQUE_ADDRESS TGetFunc mGetter;
	using TResult = decltype(*mGetter());
	static_assert(CReference<TResult>);
public:
	template<typename... Args> explicit GenericHolder(Args&&... args): mGetter(INTRA_FWD(args)...) {}

	template<typename... Args> requires CCallable<TResult, Args...>
	decltype(auto) operator()(Args&&... args) {return (*mGetter())(INTRA_FWD(args)...);}
};
template<class TGetFunc> GenericHolder(TGetFunc&&) -> GenericHolder<TUnqualRef<TGetFunc>>;

constexpr auto SmartPtrGetFunc = [](auto ptr) {
	return [ptr = INTRA_MOVE(ptr)] {return ptr.get();};
};

template<class Ptr> class GenericPointerHolder:
	public GenericHolder<decltype(SmartPtrGetFunc(Ptr()))>
{
public:
	explicit GenericPointerHolder(Ptr ptr): GenericHolder(SmartPtrGetFunc(INTRA_MOVE(ptr))) {}
};
template<typename T> INTRA_CLASS_ALIAS(PtrHolder, GenericPointerHolder<MaybeUnique<T>>);
template<typename T> INTRA_CLASS_ALIAS(SharedHolder, GenericPointerHolder<Shared<T>>);

// TODO: сделать Mixin'ы через CRTP для обёртки над operator(), как сделано в Mixin'ах для range с методом me()
} INTRA_END


//// <Allocators>
namespace Intra { INTRA_BEGIN
using IAllocator = ICallback<TRawAllocatorSignature>;
using IDestructibleAllocator = IGenericDestructible<IAllocator>;

template<CAllocator A> class PolymorphicAllocator: public IAllocator
{
public:
	INTRA_NO_UNIQUE_ADDRESS A Allocator;

	PolymorphicAllocator() = default;
	template<typename... Args> explicit PolymorphicAllocator(Args&&... args): Allocator(INTRA_FWD(args)...) {}
	[[nodiscard]] constexpr RawAllocResult Reallocate(RawAllocParams params) final {return Allocator(params);}
};
template<class A> PolymorphicAllocator(A&&) -> PolymorphicAllocator<TUnqualRef<A>>;

template<CAllocator A> [[nodiscard]] INTRA_FORCEINLINE constexpr auto ToPolymorphic(A&& allocator)
{
	return Unique(new PolymorphicAllocator(INTRA_FWD(allocator)));
}
} INTRA_END

//// <Readers>
namespace Intra { INTRA_BEGIN

class EventQueue;

// Basic reader with virtual destructor that supports passing ownership and smart pointers.
// Can be wrapped into an input range.
template<typename T, class IParent = IDestructible> INTRA_INTERFACE IGenericReader: IParent
{
	// Read up to Length(dst) elements from the reader putting them into the beginning of dst.
	// May read fewer bytes than requested and reading more data may require retries (particularly, when reading from pipes and sockets).
	// @return The number of bytes read from the reader and written to dst (in range [0; Length(dst)]).
	//  0 means that either dst was empty or the end of stream has been reached.
	//  On error returns an error code. Caller may try again if the error is ErrorCode::TryAgain.
	//  It means that data is not available at the moment and may be available later for two possible reasons:
	//  1. The reader is non-blocking (pipe, socket or user input).
	//  2. The reader wraps a partial reader that may read fewer bytes than requested and
	//   the bytes read were not enough to finish the frame inside underlying reader.
	constexpr virtual LResult<size_t> ReadSome(Span<T> dst) = 0;
};
using IReader = IGenericReader<char>;

template<typename T, class IParent = IDestructible> INTRA_INTERFACE IGenericReaderEx: IGenericReader<T, IParent>
{
	// If supported, works similarly to ReadSome but doesn't advance the position of the reader.
	constexpr virtual LResult<size_t> PeekSome(Span<T> dst) {return ErrorCode::NotImplemented;}

	// Works the same as ReadSome but handles ErrorCode::TryAgain by scheduling
	//  a notification callback into an EventQueue to run when it's time to retry reading.
	// NOTE: make sure that passed callback is alive until the notification arrives.
	// @return Same as ReadSome.
	constexpr virtual LResult<size_t> ReadSomeOrNotify(Span<T> dst, EventQueue&, ICallback<void()>&)
	{
		auto res = ReadSome(dst);
		if(res.Error() == ErrorCode::TryAgain)
			return ErrorCode::NotImplemented;
		return res;
	}

	// Seek to the specified position. Works like lseek.
	// @return New position relative to start or ErrorCode::NotImplemented.
	constexpr virtual LResult<uint64> Seek(SeekParams params) {return ErrorCode::NotImplemented;}

	// If the reader has an internal buffer, this method allows to consume it directly.
	//  Also loads the buffer if it is currently empty.
	// @return The reference to the buffer span to consume if supported.
	//  Popping elements from the returned span advances the reader position.
	constexpr virtual Optional<Span<T>&> ConsumableBuffer() {return {};}

	// @return Number of the elements left until reaching end of stream.
	constexpr LengthInfo NumElementsLeft() const
	{
		const auto total = NumTotalElements();
		if(!total.IsKnown()) return LengthInfo::Unknown;
		const auto pos = Seek({.Origin = SeekOrigin::Current, .DiscardEffect = true}).Or(0);
		return total - pos;
	}

	// @return Total number of the elements in the reader or a special LengthInfo value.
	constexpr LengthInfo NumTotalElements() const
	{
		return Seek({.Origin = SeekOrigin::End, .DiscardEffect = true}).Or(LengthInfo::Unknown);
	}
};
using IReaderEx = IGenericReaderEx<char>;

// Reader that also allows cloning.
// Can be wrapped into a forward range.
template<typename T> using IForwardReader = IGenericCloneable<IGenericReaderEx<T>>;


template<bool ImplementSeekable, CRange R, class TParent = IGenericReaderEx<TRangeValue<R>>>
class INTRA_EMPTY_BASES RangeToReader: public TParent, public R
{
	static_assert(!ImplementSeekable || CForwardRange<R>);
	using T = TRangeValue<R>;
	struct SeekSupportStruct
	{
		INTRA_NO_UNIQUE_ADDRESS R StartRange;
		uint64 Position;
	};
	INTRA_NO_UNIQUE_ADDRESS TSelect<SeekSupportStruct, TEmpty, ImplementSeekable> mSeek{};
public:
	explicit RangeToReader(CList auto&& list): R(RangeOf(INTRA_FWD(list))) {
		if constexpr(ImplementSeekable)
			mSeek.StartRange = Range();
	}

	INTRA_FORCEINLINE constexpr R& Range() {return *this;}

	constexpr LResult<size_t> ReadSome(Span<T> dst) {return CopyTo(dst).ConsumeFrom(Range());}
	constexpr LResult<size_t> PeekSome(Span<T> dst)
	{
		if constexpr(CCopyConstructible<R>) return CopyTo(dst).ConsumeFrom(Dup(Range()));
		else return ErrorCode::RequestNotSupported;
	}
	constexpr LResult<size_t> ReadSomeBackwards(Span<T> dst) {return CopyTo(dst).ConsumeFrom(RRetro<R&>{Range()});}
	constexpr LResult<size_t> PeekSomeBackwards(Span<T> dst)
	{
		if constexpr(CCopyConstructible<R>) return CopyTo(dst).ConsumeFrom(RRetro<R&>{Dup(Range())});
		else return ErrorCode::RequestNotSupported;
	}

	constexpr virtual LResult<uint64> Seek(SeekParams params)
	{
		if constexpr(ImplementSeekable)
		{
			int64 desiredPos = params.Offset;
			if(params.Origin == SeekOrigin::End)
				if constexpr(CHasLength<R>)
					desiredPos += mSeek.StartRange.Length();
				else return ErrorCode::NotImplemented;
			else if(params.Origin == SeekOrigin::Current)
			{
				if(params.Offset >= 0)
				{
					auto offset = (params.DiscardEffect? Dup(Range()): Range())|PopFirstCount(params.Offset);
					if(params.DiscardEffect) return mSeek.Position + offset;
					return mSeek.Position += offset;
				}
				else desiredPos += mSeek.Position;
			}
			auto&& rangeToAdvance = params.DiscardEffect? Dup(mSeek.StartRange): (Range() = mSeek.StartRange);
			return rangeToAdvance|PopFirstCount(desiredPos);
		}
		else return ErrorCode::NotImplemented;
	}

	// TODO:
	// constexpr Optional<Span<T>&> ConsumableBuffer() {}
};

} INTRA_END
