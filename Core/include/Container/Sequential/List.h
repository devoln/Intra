#pragma once

#include "Algo/Comparison/Equals.h"
#include "Range/Iterator.hh"
#include "Range/Decorators/Retro.h"
#include "Memory/Allocator/AllocatorRef.h"
#include "Container/AllForwardDecls.h"
#include "Range/Generators/ListRange.h"
#include "Platform/CppWarnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION

namespace Intra { namespace Container {

template<typename T, class AllocatorType> class BList:
	public Memory::AllocatorRef<AllocatorType>
{
	typedef Memory::AllocatorRef<AllocatorType> AllocatorRef;
public:
	typedef T value_type;
	typedef Range::ForwardIterator<BListRange<T>> iterator;
	typedef Range::ForwardIterator<BListRange<const T>> const_iterator;
	typedef AllocatorType Allocator;
	typedef typename BListRange<T>::Node Node;


	forceinline BList(null_t=null): mRange(null), mCount(0) {}

	forceinline BList(Allocator& allocator):
		AllocatorRef(allocator), mRange(null), mCount(0) {}

	forceinline BList(BList&& rhs):
		AllocatorRef(rhs), mRange(rhs.mRange), mCount(rhs.mCount)
	{rhs.mRange = null; rhs.mCount = 0;}

	template<typename R, typename = Meta::EnableIf<
		Range::IsConsumableRangeOf<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, BList>::_
	>> forceinline BList(R&& values):
		mRange(null), mCount(0)
	{AddLastRange(Range::Forward<R>(values));}

	template<size_t N> BList(T(&arr)[N]) {operator=(Range::AsRange(arr));}

	BList(const BList& rhs):
		AllocatorRef(rhs), mRange(null), mCount(0)
	{operator=(rhs.AsRange());}
	
	~BList() {Clear();}

	//! Добавляет новый элемент в конец контейнера, сконструировав его на месте, используя переданные аргументы args.
	//! \param args Аргументы конструктора T
	//! \return Ссылка на добавленный элемент.
	template<typename... Args> forceinline T& EmplaceLast(Args&&... args)
	{return *new(&add_last_node()->Value) T(Meta::Forward<Args>(args)...);}

	//! Добавляет новый элемент в конец контейнера как копию переданного значения.
	//! \param value Значение, которое будет помещено в контейнер.
	//! \return Ссылка на добавленный элемент.
	forceinline T& AddLast(const T& value)
	{return *new(&add_last_node()->Value) T(value);}

	//! Добавляет элемент в конец контейнера перемещением переданного значения.
	//! \param value Значение, которое будет перемещено в контейнер.
	//! \return Ссылка на добавленный элемент.
	forceinline T& AddLast(T&& value)
	{return *new(&add_last_node()->Value) T(Meta::Move(value));}


	//! Добавляет новый элемент в начало контейнера, сконструировав его на месте, используя переданные аргументы args.
	//! \param args Аргументы конструктора T
	//! \return Ссылка на добавленный элемент.
	template<typename... Args> forceinline T& EmplaceFirst(Args&&... args)
	{return *new(&add_first_node()->Value) T(Meta::Forward<Args>(args)...);}

	//! Добавляет новый элемент в начало контейнера как копию переданного значения.
	//! \param value Значение, которое будет помещено в контейнер.
	//! \return Ссылка на добавленный элемент.
	forceinline T& AddFirst(const T& value) {return EmplaceFirst(value);}
	
	//! Добавляет элемент в начало контейнера перемещением переданного значения.
	//! \param value Значение, которое будет перемещено в контейнер.
	//! \return Ссылка на добавленный элемент.
	forceinline T& AddFirst(T&& value) {return EmplaceFirst(Meta::Move(value));}


	//! Вставляет новый элемент в указанную позицию контейнера, сконструировав его на месте, используя переданные аргументы args.
	//! \param pos Итератор элемента, непосредственно перед которым будет помещён добавляемый элемент.
	//! \param args Аргументы конструктора T
	//! \return Диапазон, содержащий вставленный элемент и все последующие элементы до конца контейнера.
	template<typename... Args> BListRange<T> Emplace(const_iterator pos, Args&&... args)
	{
		if(pos.Range.FirstNode==null)
		{
			AddLast(Meta::Forward<Args>(args)...);
			return mRange;
		}
		Node* node = insert_node(pos.Range.FirstNode);
		new(&node->Value) T(Meta::Forward<Args>(args)...);
		return BListRange<T>(node, mRange.LastNode);
	}

	//! Вставляет новый элемент в указанную позицию контейнера как копию переданного значения.
	//! \param pos Итератор элемента, непосредственно перед которым будет помещён добавляемый элемент.
	//! \param value Вставляемый элемент
	//! \return Диапазон, содержащий вставленный элемент и все последующие элементы до конца контейнера.
	BListRange<T> Insert(const_iterator pos, const T& value) {return Emplace(pos, value);}

	//! Вставляет новый элемент в указанную позицию контейнера перемещением переданного значения.
	//! \param pos Итератор элемента, непосредственно перед которым будет помещён добавляемый элемент.
	//! \param value Вставляемый элемент
	//! \return Диапазон, содержащий вставленный элемент и все последующие элементы до конца контейнера.
	BListRange<T> Insert(const_iterator pos, T&& value) {return Emplace(pos, Meta::Move(value));}

	//! Удаляет элемент, находящийся в позиции pos.
	//! \param Итератор, указывающий на удаляемый элемент.
	//! \return Диапазон, содержащий все элементы списка, идущие после удалённого элемента.
	BListRange<T> Remove(const_iterator pos)
	{
		INTRA_DEBUG_ASSERT(pos.Range.FirstNode!=null);
		pos.Range.FirstNode->Value.~T();
		Node* next = pos.Range.FirstNode->Next;
		remove_node(pos.Range.FirstNode);
		return BListRange<T>(next, mRange.LastNode);
	}

	//! Удаляет первый элемент контейнера.
	void RemoveFirst()
	{
		mRange.FirstNode->Value.~T();
		auto deletedNode = mRange.FirstNode;
		mRange.FirstNode = mRange.FirstNode->Next;
		if(mRange.FirstNode!=null) mRange.FirstNode->Prev = null;
		else mRange.LastNode=null;
		AllocatorRef::Free(deletedNode, sizeof(Node));
	}

	//! Удаляет последний элемент контейнера.
	void RemoveLast()
	{
		mRange.LastNode->Value.~T();
		auto deletedNode = mRange.LastNode;
		mRange.LastNode = mRange.LastNode->Prev;
		if(mRange.LastNode!=null) mRange.LastNode->Next = null;
		else mRange.FirstNode = null;
		AllocatorRef::Free(deletedNode, sizeof(Node));
	}

	//! Удаляет все элементы из контейнера.
	void Clear()
	{
		Node* current = mRange.FirstNode;
		while(current!=null)
		{
			current->Value.~T();
			Node* next = current->Next;
			AllocatorRef::Free(current, sizeof(Node));
			current = next;
		}
		mRange = null;
		mCount = 0;
	}

	void SetCount(size_t newCount)
	{
		while(mCount>newCount)
		{
			RemoveLast();
			mCount--;
		}
		while(mCount<newCount)
		{
			EmplaceLast();
			mCount++;
		}
	}

	void SetCount(size_t newCount, const T& value)
	{
		while(mCount>newCount)
		{
			RemoveLast();
			mCount--;
		}
		while(mCount<newCount)
		{
			AddLast(value);
			mCount++;
		}
	}


	template<typename R> Meta::EnableIf<
		Range::IsAsConsumableRangeOf<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, BList>::_
	> AddLastRange(R&& values)
	{
		auto valuesRange = Range::Forward<R>(values);
		for(; !valuesRange.Empty(); valuesRange.PopFirst())
			AddLast(valuesRange.First());
	}

	template<typename R> Meta::EnableIf<
		Range::IsAsConsumableRangeOf<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, BList>::_,
	BList&> operator=(R&& values)
	{
		Clear();
		AddLastRange(Range::Forward<R>(values));
		return *this;
	}

	template<size_t N> BList& operator=(T(&arr)[N])
	{return operator=(Range::AsRange(arr));}

	BList& operator=(const BList& rhs)
	{
		if(this==&rhs) return *this;
		return operator=(rhs.AsRange());
	}

	BList& operator=(BList&& rhs)
	{
		if(this==rhs) return *this;
		Clear();
		mRange = rhs.mRange;
		mCount = rhs.mCount;
		rhs.mRange = null;
		rhs.mCount = 0;
		return *this;
	}

	forceinline BList& operator=(null_t) {Clear(); return *this;}

	forceinline T& First() {return mRange.First();}
	forceinline T& Last() {return mRange.Last();}

	forceinline const T& First() const {return mRange.First();}
	forceinline const T& Last() const {return mRange.Last();}

	forceinline bool Empty() const {return mRange.Empty();}
	forceinline bool operator==(null_t) const {return mRange.Empty();}
	forceinline bool operator!=(null_t) const {return !mRange.Empty();}
	forceinline bool operator==(const BList& rhs) const {return mCount==rhs.mCount && Algo::Equals(mRange, rhs.mRange);}
	forceinline bool operator!=(const BList& rhs) const {return !operator==(rhs);}

	forceinline const BListRange<T>& AsRange() {return mRange;}
	forceinline const BListRange<const T>& AsRange() const {return mRange.AsConstRange();}
	forceinline const BListRange<T>& operator()() {return mRange;}
	forceinline const BListRange<const T>& operator()() const {return mRange.AsConstRange();}



	//! @defgroup BList_STL_Interface STL-подобный интерфейс для BList
	//! Этот интерфейс предназначен для совместимости с обобщённым контейнеро-независимым кодом.
	//! Использовать напрямую этот интерфейс не рекомендуется.
	//!@{
	forceinline iterator begin() {return Range::begin(mRange);}
	forceinline iterator end() {return Range::end(mRange);}
	forceinline const_iterator begin() const {return Range::begin(mRange.AsConstRange());}
	forceinline const_iterator end() const {return Range::end(mRange.AsConstRange());}
	forceinline const_iterator cbegin() const {return begin();}
	forceinline const_iterator cend() const {return end();}
	forceinline iterator rbegin() {return Range::Retro(mRange).begin();}
	forceinline iterator rend() {return Range::Retro(mRange).end();}
	forceinline const_iterator rbegin() const {return Range::begin(Retro(mRange.AsConstRange()));}
	forceinline const_iterator rend() const {return Range::end(Retro(mRange.AsConstRange()));}
	forceinline const_iterator crbegin() const {return rbegin();}
	forceinline const_iterator crend() const {return rend();}

	forceinline iterator insert(const_iterator pos, const T& value) {return Range::begin(Insert(pos, value));}
	forceinline iterator insert(const_iterator pos, T&& value) {return Range::begin(Insert(pos, Meta::Move(value)));}
	template<typename... Args> forceinline iterator emplace(const_iterator pos, Args&&... args) {return Range::begin(Emplace(pos, Meta::Forward<Args>(args)...));}

	forceinline void push_back(const T& value) {AddLast(value);}
	forceinline void push_back(T&& value) {AddLast(Meta::Move(value));}
	template<typename... Args> forceinline void emplace_back(Args&&... args) {EmplaceLast(Meta::Forward<Args>(args)...);}

	forceinline void push_front(const T& value) {AddFirst(value);}
	forceinline void push_front(T&& value) {AddFirst(Meta::Move(value));}
	template<typename... Args> forceinline void emplace_front(Args&&... args) {EmplaceFirst(Meta::Forward<Args>(args)...);}

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


private:
	Node* add_first_node()
	{
		size_t nodeSize = sizeof(Node);
		Node* node = AllocatorRef::Allocate(nodeSize, INTRA_SOURCE_INFO);
		INTRA_DEBUG_ASSERT(nodeSize==sizeof(Node));
		node->Prev = null;
		node->Next = mRange.FirstNode;
		if(mRange.FirstNode!=null) mRange.FirstNode->Prev = node;
		mRange.FirstNode = node;
		if(mRange.LastNode==null) mRange.LastNode = node;
		mCount++;
		return node;
	}

	Node* add_last_node()
	{
		size_t nodeSize = sizeof(Node);
		Node* node = AllocatorRef::Allocate(nodeSize, INTRA_SOURCE_INFO);
		node->Prev = mRange.LastNode;
		node->Next = null;
		if(mRange.LastNode!=null) mRange.LastNode->Next = node;
		mRange.LastNode = node;
		if(mRange.FirstNode==null) mRange.FirstNode = node;
		mCount++;
		return node;
	}

	Node* insert_node(Node* itnode)
	{
		auto prev = itnode->Prev;
		size_t nodeSize = sizeof(Node);
		Node* node = AllocatorRef::Allocate(nodeSize);
		INTRA_DEBUG_ASSERT(nodeSize==sizeof(Node));
		node->Prev = prev;
		node->Next = itnode;
		prev->Next = node;
		itnode->Prev = node;
		mCount++;
		return node;
	}

	void remove_node(Node* itnode)
	{
		if(itnode->Prev!=null) itnode->Prev->Next = itnode->Next;
		if(itnode->Next!=null) itnode->Next->Prev = itnode->Prev;
		if(mRange.FirstNode==itnode) mRange.FirstNode = mRange.FirstNode->Next;
		if(mRange.LastNode==itnode) mRange.LastNode = mRange.LastNode->Prev;
		AllocatorRef::Free(itnode, sizeof(Node));
		mCount--;
	}

	BListRange<T> mRange;
	size_t mCount;
};

}}

INTRA_WARNING_POP
