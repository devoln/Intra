#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"

#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

namespace Intra { namespace Funal {

template<typename FuncSignature> class IFunctor;
template<typename R, typename... Args> class IFunctor<R(Args...)>
{
public:
	virtual ~IFunctor() {}
	virtual R operator()(Args&&... args) = 0;
};

template<typename FuncSignature> class ICopyableFunctor: public IFunctor<FuncSignature>
{
public:
	virtual ICopyableFunctor* Clone() = 0;
};

template<typename FuncSignature, typename T=FuncSignature*> class Functor;
template<typename T, typename R, typename... Args>
class Functor<R(Args...), T>: public IFunctor<R(Args...)>
{
public:
	Functor(T&& obj): Obj(Cpp::Move(obj)) {}
	Functor(const T& obj): Obj(obj) {}
	R operator()(Args... args) final {return call(Cpp::Forward<Args>(args)...);}
	T Obj;

private:
	template<typename U=R> forceinline Meta::EnableIf<
		!Meta::TypeEquals<U, void>::_,
	R> call(Args&&... args) {return Obj(Cpp::Forward<Args>(args)...);}

	template<typename U=R> forceinline Meta::EnableIf<
		Meta::TypeEquals<U, void>::_,
	R> call(Args&&... args) {Obj(Cpp::Forward<Args>(args)...);}
};

template<typename FuncSignature, typename T = FuncSignature*> class CopyableFunctor;
template<typename T, typename R, typename... Args>
class CopyableFunctor<R(Args...), T>: public ICopyableFunctor<R(Args...)>
{
public:
	CopyableFunctor(T&& obj): Obj(Cpp::Move(obj)) {}
	CopyableFunctor(const T& obj): Obj(obj) {}
	ICopyableFunctor<R(Args...)>* Clone() final {return new CopyableFunctor(Obj);}
	R operator()(Args&&... args) final {return Obj(Cpp::Forward<Args>(args)...);}
	T Obj;
};

}}

INTRA_WARNING_POP
