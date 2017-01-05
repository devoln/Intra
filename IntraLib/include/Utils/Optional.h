#pragma once

#include "Platform/CppWarnings.h"
#include "Meta/Type.h"

namespace Intra { namespace Utils {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct Optional
{
	static struct ConstructT {} DefaultConstruct;
private:
	typedef T& ref;
	typedef const T& cref;

	union storage
	{
		char value[sizeof(T)];
		Meta::IntegralTypeFromMinSize<(sizeof(T)>8? 8: sizeof(T))> unused;
		storage(null_t=null): unused(0) {}
		storage(ConstructT) {new(this) T;}

		template<typename Arg0, typename... Args> storage(Arg0&& arg0, Args&&... args)
		{new(this) T(Meta::Forward<Arg0>(arg0), Meta::Forward<Args>(args)...);}
	};

public:

	Optional(null_t=null): val(null), not_null(false) {}
	Optional(ConstructT): val(DefaultConstruct), not_null(true) {}
	
	//template<typename Arg0, typename... Args> Optional(Arg0&& arg0, Args&&... args):
	    //val(Meta::Forward(arg0), Meta::Forward(args)...), not_null(true) {}
	
	template<typename Arg0, typename... Args> Optional(const Arg0& arg0, const Args&... args):
		val(arg0, args...), not_null(true) {}

	Optional(Optional&& rhs):
		val(null), not_null(rhs.not_null)
	{if(not_null) new(&val) T(Meta::Move(rhs.value()));}

	Optional(const Optional& rhs):
		val(null), not_null(rhs.not_null)
	{if(not_null) new(&val) T(rhs.value());}

	~Optional() {if(not_null) value().~T();}

	Optional<T>& operator=(cref rhs)
	{
		if(not_null) assign(rhs);
		else new(&val) T(rhs);
		return *this;
	}

	Optional<T>& operator=(const Optional<T>& rhs)
	{
		if(rhs.not_null) return operator=(rhs.value());
		if(not_null) value().~T();
		not_null = false;
		return *this;
	}

	Optional<T>& operator=(T&& rhs)
	{
		if(not_null) assign(Meta::Move(rhs));
		else new(&val) T(Meta::Move(rhs));
		return *this;
	}

	Optional<T>& operator=(Optional<T>&& rhs)
	{
		if(rhs.not_null) return operator=(Meta::Move(reinterpret_cast<T&>(rhs.val)));
		if(not_null) value().~T();
		not_null = false;
		return *this;
	}

	bool operator==(null_t) const {return !not_null;}
	bool operator!=(null_t) const {return not_null;}
	
	ref Value()
	{
		INTRA_ASSERT(not_null);
		return value();
	}

	cref Value() const
	{
		INTRA_ASSERT(not_null);
		return value();
	}

	ref operator()() {return value();}
	cref operator()() const {return value();}

private:
	storage val;
	bool not_null;

	template<typename U=T> Meta::EnableIf<
		Meta::IsCopyAssignable<U>::_
	> assign(cref rhs)
	{value() = rhs;}

	template<typename U=T> Meta::EnableIf<
		!Meta::IsCopyAssignable<U>::_
	> assign(cref rhs)
	{
		value().~T();
		new(&val) T(rhs);
	}

	template<typename U=T> Meta::EnableIf<
		Meta::IsMoveAssignable<U>::_
	> assign(T&& rhs)
	{value() = Meta::Move(rhs);}

	forceinline ref value()
	{return reinterpret_cast<ref>(val);}

	forceinline cref value() const
	{return reinterpret_cast<cref>(val);}
};

INTRA_WARNING_POP

}}
