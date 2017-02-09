
# IntraLib

## English description

This repository contains Intra library and its modules:
- [Core](Intra Core)
- [Audio](Intra Audio)
- [Image](Intra Image)

This library contains the following functionality:
- Containers: Array, BList, HashMap, LinearMap, String and others. Array is different from existing implementations of dynamic arrays by fast O(1) appending to the beginning of array.
- Ranges and algorithms working with them.
 Range are implemented in the style of D language standard library. Range concept replaces iterator concept and provides more comfortable, functional and secure abstraction than STL iterators.
 Unlike STL iterators, ranges support decoration and composition, that enables programmer to create complex ranges, write code in functional style and to implement lazy evaluation.
 See examples [here](Demos/Tests/src/Ranges).
- Automatic recursive structure serialization: binary and text. See examples [here](Demos/Tests/src/PerfTestSerialization.cpp).
- Math functions and classes: FixedPoint, vectors, matrices, quaternions, geometric primitives.
- Multiple image format loading.
- Sound system and music intrument synthesis.
- etc: timer, IO streams, basic classes for multithreading.

In addition to the library this repository also contains 3 demos projects:
- [Demos/MusicSynthesizer](MusicSynthesizer) - MIDI synthesizer. This project consist only of one file with main function. Most of the synthesizer code is located at IntraLib/Sound.
- [Demos/Tests](Tests) - Performance tests of containers, algorithms and serialization with comparison to their counterparts in STL. This project's source code contains many examples of Intra usage.
- [Demos/Bin2C](Bin2C) - Utility to convert any file into byte array in C code.

### Supported compilers:
- MSVC 2013+;
- g++ 4.8+;
- Clang 3.3+.

### Supported platforms:
- Windows;
- Linux;
- FreeBSD;
- Emscripten;
- Expected soon: Android.


For now the library is unstable and lacks tests and documentation. At this stage it is not recommended to use in production.

### Setup and compilation on Debian based linux:
```bash
sudo apt-get install libopenal-dev git cmake
git clone https://github.com/gammaker/Intra
cd Intra
cmake -G"Unix Makefiles"
make -j4
```


## Описание на русском языке

Данный репозиторий содержит библиотеку Intra и её модули:
- [Core](Intra Core)
- [Audio](Intra Audio)
- [Image](Intra Image)

Библиотека содержит в себе следующий функционал:
- Контейнеры: Array, BList, HashMap, LinearMap, String и другие. Отличительная черта Array от существующих реализаций динамических массивов - быстрое O(1) добавление элементов в начало массива. При этом массив не теряет никаких преимуществ перед другими контейнерами, имея во всех остальных случаях производительность, схожую с std::vector.
- Диапазоны и алгоритмы для работы с ними. Диапазоны реализованы в стиле стандартной библиотеки Phobos языка D.
 Концепция диапазонов заменяет собой итераторы STL и представляют собой более удобную, функциональную и безопасную абстракцию, чем итераторы STL.
 В отличие от итераторов STL, диапазоны поддерживают декорирование и композицию, образуя сложные диапазоны. Это позволяет легко писать код в функциональном стиле и реализовывать ленивые вычисления.
 Примеры применения диапазонов расположены [здесь](Demos/Tests/src/Ranges).
- Автоматическая рекурсивная сериализация структур: бинарная и текстовая. Примеры [здесь](Demos/Tests/src/PerfTestSerialization.cpp).
- Математика: FixedPoint, векторы, матрицы, кватернионы, геометрические примитивы
- Загрузка множества форматов изображений.
- Звук и синтез различных музыкальных инструментов.
- Другое: таймер, потоки ввода-вывода, основные классы для многопоточности.

Кроме самой библиотеки в репозитории также находятся 3 демо-проекта:
- [Demos/MusicSynthesizer](MusicSynthesizer) - синтезатор MIDI. Этот проект состоит только из одного файла с функцией main, а основной код синтезатора находится в IntraLib/Sound.
- [Demos/Tests](Tests) - Тесты производительности контейнеров, алгоритмов и сериализации и сравнение с аналогами из STL. В исходных кодах этого проекта можно увидеть множество примеров использования библиотеки IntraLib.
- [Demos/Bin2C](Bin2C) - Утилита для преобразования файла в массив байт на C.
 

### Поддерживаемые компиляторы:
- MSVC 2013+;
- g++ 4.8+;
- Clang 3.3+.
 

### Поддерживаемые платформы:
- Windows;
- Linux;
- FreeBSD;
- Emscripten;
- Ожидается в ближайшее время Android.


### Установка и компиляция на Debian-based ОС:
```bash
sudo apt-get install libopenal-dev git cmake
git clone https://github.com/gammaker/Intra
cd Intra
cmake -G"Unix Makefiles"
make -j4
```

На данный момент библиотека находится в нестабильном состоянии и недостаточно хорошо протестирована и документирована. На данном этапе она не рекомендуется для использования в production.
