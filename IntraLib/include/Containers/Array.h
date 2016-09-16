#pragma once

#include "Core/Core.h"
#include "Containers/ForwardDeclarations.h"
#include "Algorithms/Range.h"
#include "Algorithms/Algorithms.h"
#include "CompilerSpecific/InitializerList.h"
#include "Memory/AllocatorInterface.h"

namespace Intra {

template<typename T, class AllocatorType> class Array: Memory::AllocatorRef<AllocatorType>
{
	typedef Memory::AllocatorRef<AllocatorType> AllocatorRef;
	typedef AllocatorType Allocator;
public:
	typedef T value_type;
	typedef T* Iterator;
	typedef const T* ConstIterator;

	Array(null_t=null): buffer(null), range(null) {}
	explicit Array(size_t size): buffer(null), range(null) {Reserve(size);}
	explicit Array(size_t size, Allocator& allocator): AllocatorRef(allocator), buffer(null), range(null) {Reserve(size);}
	Array(std::initializer_list<T> values): Array(ArrayRange<const T>(values)) {}
	Array(std::initializer_list<T> values, Allocator& allocator): Array(ArrayRange<const T>(values), allocator) {}
	
	Array(ArrayRange<const T> values): buffer(null), range(null)
	{
		SetCountUninitialized(values.Count());
		Memory::CopyInit(range, values.AsConstRange());
	}

	template<size_t N> Array(const T(&values)[N])
	{
		SetCountUninitialized(N);
		Memory::CopyInit(range, ArrayRange<const T>(values));
	}

	explicit Array(ArrayRange<const T> values, Allocator& allocator): AllocatorRef(allocator), buffer(null), range(null)
	{
		SetCountUninitialized(values.Count());
		Memory::CopyInit(range, values.AsConstRange());
	}

	Array(const Array& rhs): AllocatorRef(rhs.GetRef()), buffer(null), range(null)
	{
		SetCountUninitialized(rhs.Count());
		Memory::CopyInit(range, rhs.AsConstRange());
	}

	explicit Array(const Array& rhs, Allocator& allocator): Array(rhs.AsConstRange(), allocator) {}
	Array(Array&& rhs): AllocatorRef(rhs), buffer(rhs.buffer), range(rhs.range) {rhs.buffer = null; rhs.range = null;}
	~Array() {operator=(null);}

	Array& operator=(const Array& rhs)
	{
		if(this==&rhs) return *this;
		Assign(rhs.AsConstRange());
		return *this;
	}

	Array& operator=(Array&& rhs)
	{
		Clear();
		range = rhs.range;
		rhs.range.End = rhs.range.Begin;
		core::swap(buffer, rhs.buffer);
		return *this;
	}

	Array& operator=(ArrayRange<const T> values) {return operator=(Array(values));}

	//! Удалить все элементы и освободить память.
	Array& operator=(null_t)
	{
		Clear();
		free_buffer();
		buffer = null;
		range = null;
		return *this;
	}

	template<typename U> void Assign(ArrayRange<const U> rhs)
	{
		Clear();
		SetCountUninitialized(rhs.Count());
		Memory::CopyInit(range, rhs);
	}

	template<typename U> void Assign(ArrayRange<U> rhs) {Assign(rhs.AsConstRange());}

	//! Добавить новый элемент в начало массива копированием или перемещением value.
	forceinline T& AddFirst(T&& value)
	{
		INTRA_ASSERT(!range.ContainsAddress(core::addressof(value)));
		if(NoLeftSpace()) CheckSpace(0, 1);
		return *new(--range.Begin) T(core::move(value));
	}

	forceinline T& AddFirst(const T& value)
	{
		INTRA_ASSERT(!range.ContainsAddress(core::addressof(value)));
		if(NoLeftSpace()) CheckSpace(0, 1);
		return *new(--range.Begin) T(value);
	}

	//! Добавить новый элемент в начало массива, сконструировав его на месте с параметрами args.
	template<typename... Args> forceinline Meta::EnableIf<
		Meta::IsConstructible<T, Args...>::_,
	T&> EmplaceFirst(Args&&... args)
	{
		if(NoLeftSpace()) CheckSpace(0, 1);
		return *new(--range.Begin) T(core::forward<Args>(args)...);
	}
	
	//! Добавить все значения указанного диапазона в начало массива
	template<typename RangeOfValues> Meta::EnableIf<
		Range::IsFiniteForwardRangeOf<RangeOfValues, T>::_
	> AddFirstRange(const RangeOfValues& values)
	{
		RangeOfValues valueRange = values;
		//INTRA_ASSERT(!range.Overlaps((ArrayRange<const byte>&)values));
		const size_t valuesCount = values.Count();
		if(LeftSpace()<valuesCount) CheckSpace(0, valuesCount);
		T* dst = (range.Begin -= valuesCount);
		while(!valueRange.Empty())
		{
			new(dst++) T(static_cast<T>(valueRange.First()));
			valueRange.PopFirst();
		}
	}

	//! Добавить новый элемент в конец массива перемещением value.
	forceinline T& AddLast(T&& value)
	{
		INTRA_ASSERT(!range.ContainsAddress(core::addressof(value)));
		if(NoRightSpace()) CheckSpace(1, 0);
		return *new(range.End++) T(core::move(value));
	}

	//! Добавить новый элемент в конец массива копированием value.
	forceinline T& AddLast(const T& value)
	{
		INTRA_ASSERT(!range.ContainsAddress(core::addressof(value)));
		if(NoRightSpace()) CheckSpace(1, 0);
		return *new(range.End++) T(value);
	}

	//! Добавить новый элемент в конец массива, сконструировав его на месте с параметрами args.
	template<typename... Args> forceinline Meta::EnableIf<
		Meta::IsConstructible<T, Args...>::_,
	T&> EmplaceLast(Args&&... args)
	{
		if(NoRightSpace()) CheckSpace(1, 0);
		return *new(range.End++) T(core::forward<Args>(args)...);
	}

	//! Добавить все значения указанного диапазона в конец массива.
	template<typename RangeOfValues> Meta::EnableIf<
		Range::IsFiniteForwardRangeOf<RangeOfValues, T>::_
	> AddLastRange(const RangeOfValues& values)
	{
		RangeOfValues valueRange = values;
		//INTRA_ASSERT(!data.Overlaps((ArrayRange<const byte>&)values));
		const size_t valuesCount = values.Count();
		if(RightSpace()<valuesCount) CheckSpace(valuesCount, 0);
		while(!valueRange.Empty())
		{
			new(range.End++) T(static_cast<T>(valueRange.First()));
			valueRange.PopFirst();
		}
	}

	//! Установить элемент с индексом pos в value. Если pos>=Count(), в массив будет добавлено pos-Count()
	//! элементов конструктором по умолчанию и один элемент конструктором копирования или перемещения от value.
	template<typename U> T& Set(size_t pos, U&& value)
	{
		if(pos>=Count())
		{
			Reserve(pos+1);
			SetCount(pos);
			return AddLast(core::forward<U>(value));
		}
		operator[](pos) = core::forward<U>(value);
	}
	

	//! Вставить новые элементы в указанную позицию.
	//! Если pos<(Count()+values.Count())/2 и LeftSpace()>=values.Count(), элементы с индексами < pos
	//! будут перемещены конструктором перемещения и деструктором на values.Count() элементов назад.
	//! Если pos>=(Count()+values.Count())/2 или LeftSpace()<values.Count(), элементы с индексами >= pos
	//! будут перемещены конструктором перемещения и деструктором на values.Count() элементов вперёд.
	//! Элементы, имеющие индексы >= pos, будут иметь индексы, увеличенные на values.Count().
	//! Первый вставленный элемент будет иметь индекс pos.
	template<typename U> void Insert(size_t pos, ArrayRange<const U> values)
	{
		if(values.Empty()) return;
		INTRA_ASSERT(!range.Overlaps(values));
		const size_t valuesCount = values.Count();

		//Если не хватает места, перераспределяем память и копируем элементы
		if(Count()+valuesCount>Capacity())
		{
			size_t newCapacity = Count()+valuesCount+Capacity()/2;
			ArrayRange<T> newBuffer = AllocatorRef::template AllocateRangeUninitialized<T>(newCapacity, INTRA_SOURCE_INFO);
			ArrayRange<T> newRange = newBuffer.Drop(LeftSpace()).Take(Count()+valuesCount);
			Memory::MoveInitDelete(newRange.Take(pos), range.Take(pos));
			Memory::MoveInitDelete(newRange.Drop(pos+valuesCount), range.Drop(pos));
			Memory::CopyInit(newRange.Drop(pos).Take(valuesCount), values);
			free_buffer();
			buffer = newBuffer;
			range = newRange;
			return;
		}

		//Добавляем элемент, перемещая ближайшую к концу часть массива
		if(pos>=(Count()+valuesCount)/2 || LeftSpace()<valuesCount)
		{
			range.End += valuesCount;
			Memory::MoveInitDeleteBackwards<T>(range.Drop(pos+valuesCount), range.Drop(pos).DropBack(valuesCount));
		}
		else
		{
			range.Begin -= valuesCount;
			Memory::MoveInitDelete<T>(range.Take(pos), range.Drop(valuesCount).Take(pos));
		}
		Memory::CopyInit<T>(range.Drop(pos).Take(valuesCount), values);
	}

	template<typename U> forceinline void Insert(const T* it, ArrayRange<const U> values)
	{
		INTRA_ASSERT(range.ContainsAddress(it));
		Insert(it-range.Begin, values);
	}

	forceinline void Insert(size_t pos, const T& value) {Insert(pos, {&value, 1});}

	forceinline void Insert(const T* it, const T& value)
	{
		INTRA_ASSERT(range.ContainsAddress(it));
		Insert(it-range.Begin, value);
	}


	template<typename U> forceinline void Insert(Range::RelativeIndex pos, ArrayRange<const U> values) {Insert(pos.GetRealIndex(Count()), values);}


	//! Добавить новый элемент в конец.
	forceinline Array& operator<<(const T& value) {AddLast(value); return *this;}

	//! Прочитать и удалить последний элемент.
	forceinline Array& operator>>(T& value)
	{
		value = core::move(Last());
		RemoveLast();
		return *this;
	}

	//! Возвращает последний элемент, удаляя его из массива.
	forceinline T PopLastElement()
	{
		T result = core::move(Last());
		RemoveLast();
		return result;
	}


	//! Возвращает первый элемент, удаляя его из массива.
	forceinline T PopFirstElement()
	{
		T result = core::move(First());
		RemoveFirst();
		return result;
	}

	//! Установить новый размер буфера массива (не влезающие элементы удаляются).
	void Resize(size_t rightPartSize, size_t leftPartSize=0)
	{
		if(rightPartSize+leftPartSize==0) {*this=null; return;}

		//Удаляем элементы, выходящие за границы массива
		if(rightPartSize <= Count()) Memory::Destruct(range.Drop(rightPartSize));

		size_t newCapacity = rightPartSize+leftPartSize;
		ArrayRange<T> newBuffer = AllocatorRef::template AllocateRangeUninitialized<T>(newCapacity, INTRA_SOURCE_INFO);
		ArrayRange<T> newRange = newBuffer.Drop(leftPartSize).Take(Count());

		if(!buffer.Empty())
		{
			//Перемещаем элементы в новый участок памяти
			Memory::MoveInitDelete<T>(newRange, range);
			free_buffer();
		}
		buffer = newBuffer;
		range = newRange;
	}

	//! Убедиться, что буфер массива может вместить rightPart элементов при добавлении
	//! их в конец и имеет leftSpace места для добавления в начало.
	void Reserve(size_t rightPart, size_t leftSpace=0)
	{
		const size_t currentRightPartSize = size_t(buffer.End-range.Begin);
		const size_t currentLeftSpace = LeftSpace();
		if(rightPart<=currentRightPartSize && leftSpace<=currentLeftSpace) return;

		const size_t currentSize = Capacity();
		if(rightPart>0)
			if(leftSpace>0) Resize(currentSize/4+rightPart, currentSize/4+leftSpace);
			else Resize(currentSize/2+rightPart, currentLeftSpace);
		else Resize(currentRightPartSize, currentLeftSpace+currentSize/2+leftSpace);
	}

	//! Убедиться, что буфер массива может вместить rightSpace новых элементов при добавлении их в конец и имеет leftSpace места для добавления в начало.
	forceinline void CheckSpace(size_t rightSpace, size_t leftSpace=0) {Reserve(Count()+rightSpace, leftSpace);}

	//! Удалить все элементы из массива, не освобождая занятую ими память.
	forceinline void Clear() {Memory::Destruct<T>(range); range.End=range.Begin;}

	//! Возвращает, является ли массив пустым.
	forceinline bool Empty() const {return range.Empty();}

	//! Количество элементов, которые можно вставить в начало массива до перераспределения буфера.
	forceinline size_t LeftSpace() const {return size_t(range.Begin-buffer.Begin);}

	//! Возвращает true, если массив не имеет свободного места для вставки элемента в начало массива.
	forceinline bool NoLeftSpace() const {return range.Begin==buffer.Begin;}

	//! Количество элементов, которые можно вставить в конец массива до перераспределения буфера.
	forceinline size_t RightSpace() const {return size_t(buffer.End-range.End);}

	//! Возвращает true, если массив не имеет свободного места для вставки элемента в конец массива.
	forceinline bool NoRightSpace() const {return buffer.End==range.End;}

	//!@{
	//! Удаление элементов с сохранением порядка. Индексы всех элементов, находящихся после удаляемых элементов, уменьшаются на количество удаляемых элементов.
	//! При наличии итераторов на какие-либо элементы массива следует учесть, что:
	//! при удалении элементов с индексами < Count()/4 все предшествующие элементы перемещаются вправо конструктором перемещения и деструктором.
	//! при удалении элементов с индексами >= Count()/4 все последующие элементы перемещаются влево конструктором перемещения и деструктором.
	//! Таким образом адреса элементов изменяются и указатели станут указывать либо на другой элемент массива, либо на неинициализированную область массива.

	//! Удалить один элемент по индексу.
	forceinline void Remove(Range::RelativeIndex index) {Remove(index.GetRealIndex(Count()));}

	//! Удалить один элемент по индексу.
	void Remove(size_t index)
	{
		INTRA_ASSERT(index<Count());
		range[index].~T();

		// Соотношение 1/4 вместо 1/2 было выбрано, потому что перемещение перекрывающихся
		// участков памяти вправо в ~2 раза медленнее, чем влево
		if(index>=Count()/4) //Перемещаем правую часть влево
		{
			Memory::MoveInitDelete<T>({range.Begin+index, range.End-1}, {range.Begin+index+1, range.End});
			--range.End;
		}
		else //Перемещаем левую часть вправо
		{
			Memory::MoveInitDeleteBackwards<T>({range.Begin+1, range.Begin+index+1}, {range.Begin, range.Begin+index});
			++range.Begin;
		}
	}

	//! Удалить один элемент по указателю.
	forceinline void Remove(T* ptr) {INTRA_ASSERT(range.ContainsAddress(ptr)); Remove(ptr-range.Begin);}

	//!@{
	//! Удаление всех элементов в диапазоне [removeStart; removeEnd)
	void Remove(Range::RelativeIndex removeStart, Range::RelativeIndex removeEnd)
	{
		Remove(removeStart.GetRealIndex(Count()), removeEnd.GetRealIndex(Count()));
	}

	void Remove(size_t removeStart, size_t removeEnd)
	{
		INTRA_ASSERT(removeStart <= removeEnd);
		INTRA_ASSERT(removeEnd <= Count());
		if(removeEnd==removeStart) return;
		const size_t elementsToRemove = removeEnd-removeStart;
		Memory::Destruct<T>(range(removeStart, removeEnd));

		//Быстрые частные случаи
		if(removeEnd==Count())
		{
			range.End -= elementsToRemove;
			return;
		}

		if(removeStart==0)
		{
			range.Begin += elementsToRemove;
			return;
		}

		if(removeStart+elementsToRemove/2>=Count()/4)
		{
			Memory::MoveInitDelete<T>(
				range.Drop(removeStart).DropBack(elementsToRemove),
				range.Drop(removeEnd));
			range.End -= elementsToRemove;
		}
		else
		{
			Memory::MoveInitDeleteBackwards<T>(
				range(elementsToRemove, removeEnd),
				range.Take(removeStart));
			range.Begin += elementsToRemove;
		}
	}
	//!@}

	//! Удаление первого найденного элемента, равного value
	forceinline void FindAndRemove(const T& value)
	{
		size_t found = range.CountUntil(value);
		if(found!=Count()) Remove(found);
	}
	//!@}

	//! Удалить дублирующиеся элементы из массива. При этом порядок элементов в массиве не сохраняется.
	void RemoveDuplicatesUnordered()
	{
		for(size_t i=0; i<Count(); i++)
			for(size_t j=i+1; j<Count(); j++)
				if(operator[](i)==operator[](j))
					RemoveUnordered(j--);
	}

	//! Удалить первый элемент.
	forceinline void RemoveFirst() {INTRA_ASSERT(!Empty()); (*range.Begin++).~T();}

	//! Удалить последний элемент.
	forceinline void RemoveLast() {INTRA_ASSERT(!Empty()); (*--range.End).~T();}

	//!@{
	//! Быстрое удаление путём переноса последнего элемента (без смещения).
	void RemoveUnordered(Range::RelativeIndex index) {RemoveUnordered(index.GetRealIndex(Count()));}
	
	forceinline void RemoveUnordered(size_t index)
	{
		INTRA_ASSERT(index<Count());
		if(index<Count()-1) range[index] = core::move(*--range.End);
		RemoveLast();
	}

	void FindAndRemoveUnordered(const T& value)
	{
		size_t index = range.CountUntil(value);
		if(index!=Count()) RemoveUnordered(index);
	}
	//!@}

	//! Освободить незанятую память массива, уменьшив буфер до количества элементов в массиве,
	//! если ёмкость превышает количество элементов более, чем на 25%.
	forceinline void TrimExcessCapacity() {if(Capacity()>Count()*5/4) Resize(Count());}




	forceinline T& operator[](size_t index) {INTRA_ASSERT(index<Count()); return range.Begin[index];}
	forceinline const T& operator[](size_t index) const {INTRA_ASSERT(index<Count()); return range.Begin[index];}
	T& operator[](Range::RelativeIndex index) {return operator[](index.GetRealIndex(Count()));}
	const T& operator[](Range::RelativeIndex index) const {return operator[](index.GetRealIndex(Count()));}

	forceinline T& Last() {INTRA_ASSERT(!Empty()); return range.Last();}
	forceinline const T& Last() const {INTRA_ASSERT(!Empty()); return range.Last();}
	forceinline T& First() {INTRA_ASSERT(!Empty()); return range.First();}
	forceinline const T& First() const {INTRA_ASSERT(!Empty()); return range.First();}



	//!@{
	//! Совместимость с range-based for и STL
	forceinline T* begin() {return range.Begin;}
	forceinline const T* begin() const {return range.Begin;}
	forceinline T* end() {return range.End;}
	forceinline const T* end() const {return range.End;}
	//!@}


	forceinline T* Data() {return begin();}
	forceinline const T* Data() const {return begin();}

	forceinline T* End() {return end();}
	forceinline const T* End() const {return end();}

	//! Возвращает байтовый буфер с данными этого массива. При этом сам массив становится пустым. Деструкторы не вызываются!
	//ByteBuffer MoveToByteBuffer() {return core::move(data);}

	//template<typename U=T> Meta::EnableIfPod<U, ByteBuffer> CopyToByteBuffer() {return data;}
	template<typename U> forceinline Array<U, Allocator> MoveReinterpret() {return core::move(reinterpret_cast<Array<U, Allocator>&>(*this));}


	//! Возвращает суммарный размер в байтах элементов в массиве
	forceinline size_t SizeInBytes() const {return Count()*sizeof(T);}

	//!@{
	//! Возвращает количество элементов в массиве
	forceinline size_t Count() const {return range.Length();}
	forceinline size_t Length() const {return Count();}
	//!@}

	//! Изменить количество занятых элементов массива (с удалением лишних элементов или инициализацией по умолчанию новых)
	void SetCount(size_t newCount)
	{
		const size_t oldCount = Count();
		if(newCount<=oldCount)
		{
			Memory::Destruct<T>(range.Drop(newCount));
			range.End = range.Begin+newCount;
			return;
		}
		SetCountUninitialized(newCount);
		Memory::Initialize<T>(range.Drop(oldCount));
	}

	//! Изменить количество занятых элементов массива (с удалением лишних элементов или инициализацией копированием новых)
	void SetCount(size_t newCount, const T& value)
	{
		const size_t oldCount = Count();
		if(newCount<=oldCount)
		{
			Memory::Destruct<T>(range.Drop(newCount));
			range.End = range.Begin+newCount;
			return;
		}
		SetCountUninitialized(newCount);
		for(T* dst = range.Begin+oldCount; dst<range.End; dst++) new(dst) T(value);
	}

	//! Изменить количество занятых элементов массива без вызова лишних элементов или инициализации новых.
	//! Инициализировать\удалять элементы придётся вручную, либо использовать этот метод только для POD типов!
	void SetCountUninitialized(size_t newCount)
	{
		Reserve(newCount, 0);
		if(newCount==0) range.Begin = buffer.Begin;
		range.End = range.Begin + newCount;
	}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	//! Получить текущий размер буфера массива
	forceinline size_t Capacity() const {return buffer.Length();}

	forceinline bool IsFull() const {return Count()==Capacity();}


	forceinline bool operator==(const Array& rhs) const
	{
		return Range::Equals(AsConstRange(), rhs.AsConstRange());
	}

	forceinline bool operator!=(const Array& rhs) const {return !operator==(rhs);}


	forceinline operator ArrayRange<T>() {return AsRange();}
	forceinline operator ArrayRange<const T>() const {return AsRange();}
	forceinline ArrayRange<T> AsRange() {return range;}
	forceinline ArrayRange<const T> AsConstRange() const {return range.AsConstRange();}
	forceinline ArrayRange<const T> AsRange() const {return AsConstRange();}

	forceinline ArrayRange<T> operator()() {return AsRange();}
	forceinline ArrayRange<const T> operator()() const {return AsConstRange();}

	forceinline ArrayRange<T> operator()(size_t first, size_t end)
	{
		INTRA_ASSERT(first <= end);
		INTRA_ASSERT(end <= Count());
		return range(first, end);
	}

	forceinline ArrayRange<const T> operator()(size_t first, size_t end) const
	{
		INTRA_ASSERT(first <= end);
		INTRA_ASSERT(end <= Count());
		return AsConstRange()(first, end);
	}

	ArrayRange<T> operator()(Range::RelativeIndex first, Range::RelativeIndex end)
	{
		return range(first.GetRealIndex(Count()), end.GetRealIndex(Count()));
	}

	ArrayRange<const T> operator()(Range::RelativeIndex first, Range::RelativeIndex end) const
	{
		return range(first.GetRealIndex(Count()), end.GetRealIndex(Count()));
	}

	ArrayRange<T> operator()(Range::RelativeIndex first, Range::RelativeIndexEnd)
	{
		return range(first.GetRealIndex(Count()), $);
	}

	ArrayRange<const T> operator()(Range::RelativeIndex first, Range::RelativeIndexEnd) const
	{
		return range(first.GetRealIndex(Count()), $);
	}

	struct BackInserter
	{
		typedef T value_type;

		BackInserter(Array* dst): DstArr(dst) {}

		Array* DstArr;
		forceinline void Put(T&& v) {DstArr->AddLast(core::forward<T>(v));}
		forceinline void Put(const T& v) {DstArr->AddLast(v);}
	};
	BackInserter Insert(Range::RelativeIndexEnd) {return BackInserter(this);}

#ifdef INTRA_STL_INTERFACE
	typedef T* iterator;
	typedef const T* const_iterator;
	forceinline void push_back(T&& value) {AddLast(core::move(value));}
	forceinline void push_back(const T& value) {AddLast(value);}
	forceinline void push_front(T&& value) {AddFirst(core::move(value));}
	forceinline void push_front(const T& value) {AddFirst(value);}
	forceinline void pop_back() {RemoveLast();}
	forceinline void pop_front() {RemoveFirst();}
	forceinline T& back() {return Last();}
	forceinline const T& back() const {return Last();}
	forceinline T& front() {return First();}
	forceinline const T& front() const {return First();}
	forceinline void insert(T* it);
	forceinline bool empty() {return Empty();}
	forceinline T* data() {return Data();}
	forceinline const T* data() const {return Data();}
	forceinline T& at(size_t index) {return operator[](index);}
	forceinline const T& at(size_t index) const {return operator[](index);}
	forceinline const T* cbegin() const {return begin();}
	forceinline const T* cend() const {return end();}
	forceinline size_t size() const {return Count();}
	forceinline size_t capacity() const {return Capacity();}
	forceinline void shrink_to_fit() {TrimExcessCapacity();}
	forceinline void clear() {Clear();}

	forceinline T* insert(const T* pos, const T& value) {Insert(size_t(pos-Data()), value);}
	forceinline T* insert(const T* pos, T&& value) {Insert(size_t(pos-Data()), core::move(value));}
	forceinline T* insert(const T* pos, size_t count, const T& value);
	template<typename InputIt> forceinline T* insert(const T* pos, InputIt first, InputIt last);
	forceinline T* insert(const T* pos, std::initializer_list<T> ilist) {Insert(size_t(pos-Data()), ArrayRange<const T>(ilist));}

	//! Отличается от std::vector<T>::erase тем, что инвалидирует все итераторы, а не только те, которые идут после удаляемого элемента.
	forceinline T* erase(const T* pos) {Remove(size_t(pos-Data())); return Data()+(pos-Data());}
	forceinline T* erase(const T* first, const T* end) {Remove(size_t(first-Data()), size_t(end-Data())); return Data()+(first-Data());}

	void reserve(size_t capacity) {Reserve(capacity);}
	void resize(size_t count) {SetCount(count);}
	//void resize(size_t count, const T& value);
	void swap(Array& rhs) {core::swap(range, rhs.range); core::swap(buffer, rhs.buffer);}
#endif

private:
	void free_buffer() {AllocatorRef::Free(buffer.Begin, Capacity()*sizeof(T));}

	ArrayRange<T> buffer, range;
};

//! Добавление элемента в начало
template<typename T, class Allocator> forceinline Array<T, Allocator>& operator>>(const T& value, Array<T, Allocator>& arr)
{
	arr.AddFirst(value);
	return arr;
}

//! Переместить первый элемент массива arr в value и удалить его из массива.
template<typename T> Array<T>& operator<<(T& value, Array<T>& arr)
{
	value = arr.First();
	arr.RemoveFirst();
	return arr;
}

//! Для совместимости с range-based for и STL
template<typename T, class Allocator> forceinline T* begin(Array<T, Allocator>& arr) {return arr.begin();}
template<typename T, class Allocator> forceinline const T* begin(const Array<T, Allocator>& arr) {return arr.begin();}
template<typename T, class Allocator> forceinline T* end(Array<T, Allocator>& arr) {return arr.end();}
template<typename T, class Allocator> forceinline const T* end(const Array<T, Allocator>& arr) {return arr.end();}


template<typename T> using Deque = Array<T>;

namespace Meta {template<typename T, class Allocator> struct IsTriviallyRelocatable<Array<T, Allocator>>: TypeFromValue<bool, true> {};}

}
