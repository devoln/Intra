#pragma once

#include <Intra/Functional.h>
#include <Intra/Concepts.h>
#include <Intra/Concurrency/Atomic.h>

namespace Intra { INTRA_BEGIN

// Simplifies the implementation of move semantics. Such fields may allow to use compiler-generated move constructors.
// NOTE: move assignment just swaps the wrappers. Make sure that such moved-from objects are destroyed correctly.
template<typename T, T NullValue = T()> struct HandleMovableWrapper
{
	T Id = NullValue;

	HandleMovableWrapper() = default;
	INTRA_FORCEINLINE constexpr HandleMovableWrapper(T id): Id(id) {}

	INTRA_FORCEINLINE constexpr operator T() const {return Id;}
	INTRA_FORCEINLINE constexpr T operator->() const {return Id;}

	INTRA_FORCEINLINE constexpr HandleMovableWrapper(HandleMovableWrapper&& rhs) noexcept: Id(rhs.Id) {rhs.Id = NullValue;}

	INTRA_FORCEINLINE constexpr HandleMovableWrapper& operator=(HandleMovableWrapper&& rhs) noexcept
	{
		Swap(Id, rhs.Id);
		return *this;
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool IsNull() const {return Id == NullValue;}
};

template<typename H> concept CHandle = CMoveAssignable<H> && CConstructible<H> && CConvertibleTo<H, bool>;
template<typename H> concept CSmartHandle = CHandle<H> && requires(H h) {{h.get()} -> CNonVoid;};

template<CHandle H, CCallable<H> Deleter> class ResourceOwner
{
	template<CHandle U> friend ResourceOwner<U, Deleter>;
	template<CHandle U, CCallable<H> D> static constexpr bool IsCompatible = CConstructible<H, U> &&
		(CSame<D, Deleter> || CDerived<Deleter, D>) && (CSmartHandle<H> ||
			CBasicPointer<H> && CBasicPointer<U> && (!CDerived<TRemovePointer<U>, TRemovePointer<H>> || CHasVirtualDestructor<TRemovePointer<H>>));
public:
	ResourceOwner() = default;

	INTRA_FORCEINLINE explicit constexpr ResourceOwner(Owner<H> handle, auto&&... deleterArgs) noexcept:
		mHandle(INTRA_MOVE(handle)), mDeleter(INTRA_FWD(deleterArgs)...) {}

	INTRA_FORCEINLINE constexpr ~ResourceOwner()
	{
		if constexpr(!requires {mHandle.IsOwner();})
			mDeleter(mHandle);
		else if(mHandle.IsOwner()) mDeleter(mHandle); // for handles with optional ownership
	}

	ResourceOwner(const ResourceOwner& rhs) = delete;
	ResourceOwner& operator=(const ResourceOwner& rhs) = delete;

	INTRA_FORCEINLINE constexpr ResourceOwner(ResourceOwner&& rhs) noexcept:
		mHandle(INTRA_MOVE(rhs.mHandle)), mDeleter(INTRA_MOVE(rhs.mDeleter)) {rhs.mHandle = H();}

	INTRA_FORCEINLINE constexpr void reset(Owner<H> otherHandle) noexcept {*this = ResourceOwner(otherHandle);}
	INTRA_FORCEINLINE constexpr ResourceOwner& operator=(ResourceOwner&& rhs) noexcept {Swap(mHandle, rhs.mHandle); return *this;}


	template<typename U, class D> requires IsCompatible<U, D>
	INTRA_FORCEINLINE constexpr ResourceOwner(ResourceOwner<U, D>&& rhs) noexcept:
		mHandle(INTRA_MOVE(rhs.mHandle)), mDeleter(INTRA_MOVE(rhs.mDeleter)) {rhs.mHandle = H();}

	template<typename U, class D> requires IsCompatible<U, D>
	INTRA_FORCEINLINE constexpr ResourceOwner& operator=(ResourceOwner<U, D>&& rhs) noexcept {return *this = ResourceOwner(INTRA_MOVE(rhs));}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto get() const noexcept requires CSmartHandle<H> || CCopyable<H>
	{
		if constexpr(CSmartHandle<H>) return mHandle.get();
		else return mHandle;
	}

	INTRA_FORCEINLINE constexpr Owner<H> release() noexcept {return Exchange(mHandle, H());}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool IsOwner() const noexcept requires requires {mHandle.IsOwner();} {return mHandle.IsOwner();}

	[[nodiscard]] INTRA_FORCEINLINE constexpr explicit operator bool() const noexcept {return bool(mHandle);}

	using TagTriviallyRelocatable = TTag<CTriviallyRelocatable<H> && CTriviallyRelocatable<Deleter>>;
protected:
	H mHandle{};
	INTRA_NO_UNIQUE_ADDRESS Deleter mDeleter{};
};

namespace z_D {
template<CHandle H> struct OptOwnerHandle
{
	H Handle{};
	bool IsOwning{};

	INTRA_FORCEINLINE constexpr bool IsOwner() const {return IsOwning;}
	INTRA_FORCEINLINE constexpr H get() const {return Handle;}
};
template<CHandle H, CCallable<H> Deleter> struct OptOwnerDeleter: Deleter
{
	using Deleter::Deleter;
	using Deleter::operator(); // make this deleter also compatible with wrapped handle H
	INTRA_FORCEINLINE constexpr void operator()(const OptOwnerHandle<H>& h) {if(h.IsOwning) Deleter::operator()(h.Handle);}
};
}
template<CHandle H, CCallable<H> Deleter> class ResourceOptOwner: public ResourceOwner<z_D::OptOwnerHandle<H>, z_D::OptOwnerDeleter<Deleter>>
{
	using Base = ResourceOwner<z_D::OptOwnerHandle<H>, z_D::OptOwnerDeleter<Deleter>>;
public:
	using Base::Base;
};

template<typename TOrArr> using TDefaultDeleter = TCall<CArrayType<TOrArr>? &DeleteArr<TRemoveExtent<TOrArr>>: &Delete<TRemoveExtent<TOrArr>>>;

template<typename TOrArr, class TParent = ResourceOwner<TRemoveExtent<TOrArr>*, TDefaultDeleter<TOrArr>>>
class SmartPointerBase: public TParent
{
	using T = TRemoveExtent<TOrArr>;
public:
	using TParent::TParent;
	using TParent::operator=;
	[[nodiscard]] constexpr T* operator->() const requires (!CArrayType<TOrArr>) {INTRA_PRECONDITION(*this); return this->get();}
	[[nodiscard]] constexpr T& operator*() const requires (!CArrayType<TOrArr>) {INTRA_PRECONDITION(*this); return *this->get();}
	
	[[nodiscard]] constexpr T& operator[](Index index) const requires CArrayType<TOrArr>
	{
		if constexpr(!CUnknownBoundArrayType<TOrArr>)
			INTRA_PRECONDITION(size_t(index) < Length());
		return this->get()[size_t(index)];
	}

	INTRA_FORCEINLINE constexpr SmartPointerBase& operator=(decltype(nullptr)) noexcept {this->reset(nullptr); return *this;}

	[[nodiscard]] INTRA_FORCEINLINE constexpr T* Data() const noexcept {return get();}
	[[nodiscard]] INTRA_FORCEINLINE constexpr index_t Length() const noexcept requires (!CUnknownBoundArrayType<TOrArr>)
	{
		enum: index_t {Extent = CKnownBoundArrayType<TOrArr>? ArrayExtent<TOrArr>: 1};
		return bool(*this)? Extent: 0;
	}
	
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool operator==(decltype(nullptr)) const noexcept {return !bool(*this);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool operator!=(decltype(nullptr)) const noexcept {return bool(*this);}
};

template<typename TOrArr> class Unique: public SmartPointerBase<TOrArr>
{
	using T = TRemoveExtent<TOrArr>;
public:
	using SmartPointerBase<TOrArr>::SmartPointerBase;
	[[nodiscard]] static INTRA_FORCEINLINE constexpr Unique New(auto&&... args) requires (!CUnknownBoundArrayType<TOrArr>)
	{
		if constexpr(CKnownBoundArrayType<TOrArr>) return Unique(new TOrArr{INTRA_FWD(args)...});
		return Unique(new TOrArr(INTRA_FWD(args)...));
	}

	[[nodiscard]] static INTRA_FORCEINLINE constexpr Unique New(Size count, auto&&... args) requires CUnknownBoundArrayType<TOrArr>
	{return Unique(new T[size_t(count)]{INTRA_FWD(args)...});}
};
static_assert(sizeof(Unique<int>) == sizeof(int*));
static_assert(CTriviallyRelocatable<Unique<int>>);

template<typename T> INTRA_FORCEINLINE constexpr Unique<T> UniqueMove(T& rhs) {return new T(INTRA_MOVE(rhs));}

// A smart pointer with optional ownership.
template<typename TOrArr> class MaybeUnique:
	public SmartPointerBase<TOrArr, ResourceOptOwner<TRemoveExtent<TOrArr>*, TDefaultDeleter<TOrArr>>>
{
	using Base = SmartPointerBase<TOrArr, ResourceOptOwner<TRemoveExtent<TOrArr>*, TDefaultDeleter<TOrArr>>>;
	template<typename U> friend class MaybeUnique;
public:
	using Base::Base;

	[[nodiscard]] static INTRA_FORCEINLINE constexpr MaybeUnique New(auto&&... args) noexcept {return Unique<TOrArr>::New(INTRA_FWD(args)...);}

	[[nodiscard]] static INTRA_FORCEINLINE constexpr MaybeUnique Make(TRemoveExtent<TOrArr>* ptr, bool passOwnership)
	{
		MaybeUnique res;
		res.mHandle = {.Handle = nonOwningPtr, .IsOwning = passOwnership};
		return res;
	}
	
	constexpr Unique<TOrArr> ReleaseAsUnique()
	{
		INTRA_PRECONDITION(this->IsOwning());
		this->mHandle.IsOwning = false;
		return Unique(this->mHandle);
	}
};
template<typename T> MaybeUnique(Unique<T>) -> MaybeUnique<T>;
template<typename T> [[nodiscard]] INTRA_FORCEINLINE constexpr MaybeUnique<T> NonUnique(TRemoveExtent<T>* nonOwningPtr)
{
	return MaybeUnique<T>::Make(nonOwningPtr, false);
}

template<class T> concept CRefCounter = requires(T& x) {
	{x.IncRef()} -> CConvertible<size_t>;
	{x.GetRC()} -> CConvertible<size_t>;
	{x.DecRef()} -> CSame<bool>;
};
template<class T> concept CRefCounted = requires(T& x) {{x.RefCount} -> CRefCounter;};

template<CIntegral TCounter = size_t, bool ThreadSafe = true> class RefCounter
{
	TSelectAtomic<TCounter, ThreadSafe> mRefCount{1};
public:
	INTRA_FORCEINLINE constexpr TCounter IncRef() {return mRefCount.Add<MemoryOrder::Relaxed>(1);}
	INTRA_FORCEINLINE constexpr TCounter GetRC() {return mRefCount.Get<MemoryOrder::Relaxed>();}
	INTRA_FORCEINLINE constexpr bool DecRef() {return mRefCount.Sub<MemoryOrder::AcquireRelease>(1) == 0;}

	RefCounter() = default;
	INTRA_FORCEINLINE constexpr RefCounter(TCounter initialValue): mRefCount(initialValue) {}
	RefCounter(const RefCounter&) = delete;
	RefCounter& operator=(const RefCounter&) = delete;
};
#if INTRA_CONSTEXPR_TEST
static_assert(CRefCounter<RefCounter<>>);
static_assert(CRefCounter<RefCounter<int, false>>);
#endif

/** Lightweight shared pointer with the following differences from std::shared_ptr:
1. Doesn't take raw pointers, use Shared<...>::New(...) instead.
2. Uses new/delete to make a merged allocation with reference counter and the object itself.
3. No support for weak references.
4. Full constexpr support.
5. Derived class may only be casted to a base class if they both have the same alignment and only if it's the first base.
WARNING: Casting to the base other than the first one will cause UB which is checked only in Debug at runtime.
*/
template<typename T, bool ThreadSafe = true> class Shared
{
	using TCounter = RefCounter<size_t, ThreadSafe>;
	template<typename U, bool ThreadSafeU> friend class Shared;
	template<typename U> static constexpr bool IsCompatible = !CSame<U, T> && (CSame<U, TRemoveConst<T>> || CDerived<U, T> && CHasVirtualDestructor<T>);

	struct ConstexprData
	{
		TCounter* Counter;
		T* Object;
	};

	struct Data: TCounter
	{
		union
		{
			T Value;
			char Bytes[sizeof(T)];
		};
		Data() {}
		~Data() {}
	};
	struct Storage;

	explicit INTRA_FORCEINLINE constexpr Shared(TCounter* counterAndData) noexcept: mData(counterAndData) {}
public:
	using StorageHandle = Storage*;
	static constexpr bool IsThreadSafe = ThreadSafe;

	Shared() = default;
	INTRA_FORCEINLINE constexpr Shared(decltype(nullptr)) {}

	/// Release reference ownership as a pointer that can be used to reconstruct Shared later.
	/// After this operation, the pointer is set to nullptr. This call does not decrease reference counter.
	INTRA_FORCEINLINE StorageHandle ReleaseHandleToReconstructLater(TUnsafe)
	{
		auto res = reinterpret_cast<StorageHandle>(mData);
		mData = nullptr;
		return res;
	}

	/// Use to reconstruct a shared pointer passed via C APIs as user-defined data untyped user-defined data (pointers/ULONG_PTR/uintptr).
	/// NOTE: Each handle must be reconstructed exactly once. Fewer reconstructions would result in a memory leak. More than one would result in double free.
	static INTRA_FORCEINLINE Shared ReconstructFromReleasedHandle(TUnsafe, StorageHandle handle)
	{
		Shared res;
		res.mData = reinterpret_cast<Data*>(handle);
		return res;
	}

	INTRA_FORCEINLINE constexpr Shared(const Shared& rhs)
	{
		if(IsConstantEvaluated())
		{
			mCData = new ConstexprData{.Counter = rhs.mCData->Counter, .Object = rhs.mCData->Object};
			if(mCData->Counter) mCData->Counter->IncRef();
			return;
		}
		mData = rhs.mData;
		if(mData) mData->IncRef();
	}

	INTRA_FORCEINLINE constexpr Shared(Shared&& rhs) noexcept: mCData(rhs.mCData) {rhs.mCData = nullptr;}

	template<typename U> requires IsCompatible<U>
	INTRA_FORCEINLINE constexpr Shared(const Shared<U, ThreadSafe>& rhs)
	{
		if(IsConstantEvaluated())
		{
			mCData = new ConstexprData{.Counter = rhs.mCData->Counter, .Object = rhs.mCData->Object};
			if(mCData->Counter) mCData->Counter->IncRef();
			return;
		}
		mData = reinterpret_cast<Data*>(rhs.mData);
		castAllowedRuntimeCheck(rhs);
		if(mData) mData->IncRef();
	}

	template<typename U> requires IsCompatible<U>
	INTRA_FORCEINLINE constexpr Shared(Shared<U, ThreadSafe>&& rhs) noexcept
	{
		if(IsConstantEvaluated())
		{
			mCData = new ConstexprData{.Counter = rhs.mCData->Counter, .Object = rhs.mCData->Object};
			delete rhs.mCData;
			rhs.mCData = nullptr;
			return;
		}
		mData = reinterpret_cast<Data*>(rhs.mData);
		castAllowedRuntimeCheck(rhs);
		rhs.mData = nullptr;
	}

	constexpr ~Shared()
	{
		if(!mCData) return;
		if(IsConstantEvaluated())
		{
			INTRA_ASSERT(use_count() != 0);
			if(mCData->Counter->DecRef())
			{
				delete mCData->Counter;
				delete mCData->Object;
			}
			delete mCData;
			return;
		}

		INTRA_PRECONDITION(use_count() != 0);
		if(!mData->DecRef()) return;

		reinterpret_cast<T*>(mData->Bytes)->~T(); // avoid accessing Value to prevent devirtualization and make sure virtual destructor is called if present 
		delete mData;
	}

	INTRA_FORCEINLINE constexpr Shared& operator=(const Shared& rhs) {return *this = Shared(rhs);}
	INTRA_FORCEINLINE constexpr Shared& operator=(Shared&& rhs) noexcept {Swap(mCData, rhs.mCData); return *this;}

	template<typename U> requires IsCompatible<U>
	INTRA_FORCEINLINE constexpr Shared& operator=(const Shared<U, ThreadSafe>& rhs) {return *this = Shared(rhs);}
	template<typename U> requires IsCompatible<U>
	INTRA_FORCEINLINE constexpr Shared& operator=(Shared<U, ThreadSafe>&& rhs) {return *this = Shared(INTRA_MOVE(rhs));}

	INTRA_FORCEINLINE constexpr Shared& operator=(decltype(nullptr)) {return *this = Shared();}

	[[nodiscard]] INTRA_FORCEINLINE constexpr T* get() const noexcept {return mCData? getUnchecked(): nullptr;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr size_t use_count() const {return mCData? (IsConstantEvaluated()? mCData->Counter: mData)->GetRC(): 0;}

	[[nodiscard]] INTRA_FORCEINLINE constexpr T* Data() const noexcept {return get();}
	[[nodiscard]] INTRA_FORCEINLINE constexpr index_t Length() const noexcept {return mCData? 1: 0;}

	template<typename... Args> [[nodiscard]] static constexpr Shared New(Args&&... args)
	{
		if(IsConstantEvaluated())
		{
			mCData = new ConstexprData{.Counter = new ConstexprCounter, .Data = new T(INTRA_FWD(args)...)};
			return;
		}
		const auto data = new Data;
		new(data->Bytes) T(INTRA_FWD(args)...);
		return Shared(data);
	}

	[[nodiscard]] constexpr T& operator*() const {INTRA_PRECONDITION(*this); return *getUnchecked();}
	[[nodiscard]] constexpr T* operator->() const {INTRA_PRECONDITION(*this); return getUnchecked();}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool operator==(decltype(nullptr)) const {return mCData == nullptr;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool operator!=(decltype(nullptr)) const {return !operator==(nullptr);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool operator==(const Shared& rhs) const {return mCData == rhs.mCData;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool operator!=(const Shared& rhs) const {return !operator==(rhs);}

	[[nodiscard]] INTRA_FORCEINLINE constexpr explicit operator bool() const {return mCData != nullptr;}

private:
	union
	{
		Data* mData;
		ConstexprData* mCData = nullptr;
	};
	template<typename U> void castAllowedRuntimeCheck(const Shared<U, ThreadSafe>& from)
	{
		static_assert(alignof(U) == alignof(T));
		INTRA_DEBUG_ASSERT(static_cast<const void*>(static_cast<T*>(from.get())) == static_cast<const void*>(from.get()));
	}
	[[nodiscard]] INTRA_FORCEINLINE constexpr T* getUnchecked() const noexcept
	{
		if(IsConstantEvaluated()) return mCData->Object;
		return &mData->Value;
	}
	template<typename T> friend class SharedClass;
};

template<typename T, bool ThreadSafe = true> class SharedClass
{
	void* operator new(size_t bytes) = delete;
	void operator delete(void* ptr, size_t bytes) = delete;

	using DerivedData = typename Shared<T>::Data;
public:
	/// Get a smart Shared pointer from this class instance.
	/// If the class instance is being destroyed now, returns nullptr.
	/// This fact can be used in case when the object's destructor is waiting until the current thread frees the resource.
	/// Cannot be used with already destructed object.
	[[nodiscard]] Shared<T> SharedThis()
	{
		void* const address = reinterpret_cast<char*>(this) - __builtin_offsetof(DerivedData, Value);
		const auto data = static_cast<DerivedData*>(address);
		if(data->IncRef() > 1) return data;

		// The refcount is already zero, the object is being destroyed
		return nullptr;
	}
};

template<typename T> [[nodiscard]] INTRA_FORCEINLINE constexpr Shared<TRemoveReference<T>> SharedMove(T&& rhs)
{
	return Shared<TRemoveReference<T>>::New(INTRA_MOVE(rhs));
}

template<typename T> constexpr bool IsTriviallyRelocatable<Shared<T>> = true;

} INTRA_END
