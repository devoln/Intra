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
	virtual R operator()(Args... args) const = 0;
};

template<typename FuncSignature> class ICopyableFunctor: public IFunctor<FuncSignature>
{
public:
	virtual ICopyableFunctor* Clone() const = 0;
};


template<typename FuncSignature> class IMutableFunctor;
template<typename R, typename... Args> class IMutableFunctor<R(Args...)>
{
public:
	virtual ~IMutableFunctor() {}
	virtual R operator()(Args... args) = 0;
};

template<typename FuncSignature> class ICopyableMutableFunctor: public IMutableFunctor<FuncSignature>
{
public:
	virtual ICopyableMutableFunctor* Clone() const = 0;
};


template<typename FuncSignature, typename T=FuncSignature*> class Functor;
template<typename T, typename R, typename... Args>
class Functor<R(Args...), T>: public IFunctor<R(Args...)>
{
public:
	Functor(T&& obj): Obj(Cpp::Move(obj)) {}
	Functor(const T& obj): Obj(obj) {}
	R operator()(Args... args) const final {return call(Cpp::Forward<Args>(args)...);}
	T Obj;

private:
	template<typename U=R> forceinline Meta::EnableIf<
		!Meta::TypeEquals<U, void>::_,
	R> call(Args&&... args) const {return Obj(Cpp::Forward<Args>(args)...);}

	template<typename U=R> forceinline Meta::EnableIf<
		Meta::TypeEquals<U, void>::_,
	R> call(Args&&... args) const {Obj(Cpp::Forward<Args>(args)...);}
};

template<typename FuncSignature, typename T = FuncSignature*> class CopyableFunctor;
template<typename T, typename R, typename... Args>
class CopyableFunctor<R(Args...), T>: public ICopyableFunctor<R(Args...)>
{
public:
	CopyableFunctor(T&& obj): Obj(Cpp::Move(obj)) {}
	CopyableFunctor(const T& obj): Obj(obj) {}
	ICopyableFunctor<R(Args...)>* Clone() const final {return new CopyableFunctor(Obj);}
	R operator()(Args... args) const final {return call(Cpp::Forward<Args>(args)...);}
	T Obj;

	
private:
	template<typename U=R> forceinline Meta::EnableIf<
		!Meta::TypeEquals<U, void>::_,
	R> call(Args&&... args) const {return Obj(Cpp::Forward<Args>(args)...);}

	template<typename U=R> forceinline Meta::EnableIf<
		Meta::TypeEquals<U, void>::_,
	R> call(Args&&... args) const {Obj(Cpp::Forward<Args>(args)...);}
};


template<typename FuncSignature, typename T=FuncSignature*> class MutableFunctor;
template<typename T, typename R, typename... Args>
class MutableFunctor<R(Args...), T>: public IMutableFunctor<R(Args...)>
{
public:
	MutableFunctor(T&& obj): Obj(Cpp::Move(obj)) {}
	MutableFunctor(const T& obj): Obj(obj) {}
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

template<typename FuncSignature, typename T = FuncSignature*> class CopyableMutableFunctor;
template<typename T, typename R, typename... Args>
class CopyableMutableFunctor<R(Args...), T>: public ICopyableMutableFunctor<R(Args...)>
{
public:
	CopyableMutableFunctor(T&& obj): Obj(Cpp::Move(obj)) {}
	CopyableMutableFunctor(const T& obj): Obj(obj) {}
	ICopyableMutableFunctor<R(Args...)>* Clone() const final {return new CopyableMutableFunctor(Obj);}
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

}}

INTRA_WARNING_POP
