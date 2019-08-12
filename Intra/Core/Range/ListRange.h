#pragma once

#include "Core/Assert.h"
#include "Core/Type.h"
#include "Core/Range/Concepts.h"


INTRA_CORE_RANGE_BEGIN
template<typename T> struct FListNode
{
	FListNode* Next;
	T Value;

	INTRA_NODISCARD constexpr forceinline FListNode* NextListNode() const {return Next;}
};

template<typename T, typename NodeType> struct FListRange
{
	typedef NodeType Node;

	FListRange() = default;
	constexpr forceinline FListRange(null_t) {}

	constexpr forceinline FListRange(Node* first): FirstNode(first) {}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return FirstNode == null;}

	INTRA_CONSTEXPR2 forceinline void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		FirstNode = FirstNode->NextListNode();
	}

	template<typename U=T> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
		CSame<U, Node>,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *FirstNode;
	}

	template<typename U=T> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
		!CSame<U, Node>,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return FirstNode->Value;
	}

	INTRA_NODISCARD constexpr forceinline bool operator==(const FListRange& rhs) const
	{return FirstNode == rhs.FirstNode;}

	Node* FirstNode = null;
};

template<typename T> Requires<
	!CInputRange<T> &&
	CHasNextListNodeMethod<T>/* &&
	!CHasPrevListNodeMethod<T>*/,
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

template<typename T, typename NodeType> struct BListRange
{
	typedef NodeType Node;

	BListRange() = default;
	constexpr forceinline BListRange(null_t) {}

	constexpr forceinline BListRange(Node* first, Node* last):
		FirstNode(first), LastNode(last) {}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline bool Empty() const
	{
		INTRA_DEBUG_ASSERT((FirstNode == null) == (LastNode == null));
		return FirstNode == null || LastNode == null;
	}

	INTRA_CONSTEXPR2 forceinline void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		FirstNode = FirstNode->NextListNode();
	}

	INTRA_CONSTEXPR2 forceinline void PopLast()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		LastNode = LastNode->PrevListNode();
	}

	template<typename U=T> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
		CSame<U, Node>,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *FirstNode;
	}

	template<typename U=T> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
		!CSame<U, Node>,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return FirstNode->Value;
	}

	template<typename U=T> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
		CSame<U, Node>,
	T&> Last() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *LastNode;
	}

	template<typename U=T> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
		!CSame<U, Node>,
	T&> Last() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return LastNode->Value;
	}

	template<typename U=T> INTRA_NODISCARD forceinline Requires<
		!CSame<U, Node>,
	const BListRange<const T, BListNode<const T>>&> AsConstRange() const
	{return *reinterpret_cast<const BListRange<const T, BListNode<const T>>*>(this);}

	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const {return Empty();}
	INTRA_NODISCARD constexpr forceinline bool operator==(const BListRange& rhs) const
	{return FirstNode == rhs.FirstNode && LastNode == rhs.LastNode;}

	Node* FirstNode = null;
	Node* LastNode = null;
};

/*template<typename T> Requires<
	!IsInputRange<T>::_ &&
	HasNextListNodeMethod<T>::_ &&
	HasPrevListNodeMethod<T>::_,
BListRange<T, T>> RangeOf(T& objectWithIntrusiveList)
{return BListRange<T, T>(&objectWithIntrusiveList);}*/
INTRA_CORE_RANGE_END
