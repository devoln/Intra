#pragma once

#include "Platform/Intrinsics.h"
#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"
#include "Platform/Debug.h"
#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
#ifdef _MSC_VER
#pragma warning(disable: 4191 4626)
#if(_MSC_VER>=1900)
#pragma warning(disable: 5027)
#endif
#endif

namespace Intra { namespace Utils {

template<typename TFuncSignature> class Callback;
template<typename R, typename... Args> class Callback<R(Args...)>
{
public:
	typedef R(*TFunc)(void*, Args...);
	typedef R(*FreeFunc)(Args...);

	Callback(null_t=null): obj(null), func(null) {}
	Callback(void* o, TFunc f): obj(o), func(f) {}

	R operator()(Args... a) const
	{
		return obj? func(obj, Meta::Forward<Args>(a)...):
			freefunc(Meta::Forward<Args>(a)...);
	}

	bool operator==(null_t) const {return func==null;}
	bool operator!=(null_t) const {return !operator==(null);}
	bool operator!() const {return operator==(null);}

	bool operator==(const Callback<R(Args...)>& rhs) const {return obj==rhs.obj && func==rhs.func;}
	bool operator!= (const Callback<R(Args...)>& rhs) const {return !operator==(rhs);}

private:
	void* obj;
	union
	{
		TFunc func;
		FreeFunc freefunc;
	};
};

}}

INTRA_WARNING_POP
