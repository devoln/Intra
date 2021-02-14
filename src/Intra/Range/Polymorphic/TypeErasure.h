#pragma once

#include "Intra/Core.h"
#include "Intra/Range.h"

namespace Intra { INTRA_BEGIN

template<size_t MaxSize, typename TItem = char> struct LocalStorage
{
	LocalStorage() = default;
	template<typename... Args> constexpr LocalStorage(Args&&... args): mBuf{TBufItem{INTRA_FWD(args)...}} {}
	template<typename... Args> constexpr LocalStorage(TConstructT<TBufItem>, Args&&... args): mBuf{TBufItem{INTRA_FWD(args)...}} {}

	template<CDerived<TItem> T, typename... Args> requires CHasVirtualDestructor<TItem>
	constexpr LocalStorage(TConstructT<T>, Args&&... args) {new(Construct, mBuf) T(INTRA_FWD(args)...);}

	~LocalStorage() = default;
	INTRA_CONSTEXPR_DESTRUCTOR ~LocalStorage() requires CHasVirtualDestructor<TItem>
	{
		reinterpret_cast<TItem*>(mBuf)->~TItem();
	}

	constexpr TItem* GetPointer(Size count = 1)
	{
		INTRA_PRECONDITION(count*sizeof(T) <= sizeof(mBuf));
		if constexpr(CSameUnqual<TBufItem, TItem>) return mBuf;
		else return reinterpret_cast<TItem*>(mBuf);
	}
	template<typename T> constexpr const TItem* GetPointer() const
	{
		if constexpr(CSameUnqual<TBufItem, TItem>) return mBuf;
		else return reinterpret_cast<const TItem*>(mBuf);
	}

	constexpr index_t Capacity() const {return sizeof(mBuf)/sizeof(TItem);}

private:
	using TBufItem = TSelect<char, TItem, CHasVirtualDestructor<TItem>>;
	TBufItem mBuf[MaxSize/sizeof(TBufItem)]{};
};

template<class A, typename TItem = char> struct DynamicStorage
{
	template<typename T> constexpr T* Get(TType<T>)
	{
		static_assert(sizeof(T) <= MaxSizeInBytes);
		if constexpr(CSameUnqual<T, TItem>) return Buf;
		else return reinterpret_cast<T*>(Buf);
	}
	template<typename T> constexpr const T* Get(TType<T>) const
	{
		static_assert(sizeof(T) <= MaxSizeInBytes);
		if constexpr(CSameUnqual<T, TItem>) return Buf;
		else return reinterpret_cast<const T*>(Buf);
	}

	constexpr index_t Capacity() const {return 1;}

private:
	[[no_unique_address]] A Allocator;
};

template<class A, typename TItem = char> struct DynamicArrayStorage
{
	template<typename T> constexpr T* Get(TType<T>, Size count)
	{
		if constexpr(CSameUnqual<T, TItem>) return Buf;
		else return reinterpret_cast<T*>(Buf);
	}
	template<typename T> constexpr const T* Get(TType<T>, Size count) const
	{
		if constexpr(CSameUnqual<T, TItem>) return Buf;
		else return reinterpret_cast<const T*>(Buf);
	}

	constexpr index_t Capacity() const {return sizeof(Buf)/sizeof(TItem);}

private:
	[[no_unique_address]] A Allocator;
};

template<size_t MaxSizeInBytes, class A> struct SboStorage
{
	LocalStorage<MaxSizeInBytes> Local;
	[[no_unique_address]] DynamicStorage<A> Dynamic;
};

namespace z_D {
template<typename T> class IOutput: public IDestructible
{
public:
	void Put(const T& value)
	{
		const bool wasNotFull = TryPut(value);
		INTRA_PRECONDITION(wasNotFull);
	}

	bool TryPut(const T& value) {return !!doWrite(&value, 1);}

	template<CConsumableList L> index_t PutAll(L&& src)
	{
		if constexpr(CArrayList<L> && CSame<TArrayListValue<L>, T>)
			return index_t(Write(DataOf(src), LengthOf(src)));
		else
		{
			enum: size_t {BufLen = (128 + sizeof(T) - 1) / sizeof(T)};
			return INTRA_FWD(src)|Buffered(Array<T, BufLen>())|WriteTo(*this);
		}
	}

protected:
	// Output interfaces are implemented internally in terms of streams to minimize virtual call overhead for bulk operations.
	constexpr virtual size_t doWrite(T* src, size_t maxElements) = 0;
};

template<typename T, class ParentInterface> struct IRange: ParentInterface
{
	static_assert(!CAbstractClass<T>);
	static_assert(CConstructible<T> && CCopyAssignable<T> || CReference<T>, "Not all cases are implemented yet.");

	using TP = TSelect<TRemoveReference<T>*, T, CReference<T>>;

	// Range interfaces are implemented internally in terms of streams to minimize virtual call overhead for bulk operations.
	// Perf tests: http://quick-bench.com/d4PM7BSJJvzCaIt_mJCjHYa9jNc; Compiler explorer: https://godbolt.org/z/60Pej2
	constexpr virtual size_t Read(TP* dst, size_t maxElements) = 0;
	constexpr virtual bool TryFirst(TP* dst) const = 0;
	constexpr virtual size_t PopFirstCount(size_t maxElementsToPop) = 0;
};

template<typename T, class ParentInterface> struct IBidirectionalRange: IRange<T, ParentInterface>
{
	using TP = TSelect<TRemoveReference<T>*, T, CReference<T>>;

	constexpr virtual size_t ReadLastBackwards(TP* dst, size_t maxElements) = 0;
	constexpr virtual bool TryLast(TP* dst) const = 0;
	constexpr virtual size_t PopLastCount(size_t maxElementsToPop) = 0;
};

template<typename T, class ParentInterface> struct IFiniteRandomAccessRange: IBidirectionalRange<T, ParentInterface>
{
	constexpr virtual T operator[](size_t index) = 0;
	constexpr virtual size_t Length() const = 0;
};

template<typename T, class ParentInterface> struct IInfiniteRandomAccessRange: IRange<T, ParentInterface>
{
	constexpr virtual T operator[](size_t index) = 0;
};

template<typename T, typename R, typename PARENT> struct ImplRange: PARENT
{
	R mRange;

	template<CList L> ImplRange(L&& list): mRange(RangeOf(INTRA_FWD(list))) {}

	using TP = TSelect<TRemoveReference<T>*, T, CReference<T>>;
	INTRA_CONSTEXPR_VIRTUAL size_t Read(TP* dst, size_t maxElements) final
	{
		auto copy = CopyTo(Span<TP>(Unsafe, dst, maxElements));
		if constexpr(CReference<T>) return size_t(mRange|Map(AddressOf)|copy);
		else return size_t(mRange|copy);
	}

	INTRA_CONSTEXPR_VIRTUAL bool TryFirst(TP* dst) const final
	{
		if(mRange.Empty()) return false;
		if constexpr(CReference<T>) *dst = &mRange.First();
		else *dst = mRange.First();
		return true;
	}

	INTRA_CONSTEXPR_VIRTUAL size_t PopFirstCount(size_t maxElementsToPop) final
	{
		return size_t(mRange|Intra::PopFirstCount(maxElementsToPop));
	}
};

template<typename T, typename R, typename PARENT> struct ImplBidirectionalRange: ImplRange<T, R, PARENT>
{
	using Base = ImplRange<T, R, PARENT>;
	using Base::Base;

	using TP = typename Base::TP;
	INTRA_CONSTEXPR_VIRTUAL size_t ReadLastBackwards(TP* dst, size_t maxElements) final
	{
		auto copy = CopyTo(Span<TP>(Unsafe, dst, maxElements));
		if constexpr(CReference<T>) return size_t(Advance(Base::mRange)|Retro|Map(AddressOf)|copy);
		else return size_t(Advance(Base::mRange)|Retro|copy);
	}

	INTRA_CONSTEXPR_VIRTUAL bool TryLast(TP* dst) const final
	{
		if(Base::mRange.Empty()) return false;
		if constexpr(CReference<T>) *dst = &Base::mRange.Last();
		else *dst = Base::mRange.Last();
		return true;
	}

	INTRA_CONSTEXPR_VIRTUAL size_t PopLastCount(size_t maxElementsToPop) final
	{
		return size_t(Base::mRange|Intra::PopLastCount(maxElementsToPop));
	}
};

template<typename T, typename R, typename PARENT> struct ImplFiniteRandomAccessRange: ImplBidirectionalRange<T, R, PARENT>
{
	using Base = ImplBidirectionalRange<T, R, PARENT>;
	using Base::Base;

	INTRA_CONSTEXPR_VIRTUAL T operator[](size_t index) final
	{
		return Base::mRange[index];
	}

	INTRA_CONSTEXPR_VIRTUAL size_t Length() const final
	{
		return Base::mRange|Count;
	}
};

template<typename T, typename R, typename PARENT> struct ImplInfiniteRandomAccessRange: ImplRange<T, R, PARENT>
{
	using Base = ImplRange<T, R, PARENT>;
	using Base::Base;

	INTRA_CONSTEXPR_VIRTUAL T operator[](size_t index) final
	{
		return Base::mRange[index];
	}
};

template<typename T, class TFor> struct RangeWrapper
{
	constexpr bool Empty() const
	{
		TP unused{};
		return range()->TryFirst(&unused);
	}

	constexpr T First() const
	{
		INTRA_PRECONDITION(!Empty());
		TP res{};
		range()->TryFirst(&res);
		if constexpr(CReference<T>) return *res;
		else return res;
	};

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		PopFirstCount(1);
	}

