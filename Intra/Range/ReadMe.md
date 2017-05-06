
# Intra Range module

This module defines most ranges of Intra library and algorithms working with them.

## What is range

Range is a concept replacing iterators and streams. It is a universal interface to work with any collections of elements. Range are more convenient and safe than STL iterators because range is a single entity - there is no separate begin and end, that led to more function arguments and had less possibility for validity checks. One of the best advantages of ranges is that they are extremely composable. There are many decorators in this module that turn one range into another: Filter, Map, Join, Retro, Split, ByLine, Stride, Take, Buffered, Chain, Indexed, Zip, Unzip. Ranges are lazy evaluated so they don't create any intermediate arrays or allocate memory. Each of them may be used multiple times to form range of any complexity. In combination with algorithms such as Copy, CopyUntil, Find, Reduce, Transform, Fill and ForEach, it enables programmer to write very high-level functional code in C++. And due to compiler inlining, this doesn't decrease run-time performance.

There are several range categories. In the order from the most common to the most powerful:
- Input Range: any class that has the following methods: `bool Empty()`, `T First()` and `void PopFirst()`.
- Forward Range: any copyable Input Range.
- Bidirectional Range: Forward Range with additional methods `T Last()` and `void PopLast()`.
- Random Access Range: Forward Range with `T operator[](size_t index)`.
- Finite Random Access Range: Bidirectional Range with `T operator[](size_t index)` and `size_t Length()`. It is also simply Random Access Range.
- Array Range: Finite Random Access Range with `T* Data()`.

Also there is Output Range concept - it is class with `void Put(T)` and `bool Full()` methods.

Any char range is a stream and may be used with << (output stream) and >> (input stream) operators.
[Example](../../Demos/UnitTests/src/Range/Streams.cpp) of defining own stream and to formatting string on the stack via Span<char>.

There are type-erased polymorphic ranges that may be passed as function arguments without templates. These ranges have optimized copy operation so you don't need to pay virtual call overhead per each element being copied. This optimization is especially necessary for streams.

Ranges can be constructed from iterators, so they are partially compatible with STL containers. [Example](../../Demos/UnitTests/src/Range/StlInterop.cpp)

All these concepts and other useful traits are defined in the [Concepts](../Concepts) module.
It is very simple to create class satisfying range concept. You only need to define a couple of methods required by concept.
Unlike STL iterators you don't need to write boilerplate code - there is no need to declare special types with both variants of increment\decrement operation or to derive your class from special base class.
Once your class has necessary methods, it automatically becomes compatible with most algorithms defined in this module, with other modules that use concept of range and even with range-based for.

See examples [here](../../Demos/UnitTests/src/Range).
 
## �������� �� ������� �����

� ������ ������ ������������ ������� ����� ���������� ���������� Intra, � ����� ���������, ���������� � ����.

## ��� ����� ��������

�������� - ��� �������, ���������� ��������� � ������. ��� ������������� ��������� ��� ������ � ������ ����������� ���������. ��������� ����� ������ � ���������, ��� ���������, ������ ��� �������� ������������ ����� ������ ��������, � �� ��������� ������ � �����, ������� ��������� � �������� ���������� ���������� ������� � ����� ������ ������������ ��� �������� ������������. ����� �� ����� ������� ����������� ���������� �������� ��, ��� �� ����� ����� �������������. � ���� ������ ������������ ��������� �����������, ������� ���������� ���� �������� � ������: Filter, Map, Join, Retro, Split, ByLine, Stride, Take, Buffered, Chain, Indexed, Zip, Unzip. ��������� ���������� ������� ����������, ��� ��� ��� �� ������� ������������� �������� � �� �������� ������. ������ �� ��� ����� ������������ ����� ������ ����� ���, �������� ��������� ����� ���������. � ��������� � ������ �����������, ��� Copy, CopyUntil, Find, Reduce, Transform, Fill � ForEach, ��� ��������� ������������ ������ ����� ��������������� �������������� ��� �� C++. � ��������� ����������� ������������ ������������� �������, ��� �� ������� �������� ���������� ����������������� ����.

���������� ��������� ��������� ����������. � ������� �� ����� ����� � �������� ��������������:
- Input Range: ����� �����, ������� ��������� ������: `bool Empty()`, `T First()` � `void PopFirst()`.
- Forward Range: ����� Input Range, ������� ����� ����������.
- Bidirectional Range: Forward Range � ��������������� �������� `T Last()` � `void PopLast()`.
- Random Access Range: Forward Range � `T operator[](size_t index)`.
- Finite Random Access Range: Bidirectional Range � `T operator[](size_t index)` � `size_t Length()`. �� ����� �������� Random Access Range.
- Array Range: Finite Random Access Range � `T* Data()`.

����� ���� ������� Output Range - ��� ����� � �������� `void Put(T)` � `bool Full()`.

����� �������� �������� ����� �������� ������� � ����� ���� ����������� � ����������� << (��� output-���������) � >> (��� input-���������).
[������](../../Demos/UnitTests/src/Range/Streams.cpp) ����������� ������������ ������ � �������������� ������ �� ����� � ������� Span<char>.

����� � ������ ���������� type-erased ����������� ���������, ������� ����� ���������� ��� ��������� ����������� �������. ��� ��������� ����� ���������������� �������� �����������, ������� ��� �� ����� ������� ������������������� �� ������ ���������� �������. �������� ���������� ��� ����������� ��� �������.

��������� ����� �������������� �� ����������. ����� �������, ��� �������� ���������� � STL-������������. [������](../../Demos/UnitTests/src/Range/StlInterop.cpp)

��� ��� �������� � ������ �������� �������������� ���������� � ������ [Concepts](../Concepts).
������� �����, ��������������� �������� ��������� ����� ������. ��� ����� ������ ���������� ���� �������, ������� ��������� ���������.
� ������� �� ���������� STL ��� �� ����� ������ ������ ���, ��������� � ����������� ����������� ����� � ���� ��������� ��������� ���������� � ����������. � �� ����� ������������� �� ������������ �������� ������.
��� ������ � ����� ������ ���������� ������ ������, �� ������������� ���������� ����������� � ������������ ����������, ����������� � ������ ������, � ������� ��������, ������������� ��������� ����������, � ���� � ������� range-based for �� C++11.

������� �������� [�����](../../Demos/UnitTests/src/Range).
