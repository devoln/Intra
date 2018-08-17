#pragma once

#include "Utils/FixedArray.h"
#include "Memory/Align.h"
#include "Memory/Memory.h"
#include "Memory/Allocator/System.h"
#include "Memory/Allocator/AllocatorRef.h"
#include "Range/Mutation/Copy.h"

namespace Intra { namespace Container {

template<typename T, typename OffsetType> class BlobTypedRange
{
	CSpan<byte> mData;
	CSpan<OffsetType> mOffsets;
public:
	forceinline BlobTypedRange(CSpan<byte> blobData)
	{
		CSpan<OffsetType> otdata = blobData.Reinterpret<const OffsetType>();
		mOffsets = otdata.DropLast().Tail(otdata.Last());
		mData = blobData.Take(*(otdata.End - (mOffsets.Length() + 2)));
	}

	forceinline T& First() const {return *reinterpret_cast<T>(mData.Begin + mOffsets.Last());}
	forceinline void PopFirst() {mOffsets.PopLast();}
	forceinline void PopLast() {mOffsets.PopFirst();}
	forceinline bool Empty() const {return mOffsets.Empty();}
	
	forceinline T& Next()
	{T& result = First(); PopFirst(); return result;}
	
	forceinline T& operator[](size_t index) const
	{return *reinterpret_cast<T*>(mData.Begin + mOffsets[index]);}

	forceinline size_t PopFirstN(size_t count) {return mOffsets.PopLastN(count);}
	forceinline size_t PopLastN(size_t count) {return mOffsets.PopFirstN(count);}
};


//! Класс, интерпретирующий массив байт как blob, который состоит из объектов переменной длины,
//! размещающихся слева направо и заголовка, растущего справа налево
//! Все методы константные, так как сами данные не являются внутренним состоянием объекта и могут быть изменены извне прямой записью байт в RawData.
template<typename OffsetType> struct BlobAdapter
{
	Span<byte> RawData;

	forceinline OffsetType* HeaderEnd() const {return reinterpret_cast<OffsetType*>(RawData.End);}
	forceinline OffsetType* HeaderBegin() const {return HeaderEnd() - (2 + Count());}

	forceinline byte* DataBegin() const {return RawData.Begin;}
	forceinline byte* DataEnd() const {return RawData.Begin + DataSizeInBytes();}

	forceinline OffsetType& OffsetOf(size_t index) const
	{
		INTRA_DEBUG_ASSERT(index < Count());
		return *(HeaderEnd() - (1 + index));
	}

	forceinline static bool CheckOffsetForOverflow(size_t offset) const
	{return size_t(OffsetType(offset)) == offset;}

	forceinline size_t Align(size_t requiredAlignmentBytes) const
	{
		auto& dataSize = OffsetOf(Count());
		auto oldDataSize = dataSize;
		dataSize = Memory::Aligned(dataSize, requiredAlignmentBytes);
		return dataSize - oldDataSize;
	}

	//! Подготовить блоб к десериализации, создав в нём секции данных и заголовка нужного размера.
	//! Все предыдущие данные становятся невалидны и должны быть перезаписаны.
	//! При необходимости пользователь предварительно должен корректно удалить все объекты, которые здесь находятся.
	//! Если выделенный блок памяти содержит недостаточно места для размещения данных указанного размера, функция ничего не делает и возвращает false.
	bool SetCounts(size_t dataSize, size_t elemCount) const
	{
		if(!CheckOffsetForOverflow(dataSize)) return false;
		if(dataSize + (elemCount + 2) * sizeof(OffsetType) > RawData.Length()) return false;
		Count() = elemCount;
		OffsetOf(elemCount) = OffsetType(dataSize);
		return true;
	}

	forceinline OffsetType& Count() const {return *(HeaderEnd() - 1);}
	forceinline bool Empty() const {return Count() != 0;}

	//! Суммарный размер всех элементов, помещённых в контейнер без учёта заголовка
	forceinline OffsetType& DataSizeInBytes() const {return OffsetOf(Count());}
	
	//! Суммарный размер заголовка
	forceinline size_t HeaderSizeInBytes() const {return (Count() + 2) * sizeof(OffsetType);}

	//! Свободное место в текущем блоке памяти
	forceinline size_t FreeSizeInBytes() const
	{return RawData.Length() - UsedSizeInBytes();}

	//! Размер элемента с указанным индексом
	forceinline OffsetType SizeOf(size_t index) const
	{return OffsetOf(index + 1) - OffsetOf(index);}

	//! Размер данных и заголовка, содержащихся в блобе
	forceinline size_t UsedSizeInBytes() const {return DataSizeInBytes() + HeaderSizeInBytes();}


	forceinline bool CanFitElement(size_t bytes) const
	{return FreeSizeInBytes() >= bytes + sizeof(OffsetType);}

	//! Вставить элемент указанного размера в блоб.
	//! Метод лишь выдаёт сырую неинициализированную память.
	//! Ответственность по её инициализации и последующему вызову деструкторов объектов лежит на пользователе.
	void* AddElement(OffsetType bytes) const
	{
		const size_t newDataSizeInBytes = DataSizeInBytes() + bytes;
		if(!CheckOffsetForOverflow(newDataSizeInBytes)) return null;
		const bool needReallocateBlob = !CanFitElement(bytes);
		if(needReallocateBlob) return null;
		void* result = DataBegin();
		OffsetOf(++Count()) = OffsetType(newDataSizeInBytes);
		return result;
	}

	void* AddElement(OffsetType bytes, OffsetType alignment) const
	{
		const OffsetType oldDataEndOffset = DataSizeInBytes();
		Align(alignment);
		void* result = AddElement(bytes);
		if(result == null) DataSizeInBytes() = oldDataEndOffset;
		return result;
	}


	//! Сконструировать новый объект в блобе, передав в конструктор указанные аргументы.
	//! Деструктор впоследствии не вызывается, ответственность за его вызов лежит на пользователе!
	//! Метод предполагает, что пользователь предварительно убедился в том, что добавляемый объект влезает в блоб.
	//! @returns Ссылка на сконструированный объект
	template<typename T, typename... Args> forceinline T& Add(Args&&... args) const
	{
		T* const result = TryAdd<T>(Cpp::Forward<Args>(args)...);
		INTRA_DEBUG_ASSERT(result != null);
		return *result;
	}

	//! Сконструировать новый объект в блобе, передав в конструктор указанные аргументы.
	//! Деструктор впоследствии не вызывается, ответственность за его вызов лежит на пользователе!
	//! @returns Указатель на сконструированный объект или null, если в блобе недостаточно места для его аллокации.
	template<typename T, typename... Args> forceinline T* TryAdd(Args&&... args) const
	{
		void* const data = AddElement(sizeof(T), alignof(T));
		if(data == null) return null;
		return &new(data) T(Cpp::Forward<Args>(args)...);
	}

	template<typename T> forceinline Span<T> AddArray(CSpan<T> arr) const
	{
		OffsetType oldDataEndOffset = DataSizeInBytes();
		void* const resultPtr = AddElement(sizeof(uint), sizeof(uint));
		if(resultPtr == null) return null;
		Align(alignof(T));
		const bool successfullyExtended = ExtendLastElement(OffsetType(arr.SizeInBytes())) == arr.SizeInBytes();
		if(!successfullyExtended)
		{
			RemoveLastElement();
			DataSizeInBytes() = oldDataEndOffset;
			return null;
		}
		uint* const lengthPtr = reinterpret_cast<uint*>(resultPtr);
		*lengthPtr = uint(arr.Length());
		void* const elementsStartPtr = DataBegin() + DataSizeInBytes();
		const Span<T> result = SpanOfRaw<T>(elementsStartPtr, arr.SizeInBytes());
		Memory::CopyInit(result, arr);
		return result;
	}

	forceinline void RemoveLastElement() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		Count()--;
	}

