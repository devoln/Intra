#pragma once

#include "Extra/Utils/FixedArray.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Extra/Memory/Align.h"
#include "Extra/Memory/Memory.h"
#include "Extra/Memory/Allocator/System.h"
#include "Extra/Memory/Allocator/AllocatorRef.h"

INTRA_BEGIN
//TODO: use <IndexListItems> in natvis to visualize this type

template<typename T, typename OffsetType> class BlobTypedRange
{
	CSpan<byte> mData;
	CSpan<OffsetType> mOffsetSizes;
public:
	INTRA_FORCEINLINE BlobTypedRange(CSpan<byte> blobData)
	{
		CSpan<OffsetType> otdata = blobData.ReinterpretUnsafe<const OffsetType>();
		mOffsetSizes = otdata.DropLast().Tail(2*otdata.Last());
		mData = blobData.Take(mOffsetSizes[0]+mOffsetSizes[1]);
	}

	INTRA_FORCEINLINE T* First() const {return reinterpret_cast<T*>(mData.Begin + mOffsetSizes[mOffsetSizes.Length()-2]);}
	INTRA_FORCEINLINE T* Last() const {return reinterpret_cast<T*>(mData.Begin + mOffsetSizes[0]);}
	INTRA_FORCEINLINE void PopFirst() {mOffsetSizes.PopLastExactly(2);}
	INTRA_FORCEINLINE void PopLast() {mOffsetSizes.PopFirstExactly(2);}
	INTRA_FORCEINLINE bool Empty() const {return mOffsetSizes.Empty();}
	INTRA_FORCEINLINE index_t Length() const {return mOffsetSizes.Length()/2;}
	
	INTRA_FORCEINLINE T* Next()
	{T* const result = First(); PopFirst(); return result;}
	
	INTRA_FORCEINLINE T* operator[](size_t index) const
	{return reinterpret_cast<T*>(mData.Begin + mOffsetSizes[mOffsetSizes.Length() - 2 - 2*index]);}

	INTRA_FORCEINLINE size_t PopFirstCount(size_t count) {return mOffsetSizes.PopLastCount(2*count);}
	INTRA_FORCEINLINE size_t PopLastCount(size_t count) {return mOffsetSizes.PopFirstCount(2*count);}
};


