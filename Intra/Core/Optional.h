#pragma once

#include "Core/Assert.h"
#include "Core/Type.h"
#include "Core/Operations.h"
#include "Core/Functional.h"

INTRA_CORE_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
#if defined(_MSC_VER) && _MSC_VER >= 1900
#pragma warning(disable: 4582) //constructor is not implicitly called
#endif
namespace D {

struct constructEmptyT {};

template<typename T> class OptionalTrivBase
{
protected:
	union storage
	{
		char dummy[1];
		T value;
		constexpr forceinline storage(constructEmptyT) noexcept: dummy{} {};
		template<typename... Args> constexpr forceinline storage(TConstruct, Args&&... args):
			value(Forward<Args>(args)...) {}
		constexpr forceinline storage(const storage&) = default;
		constexpr forceinline storage(storage&&) = default;
	} mVal;
	bool mNotNull;

	constexpr forceinline OptionalTrivBase(constructEmptyT) noexcept: mVal(constructEmptyT()), mNotNull(false) {};
	template<typename... Args> constexpr forceinline OptionalTrivBase(Args&&... args): mVal(Construct, Forward<Args>(args)...), mNotNull(true) {}
	constexpr forceinline OptionalTrivBase(OptionalTrivBase&& rhs) = default;
	constexpr forceinline OptionalTrivBase(const OptionalTrivBase& rhs) = default;
};

template<typename T> class OptionalNonTrivBase
{
protected:
	union storage
	{
		char dummy[1];
		T value;
		constexpr forceinline storage(constructEmptyT) noexcept: dummy{} {};
		template<typename... Args> constexpr forceinline storage(TConstruct, Args&&... args):
			value(Forward<Args>(args)...) {}
		forceinline ~storage() {}
	} mVal;
	bool mNotNull;

	forceinline OptionalNonTrivBase(constructEmptyT) noexcept: mVal(constructEmptyT()), mNotNull(false) {};
	template<typename... Args> forceinline OptionalNonTrivBase(Args&&... args): mVal(Construct, Forward<Args>(args)...), mNotNull(true) {}
	forceinline ~OptionalNonTrivBase() {if(mNotNull) mVal.value.~T();}

	forceinline OptionalNonTrivBase(OptionalNonTrivBase&& rhs): mNotNull(false)
	{
		if(rhs.mNotNull) new(Construct, &mVal.value) T(Move(rhs.mVal.value));
		mNotNull = rhs.mNotNull;
	}

	forceinline OptionalNonTrivBase(const OptionalNonTrivBase& rhs)
	{
		if(rhs.mNotNull) new(Construct, &mVal.value) T(rhs.mVal.value);
		mNotNull = rhs.mNotNull;
	}
};

template<typename T> using OptionalBase = TSelect<OptionalTrivBase<T>, OptionalNonTrivBase<T>, CTriviallyDestructible<T>>;

}

