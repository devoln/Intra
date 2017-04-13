#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Meta/TypeList.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

namespace Intra { namespace Meta {

template<typename... Args> struct Tuple;


namespace D {
template<int...> struct index_tuple {};
template<int I, typename IndexTuple, typename... Types> struct make_indexes_impl;

template<int I, int... Indexes, typename T, typename ... Types>
struct make_indexes_impl<I, index_tuple<Indexes...>, T, Types...>
{
	typedef typename make_indexes_impl<I+1, index_tuple<Indexes..., I>, Types...>::_ _;
};
template<int I, int... Indexes> struct make_indexes_impl<I, index_tuple<Indexes...>> {typedef index_tuple<Indexes...> _;};
template<typename... Types> struct make_indexes: make_indexes_impl<0, index_tuple<>, Types...> {};
}


template<typename ...Args> struct Tuple;
template<> struct Tuple<>
{
	typedef TypeList<> TL;
	enum {ElementCount=0};
};


template<typename H, typename... Args> struct Tuple<H, Args...>
{
	typedef Tuple<Args...> NextTuple;
	typedef TypeList<H, Args...> TL;
	enum {ElementCount = 1+sizeof...(Args)};

	template<typename H1, typename H2, typename... Args1> constexpr Tuple(H1&& h, H2&& h2, Args1&&... args):
		first(Meta::Forward<H1>(h)), next(Meta::Forward<H2>(h2), Meta::Forward<Args1>(args)...) {}

	constexpr Tuple(const Tuple& rhs) = default;

	template<typename H1> constexpr explicit Tuple(H1&& h, Tuple<Args...>&& args):
		first(Meta::Forward<H>(h)), next(Meta::Forward<Tuple<Args...>>(args)) {}

	constexpr Tuple(): first() {}

	bool operator==(const Tuple<H, Args...>& rhs) {return first==rhs.first && next==rhs.next;}
	bool operator!=(const Tuple<H, Args...>& rhs) {return !operator==(rhs);}

	H first;
	NextTuple next;

public:
};


template<typename H> struct Tuple<H>
{
private:
	typedef RemoveConstRef<H> HNoCR;
public:
	typedef Tuple<> NextTuple;
	typedef TypeList<H> TL;
	enum {ElementCount=1};

	template<typename H1> constexpr Tuple(const Tuple<H1>& h): first(h.first) {}

	constexpr Tuple(HNoCR&& h): first(Meta::Move(h)) {}
	constexpr Tuple(const HNoCR& h): first(h) {}

	constexpr Tuple(const Tuple& rhs) = default;
	//constexpr Tuple(const H& h, const Tuple<>&): first(h) {}
	//constexpr Tuple(H&& h, Tuple<>&&): first(Meta::Forward<H>(h)) {}
	constexpr Tuple(): first() {}
	//~Tuple() {first.~H();}

	bool operator==(const Tuple<H>& rhs) {return first==rhs.first;}
	bool operator!=(const Tuple<H>& rhs) {return !operator==(rhs);}

	H first;
};

template<typename... Args> forceinline Tuple<Args...> TupleL(Args&&... args)
{return Tuple<Args...>(Meta::Forward<Args>(args)...);}

}


}

INTRA_WARNING_POP
