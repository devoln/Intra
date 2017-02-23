#pragma once

#include "Container/Concepts.h"

namespace Intra { namespace Container {

template<typename C> Meta::EnableIf<
	Has_reserve<C>::_
> Reserve(C& container, size_t capacity) {container.reserve(capacity);}

template<typename C> Meta::EnableIf<
	!Has_reserve<C>::_
> Reserve(C&, size_t) {}

template<typename C> Meta::EnableIf<
	HasSetCountUninitialized<C>::_
> SetCountTryNotInit(C& container, size_t newCount) {container.SetCountUninitialized(newCount);}

template<typename C> Meta::EnableIf<
	!HasSetCountUninitialized<C>::_ && Has_resize<C>::_
> SetCountTryNotInit(C& container, size_t newCount) {container.resize(newCount);}

}}
