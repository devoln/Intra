#pragma once

#include "Intra/Assert.h"
#include "Intra/Type.h"
#include "Intra/Functional.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_MOVE_IMPLICITLY_DELETED
#ifdef _MSC_VER
#pragma warning(disable: 4582) //constructor is not implicitly called
#endif
namespace z_D {

struct constructEmptyT {};

template<typename T> class OptionalTrivBase
{
protected:
	union storage
	{
		char dummy[1];
		T value;
		constexpr storage(constructEmptyT) noexcept: dummy{} {};
		template<typename... Args> constexpr storage(decltype(Construct), Args&&... args):
			value(Forward<Args>(args)...) {}
		constexpr storage(const storage&) = default;
		constexpr storage(storage&&) = default;
	} mVal;
	bool mNotNull;

	constexpr OptionalTrivBase(constructEmptyT) noexcept: mVal(constructEmptyT()), mNotNull(false) {};
	template<typename... Args> constexpr OptionalTrivBase(Args&&... args): mVal(Construct, Forward<Args>(args)...), mNotNull(true) {}
	constexpr OptionalTrivBase(OptionalTrivBase&& rhs) = default;
	constexpr OptionalTrivBase(const OptionalTrivBase& rhs) = default;
};

template<typename T> class OptionalNonTrivBase
{
protected:
	union storage
	{
		char dummy[1];
		T value;
		constexpr storage(constructEmptyT) noexcept: dummy{} {};
		template<typename... Args> constexpr storage(decltype(Construct), Args&&... args):
			value(Forward<Args>(args)...) {}
		~storage() {}
	} mVal;
	bool mNotNull;

	OptionalNonTrivBase(constructEmptyT) noexcept: mVal(constructEmptyT()), mNotNull(false) {};
	template<typename... Args> OptionalNonTrivBase(Args&&... args): mVal(Construct, Forward<Args>(args)...), mNotNull(true) {}
	~OptionalNonTrivBase() {if(mNotNull) mVal.value.~T();}

	OptionalNonTrivBase(OptionalNonTrivBase&& rhs): mNotNull(false)
	{
		if(rhs.mNotNull) new(Construct, &mVal.value) T(Move(rhs.mVal.value));
		mNotNull = rhs.mNotNull;
	}

	OptionalNonTrivBase(const OptionalNonTrivBase& rhs)
	{
		if(rhs.mNotNull) new(Construct, &mVal.value) T(rhs.mVal.value);
		mNotNull = rhs.mNotNull;
	}
};

template<typename T> using OptionalBase = TSelect<OptionalTrivBase<T>, OptionalNonTrivBase<T>, CTriviallyDestructible<T>>;

}