	//! Изменить размер последнего созданного элемента.
	//! При невозможности изменить размер на указанный, увеличивает размер до возможного предела.
	//! @returns Новый размер элемента.
	forceinline size_t ResizeLastElement(size_t newSizeInBytes) const
	{
		const size_t prevOffset = OffsetOf(Count() - 1);
		const size_t maxFitNewOffset = RawData.Length() - HeaderSizeInBytes();
		size_t newOffset = prevOffset + newSizeInBytes;
		if(newOffset > maxFitNewOffset) newOffset = maxFitNewOffset;
		OffsetOf(Count()) = OffsetType(newOffset);
		newSizeInBytes = newOffset - prevOffset;
		return newSizeInBytes;
	}

	//! Увеличить размер последнего созданного элемента.
	//! При невозможности изменить размер на указанный, увеличивает размер до возможного предела.
	//! @returns Количество байт, на которые увеличен размер элемента.
	forceinline size_t ExtendLastElement(size_t extraBytes) const
	{
		const size_t maxFitExtraBytes = FreeSizeInBytes();
		if(extraBytes > maxFitExtraBytes) extraBytes = maxFitExtraBytes;
		OffsetOf(Count()) += OffsetType(extraBytes);
		return extraBytes;
	}

	//! Увеличить размер последнего созданного элемента.
	//! При невозможности изменить размер на указанный, ничего не делает.
	//! @returns true, если операция была успешна.
	forceinline bool TryExtendLastElement(size_t extraBytes) const
	{
		if(extraBytes > FreeSizeInBytes()) return false;
		OffsetOf(Count()) += OffsetType(extraBytes);
		return true;
	}

