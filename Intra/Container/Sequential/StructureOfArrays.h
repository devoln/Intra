#pragma once

#if INTRA_DISABLED

#include "Meta/TypeList.h"
#include "Meta/Tuple.h"

namespace Intra {

template<typename ...T> class StructureOfArrays
{
public:
	typedef Tuple<T...> AoS;
	typedef Tuple<T*...> SoA;
	typedef TypeList<T...> TL;
	enum: uint {ColumnCount = TypeListLength(TL);};

	void AddLast()
	{
		CheckSpace(1);
		add<0>();
	}

	void CheckSpace(size_t space) {Reserve(count+space);}

	void Reserve(size_t capacityToReserve)
	{
		if(capacityToReserve>size)
			Resize(Math::max(capacityToReserve, size+size/2));
	}

	void Resize(size_t size);

	void SetCount(size_t newCount);

	size_t Count() const {return count;}
	size_t Size() const {return size;}

private:
	SoA data;
	size_t count, size;

	template<uint I> void add()
	{
		new(data.Get<I>()) TypeListTypeAt<I, TL>;
		add<I+1>();
	}

	template<> void add() {}


	template<uint I> void add(const AoS& aos)
	{
		new(data.Get<I>()) TypeListTypeAt<I, TL>(aos.Get<I>());
		add<I+1>(aos);
	}

	template<> void add(const AoS& aos) {}


	template<uint I> void add(AoS&& aos)
	{
		new(data.Get<I>()) TypeListTypeAt<I, TL>(std::move(aos.Get<I>()));
		add<I+1>(std::move(aos));
	}

	template<> void add(AoS&& aos) {}
};

}

#endif
