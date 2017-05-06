
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
 
## Описание на русском языке

В данном модуле определяется большая часть диапазонов библиотеки Intra, а также алгоритмы, работающие с ними.

## Что такое диапазон

Диапазон - это понятие, заменяющее итераторы и потоки. Это универсальный интерфейс для работы с любыми коллекциями элементов. Итераторы более удобны и безопасны, чем итераторы, потому что диапазон представляет собой единую сущность, а не отдельные начало и конец, которые приводили к большему количеству аргументов функций и имели меньше возможностей для проверок корректности. Одним из самых главных преимуществ диапазонов является то, что их очень легко комбинировать. В этом модуле присутствует множество декораторов, которые превращают один диапазон в другой: Filter, Map, Join, Retro, Split, ByLine, Stride, Take, Buffered, Chain, Indexed, Zip, Unzip. Диапазоны используют ленивые вычисления, так что они не создают промежуточных массивов и не выделяют память. Каждый из них можно использовать сколь угодно много раз, формируя диапазоны любой сложности. В сочетании с такими алгоритмами, как Copy, CopyUntil, Find, Reduce, Transform, Fill и ForEach, это позволяет программисту писать очень высокоуровневый функциональный код на C++. И благодаря подстановки компилятором промежуточных вызовов, это не снижает скорость выполнения скомпилированного кода.

Существует несколько категорий диапазонов. В порядке от самых общих к наиболее функциональным:
- Input Range: любой класс, имеющий следующие методы: `bool Empty()`, `T First()` и `void PopFirst()`.
- Forward Range: любой Input Range, который можно копировать.
- Bidirectional Range: Forward Range с дополнительными методами `T Last()` и `void PopLast()`.
- Random Access Range: Forward Range с `T operator[](size_t index)`.
- Finite Random Access Range: Bidirectional Range с `T operator[](size_t index)` и `size_t Length()`. Он также является Random Access Range.
- Array Range: Finite Random Access Range с `T* Data()`.

Также есть концепт Output Range - это класс с методами `void Put(T)` и `bool Full()`.

Любой диапазон символов также является потоком и может быть использован с операторами << (для output-диапазона) и >> (для input-диапазона).
[Пример](../../Demos/UnitTests/src/Range/Streams.cpp) определения собственного потока и форматирования строки на стеке с помощью Span<char>.

Также в модуле определены type-erased полиморфные диапазоны, которые можно передавать как аргументы нешаблонных функций. Эти диапазоны имеют оптимизированную операцию копирования, поэтому вам не нужно платить производительностью за каждый копируемый элемент. Особенно необходима эта оптимизация для потоков.

Диапазоны можно конструировать из итераторов. Таким образом, они частично совместимы с STL-контейнерами. [Пример](../../Demos/UnitTests/src/Range/StlInterop.cpp)

Все эти концепты и другие полезные характеристики определены в модуле [Concepts](../Concepts).
Создать класс, удовлетворяющий концепту диапазона очень просто. Вам нужно только определить пару методов, которые требуются концептом.
В отличие от итераторов STL вам не нужно писать лишний код, связанный с объявлением специальных типов и двух вариантов оператора инкремента и декремента. И не нужно наследоваться от специального базового класса.
Как только в вашем классе появляются нужные методы, он автоматически становится совместимым с большинством алгоритмов, определённых в данном модуле, с другими модулями, использующими концепцию диапазонов, и даже с циклами range-based for из C++11.

Примеры смотрите [здесь](../../Demos/UnitTests/src/Range).