//! Класс, интерпретирующий массив байт как blob, который состоит из объектов переменной длины,
//! размещающихся слева направо и заголовка, растущего справа налево.
//! Все методы константные, так как сами данные не являются внутренним состоянием объекта и могут быть изменены извне прямой записью байт в RawData.
//! Формат хранения: [[0], [1], ..., [Count-1], OffsetOf(Count-1), SizeOf(Count-1), OffsetOf(1), SizeOf(1), OffsetOf(0), SizeOf(0), Count]
//! Этот класс не является типобезопасным и требует соблюдения определённых правил, указанных в описании методов.
//! Нарушение этих правил приводит к неопределённому поведению программы (отсутствие вызова деструктора, кратный вызов деструктора, изменение адреса объекта в памяти и т.п.).
template<typename OffsetType> struct BlobAdapterUnsafe
{
	enum: size_t {PerElementOverheadBytes = 2 * sizeof(OffsetType)};

	Span<byte> RawData;

	INTRA_FORCEINLINE OffsetType* HeaderEnd() const {return reinterpret_cast<OffsetType*>(RawData.End);}
	INTRA_FORCEINLINE OffsetType* HeaderBegin() const {return HeaderEnd() - (1 + 2*Count());}

	INTRA_FORCEINLINE byte* DataBegin() const {return RawData.Begin;}
	INTRA_FORCEINLINE byte* DataEnd() const {return RawData.Begin + DataSizeInBytes();}

	INTRA_FORCEINLINE OffsetType& OffsetOf(size_t index) const
	{
		INTRA_DEBUG_ASSERT(index < Count());
		return *(HeaderEnd() - (3 + 2*index));
	}

	//! Размер элемента с указанным индексом
	INTRA_FORCEINLINE OffsetType& SizeOf(size_t index) const
	{
		INTRA_DEBUG_ASSERT(index < Count());
		return *(HeaderEnd() - 2 * (1 + index));
	}

	INTRA_FORCEINLINE static bool CheckOffsetForOverflow(size_t offset)
	{return size_t(OffsetType(offset)) == offset;}

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

	INTRA_FORCEINLINE OffsetType& Count() const {return *(HeaderEnd() - 1);}
	INTRA_FORCEINLINE bool Empty() const {return Count() != 0;}

	//! Суммарный размер всех элементов, помещённых в контейнер без учёта заголовка
	INTRA_FORCEINLINE size_t DataSizeInBytes() const {return size_t(OffsetOf(Count()-1))+size_t(SizeOf(Count()-1));}
	
	//! Суммарный размер заголовка
	INTRA_FORCEINLINE size_t HeaderSizeInBytes() const {return (1 + 2*Count()) * sizeof(OffsetType);}

	//! Свободное место в текущем блоке памяти
	INTRA_FORCEINLINE size_t FreeSizeInBytes() const
	{return RawData.Length() - UsedSizeInBytes();}

	//! Размер данных и заголовка, содержащихся в блобе
	INTRA_FORCEINLINE size_t UsedSizeInBytes() const {return DataSizeInBytes() + HeaderSizeInBytes();}


	INTRA_FORCEINLINE bool CanFitElement(size_t bytes) const
	{return FreeSizeInBytes() >= bytes + PerElementOverheadBytes;}

	//! Вставить элемент указанного размера в блоб.
	//! Метод лишь выдаёт сырую неинициализированную память.
	//! Ответственность по её инициализации и последующему вызову деструкторов объектов лежит на пользователе.
	void* AddElement(OffsetType bytes, OffsetType alignment) const
	{
		const auto prevDataSize = DataSizeInBytes();
		const auto offset = Aligned(prevDataSize, alignment);
		const size_t newDataSizeInBytes = offset + bytes;
		const bool elementFitsInContainer = RawData.Begin+newDataSizeInBytes < reinterpret_cast<byte*>(HeaderBegin()-2);
		if(!CheckOffsetForOverflow(offset) || !elementFitsInContainer) return null;
		Count()++;
		HeaderBegin()[0] = OffsetType(prevDataSize);
		HeaderBegin()[1] = bytes;
		return RawData.Begin + offset;
	}


	//! Сконструировать новый объект в блобе, передав в конструктор указанные аргументы.
	//! Деструктор впоследствии не вызывается, ответственность за его вызов лежит на пользователе!
	//! Метод предполагает, что пользователь предварительно убедился в том, что добавляемый объект влезает в блоб.
	//! @returns Ссылка на сконструированный объект
	template<typename T, typename... Args> INTRA_FORCEINLINE T& Add(Args&&... args) const
	{
		T* const result = TryAdd<T>(Forward<Args>(args)...);
		INTRA_DEBUG_ASSERT(result != null);
		return *result;
	}

	//! Сконструировать новый объект в блобе, передав в конструктор указанные аргументы.
	//! Деструктор впоследствии не вызывается, ответственность за его вызов лежит на пользователе!
	//! @returns Указатель на сконструированный объект или null, если в блобе недостаточно места для его аллокации.
	template<typename T, typename... Args> INTRA_FORCEINLINE T* TryAdd(Args&&... args) const
	{
		void* const data = AddElement(sizeof(T), alignof(T));
		if(data == null) return null;
		return &new(data) T(Forward<Args>(args)...);
	}

	INTRA_FORCEINLINE void RemoveLastElement() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		Count()--;
	}

	//! Изменить размер последнего созданного элемента.
	//! При невозможности изменить размер на указанный, увеличивает размер до возможного предела.
	//! @returns Новый размер элемента.
	INTRA_FORCEINLINE size_t ResizeLastElement(size_t newSizeInBytes) const
	{
		const size_t maxFitSizeInBytes = HeaderBegin()[1] + FreeSizeInBytes();
		if(newSizeInBytes > maxFitSizeInBytes) newSizeInBytes = maxFitSizeInBytes;
		HeaderBegin()[1] = OffsetType(newSizeInBytes);
		return newSizeInBytes;
	}

	//! Увеличить размер последнего созданного элемента.
	//! При невозможности изменить размер на указанный, увеличивает размер до возможного предела.
	//! @returns Количество байт, на которые увеличен размер элемента.
	INTRA_FORCEINLINE size_t ExtendLastElement(size_t extraBytes) const
	{
		const size_t maxFitExtraBytes = FreeSizeInBytes();
		if(extraBytes > maxFitExtraBytes) extraBytes = maxFitExtraBytes;
		HeaderBegin()[1] += OffsetType(extraBytes);
		return extraBytes;
	}

	//! Увеличить размер последнего созданного элемента.
	//! При невозможности изменить размер на указанный, ничего не делает.
	//! @returns true, если операция была успешна.
	INTRA_FORCEINLINE bool TryExtendLastElement(size_t extraBytes) const
	{
		if(extraBytes > FreeSizeInBytes()) return false;
		HeaderBegin()[1] += OffsetType(extraBytes);
		return true;
	}

	INTRA_FORCEINLINE void* operator[](size_t index) const {return RawData.Begin + OffsetOf(index);}

	//! Получить типизированную ссылку на элемент с указанным индексом.
	//! Тип T должен совпадать с фактическим типом элемента, который был создан с этим индексом.
	//! Структура не отслеживает хранимые типы, поэтому это находится под ответственностью пользователя.
	template<typename T> INTRA_FORCEINLINE T& Get(OffsetType index) const
	{
		INTRA_DEBUG_ASSERT(SizeOf(index) >= sizeof(T));
		return *static_cast<T*>(operator[](index));
	}

	//! Получить диапазон байт блоба, используетсый в данный момент
	//! для хранения самих данных - отдельно от заголовка и свободногго пространства
	INTRA_FORCEINLINE Span<byte> DataBytes() const {return RawData.Take(DataSizeInBytes());}
	
	//! Получить диапазон блоба, содержащий размер данных, смещения содержащихся в блобе элементов в обратном порядке, а также их число в конце.
	INTRA_FORCEINLINE Span<OffsetType> Header() const {return SpanOfPtr(HeaderBegin(), Count() + 2);}

	//! Пометить блоб как пустой. Все дальнейшие вызовы AddElement будут перезаписывать данные поверх предыдущих.
	//! Перед выполнением этой операции пользователь должен вызвать деинициализацию всех объектов.
	INTRA_FORCEINLINE void Clear() const
	{
		Count() = 0;
		OffsetOf(0) = 0;
	}

	//! Предполагая, что все элементы блоба имеют тип T, получить диапазон этих элементов.
	//! Использовать с осторожностью, так как если это не так, будет UB.
	template<typename T> INTRA_FORCEINLINE auto AsRangeOf() const
	{return BlobTypedRange<T, OffsetType>(RawData);}

	//! Копирует все данные блоба в новый блоб как есть - вместе со всеми его пустотами, стирая все старые данные целевого блоба.
	//! При условии, что элементы являются тривиально копируемыми, dst по использованию становится эквивалентен исходному блобу.
	//! @returns true, если копирование произведено успешно - все данные поместились в dst.
	bool CopyTo(BlobAdapterUnsafe<OffsetType> dst) const
	{
		if(!dst.SetCounts(DataSizeInBytes(), Count())) return false;
		Intra::CopyTo(DataBytes(), dst.DataBytes());
		Intra::CopyTo(Header(), dst.Header());
		return true;
	}

	//! Переносит все данные блоба в новый блоб как есть - вместе со всеми его пустотами, стирая все старые данные целевого блоба и очищая текущий блоб.
	//! При условии, что элементы являются тривиально перемещаемыми, dst по использованию становится эквивалентен исходному блобу до перемещения.
	//! Если в dst недостаточно места, оба блоба остаются в исходном состоянии.
	//! @returns true, если копирование произведено успешно - все данные поместились в dst.
	INTRA_FORCEINLINE bool MoveTo(BlobAdapterUnsafe<OffsetType> dst) const
	{
		if(!CopyTo(dst)) return false;
		Clear();
		return true;
	}

	//! Копирует элементы текущего блоба, добавляя их в новый блоб. При этом игнорирует все пустоты и не копирует элементы нулевого размера.
	//! При ошибке нехватки места в dst, dst приводится в состояние, эквивалентное исходному - изменяются только данные между DataEnd() и HeaderBegin().
	//! Все элементы должны быть тривиально копируемыми, иначе поведение не определено.
	//! @returns true, если копирование произведено успешно - все данные поместились в dst.
	bool AppendDefragmentTo(BlobAdapterUnsafe<OffsetType> dst, size_t alignment) const
	{
		const OffsetType prevDstCount = dst.Count();
		for(size_t i = 0; i < Count(); i++)
		{
			const size_t size = SizeOf(i);
			if(size == 0) continue;
			void* const el = dst.AddElement(size, alignment);
			if(el == null)
			{
				dst.Count() = prevDstCount;
				return false;
			}
			memcpy(el, operator[](i), SizeOf(i));
		}
		return true;
	}

	//! Перемещает элементы текущего блоба, добавляя их в новый блоб. При этом игнорирует все пустоты и не копирует элементы нулевого размера.
	//! При ошибке нехватки места в dst, dst приводится в состояние, эквивалентное исходному - изменяются только данные между DataEnd() и HeaderBegin().
	//! Все элементы должны быть тривиально перемещаемыми, иначе поведение не определено.
	//! @returns true, если копирование произведено успешно - все данные поместились в dst.
	INTRA_FORCEINLINE bool MoveAppendDefragmentTo(BlobAdapterUnsafe<OffsetType> dst, size_t alignment) const
	{
		if(!AppendDefragmentTo(dst, alignment)) return false;
		Clear();
		return true;
	}
};