	constexpr T Next()
	{
		INTRA_PRECONDITION(!Empty());
		TP res{};
		range()->Read(&res, 1);
		if constexpr(CReference<T>) return *res;
		else return res;
	}

	constexpr index_t PopFirstCount(ClampedSize maxElementsToPop)
	{
		return index_t(range()->PopFirstCount(maxElementsToPop));
	}

	template<COutputOf<T> O> constexpr index_t ReadWrite(O&& dst)
	{
		size_t res = 0;
		if constexpr(CArrayList<O> && CSame<TArrayListValue<O>, T> && CRange<O>)
		{
			res = range()->Read(DataOf(dst), LengthOf(dst));
			dst|Intra::PopFirstCount(res);
		}
		else
		{
			enum: size_t {BufLen = (128 + sizeof(TP) - 1) / sizeof(TP)};
			TP buf[BufLen] = {};
			for(;;)
			{
				const size_t numRead = range()->Read(buf, LengthOf(buf));
				if constexpr(CReference<T>) buf|Take(numRead)|Map(Deref)|WriteTo(dst);
				else buf|Take(numRead)|WriteTo(dst);
				res += numRead;
				if(numRead < BufLen) break;
			}
		}
		return index_t(res);
	}

private:
	using TP = TSelect<TRemoveReference<T>*, T, CReference<T>>;
	auto range() const {return static_cast<const TFor*>(this)->range();}
};

template<typename T, class TFor> struct NVIBidirectionalRangeWrapper: NVIRangeWrapper<T, TFor>
{
	constexpr T Last() const
	{
		INTRA_PRECONDITION(!Base::Empty());
		TP res{};
		nviRange()->doTryLast(&res);
		if constexpr(CReference<T>) return *res;
		else return res;
	};

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Base::Empty());
		popLastCount(1);
	}

	constexpr index_t PopLastCount(ClampedSize maxElementsToPop)
	{
		return index_t(nviRange()->doPopLastCount(maxElementsToPop));
	}

	template<COutputOf<T> O> constexpr index_t ReadLastWrite(O&& dst)
	{
		size_t res = 0;
		if constexpr(CArrayList<O> && CSame<TArrayListValue<O>, T> && CRange<O>)
		{
			res = nviRange()->doReadLast(DataOf(dst), LengthOf(dst));
			dst|Intra::PopLastCount(res);
		}
		else
		{
			enum: size_t {BufLen = (128 + sizeof(TP) - 1) / sizeof(TP)};
			TP buf[BufLen] = {};
			for(;;)
			{
				const size_t numRead = nviRange()->doReadLast(buf, LengthOf(buf));
				if constexpr(CReference<T>) buf|Take(numRead)|Map(Deref)|WriteTo(dst);
				else buf|Take(numRead)|WriteTo(dst);
				res += numRead;
				if(numRead < BufLen) break;
			}
		}
		return index_t(res);
	}

private:
	using Base = NVIRangeWrapper<T, TFor>;
	using TP = TSelect<TRemoveReference<T>*, T, CReference<T>>;
	auto nviRange() const {return static_cast<const TFor*>(this)->nviRange();}
};

}
} INTRA_END
