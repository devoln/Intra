#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Core/Debug.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"
#include "Range/ForwardDecls.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

namespace Intra { namespace Range {

INTRA_DEFINE_EXPRESSION_CHECKER(HasNextListNodeMethod, ::Intra::Meta::Val<T>().NextListNode());

template<typename T> struct FListNode
{
	FListNode* Next;
	T Value;

	FListNode* NextListNode() const {return Next;}
};

template<typename T, typename Node = Meta::SelectType<T, FListNode<T>, HasNextListNodeMethod<T&>::_>>
struct FListRange
{
	forceinline FListRange(null_t=null):
		FirstNode(null) {}

	forceinline FListRange(Node* first):
		FirstNode(first) {}

	forceinline bool Empty() const {return FirstNode==null;}

	forceinline void PopFirst()
	{
		INTRA_ASSERT(!Empty());
		FirstNode = FirstNode->NextListNode();
	}

	template<typename U=T> forceinline Meta::EnableIf<
		Meta::TypeEquals<U, Node>::_,
	T&> First() const
	{
		INTRA_ASSERT(!Empty());
		return *FirstNode;
	}

	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::TypeEquals<U, Node>::_,
	T&> First() const
	{
		INTRA_ASSERT(!Empty());
		return FirstNode->Value;
	}

	forceinline bool operator==(const FListRange& rhs) const
	{return FirstNode==rhs.FirstNode;}

	Node* FirstNode;
};

template<typename T> struct BListRange
{
	struct Node
	{
		Node* Prev;
		Node* Next;
		T Value;
	};

	forceinline BListRange(null_t=null):
		FirstNode(null), LastNode(null) {}

	forceinline BListRange(Node* first, Node* last):
		FirstNode(first), LastNode(last) {}

	forceinline bool Empty() const
	{return FirstNode==null || LastNode==null;}

	forceinline void PopFirst()
	{
		INTRA_ASSERT(!Empty());
		FirstNode = FirstNode->Next;
	}

	forceinline void PopLast()
	{
		INTRA_ASSERT(!Empty());
		LastNode = LastNode->Prev;
	}

	forceinline T& First() const
	{
		INTRA_ASSERT(!Empty());
		return FirstNode->Value;
	}

	forceinline T& Last() const
	{
		INTRA_ASSERT(!Empty());
		return LastNode->Value;
	}

	forceinline void Put(const T& value) {First()=value; PopFirst();}

	forceinline const BListRange<const T>& AsConstRange() const
	{return *reinterpret_cast<const BListRange<const T>*>(this);}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator==(const BListRange& rhs) const
	{return FirstNode==rhs.FirstNode && LastNode==rhs.LastNode;}

	Node* FirstNode;
	Node* LastNode;
};

}

using Range::FListRange;
using Range::BListRange;

}
INTRA_WARNING_POP