//TODO: использовать <IndexListItems> в natvis для визуализации этого типа

INTRA_DEFINE_CONCEPT_REQUIRES(HasMoveConstruct, Val<T>().MoveConstruct(Val<void*>()));

//! Динамический массив для хранения полиморфных объектов типов, производных от T, содержащего виртуальный метод CopyConstruct(void* dst) и виртуальный деструктор.
//! Требуется, чтобы T был единственным базовым классом хранимых объектов - множественное наследование не поддерживается.
template<typename T, size_t Alignment = alignof(T), typename OffsetType = unsigned,
	class AllocatorType = SystemHeapAllocator>
class DynamicBlob: public AllocatorRef<AllocatorType>
{
	static_assert(HasMoveConstruct<T> && CHasVirtualDestructor<T>, "T must implement a virtual destructor and MoveConstruct(void* dst)!");
	typedef AllocatorRef<AllocatorType> AllocatorRef;

	BlobAdapterUnsafe<OffsetType> mData;
	OffsetType mFirstDeleted = OffsetType(-1);

	INTRA_FORCEINLINE BlobAdapterUnsafe<OffsetType> allocate(size_t sizeInBytes)
	{
		return {AllocatorRef::template AllocateRangeUninitialized<byte>(sizeInBytes, INTRA_SOURCE_INFO)};
	}

