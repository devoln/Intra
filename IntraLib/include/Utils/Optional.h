#pragma once

#include "Meta/Type.h"

namespace Intra { namespace Utils {

template<typename T> struct Optional
{
	struct ConstructT {} DefaultConstruct;
	union storage
	{
		char value[sizeof(T)];
		Meta::TypeFromSize<(sizeof(T)>8? 8: sizeof(T))> unused;
		storage(null_t=null): unused(0) {}
		storage(ConstructT) {new(this) T;}

		template<typename Arg0, typename... Args> storage(Arg0&& arg0, Args&&... args)
		{
			new(this) T(core::forward(arg0), core::forward(args)...);
		}

		template<typename Arg0, typename... Args> storage(const Arg0& arg0, const Args&... args)
		{
			new(this) T(arg0, args...);
		}
	};


	Optional(null_t=null): val(null), not_null(false) {}
	Optional(ConstructT): val(DefaultConstruct), not_null(true) {}
	
	//template<typename Arg0, typename... Args> Optional(Arg0&& arg0, Args&&... args):
	    //val(core::forward(arg0), core::forward(args)...), not_null(true) {}
	
	template<typename Arg0, typename... Args> Optional(const Arg0& arg0, const Args&... args):
		val(arg0, args...), not_null(true) {}

	Optional(Optional&& rhs): val(null), not_null(rhs.not_null) {if(not_null) new((void*)&val) T(core::move((T&&)*(T*)&rhs.val));}
	Optional(const Optional& rhs): val(null), not_null(rhs.not_null) {if(not_null) new((void*)&val) T(*(const T*)&rhs.val);}
	~Optional() {if(not_null) ((T*)&val)->~T();}

	Optional<T>& operator=(const T& rhs)
	{
		if(not_null) assign(rhs);
		else new(&val) T(rhs);
		return *this;
	}

	Optional<T>& operator=(const Optional<T>& rhs)
	{
		if(rhs.not_null) return operator=((const T&)rhs.val);
		if(not_null) ((T&)val).~T();
		not_null = false;
		return *this;
	}

	Optional<T>& operator=(T&& rhs)
	{
		if(not_null) assign(core::move(rhs));
		else new(&val) T(core::move(rhs));
		return *this;
	}

	Optional<T>& operator=(Optional<T>&& rhs)
	{
		if(rhs.not_null) return operator=(core::move((T&)rhs.val));
		if(not_null) ((T&)val).~T();
		not_null = false;
		return *this;
	}

	bool operator==(null_t) const {return !not_null;}
	bool operator!=(null_t) const {return not_null;}
	
	T& value()
	{
		INTRA_ASSERT(not_null);
		return (T&)val;
	}

	const T& value() const
	{
		INTRA_ASSERT(not_null);
		return (const T&)val;
	}

	T& operator()() {return value();}
	const T& operator()() const {return value();}

private:
	storage val;
	bool not_null;

	template<typename U=T> Meta::EnableIf<Meta::IsCopyAssignable<U>::_> assign(const T& rhs)
	{
		(T&)val = rhs;
	}

	template<typename U=T> Meta::EnableIf<!Meta::IsCopyAssignable<U>::_> assign(const T& rhs)
	{
		((T&)val).~T();
		new(&val) T(rhs);
	}

	template<typename U=T> Meta::EnableIf<Meta::IsMoveAssignable<U>::_> assign(T&& rhs)
	{
		(T&)val = core::move(rhs);
	}
};

}}

