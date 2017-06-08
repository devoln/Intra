#pragma once

#include "Cpp/Warnings.h"
#include "Meta/Type.h"
#include "Meta/Operators.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Utils {

template<typename T> struct Optional
{
	static struct ConstructT {} DefaultConstruct;
private:
	typedef T Type; //Нужно для natvis
	union storage
	{
		char value[sizeof(T)];
		Meta::IntegralTypeFromMinSize<(sizeof(T)>8? 8: sizeof(T))> unused; //Для выравнивания
		storage(null_t=null): unused(0) {}
		storage(ConstructT) {new(this) T;}

		template<typename Arg0, typename... Args> storage(Arg0&& arg0, Args&&... args)
		{new(this) T(Cpp::Forward<Arg0>(arg0), Cpp::Forward<Args>(args)...);}
	};

public:
	Optional(null_t=null): mVal(null), mNotNull(false) {}
	Optional(ConstructT): mVal(DefaultConstruct), mNotNull(true) {}
	
	//template<typename Arg0, typename... Args> Optional(Arg0&& arg0, Args&&... args):
	    //val(Cpp::Forward(arg0), Cpp::Forward(args)...), not_null(true) {}
	
	template<typename Arg0, typename... Args> Optional(Arg0&& arg0, Args&&... args):
		mVal(Cpp::Forward<Arg0>(arg0), Cpp::Forward<Args>(args)...), mNotNull(true) {}

	Optional(Optional&& rhs):
		mVal(null), mNotNull(rhs.mNotNull)
	{if(mNotNull) new(&mVal) T(Cpp::Move(rhs.value()));}

	Optional(const Optional& rhs):
		mVal(null), mNotNull(rhs.mNotNull)
	{if(mNotNull) new(&mVal) T(rhs.value());}

	~Optional() {if(mNotNull) value().~T();}

	Optional<T>& operator=(const T& rhs)
	{
		if(mNotNull) assign(rhs);
		else new(&mVal) T(rhs);
		return *this;
	}

	Optional<T>& operator=(const Optional<T>& rhs)
	{
		if(rhs.mNotNull) return operator=(rhs.value());
		if(mNotNull) value().~T();
		mNotNull = false;
		return *this;
	}

	Optional<T>& operator=(T&& rhs)
	{
		if(mNotNull) assign(Cpp::Move(rhs));
		else new(&mVal) T(Cpp::Move(rhs));
		return *this;
	}

	Optional<T>& operator=(Optional<T>&& rhs)
	{
		if(rhs.mNotNull) return operator=(Cpp::Move(reinterpret_cast<T&>(rhs.mVal)));
		if(mNotNull) value().~T();
		mNotNull = false;
		return *this;
	}

	forceinline bool operator==(null_t) const {return !mNotNull;}
	forceinline bool operator!=(null_t) const {return mNotNull;}

	//! Если тип T не имеет оператора сравнения, то сравнивается только наличие значения в обоих объектах.
	template<typename U = T> forceinline Meta::EnableIf<
		!Meta::HasOpEquals<U>::_,
	bool> operator==(const Optional& rhs) const
	{return mNotNull == rhs.mNotNull;}

	//! Если тип T имеет оператор сравнения, то сравнивается наличие значения в обоих объектах, и,
	//! если значение содержится в обоих объектах, то сравниваются хранящиеся в нём значения.
	template<typename U=T> forceinline Meta::EnableIf<
		Meta::HasOpEquals<U>::_,
	bool> operator==(const Optional& rhs) const
	{
		return mNotNull == rhs.mNotNull &&
			(!mNotNull || value() == rhs.value());
	}

	forceinline bool operator!=(const Optional& rhs) const {return !operator==(rhs);}
	
	T& Value()
	{
		INTRA_DEBUG_ASSERT(mNotNull);
		return value();
	}

	const T& Value() const
	{
		INTRA_DEBUG_ASSERT(mNotNull);
		return value();
	}

	T& operator()() {return Value();}
	const T& operator()() const {return Value();}

private:
	storage mVal;
	bool mNotNull;

	template<typename U=T> Meta::EnableIf<
		Meta::IsCopyAssignable<U>::_
	> assign(const T& rhs)
	{value() = rhs;}

	template<typename U=T> Meta::EnableIf<
		!Meta::IsCopyAssignable<U>::_
	> assign(const T& rhs)
	{
		value().~T();
		new(&mVal) T(rhs);
	}

	template<typename U=T> Meta::EnableIf<
		Meta::IsMoveAssignable<U>::_
	> assign(T&& rhs)
	{value() = Cpp::Move(rhs);}

	forceinline T& value()
	{return reinterpret_cast<T&>(mVal);}

	forceinline const T& value() const
	{return reinterpret_cast<const T&>(mVal);}
};

}}

INTRA_WARNING_POP
