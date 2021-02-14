#pragma once

#include "Intra/Core.h"

namespace Intra { INTRA_BEGIN
class IDestructible
{
public:
	INTRA_CONSTEXPR_DESTRUCTOR virtual ~IDestructible() {}
};

namespace z_D {
template<class ParentInterface> class INTRA_NOVTABLE ICloneConstructible: public ParentInterface
{
public:
	virtual ICloneConstructible* CloneConstruct(ICloneConstructible* dst, int dstMaxSize, bool move) = 0;
};

template<typename TFor, typename TParent> class ImplCloneConstructible: public TParent
{
public:
	TFor* CloneConstruct(ICloneConstructible* dst, int dstMaxSize, bool move)
	{
		if(sizeof(TFor) < dstMaxSize) return nullptr;
		if(move) return new(Construct, dst) TFor(static_cast<TFor&&>(*this));
		return new(Construct, dst) TFor(static_cast<const TFor&>(*this));
	}
};

template<class ParentInterface> INTRA_NOVTABLE class ICloneable: public ParentInterface
{
public:
	virtual ICloneable* Clone() const = 0;
};

template<typename TFor, typename TParent> class ImplCloneable: public TParent
{
public:
	TFor* Clone() const final {return new TFor(static_cast<const TFor&>(*this));}
};

template<typename FuncSignature, class ParentInterface> class IFunctor;
template<bool Cloneable, typename R, typename... Args> class INTRA_NOVTABLE IFunctor<R(Args...)>: public ParentInterface
{
public:
	constexpr virtual R operator()(Args... args) = 0;
};

template<typename FuncSignature, typename T, class TParent> class FunctorPolymorphicWrapper;
template<class TParent, typename T, typename R, typename... Args>
class FunctorPolymorphicWrapper<R(Args...), T, Cloneable>: public TSelect<
		ImplCloneable<
			FunctorPolymorphicWrapper<R(Args...), T, Cloneable>,
			IFunctor<R(Args...), true>
		>,
		IFunctor<R(Args...), false>,
	Cloneable>
{
public:
	template<typename... Ts> constexpr FunctorPolymorphicWrapper(Ts&&... constructArgs): Obj(INTRA_FWD(constructArgs)...) {}
	INTRA_CONSTEXPR_VIRTUAL R operator()(Args... args) final {return static_cast<R>(Obj(INTRA_FWD(args)...));}
	T Obj;
};
}

} INTRA_END
