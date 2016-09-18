#pragma once

#include "Memory/Allocator.h"
#include "Memory/AllocatorInterface.h"
#include "Algorithms/Range.h"
#include "Containers/ForwardDeclarations.h"

namespace Intra {

template<typename T> struct FListRange:
	Range::RangeMixin<FListRange<T>, T, Range::TypeEnum::Forward, true>
{
	struct Node
	{
		Node* Next;
		T Value;
	};

	forceinline FListRange(null_t=null): FirstNode(null), LastNode(null) {}
	forceinline FListRange(Node* first, Node* last): FirstNode(first), LastNode(last) {}

	forceinline bool Empty() const
	{
		INTRA_ASSERT((FirstNode==null) == (LastNode==null));
		return FirstNode==LastNode;
	}

	forceinline void PopFirst()
	{
		INTRA_ASSERT(!Empty());
		FirstNode = FirstNode->Next;
	}

	forceinline T& First() const
	{
		INTRA_ASSERT(!Empty());
		return FirstNode->Value;
	}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}
	forceinline bool operator==(const FListRange& rhs) const {return FirstNode==rhs.FirstNode && LastNode==rhs.LastNode;}
	forceinline bool operator!=(const FListRange& rhs) const {return !operator==(rhs);}

	Node* FirstNode;
	Node* LastNode;
};

template<typename T> struct BListRange:
	Range::RangeMixin<BListRange<T>, T, Range::TypeEnum::Bidirectional, true>
{
	typedef T& return_value_type;

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

	struct Node
	{
		Node* Prev;
		Node* Next;
		T Value;
	};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

	forceinline BListRange(null_t=null): FirstNode(null), LastNode(null) {}
	forceinline BListRange(Node* first, Node* last): FirstNode(first), LastNode(last) {}

	forceinline bool Empty() const
	{
		INTRA_ASSERT((FirstNode==null) == (LastNode==null));
		return FirstNode==LastNode;
	}

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

	forceinline const BListRange<const T>& AsConstRange() const {return *reinterpret_cast<const BListRange<const T>*>(this);}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}
	forceinline bool operator==(const BListRange& rhs) const {return FirstNode==rhs.FirstNode && LastNode==rhs.LastNode;}
	forceinline bool operator!=(const BListRange& rhs) const {return !operator==(rhs);}

private:
	template<typename U, typename Allocator> friend class BList;
	Node* FirstNode;
	Node* LastNode;
};