//! Contiguous container stores either 0 or 1 elements without dynamic allocation.
template<typename T> class Optional: public z_D::OptionalBase<T>
{
	using BASE = z_D::OptionalBase<T>;
public:
	constexpr Optional(decltype(null) = null): z_D::OptionalBase<T>(z_D::constructEmptyT()) {}
	
	template<typename... Args, typename = Requires<CConstructible<T, Args&&...>>>
	explicit constexpr Optional(decltype(Construct), Args&&... args):
		z_D::OptionalBase<T>(Forward<Args>(args)...) {}

	template<typename... Args, typename = Requires<CConstructible<T, Args&&...>>>
	explicit constexpr Optional(Args&&... args):
		z_D::OptionalBase<T>(Forward<Args>(args)...) {}

	template<typename U = T, typename = Requires<CConvertibleTo<U&&, T>>>
	constexpr Optional(U&& value): z_D::OptionalBase<T>(Forward<U>(value)) {}

	template<typename U = T, typename = void, typename = Requires<!CConvertibleTo<U&&, T>>>
	explicit constexpr Optional(U&& value): z_D::OptionalBase<T>(Forward<U>(value)) {}

	constexpr Optional(Optional&&) = default;
	constexpr Optional(const Optional&) = default;

	template<typename U = T, typename = Requires<CCopyAssignable<U>>>
	constexpr U& Set(const U& rhs)
	{
		if(BASE::mNotNull) BASE::mVal.value = rhs;
		else new(Construct, &BASE::mVal.value) T(rhs);
		return BASE::mVal.value;
	}

	template<typename U = T, typename = Requires<CMoveAssignable<U>>>
	constexpr U& Set(U&& rhs)
	{
		if(BASE::mNotNull) BASE::mVal.value = Move(rhs);
		else new(Construct, &BASE::mVal.value) T(Move(rhs));
		return BASE::mVal.value;
	}

	//! Construct the contained value in-place. Destruct an existing contained value if any.
	template<typename... Args, typename = Requires<CConstructible<T, Args&&...>>>
	constexpr T& Emplace(Args&&... args)
	{
		if(BASE::mNotNull) BASE::mVal.value.~T();
		new(Construct, &BASE::mVal.value) T(Forward<Args>(args)...);
		return BASE::mVal.value;
	}

	template<typename U = T, typename = Requires<CCopyAssignable<U>>>
	constexpr Optional<U>& operator=(const Optional<U>& rhs)
	{
		if(rhs.mNotNull) return operator=(rhs.mVal.value);
		if(BASE::mNotNull) BASE::mVal.value.~T();
		BASE::mNotNull = false;
		return *this;
	}

	template<typename U = T, typename = Requires<CMoveAssignable<U>>>
	constexpr Optional<U>& operator=(Optional<U>&& rhs)
	{
		if(rhs.mNotNull) return operator=(Move(rhs.mVal.value));
		if(BASE::mNotNull) BASE::mVal.value.~T();
		BASE::mNotNull = false;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(decltype(null)) const {return !BASE::mNotNull;}
	[[nodiscard]] constexpr bool operator!=(decltype(null)) const {return !operator==(null);}
	[[nodiscard]] constexpr explicit operator bool() const {return !operator==(null);}

	[[nodiscard]] constexpr T* Data() {return &BASE::mVal.value;}
	[[nodiscard]] constexpr const T* Data() const {return &BASE::mVal.value;}
	[[nodiscard]] constexpr index_t Length() const {return index_t(BASE::mNotNull);}

	//! @return true if both objects have equal values or both have no value.
	template<typename U = T, typename = Requires<CEqualityComparable<U>>>
	[[nodiscard]] constexpr bool operator==(const Optional<U>& rhs) const
	{
		return BASE::mNotNull == rhs.mNotNull &&
			(!BASE::mNotNull || BASE::mVal.value == rhs.mVal.value);
	}

	template<typename U = T, typename = Requires<CEqualityComparable<U>>>
	[[nodiscard]] constexpr bool operator!=(const Optional<U>& rhs) const {return !operator==(rhs);}

	///@{
	//! @return contained object. Must be called only on non-empty Optional!
	[[nodiscard]] constexpr T& Unwrap()
	{
		INTRA_PRECONDITION(bool(*this));
		return BASE::mVal.value;
	}

	[[nodiscard]] constexpr const T& Unwrap() const
	{
		INTRA_PRECONDITION(bool(*this));
		return BASE::mVal.value;
	}
	///@}

	[[nodiscard]] constexpr T GetOr(T nullValue = T()) const
	{
		if(!*this) return nullValue;
		return BASE::mVal.value;
	}
};

template<typename T> class Optional<T&>
{
	T* mVal = null;
public:
	Optional() = default;
	constexpr Optional(decltype(null)) {}
	explicit constexpr Optional(T& v): mVal(&v) {}

	[[nodiscard]] constexpr bool operator==(decltype(null)) const {return mVal == null;}
	[[nodiscard]] constexpr bool operator!=(decltype(null)) const {return mVal != null;}
	[[nodiscard]] constexpr explicit operator bool() const {return mVal != null;}

	[[nodiscard]] constexpr T* Data() {return mVal;}
	[[nodiscard]] constexpr const T* Data() const {return mVal;}
	[[nodiscard]] constexpr index_t Length() const {return index_t(mVal != null);}

	//! @return true if both objects have equal values or both have no value.
	[[nodiscard]] constexpr bool operator==(const Optional& rhs) const {return mVal == rhs.mVal;}
	[[nodiscard]] constexpr bool operator!=(const Optional& rhs) const {return !operator==(rhs);}

	INTRA_FORCEINLINE constexpr T& Set(const T& rhs) {mVal = &rhs;}
	INTRA_FORCEINLINE constexpr T& Emplace(const T& rhs) {Set(rhs);}

	///@{
	//! @return contained object. Must be called only on non-empty Optional!
	[[nodiscard]] constexpr T& Unwrap()
	{
		INTRA_PRECONDITION(mVal != null);
		return *mVal;
	}

	[[nodiscard]] constexpr const T& Unwrap() const
	{
		INTRA_PRECONDITION(mVal != null);
		return *mVal;
	}
	///@}
};
template<typename T> Optional(T) -> Optional<T>;

template<typename T> constexpr Optional<T&> OptRef(T& rhs) {return Optional<T&>(rhs);}

#if INTRA_CONSTEXPR_TEST
static_assert(Optional(5) == Optional<int>(5));
static_assert(Optional<int>(5) != Optional<int>{4});
static_assert(Optional<int>() == null);
static_assert(Optional<int>(0) != Optional<int>());
static_assert(Optional<int>(Construct).Unwrap() == 0);
static_assert(Optional<int>(7).Unwrap() == 7);
static_assert(Optional(7.6f).Unwrap() == 7.6f);
//static_assert(Optional<int>().Unwrap() != 0, "Getting the value of the empty optional is an error!");
#endif

INTRA_END
