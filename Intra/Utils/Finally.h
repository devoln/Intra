#pragma once

#include "Cpp/Warnings.h"
#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

namespace Intra { namespace Utils {

template<typename F> struct TFinally
{
	forceinline ~TFinally() {OnDestruct();}
	F OnDestruct;
};

template<typename F, typename A1> struct TFinally1
{
	forceinline ~TFinally1() {OnDestruct(Cpp::Move(Arg1));}
	F OnDestruct;
	A1 Arg1;
};

template<typename F, typename A1, typename A2> struct TFinally2
{
	forceinline ~TFinally2() {OnDestruct(Cpp::Move(Arg1), Cpp::Move(Arg2));}
	F OnDestruct;
	A1 Arg1;
	A2 Arg2;
};

template<typename F, typename A1, typename A2, typename A3> struct TFinally3
{
	forceinline ~TFinally3() {OnDestruct(Cpp::Move(Arg1), Cpp::Move(Arg2), Cpp::Move(Arg3));}
	F OnDestruct;
	A1 Arg1;
	A2 Arg2;
	A3 Arg3;
};

template<typename F> forceinline Meta::EnableIf<
	Meta::IsCallable<F>::_,
TFinally<F>> Finally(F&& f) {return {Cpp::Forward<F>(f)};}

template<typename F, typename Arg1> forceinline Meta::EnableIf<
	Meta::IsCallable<F, Arg1>::_,
TFinally1<F, Arg1>> Finally(F&& f, Arg1&& arg1)
{return {Cpp::Forward<F>(f), Cpp::Forward<Arg1>(arg1)};}

template<typename F, typename Arg1, typename Arg2> forceinline Meta::EnableIf<
	Meta::IsCallable<F, Arg1, Arg2>::_,
TFinally2<F, Arg1, Arg2>> Finally(F&& f, Arg1&& arg1, Arg2&& arg2)
{return {Cpp::Forward<F>(f), Cpp::Forward<Arg1>(arg1), Cpp::Forward<Arg2>(arg2)};}

template<typename F, typename Arg1, typename Arg2, typename Arg3> forceinline Meta::EnableIf<
	Meta::IsCallable<F, Arg1, Arg2, Arg3>::_,
TFinally3<F, Arg1, Arg2, Arg3>> Finally(F&& f, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
{return {Cpp::Forward<F>(f), Cpp::Forward<Arg1>(arg1), Cpp::Forward<Arg2>(arg2), Cpp::Forward<Arg3>(arg3)};}

}
using Utils::Finally;

}

INTRA_WARNING_POP
