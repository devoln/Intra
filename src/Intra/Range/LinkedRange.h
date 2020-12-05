#pragma once

#include "Intra/Concepts.h"

INTRA_BEGIN
template<typename T> struct LinkedNode
{
	LinkedNode* Next;
	T Value;

	[[nodiscard]] constexpr LinkedNode* NextListNode() const {return Next;}
};

template<typename T, typename NodeType> struct LinkedRange
{
	using Node = NodeType;

	LinkedRange() = default;

	constexpr LinkedRange(Node* first): FirstNode(first) {}

	[[nodiscard]] constexpr bool Empty() const {return FirstNode == nullptr;}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		FirstNode = FirstNode->NextListNode();
	}

	[[nodiscard]] constexpr T& First() const
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(CSame<T, Node>) return *FirstNode;
		else FirstNode->Value;
	}

	[[nodiscard]] constexpr bool operator==(const LinkedRange& rhs) const {return FirstNode == rhs.FirstNode;}

	Node* FirstNode = nullptr;
};


template<typename T> struct BidirectionalLinkedNode
{
	BidirectionalLinkedNode* Prev;
	BidirectionalLinkedNode* Next;
	T Value;

	[[nodiscard]] BidirectionalLinkedNode* PrevListNode() const {return Prev;}
	[[nodiscard]] BidirectionalLinkedNode* NextListNode() const {return Next;}
};

template<typename T, typename NodeType> struct BidirectionalLinkedRange: LinkedRange<T, NodeType>
{
	using BASE = LinkedRange<T, NodeType>;
	using Node = NodeType;

	BidirectionalLinkedRange() = default;

	constexpr BidirectionalLinkedRange(Node* first, Node* last): BASE(first), LastNode(last) {}

	using BASE::Empty;
	using BASE::First;

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		const bool last = BASE::FirstNode == LastNode;
		BASE::PopFirst();
		if(last) LastNode = BASE::FirstNode = nullptr;
	}

	constexpr void PopLast()
	{
		INTRA_PRECONDITION(!Empty());
		const bool last = FirstNode == LastNode;
		LastNode = LastNode->PrevListNode();
		if(last) LastNode = BASE::FirstNode = nullptr;
	}

	[[nodiscard]] constexpr T& Last() const
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(CSame<T, Node>) return *LastNode;
		else LastNode->Value;
	}

	[[nodiscard]] constexpr bool operator==(const BidirectionalLinkedRange& rhs) const
	{
		return BASE::FirstNode == rhs.FirstNode && LastNode == rhs.LastNode;
	}

private:
	Node* LastNode = null;
};

INTRA_END
