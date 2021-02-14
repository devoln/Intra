#pragma once

#include "IntraX/Container/Sequential/Array.h"

namespace Intra { INTRA_BEGIN
template<typename T> struct OwningArrayRange
{
	INTRA_FORCEINLINE OwningArrayRange(Array<T> elements): Elements(Move(elements)) {}

	INTRA_FORCEINLINE T& First() {return Elements.First();}
	INTRA_FORCEINLINE const T& First() const {return Elements.First();}
	INTRA_FORCEINLINE void PopFirst() {Elements.RemoveFirst();}

	INTRA_FORCEINLINE T& Last() {return Elements.Last();}
	INTRA_FORCEINLINE const T& Last() const {return Elements.Last();}
	INTRA_FORCEINLINE void PopLast() {Elements.RemoveLast();}

	INTRA_FORCEINLINE bool Empty() const {return Elements.Empty();}

	INTRA_FORCEINLINE index_t Length() const {return Elements.Length();}
	INTRA_FORCEINLINE T& operator[](size_t index) {return Elements[index];}
	INTRA_FORCEINLINE const T& operator[](size_t index) const {return Elements[index];}

	INTRA_FORCEINLINE T* Data() {return Elements.Data();}
	INTRA_FORCEINLINE const T* Data() const {return Elements.Data();}

	INTRA_FORCEINLINE Span<T> AsSpan() {return Elements;}
	INTRA_FORCEINLINE Span<T> AsSpan() const {return Elements;}

	Array<T> Elements;
};
} INTRA_END
