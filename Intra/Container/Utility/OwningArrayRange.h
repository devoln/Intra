#pragma once

#include "Container/Sequential/Array.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Container {

template<typename T> struct OwningArrayRange
{
	forceinline OwningArrayRange(Array<T> elements): Elements(Cpp::Move(elements)) {}

	forceinline T& First() {return Elements.First();}
	forceinline const T& First() const {return Elements.First();}
	forceinline void PopFirst() {Elements.RemoveFirst();}

	forceinline T& Last() {return Elements.Last();}
	forceinline const T& Last() const {return Elements.Last();}
	forceinline void PopLast() {Elements.RemoveLast();}

	forceinline bool Empty() const {return Elements.Empty();}

	forceinline size_t Length() const {return Elements.Length();}
	forceinline T& operator[](size_t index) {return Elements[index];}
	forceinline const T& operator[](size_t index) const {return Elements[index];}

	forceinline T* Data() {return Elements.Data();}
	forceinline const T* Data() const {return Elements.Data();}

	forceinline Span<T> AsSpan() {return Elements;}
	forceinline Span<T> AsSpan() const {return Elements;}

	Array<T> Elements;
};

}
using Container::OwningArrayRange;

}

INTRA_WARNING_POP
