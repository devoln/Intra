#pragma once

#include "Intra/Assert.h"
#include "Intra/Type.h"
#include "Intra/Range/Concepts.h"

INTRA_BEGIN
template<typename T> struct FListNode
{
	FListNode* Next;
	T Value;

	[[nodiscard]] constexpr FListNode* NextListNode() const {return Next;}
};

template<typename T, typename NodeType> struct FListRange
{
	typedef NodeType Node;

	FListRange() = default;
	constexpr FListRange(decltype(null)) {}

	constexpr FListRange(Node* first): FirstNode(first) {}

	[[nodiscard]] constexpr bool Empty() const {return FirstNode == null;}

	constexpr void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		FirstNode = FirstNode->NextListNode();
	}

	template<typename U=T> [[nodiscard]] constexpr Requires<
		CSame<U, Node>,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *FirstNode;
	}

	template<typename U=T> [[nodiscard]] constexpr Requires<
		!CSame<U, Node>,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return FirstNode->Value;
	}

	[[nodiscard]] constexpr bool operator==(const FListRange& rhs) const
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

	INTRA_FORCEINLINE BListNode* PrevListNode() const {return Prev;}
	INTRA_FORCEINLINE BListNode* NextListNode() const {return Next;}
};

template<typename T, typename NodeType> struct BListRange
{
	typedef NodeType Node;

	BListRange() = default;
	constexpr BListRange(decltype(null)) {}

	constexpr BListRange(Node* first, Node* last):
		FirstNode(first), LastNode(last) {}

	[[nodiscard]] constexpr bool Empty() const
	{
		INTRA_DEBUG_ASSERT((FirstNode == null) == (LastNode == null));
		return FirstNode == null || LastNode == null;
	}

	constexpr void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		FirstNode = FirstNode->NextListNode();
	}

	constexpr void PopLast()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		LastNode = LastNode->PrevListNode();
	}

	template<typename U=T> [[nodiscard]] constexpr Requires<
		CSame<U, Node>,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *FirstNode;
	}

	template<typename U=T> [[nodiscard]] constexpr Requires<
		!CSame<U, Node>,
	T&> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return FirstNode->Value;
	}

	template<typename U=T> [[nodiscard]] constexpr Requires<
		CSame<U, Node>,
	T&> Last() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *LastNode;
	}

	template<typename U=T> [[nodiscard]] constexpr Requires<
		!CSame<U, Node>,
	T&> Last() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return LastNode->Value;
	}

	template<typename U=T> [[nodiscard]] INTRA_FORCEINLINE Requires<
		!CSame<U, Node>,
	const BListRange<const T, BListNode<const T>>&> AsConstRange() const
	{return *reinterpret_cast<const BListRange<const T, BListNode<const T>>*>(this);}

	[[nodiscard]] constexpr bool operator==(decltype(null)) const {return Empty();}
	[[nodiscard]] constexpr bool operator==(const BListRange& rhs) const
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
INTRA_END
