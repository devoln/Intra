#pragma once

#include "Type.h"


namespace Intra { namespace Meta {

namespace detail {struct NoType {};}

template<typename... Args>
struct TypeList
{
	typedef detail::NoType Head;
	typedef detail::NoType Tail;
};

typedef TypeList<> EmptyTypeList;

template<typename H, typename ...T>
struct TypeList<H, T...>
{
	typedef H Head;
	typedef TypeList<T...> Tail;
};

template<typename TL> struct TypeListIsEmpty: TypeFromValue<bool, true> {};
template<> struct TypeListIsEmpty<TypeList<detail::NoType, detail::NoType>>:
	TypeFromValue<bool, true> {};

template<typename ...Args> struct TypeListIsEmpty<TypeList<Args...>>:
	TypeFromValue<bool, TypeEquals<typename TypeList<Args...>::Head, detail::NoType>::_ &&
	TypeListIsEmpty<typename TypeList<Args...>::Tail>::_> {};


template<typename T, typename TL> struct TypeListContains: TypeFromValue<bool, false> {};

template<typename... Args> struct TypeListContains<detail::NoType, Args...>: TypeFromValue<bool, false> {};

template<typename T, typename ...Args>
struct TypeListContains<T, TypeList<Args...>>:
	TypeFromValue<bool, TypeEquals<typename TypeList<Args...>::Head, T>::_ ||
	    TypeListContains<T, typename TypeList<Args...>::Tail>::_> {};


template<typename TL> struct TypeListLength: TypeFromValue<uint, 0> {};

template<typename... Args> struct TypeListLength<TypeList<Args...>>:
	TypeFromValue<uint,
	    TypeListIsEmpty<TypeList<Args...>>::_? 0:
	        1+TypeListLength<typename TypeList<Args...>::Tail>::_> {};


namespace detail {
template<uint N, typename TL> struct TypeListAt {typedef NoType type;};

template<typename... Args> struct TypeListAt<0, TypeList<Args...>>
{
	typedef typename TypeList<Args...>::Head type;
};

template<uint N, typename... Args> struct TypeListAt<N, TypeList<Args...>>
{
	static_assert(N < TypeListLength<TypeList<Args...>>::_, "N is too big");
	typedef typename TypeListAt<N-1, typename TypeList<Args...>::Tail>::type type;
};

}

template<uint N, typename TL> using TypeListAt = typename Meta::detail::TypeListAt<N, TL>::type;


namespace detail
{
template<typename TOrTL2, typename TL> struct TypeListAppend {};

template<typename T, typename ...Args> struct TypeListAppend<T, TypeList<Args...>>
{
	typedef TypeList<Args..., T> type;
};
}

template<typename TL, typename... Args2>
using TypeListAppend = typename Meta::detail::TypeListAppend<TL, Args2...>::type;


namespace detail {

template<typename T, typename TL> struct TypeListAdd {};

template<typename T, typename ...Args> struct TypeListAdd<T, TypeList<Args...>>
{
	typedef TypeList<Args..., T> type;
};

}

template<typename T, typename ...Args>
using TypeListAdd = typename Meta::detail::TypeListAdd<T, Args...>::type;


namespace detail
{
template<typename TOrTL2, typename TL> struct RemoveType {};

template<typename T, typename ...Args> struct RemoveType<T, TypeList<Args...>>
{
private:
	typedef typename RemoveType<T, typename TypeList<Args...>::Tail>::type Removed;
	typedef typename TypeList<Args...>::Head Head;

public:
	typedef SelectType<Removed, TypeListAppend<Removed, TypeList<Head>>, TypeEquals<Head, T>::_> type;
};

template<typename T, typename Head> struct RemoveType<T, TypeList<Head>>
{
	typedef SelectType<EmptyTypeList, TypeList<Head>, TypeEquals<Head, T>::_> type;
};

template<typename T> struct RemoveType<T, EmptyTypeList>
{
	typedef EmptyTypeList type;
};
}

template<typename T, typename TL>
using TypeListRemoveType = typename Meta::detail::RemoveType<T, TL>::type;



namespace detail {
template<typename TL> struct RemoveDuplicates {};

template<> struct RemoveDuplicates<EmptyTypeList>
{
	typedef EmptyTypeList type;
};

template<typename... Args> struct RemoveDuplicates<TypeList<Args...>>
{
private:
	typedef TypeList<Args...> TL;
	typedef TypeListRemoveType<typename TL::Head, typename TL::Tail> HeadRemovedFromTail;
	typedef typename RemoveDuplicates<HeadRemovedFromTail>::type TailWithoutDuplicates;
public:
	typedef TypeListAppend<TailWithoutDuplicates, TypeList<typename TL::Head>> type;
};
}

template<typename TL> using TypeListRemoveDuplicates = typename Meta::detail::RemoveDuplicates<TL>::type;





struct Constants
{
	typedef TypeFromValue<uint, uint_MAX> npos;
};


namespace detail {

template<typename T, uint IndexFrom, typename TL>
struct TypeListFindHelper: TypeFromValue<uint, 0> {};

template<typename T, uint IndexFrom>
struct TypeListFindHelper<T, IndexFrom, EmptyTypeList>:
	TypeFromValue<uint, 0> {};

template<typename T, uint IndexFrom, typename... Args>
struct TypeListFindHelper<T, IndexFrom, TypeList<Args...>>:
	TypeFromValue<uint,
		TypeEquals<typename TypeList<Args...>::Head, T>::_?
		IndexFrom:
		IndexFrom + 1 + TypeListFindHelper<T, IndexFrom, typename TypeList<Args...>::Tail>::_> {};

}

template<typename T, typename TL> struct TypeListFind {};

template<typename T> struct TypeListFind<T, EmptyTypeList>: Constants::npos {};

template<typename ...Args> struct TypeListFind<detail::NoType, TypeList<Args...>>: Constants::npos {};

template<typename T, typename ...Args> struct TypeListFind<T, TypeList<Args...>>:
	TypeFromValue<uint, TypeListContains<T, TypeList<Args...>>::_?
	detail::TypeListFindHelper<T, 0, TypeList<Args...>>::_:
	    Constants::npos::_> {};


namespace detail
{
template<uint IndexBegin, uint IndexEnd, typename TL> struct SliceHelper {};

template<uint IndexBegin, uint IndexEnd> struct SliceHelper<IndexBegin, IndexEnd, EmptyTypeList>
{
	typedef EmptyTypeList type;
};

template<uint IndexBegin, typename ...Args> struct SliceHelper<IndexBegin, IndexBegin, TypeList<Args...>>
{
	typedef TypeList<Meta::TypeListAt<IndexBegin, TypeList<Args...>>> type;
};

template<uint IndexBegin, uint IndexEnd, typename ...Args> struct SliceHelper<IndexBegin, IndexEnd, TypeList<Args...>>
{
private:
	static_assert(IndexEnd >= IndexBegin, "Invalid range");
	typedef TypeList<Args...> TL;
public:
	typedef TypeListAdd<
		Meta::TypeListAt<IndexEnd, TL>,
		typename SliceHelper<IndexBegin, IndexEnd - 1, TL>::type
	> type;
};

template<uint IndexBegin, uint IndexAfterEnd, typename TL> struct Slice {};

template<uint IndexBegin, uint IndexEnd, typename ...Args> struct Slice<IndexBegin, IndexEnd, TypeList<Args...>>
{
	typedef typename SliceHelper<IndexBegin, IndexEnd, TypeList<Args...>>::type type;
};
}

template<uint IndexBegin, uint IndexAfterEnd, typename TL>
using TypeListSlice = typename Meta::detail::Slice<IndexBegin, IndexAfterEnd, TL>::type;


namespace detail
{
template<uint Index, typename TL> struct TypeListCutTo {};

template<uint Index, typename... Args> struct TypeListCutTo<Index, TypeList<Args...>>
{
	typedef Meta::TypeListSlice<0, Index, TypeList<Args...>> type;
};


template<uint Index, typename TL> struct TypeListCutFrom {};

template<uint Index, typename... Args> struct TypeListCutFrom<Index, TypeList<Args...>>
{
private:
	typedef TypeList<Args...> TL;
public:
	typedef TypeListSlice<Index, TypeListLength<TL>::_-1, TL> type;
};

}

template<uint Index, typename TL> using TypeListCutTo = typename Meta::detail::TypeListCutTo<Index, TL>::type;
template<uint Index, typename TL> using TypeListCutFrom = typename Meta::detail::TypeListCutFrom<Index, TL>::type;


#if INTRA_DISABLED
namespace detail
{
template<uint Index, typename NewValue, typename TL> struct TypeListReplace {};

template<typename NewValue, typename... Args> struct TypeListReplace<0, NewValue, TypeList<Args...>>
{
	typedef TypeListAppend<typename TypeList<Args...>::Tail, TypeList<NewValue>> type;
};

template<uint Index, typename NewValue, typename ...Args>
struct TypeListReplace<Index, NewValue, TypeList<Args...>>
{
private:
	typedef TypeList<Args...> TL;
	typedef TypeFromValue<bool, Index == TypeListLength<TL>::_-1> AtEndWorkAround;

public:
	typedef SelectType<
		typename ReplaceEnd<NewValue, TL>::type,
		typename ReplaceMiddle<AtEndWorkAround::_, Index, NewValue, TL>::type,
		AtEndWorkAround::_
	> type;
};
}

template<uint Index, typename NewValue, typename TL>
using TypeListReplace = typename Meta::detail::TypeListReplace<Index, NewValue, TL>::type;


namespace detail
{
template<bool NotFoundWorkaround, typename OldValue, typename NewValue, typename TL> struct ReplaceTypeHelper
{
	typedef EmptyTypeList type;
};

template<typename OldValue, typename NewValue, typename ...Args> struct ReplaceTypeHelper<false, OldValue, NewValue, TypeList<Args...>>
{
private:
	typedef TypeList<Args...> TL;
public:
	typedef TypeListReplace<TypeListFind<OldValue, TL>::_, NewValue, TL> type;
};


template<typename OldValue, typename NewValue, typename TL> struct ReplaceType {};

template<typename OldValue, typename NewValue, typename... Args> struct ReplaceType<OldValue, NewValue, TypeList<Args...>>
{
private:
	typedef TypeList<Args...> TL;
	typedef TypeFromValue<bool, TypeListFind<OldValue, TL>::_ == Constants::npos::_> NotFound;
public:
	typedef SelectType<TL, typename Meta::detail::ReplaceTypeHelper<NotFound::value, OldValue, NewValue, TL, NotFound::value>::type> type;
};
}

template<typename OldValue, typename NewValue, typename TL>
using TypeListReplaceType = typename Meta::detail::ReplaceType<OldValue, NewValue, TL>::type;
#endif

}

}