	INTRA_FORCEINLINE void deallocate(BlobAdapterUnsafe<OffsetType> data)
	{
		AllocatorRef::FreeRangeUninitialized(data.RawData);
	}

	bool reallocate(size_t sizeInBytes)
	{
		auto newData = allocate(sizeInBytes);
		if(newData == null) return false;
		for(size_t i = 0; i < mData.Count(); i++)
		{
			const size_t size = mData.SizeOf(i);
			auto el = newData.AddElement(size, Alignment);
			if(size != 0) mData[i].MoveConstructTo(el);
		}
		deallocate(mData);
		mData = newData;
		return true;
	}
public:
	INTRA_FORCEINLINE DynamicBlob(size_t initialSizeInBytes):
		mData(allocate(initialSizeInBytes)) {}

	INTRA_FORCEINLINE ~DynamicBlob() {Clear();}

	void Clear()
	{
		for(OffsetType i = 0; i < mData.Count(); i++)
		{
			if(mData.SizeOf(size_t(i)) == 0) continue;
			mData.template Get<T>(size_t(i)).~T();
		}
		mData.Clear();
	}

	//! Сконструировать новый объект в блобе, передав в конструктор указанные аргументы.
	//! @returns Ссылка на сконструированный объект
	template<typename T1, typename... Args> Requires<
		CDerived<T1, T>,
	T1&> Add(Args&&... args)
	{
		T1* const result = mData.template TryAdd<T1>(Forward<Args>(args)...);
		if(result == null)
		{
			const size_t minRequiredSize = mData.UsedSizeInBytes() + sizeof(T);
			reallocate(minRequiredSize + minRequiredSize / 2);
			result = mData.template TryAdd<T1>(Forward<Args>(args)...);
		}
		if(mFirstDeleted != OffsetType(-1))
		{
			const OffsetType nextDeleted = mData.OffsetOf(mFirstDeleted);
			mData.OffsetOf(mFirstDeleted) = mData.HeaderBegin()[0];
			mData.SizeOf(mFirstDeleted) = mData.HeaderBegin()[1];
			mData.Count()--;
			mFirstDeleted = nextDeleted;
		}
		return *result;
	}

	//! Удаляет элемент в массиве
	INTRA_FORCEINLINE void Delete(size_t index)
	{
		if(T* const element = operator[](index))
		{
			element->~T();
			mData.SizeOf(index) = 0;
			mData.OffsetOf(index) = mFirstDeleted;
			mFirstDeleted = OffsetType(index);
		}
	}

	INTRA_FORCEINLINE T* operator[](size_t index) const
	{
		if(mData.SizeOf(index) == 0) return null;
		return mData[OffsetType(index)];
	}

	INTRA_FORCEINLINE index_t Length() const {return size_t(mData.Count());}

	INTRA_FORCEINLINE T& Get(size_t index) const {return mData.Get(index);}

	INTRA_FORCEINLINE BlobTypedRange<T, OffsetType> AsRange() const
	{return BlobTypedRange<T, OffsetType>(mData.RawData);}
};
INTRA_END
