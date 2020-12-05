
# Core/Range subpackage

This subpackage defines range concepts, generators, wrappers, decorators and composition operations.

## What is range

Range is a concept replacing iterators and streams. It is a universal interface to work with any collections of elements. Ranges are more convenient and safe than STL iterators because range is a single entity - there is no separate begin and end, which lead to more function arguments and have less possibility for validity checks.

One of the best advantages of ranges is that they are extremely composable. There are many decorators in this module that turn one range into another: Filter, Map, Join, Retro, Split, ByLine, Stride, Take, Buffered, Chain, Indexed, Zip, Unzip. Ranges are lazy evaluated so they don't create any intermediate arrays or allocate memory. Each of them may be used multiple times to form range of any complexity. In combination with algorithms such as Copy, CopyUntil, Find, Reduce,, Fill and ForEach it enables programmer to write very high-level functional code in C++. And due to compiler inlining, this doesn't decrease run-time performance.

There are several range categories. In the order from the most common to the most powerful:
- Input Range: any class that has the following methods: `bool Empty()`, `T First()` and `void PopFirst()`.
- Forward Range: any copyable Input Range.
- Bidirectional Range: Forward Range with additional methods `T Last()` and `void PopLast()`.
- Random Access Range: Forward Range allowing indexed access with`T operator[](index_t index)`.
- Finite Random Access Range: Bidirectional Range with `T operator[](size_t index)` and `size_t Length()`. It is also a Random Access Range.

Also there is a separate concept Output Range - it is class with `void Put(T)` and `bool Full()` methods.

Any char range is a stream and may be used with << (output stream) and >> (input stream) operators.
[Example](../../Demos/UnitTests/src/Range/Streams.cpp) of defining own stream and to formatting string on the stack via Span<char>.

There are type-erased polymorphic ranges that may be passed as function arguments without templates. These ranges have optimized copy operation so you don't need to pay virtual call overhead per each element being copied. This optimization is especially necessary for streams.

Ranges can be constructed from iterators, so they are partially compatible with STL containers. [Example](../../Demos/UnitTests/src/Range/StlInterop.cpp)
It is very easy to create class satisfying CRange concept. You only need to define three methods required by it. Once your class has necessary methods, it automatically becomes compatible with most algorithms defined in this module, with other modules that use concept of range and even with range-based for.
Unlike STL iterators you don't need to write boilerplate code - there is no need to declare special types with both variants of increment\decrement operation or to derive your class from special base class. But you can derive from RangeMixin to enable left-to-right syntax to use in your code.

See examples [here](../../Demos/UnitTests/src/Range).