template<typename T, typename AllocatorType> class BList:
	public Memory::AllocatorRef<AllocatorType>
{
	typedef Memory::AllocatorRef<AllocatorType> AllocatorRef;
public:
	typedef T value_type;
	typedef AllocatorType Allocator;
	typedef Range::ForwardRangeIterator<BListRange<T>> iterator;
	typedef Range::ForwardRangeIterator<BListRange<const T>> const_iterator;
	typedef typename BListRange<T>::Node Node;


	forceinline BList(null_t=null): range(null), count(0) {}

	forceinline BList(Allocator& allocator):
		AllocatorRef(allocator), range(null), count(0) {}

	forceinline BList(BList&& rhs):
		AllocatorRef(rhs), range(rhs.range), count(rhs.count)
		{rhs.range = null; rhs.count = 0;}

	template<typename ForwardRange> forceinline BList(Meta::EnableIf<
		Range::IsFiniteForwardRangeOf<ForwardRange, T>::_,
		const ForwardRange&> rhs) {operator=(rhs);}

	forceinline BList(std::initializer_list<T> values) {operator=(values);}

	BList(const BList& rhs): AllocatorRef(rhs), range(null), count(0) {operator=(rhs.AsRange());}
	~BList() {Clear();}

	//! Добавляет новый элемент в конец контейнера, сконструировав его на месте, используя переданные аргументы args.
	//! \param args Аргументы конструктора T
	//! \return Ссылка на добавленный элемент.
	template<typename... Args> forceinline T& EmplaceLast(Args&&... args)
	{
		return *new(&add_last_node()->Value) T(core::forward<Args>(args)...);
	}

	//! Добавляет новый элемент в конец контейнера как копию переданного значения.
	//! \param value Значение, которое будет помещено в контейнер.
	//! \return Ссылка на добавленный элемент.
	forceinline T& AddLast(const T& value) {return *new(&add_last_node()->Value) T(value);}

	//! Добавляет элемент в конец контейнера перемещением переданного значения.
	//! \param value Значение, которое будет перемещено в контейнер.
	//! \return Ссылка на добавленный элемент.
	forceinline T& AddLast(T&& value) {new(&add_last_node()->Value) T(core::move(value));}


	//! Добавляет новый элемент в начало контейнера, сконструировав его на месте, используя переданные аргументы args.
	//! \param args Аргументы конструктора T
	//! \return Ссылка на добавленный элемент.
	template<typename... Args> forceinline T& EmplaceFirst(Args&&... args)
	{return *new(&add_first_node()->Value) T(core::forward<Args>(args)...);}

	//! Добавляет новый элемент в начало контейнера как копию переданного значения.
	//! \param value Значение, которое будет помещено в контейнер.
	//! \return Ссылка на добавленный элемент.
	forceinline T& AddFirst(const T& value) {return *new(&add_first_node()->Value) T(value);}
	
	//! Добавляет элемент в начало контейнера перемещением переданного значения.
	//! \param value Значение, которое будет перемещено в контейнер.
	//! \return Ссылка на добавленный элемент.
	forceinline T& AddFirst(T&& value) {return *new(&add_first_node()->Value) T(core::move(value));}


	//! Вставляет новый элемент в указанную позицию контейнера, сконструировав его на месте, используя переданные аргументы args.
	//! \param pos Итератор элемента, непосредственно перед которым будет помещён добавляемый элемент.
	//! \param args Аргументы конструктора T
	//! \return Диапазон, содержащий вставленный элемент и все последующие элементы до конца контейнера.
	template<typename... Args> BListRange<T> Emplace(const_iterator pos, Args&&... args)
	{
		if(pos.Range.FirstNode==null) {AddLast(core::forward<Args>(args)...); return;}
		Node* node = insert_node(pos.Range.FirstNode);
		new(&node->Value) T(core::forward<Args>(args)...);
		return BListRange<T>(node, range.LastNode);
	}

	//! Вставляет новый элемент в указанную позицию контейнера как копию переданного значения.
	//! \param pos Итератор элемента, непосредственно перед которым будет помещён добавляемый элемент.
	//! \param args Аргументы конструктора T
	//! \return Диапазон, содержащий вставленный элемент и все последующие элементы до конца контейнера.
	BListRange<T> Insert(const_iterator it, const T& value)
	{
		if(it.Range.FirstNode==null) {AddLast(value); return;}
		Node* node = insert_node(it.Range.FirstNode);
		new(&node->Value) T(value);
		return BListRange<T>(node, range.LastNode);
	}

	//! Вставляет новый элемент в указанную позицию контейнера перемещением переданного значения.
	//! \param pos Итератор элемента, непосредственно перед которым будет помещён добавляемый элемент.
	//! \param args Аргументы конструктора T
	//! \return Диапазон, содержащий вставленный элемент и все последующие элементы до конца контейнера.
	BListRange<T> Insert(const_iterator it, T&& value)
	{
		if(it.Range.FirstNode==null) {AddLast(core::move(value)); return;}
		Node* node = insert_node(it.Range.FirstNode);
		new(&node->Value) T(core::move(value));
		return BListRange<T>(node, range.LastNode);
	}

	//! Удаляет элемент, находящийся в позиции pos.
	//! \param Итератор, указывающий на удаляемый элемент.
	//! \return Диапазон, содержащий все элементы списка, идущие после удалённого элемента.
	BListRange<T> Remove(const_iterator pos)
	{
		INTRA_ASSERT(pos.Range.FirstNode!=null);
		pos.Range.FirstNode->Value.~T();
		Node* next = pos.Range.FirstNode->Next;
		remove_node(pos.Range.FirstNode);
		return BListRange<T>(next, range.LastNode);
	}

	//! Удаляет первый элемент контейнера.
	void RemoveFirst()
	{
		range.FirstNode->Value.~T();
		auto deletedNode = range.FirstNode;
		range.FirstNode = range.FirstNode->Next;
		if(range.FirstNode!=null) range.FirstNode->Prev = null;
		else range.LastNode=null;
		AllocatorRef::Free(deletedNode, sizeof(Node));
	}

	//! Удаляет последний элемент контейнера.
	void RemoveLast()
	{
		range.LastNode->Value.~T();
		auto deletedNode = range.LastNode;
		range.LastNode = range.LastNode->Prev;
		if(range.LastNode!=null) range.LastNode->Next = null;
		else range.FirstNode = null;
		AllocatorRef::Free(deletedNode, sizeof(Node));
	}

	//! Удаляет все элементы из контейнера.
	void Clear()
	{
		Node* current = range.FirstNode;
		while(current!=null)
		{
			current->Value.~T();
			Node* next = current->Next;
			AllocatorRef::Free(current, sizeof(Node));
			current = next;
		}
		range = null;
		count = 0;
	}

	void SetCount(size_t newCount)
	{
		while(count>newCount)
		{
			RemoveLast();
			count--;
		}
		while(count<newCount)
		{
			EmplaceLast();
			count++;
		}
	}

	void SetCount(size_t newCount, const T& value)
	{
		while(count>newCount)
		{
			RemoveLast();
			count--;
		}
		while(count<newCount)
		{
			AddLast(value);
			count++;
		}
	}

	template<typename ForwardRange> Meta::EnableIf<
		Range::IsFiniteForwardRangeOf<ForwardRange, T>::_,
	BList&> operator=(const ForwardRange& values)
	{
		Clear();
		auto rhsCopy = values;
		count = 0;
		while(!rhsCopy.Empty())
		{
			AddLast(rhsCopy.First());
			rhsCopy.PopFirst();
		}
		return *this;
	}

	BList& operator=(const BList& rhs)
	{
		if(this==&rhs) return *this;
		return operator=(rhs.AsRange());
	}

	BList& operator=(std::initializer_list<T> values)
	{
		return operator=(Range::AsRange(values));
	}

	BList& operator=(BList&& rhs)
	{
		if(this==rhs) return *this;
		Clear();
		range = rhs.range;
		count = rhs.count;
		rhs.range = null;
		rhs.count = 0;
		return *this;
	}

	forceinline BList& operator=(null_t) {Clear(); return *this;}

	forceinline T& First() {return range.First();}
	forceinline T& Last() {return range.Last();}

	forceinline const T& First() const {return range.First();}
	forceinline const T& Last() const {return range.Last();}

	forceinline bool Empty() const {return range.Empty();}
	forceinline bool operator==(null_t) const {return range.Empty();}
	forceinline bool operator!=(null_t) const {return !range.Empty();}
	forceinline bool operator==(const BList& rhs) const {return count==rhs.count && Range::Equals(range, rhs.range);}
	forceinline bool operator!=(const BList& rhs) const {return !operator==(rhs);}

	forceinline const BListRange<T>& AsRange() {return range;}
	forceinline const BListRange<const T>& AsRange() const {return range.AsConstRange();}
	forceinline const BListRange<T>& operator()() {return range;}
	forceinline const BListRange<const T>& operator()() const {return range.AsConstRange();}


	forceinline iterator begin() {return range.begin();}
	forceinline iterator end() {return range.end();}
	forceinline const_iterator begin() const {return range.AsConstRange().begin();}
	forceinline const_iterator end() const {return range.AsConstRange().end();}

#ifdef INTRA_STL_INTERFACE
	//!@{
	//! Интерфейс, совместимый с STL. Для включения доступа к этому интерфейсу требуется #define INTRA_STL_INTERFACE.
	forceinline const_iterator cbegin() const {return begin();}
	forceinline const_iterator cend() const {return end();}
	forceinline iterator rbegin() {return range.Retro().begin();}
	forceinline iterator rend() {return range.Retro().end();}
	forceinline const_iterator rbegin() const {return range.AsConstRange().Retro().begin();}
	forceinline const_iterator rend() const {return range.AsConstRange().Retro().end();}
	forceinline const_iterator crbegin() const {return rbegin();}
	forceinline const_iterator crend() const {return rend();}

	forceinline iterator insert(const_iterator pos, const T& value) {return Insert(pos, value).begin();}
	forceinline iterator insert(const_iterator pos, T&& value) {return Insert(pos, core::move(value)).begin();}
	template<typename... Args> forceinline iterator emplace(const_iterator pos, Args&&... args) {return Emplace(pos, core::forward<Args>(args)...).begin();}

	forceinline void push_back(const T& value) {AddLast(value);}
	forceinline void push_back(T&& value) {AddLast(core::move(value));}
	template<typename... Args> forceinline void emplace_back(Args&&... args) {EmplaceLast(core::forward<Args>(args)...);}

	forceinline void push_front(const T& value) {AddFirst(value);}
	forceinline void push_front(T&& value) {AddFirst(core::move(value));}
	template<typename... Args> forceinline void emplace_front(Args&&... args) {EmplaceFirst(core::forward<Args>(args)...);}

	forceinline iterator erase(const_iterator pos) {Remove(pos);}

	void pop_front() {RemoveFirst();}
	void pop_back() {RemoveLast();}

	T& front() {return First();}
	const T& front() const {return First();}
	T& back() {return Last();}
	const T& back() const {return Last();}

	void resize(size_t newCount) {SetCount(newCount);}
	void resize(size_t newCount, const T& value) {SetCount(newCount, value);}
	//!@}
#endif


private:
	Node* add_first_node()
	{
		size_t nodeSize = sizeof(Node);
		Node* node = AllocatorRef::Allocate(nodeSize, INTRA_SOURCE_INFO);
		INTRA_ASSERT(nodeSize==sizeof(Node));
		node->Prev = null;
		node->Next = range.FirstNode;
		if(range.FirstNode!=null) range.FirstNode->Prev = node;
		range.FirstNode = node;
		if(range.LastNode==null) range.LastNode = node;
		count++;
		return node;
	}

	Node* add_last_node()
	{
		size_t nodeSize = sizeof(Node);
		Node* node = AllocatorRef::Allocate(nodeSize, INTRA_SOURCE_INFO);
		node->Prev = range.LastNode;
		node->Next = null;
		if(range.LastNode!=null) range.LastNode->Next = node;
		range.LastNode = node;
		if(range.FirstNode==null) range.FirstNode = node;
		count++;
		return node;
	}

	Node* insert_node(Node* itnode)
	{
		auto prev = itnode->Prev;
		size_t nodeSize = sizeof(Node);
		Node* node = AllocatorRef::Allocate(nodeSize);
		INTRA_ASSERT(nodeSize==sizeof(Node));
		node->Prev = prev;
		node->Next = itnode;
		prev->Next = node;
		itnode->Prev = node;
		count++;
		return node;
	}

	void remove_node(Node* itnode)
	{
		if(itnode->Prev!=null) itnode->Prev->Next = itnode->Next;
		if(itnode->Next!=null) itnode->Next->Prev = itnode->Prev;
		if(range.FirstNode==itnode) range.FirstNode = range.FirstNode->Next;
		if(range.LastNode==itnode) range.LastNode = range.LastNode->Prev;
		AllocatorRef::Free(itnode, sizeof(Node));
		count--;
	}

	BListRange<T> range;
	size_t count;
};

}
