#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_LOSING_CONVERSION
INTRA_WARNING_DISABLE_SIGN_CONVERSION

namespace Intra { namespace Funal {

template<typename F, typename A1> struct TBind1
{
	template<typename... Args> forceinline Meta::ResultOf<F, A1, Args&&...> operator()(Args&&... args)
	{return Func(Arg1, Cpp::Forward<Args>(args)...);}

	F Func;
	A1 Arg1;
};

template<typename F, typename A1, typename A2> struct TBind2
{
	template<typename... Args> forceinline Meta::ResultOf<F, A1, A2, Args&&...> operator()(Args&&... args)
	{return Func(Arg1, Arg2, Cpp::Forward<Args>(args)...);}

	F Func;
	A1 Arg1;
	A2 Arg2;
};

template<typename F, typename A1, typename A2, typename A3> struct TBind3
{
	template<typename... Args> forceinline Meta::ResultOf<F, A1, A2, A3, Args&&...> operator()(Args&&... args)
	{return Func(Arg1, Arg2, Arg3, Cpp::Forward<Args>(args)...);}

	F Func;
	A1 Arg1;
	A2 Arg2;
	A3 Arg3;
};

template<typename F, typename A1, typename A2, typename A3, typename A4> struct TBind4
{
	template<typename... Args> forceinline Meta::ResultOf<F, A1, A2, A3, A4, Args&&...> operator()(Args&&... args)
	{return Func(Arg1, Arg2, Arg3, Arg4, Cpp::Forward<Args>(args)...);}

	F Func;
	A1 Arg1;
	A2 Arg2;
	A3 Arg3;
	A4 Arg4;
};

template<typename F, typename A1, typename A2, typename A3, typename A4, typename A5> struct TBind5
{
	template<typename... Args> forceinline Meta::ResultOf<F, A1, A2, A3, A4, A5, Args&&...> operator()(Args&&... args)
	{return Func(Arg1, Arg2, Arg3, Arg4, Arg5, Cpp::Forward<Args>(args)...);}

	F Func;
	A1 Arg1;
	A2 Arg2;
	A3 Arg3;
	A4 Arg4;
	A5 Arg5;
};

template<typename F, typename Arg1> forceinline
TBind1<F, Meta::RemoveConstRef<Arg1>>
Bind(F&& f, Arg1&& arg1)
{return {Cpp::Forward<F>(f), Cpp::Forward<Arg1>(arg1)};}

template<typename F, typename Arg1, typename Arg2> forceinline
TBind2<F, Meta::RemoveConstRef<Arg1>, Meta::RemoveConstRef<Arg2>>
Bind(F&& f, Arg1&& arg1, Arg2&& arg2)
{return {Cpp::Forward<F>(f), Cpp::Forward<Arg1>(arg1), Cpp::Forward<Arg2>(arg2)};}

template<typename F, typename Arg1, typename Arg2, typename Arg3> forceinline
TBind3<F, Meta::RemoveConstRef<Arg1>, Meta::RemoveConstRef<Arg2>, Meta::RemoveConstRef<Arg3>>
Bind(F&& f, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
{return {Cpp::Forward<F>(f), Cpp::Forward<Arg1>(arg1), Cpp::Forward<Arg2>(arg2), Cpp::Forward<Arg3>(arg3)};}

template<typename F, typename Arg1, typename Arg2, typename Arg3, typename Arg4> forceinline
TBind4<F, Meta::RemoveConstRef<Arg1>, Meta::RemoveConstRef<Arg2>, Meta::RemoveConstRef<Arg3>, Meta::RemoveConstRef<Arg4>>
Bind(F&& f, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
{return {Cpp::Forward<F>(f), Cpp::Forward<Arg1>(arg1), Cpp::Forward<Arg2>(arg2), Cpp::Forward<Arg3>(arg3), Cpp::Forward<Arg4>(arg4)};}

template<typename F, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> forceinline
TBind4<F, Meta::RemoveConstRef<Arg1>, Meta::RemoveConstRef<Arg2>, Meta::RemoveConstRef<Arg3>, Meta::RemoveConstRef<Arg4>>
Bind(F&& f, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5)
{return {Cpp::Forward<F>(f), Cpp::Forward<Arg1>(arg1), Cpp::Forward<Arg2>(arg2), Cpp::Forward<Arg3>(arg3), Cpp::Forward<Arg4>(arg4), Cpp::Forward<Arg5>(arg5)};}

}}

INTRA_WARNING_POP