	forceinline void* operator[](size_t index) const {return RawData.Begin + OffsetOf(index);}

	//! Получить типизированную ссылку на элемент с указанным индексом.
	//! Тип T должен совпадать с фактическим типом элемента, который был создан с этим индексом.
	//! Структура не отслеживает хранимые типы, поэтому это находится под ответственностью пользователя.
	template<typename T> forceinline T& Get(OffsetType index) const
	{
		INTRA_DEBUG_ASSERT(SizeOf(index) >= sizeof(T));
		return *static_cast<T*>(operator[](index));
	}

	//! Получить диапазон байт блоба, используетсый в данный момент
	//! для хранения самих данных - отдельно от заголовка и свободногго пространства
	forceinline Span<byte> DataBytes() const {return RawData.Take(DataSizeInBytes());}
	
	//! Получить диапазон блоба, содержащий размер данных, смещения содержащихся в блобе элементов в обратном порядке, а также их число в конце.
	forceinline Span<OffsetType> Header() const {return SpanOfPtr(HeaderBegin(), Count() + 2);}

	//! Пометить блоб как пустой. Все дальнейшие вызовы AddElement будут перезаписывать данные поверх предыдущих.
	//! Перед выполнением этой операции пользователь должен вызвать деинициализацию всех объектов.
	forceinline void Clear() const
	{
		Count() = 0;
		OffsetOf(0) = 0;
	}

	//! Предполагая, что все элементы блоба имеют тип T, получить диапазон этих элементов.
	//! Использовать с осторожностью, так как если это не так, будет UB
	template<typename T> forceinline BlobTypedRange<T, OffsetType> AsRangeOf() const
	{return BlobTypedRange<T, OffsetType>(RawData);}

	bool CopyTo(BlobAdapter<OffsetType> dst) const
	{
		if(!dst.SetCounts(DataSizeInBytes(), Count())) return false;
		CopyTo(DataBytes(), dst.DataBytes());
		CopyTo(Header(), dst.Header());
		return true;
	}

	forceinline bool MoveTo(BlobAdapter<OffsetType> dst) const
	{
		if(!CopyTo(dst)) return false;
		Clear();
		return true;
	}
};


//! Динамический массив для хранения полиморфных объектов типов, производных от T.
//! Требуется, чтобы T был единственным базовым классом хранимых объектов - множественное наследование не поддерживается.
template<typename T, typename OffsetType = uint,
	class AllocatorType = Memory::SystemHeapAllocator>
class DynamicBlob: public Memory::AllocatorRef<AllocatorType>
{
	typedef Memory::AllocatorRef<AllocatorType> AllocatorRef;

	BlobAdapter<OffsetType> mData;

	forceinline BlobAdapter<OffsetType> allocate(size_t sizeInBytes)
	{
		return {AllocatorRef::AllocateRangeUninitialized<byte>(sizeInBytes, INTRA_SOURCE_INFO)};
	}

	forceinline void deallocate(BlobAdapter<OffsetType> data)
	{
		AllocatorRef::FreeRangeUninitialized(data.RawData);
	}

	bool reallocate(size_t sizeInBytes)
	{
		auto newData = allocate(sizeInBytes);
		if(newData == null || !mData.MoveTo(newData)) return false;
		deallocate(mData);
		mData = newData;
		return true;
	}
public:
	forceinline DynamicBlob(size_t initialSizeInBytes):
		mData(allocate(initialSizeInBytes)) {}

	//! Сконструировать новый объект в блобе, передав в конструктор указанные аргументы.
	//! Тип создаваемого объекта должен быть тривиально перемещаемым.
	//! @returns Ссылка на сконструированный объект
	template<typename T1, typename... Args> forceinline Meta::EnableIf<
		Meta::IsInherited<T1, T>::_,
	T1&> Add(Args&&... args) const
	{
		mData.Align(alignof(T));
		T1* const result = mData.TryAdd<T1>(Cpp::Forward<Args>(args)...);
		if(result == null)
		{
			const size_t minRequiredSize = mData.UsedSizeInBytes() + sizeof(T);
			reallocate(minRequiredSize + minRequiredSize / 2);
			return mData.Add<T1>(Cpp::Forward<Args>(args)...);
		}
		return *result;
	}
};

}}
