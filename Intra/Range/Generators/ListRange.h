#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Utils/Debug.h"
#include "Meta/Type.h"
#include "Concepts/Range.h"
#include "Range/ForwardDecls.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

namespace Intra { namespace Range {

template<typename T> struct FListNode
{
	FListNode* Next;
	T Value;

	forceinline FListNode* NextListNode() const {return Next;}
};

template<typename T, typename NodeType> struct FListRange
{
	typedef NodeType Node;

	forceinline FListRange(null_t=null):
		FirstNode(null) {}

	forceinline FListRange(Node* first):
		FirstNode(first) {}

	forceinline bool Empty() const {return FirstNode==null;}

	forceinline void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		FirstNode = FirstNode->NextListNode();
	}

	template<typename U=T> forceinline Meta::EnableIf<
		Meta::TypeEquals<U, Node>::_,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *FirstNode;
	}

	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::TypeEquals<U, Node>::_,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return FirstNode->Value;
	}

	forceinline bool operator==(const FListRange& rhs) const
	{return FirstNode==rhs.FirstNode;}

	Node* FirstNode;
};

template<typename T> Meta::EnableIf<
	!Concepts::IsInputRange<T>::_ &&
	Concepts::HasNextListNodeMethod<T>::_/* &&
	!HasPrevListNodeMethod<T>::_*/,
FListRange<T, T>> RangeOf(T& objectWithIntrusiveList)
{return FListRange<T, T>(&objectWithIntrusiveList);}


template<typename T> struct BListNode
{
	BListNode* Prev;
	BListNode* Next;
	T Value;

	forceinline BListNode* PrevListNode() const {return Prev;}
	forceinline BListNode* NextListNode() const {return Next;}
};

template<typename T, typename NodeType = BListNode<T>> struct BListRange
{
	typedef NodeType Node;

	forceinline BListRange(null_t=null):
		FirstNode(null), LastNode(null) {}

	forceinline BListRange(Node* first, Node* last):
		FirstNode(first), LastNode(last) {}

	forceinline bool Empty() const
	{
		INTRA_DEBUG_ASSERT((FirstNode==null) == (LastNode==null));
		return FirstNode==null || LastNode==null;
	}

	forceinline void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		FirstNode = FirstNode->NextListNode();
	}

	forceinline void PopLast()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		LastNode = LastNode->PrevListNode();
	}

	template<typename U=T> forceinline Meta::EnableIf<
		Meta::TypeEquals<U, Node>::_,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *FirstNode;
	}

	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::TypeEquals<U, Node>::_,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return FirstNode->Value;
	}

	template<typename U=T> forceinline Meta::EnableIf<
		Meta::TypeEquals<U, Node>::_,
	T&> Last() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *LastNode;
	}

	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::TypeEquals<U, Node>::_,
	T&> Last() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return LastNode->Value;
	}

	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::TypeEquals<U, Node>::_,
	const BListRange<const T, BListNode<const T>>&> AsConstRange() const
	{return *reinterpret_cast<const BListRange<const T, BListNode<const T>>*>(this);}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator==(const BListRange& rhs) const
	{return FirstNode==rhs.FirstNode && LastNode==rhs.LastNode;}

	Node* FirstNode;
	Node* LastNode;
};

/*template<typename T> Meta::EnableIf<
	!IsInputRange<T>::_ &&
	HasNextListNodeMethod<T>::_ &&
	HasPrevListNodeMethod<T>::_,
BListRange<T, T>> RangeOf(T& objectWithIntrusiveList)
{return BListRange<T, T>(&objectWithIntrusiveList);}*/

}

using Range::FListRange;
using Range::BListRange;

}
INTRA_WARNING_POP