//! Contiguous container stores either 0 or 1 elements without dynamic allocation.
template<typename T> class Optional: public D::OptionalBase<T>
{
public:
	constexpr forceinline Optional(null_t=null): D::OptionalBase<T>(D::constructEmptyT()) {}
	
	template<typename... Args> explicit constexpr forceinline Optional(TConstruct, Args&&... args):
		D::OptionalBase<T>(Forward<Args>(args)...) {}

	template<typename... Args> explicit constexpr forceinline Optional(Args&&... args):
		D::OptionalBase<T>(Forward<Args>(args)...) {}

	constexpr forceinline Optional(Optional&& rhs) = default;
	constexpr forceinline Optional(const Optional& rhs) = default;

	forceinline INTRA_CONSTEXPR2 T& Set(const T& rhs)
	{
		if(mNotNull) assign(rhs);
		else new(&mVal.value) T(rhs);
		return mVal.value;
	}

	forceinline INTRA_CONSTEXPR2 T& Set(T&& rhs)
	{
		if(mNotNull) assign(Move(rhs));
		else new(&mVal.value) T(Move(rhs));
		return mVal.value;
	}

	//! Construct the contained value in-place. Destruct an existing contained value if any.
	template<typename... Args> forceinline INTRA_CONSTEXPR2 T& Emplace(Args&&... args)
	{
		if(mNotNull) mVal.value.~T();
		new(&mVal.value) T(Forward<Args>(rhs)...);
		return mVal.value;
	}

	forceinline INTRA_CONSTEXPR2 Optional<T>& operator=(const Optional<T>& rhs)
	{
		if(rhs.mNotNull) return operator=(rhs.mVal.value);
		if(mNotNull) mVal.value.~T();
		mNotNull = false;
		return *this;
	}

	forceinline INTRA_CONSTEXPR2 Optional<T>& operator=(Optional<T>&& rhs)
	{
		if(rhs.mNotNull) return operator=(Move(rhs.mVal.value));
		if(mNotNull) mVal.value.~T();
		mNotNull = false;
		return *this;
	}

	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const {return !mNotNull;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(null_t) const {return mNotNull;}
	INTRA_NODISCARD constexpr forceinline explicit operator bool() const {return mNotNull;}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T* Data() {return &mVal.value;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline const T* Data() const {return &mVal.value;}
	INTRA_NODISCARD constexpr forceinline index_t Length() const {return index_t(mNotNull);}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T* begin() {return &mVal.value;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline const T* begin() const {return &mVal.value;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T* end() {return &mVal.value + index_t(mNotNull);}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline const T* end() const {return &mVal.value + index_t(mNotNull);}

	//! @return true if both objects have equal values or both have no value.
	template<typename U=T> INTRA_NODISCARD constexpr forceinline auto operator==(const Optional& rhs) const -> decltype(mVal.value == rhs.mVal.value)
	{
		return mNotNull == rhs.mNotNull &&
			(!mNotNull || mVal.value == rhs.mVal.value);
	}

	template<typename U=T> INTRA_NODISCARD constexpr forceinline auto operator!=(const Optional& rhs) const -> decltype(mVal.value == rhs.mVal.value) {return !operator==(rhs);}

	///@{
	//! @return contained object. Must be called only on non-empty Optional!
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T& Unwrap()
	{
		INTRA_DEBUG_ASSERT(mNotNull);
		return mVal.value;
	}

	INTRA_NODISCARD constexpr forceinline const T& Unwrap() const
	{
		return INTRA_DEBUG_ASSERT(mNotNull),
			mVal.value;
	}

	INTRA_NODISCARD constexpr forceinline const T& CUnwrap() const
	{
		return INTRA_DEBUG_ASSERT(mNotNull),
			mVal.value;
	}
	///@}

private:
	template<typename U=T> INTRA_CONSTEXPR2 forceinline Requires<
		CCopyAssignable<U>
	> assign(const T& rhs)
	{mVal.value = rhs;}

	template<typename U=T> INTRA_CONSTEXPR2 forceinline Requires<
		!CCopyAssignable<U>
	> assign(const T& rhs)
	{
		mVal.value.~T();
		new(&mVal.value) T(rhs);
	}

	template<typename U=T> INTRA_CONSTEXPR2 forceinline Requires<
		CMoveAssignable<U>
	> assign(T&& rhs)
	{mVal.value = Move(rhs);}

	template<typename U=T> INTRA_CONSTEXPR2 forceinline Requires<
		!CMoveAssignable<U>
	> assign(const T& rhs)
	{
		mVal.value.~T();
		new(&mVal.value) T(Move(rhs));
	}
};

template<typename T> class Optional<T&>
{
	T* mVal = null;
public:
	Optional() = default;
	constexpr forceinline Optional(null_t) {}
	explicit constexpr forceinline Optional(T& v): mVal(&v) {}

	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const {return mVal == null;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(null_t) const {return mVal != null;}
	INTRA_NODISCARD constexpr forceinline explicit operator bool() const {return mVal != null;}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T* Data() {return mVal;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline const T* Data() const {return mVal;}
	INTRA_NODISCARD constexpr forceinline index_t Length() const {return index_t(mVal != null);}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T* begin() {return mVal;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline const T* begin() const {return mVal;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T* end() {return mVal != null? mVal + 1: null;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline const T* end() const {return mVal != null? mVal + 1: null;}

	//! @return true if both objects have equal values or both have no value.
	INTRA_NODISCARD constexpr forceinline bool operator==(const Optional& rhs) const {return mVal == rhs.mVal;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(const Optional& rhs) const {return !operator==(rhs);}

	forceinline INTRA_CONSTEXPR2 T& Set(const T& rhs) {mVal = &rhs;}
	forceinline INTRA_CONSTEXPR2 T& Emplace(const T& rhs) {Set(rhs);}

	///@{
	//! @return contained object. Must be called only on non-empty Optional!
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T& Unwrap()
	{
		INTRA_DEBUG_ASSERT(mVal != null);
		return *mVal;
	}

	INTRA_NODISCARD constexpr forceinline const T& Unwrap() const
	{
		return INTRA_DEBUG_ASSERT(mVal != null),
			*mVal;
	}

	INTRA_NODISCARD constexpr forceinline const T& CUnwrap() const
	{
		return INTRA_DEBUG_ASSERT(mVal != null),
			*mVal;
	}
	///@}
};

template<typename T> constexpr forceinline Optional<T> Opt(const T& rhs) {return Optional<T>(rhs);}
template<typename T> constexpr forceinline Optional<T> Opt(T&& rhs) {return Optional<T>(Move(rhs));}
template<typename T> constexpr forceinline Optional<T&> OptRef(T& rhs) {return Optional<T&>(rhs);}

#if INTRA_CONSTEXPR_TEST
static_assert(Optional<int>(5) == Optional<int>(5), "Optionals can be compared.");
static_assert(Optional<int>(5) != Optional<int>{4}, "Optionals can be compared.");
static_assert(Optional<int>() == null, "Default constructed optional does not contain any value.");
static_assert(Optional<int>(0) != Optional<int>(), "Zero is not equal to empty optional.");
static_assert(Optional<int>(Construct).CUnwrap() == 0, "This is a way to default construct underlying type and to get its value in C++11 constexpr.");
//static_assert(Optional<int>().CVal() != 0, "Getting the value of the empty optinal is an error!");
#if INTRA_CONSTEXPR_TEST >= 201304
static_assert(Optional<int>(7).Unwrap() == 7, "Unwrap() can be used with constexpr only in C++14.");
#endif
#endif

INTRA_CORE_END
