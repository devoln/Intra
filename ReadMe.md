
# Intra

[![Build Status (master)](https://travis-ci.org/gammaker/Intra.svg?branch=master)](https://travis-ci.org/gammaker/Intra)

[See Russian description below](#RuDesc)
## English description

This repository contains the Intra library and my [projects](Demos) based on it.

This library is my attempt to create general purpose library for C++ which is fast and convenient to use.
It may become a good alternative to STL and Boost, when it will be more stable, tested and documented. In this library I'm implementing my view of the perfect C++ standard library.
It is based on low level libraries to provide more features and more performance than it could provide, if it was based on C++ standard library or Boost.
It doesn't require C++ standard library or other libraries, but may optionally use them in some parts depending on configuration.

This library contains the following functionality:
- Containers: Array, BList, HashMap, LinearMap, String and others. Array is different from existing implementations of dynamic arrays by fast O(1) appending to the beginning of array.
- Automatic recursive structure serialization: binary and text. See examples [here](Demos/Tests/src/PerfTestSerialization.cpp).
- Math functions and classes: FixedPoint, vectors, matrices, quaternions, geometric primitives.
- Multiple image format loading.
- Sound system and music instrument synthesis.
- etc: timer, IO streams, basic classes for multithreading.

In addition to the library this repository also contains 3 demo projects:
- [MusicSynthesizer](Demos/MusicSynthesizer) - MIDI synthesizer. This project is an CLI interface to the synthesizer. Most of the synthesizer code is located in the library at Intra/Audio/Synth.
- [Tests](Demos/Tests) - Performance tests of containers, algorithms and serialization with comparison to their counterparts in STL.
- [UnitTests](Demos/UnitTests) - This project's source code contains many examples of Intra usage.
- [Bin2C](Demos/Bin2C) - Utility to convert any file into byte array in C code.

### Supported compilers:
- MSVC 2015+;
- g++ 4.8+;
- Clang 3.3+.

### Supported platforms:
- Windows;
- Linux;
- FreeBSD;
- Web (Emscripten).


For now the library is unstable and lacks tests and documentation. At this stage it is not recommended to use in production.

### Setup and compilation on Debian based linux:
```bash
sudo apt-get install libopenal-dev git cmake
git clone https://github.com/gammaker/Intra
cd Intra
cmake -G"Unix Makefiles"
make -j4
```


## <a name="RuDesc"></a> Описание на русском языке

Данный репозиторий содержит библиотеку [Intra](Intra) и мои [проекты](Demos), которые её используют.

Этот проект является моей попыткой создать библиотеку общего назначения для C++, которая будет одновременно быстрой, гибкой и удобной для использования.
Здесь я реализую своё представление о том, какой бы я хотел видеть идеальную стандартную библиотеку в C++. Она уже содержит достаточно много функционала для того, чтобы в большинстве случаев заменить STL, другие части из стандартной библиотеки C++ и некоторые части Boost.
Она построена на низкоуровневых библиотеках, большая часть из которых являются системными API ОС. За счёт этого Intra может предоставить больше возможностей, и более высокую производительность, чем она могла бы предоставить, используя стандартную библиотеку C++ или Boost.
Intra не зависит от сторонних библиотек, но опционально может быть собрана с их использованием для поддержки загрузки дополнительных форматов файлов или для поддержки большего количества платформ.

Библиотека содержит в себе следующий функционал:
- Контейнеры: Array, BList, HashMap, LinearMap, String и другие. Отличительная черта Array от существующих реализаций динамических массивов - быстрое O(1) добавление элементов в начало массива. При этом массив не теряет никаких преимуществ перед другими контейнерами, имея во всех остальных случаях производительность, схожую с std::vector.
- Диапазоны и алгоритмы для работы с ними. Диапазоны реализованы в стиле стандартной библиотеки Phobos языка D.
 Концепция диапазонов заменяет собой итераторы STL и представляют собой более удобную, функциональную и безопасную абстракцию, чем итераторы STL.
 В отличие от итераторов STL, диапазоны поддерживают декорирование и композицию, образуя сложные диапазоны. Это позволяет легко писать код в функциональном стиле и реализовывать ленивые вычисления.
 Кроме того диапазоны символов представляют собой потоки ввода-вывода, поэтому поток можно создать даже на основе массива символов на стеке и использовать его для преобразования в строку, и все алгоритмы для работы с диапазонами применимы и к потокам.
 Примеры применения диапазонов расположены [здесь](Demos/Tests/src/Ranges).
- Автоматическая рекурсивная сериализация структур: бинарная и текстовая. Примеры [здесь](Demos/Tests/src/PerfTestSerialization.cpp).
- Математика: FixedPoint, векторы, матрицы, кватернионы, геометрические примитивы.
- Загрузка множества форматов изображений.
- Звук и синтез различных музыкальных инструментов.
- Другое: таймер, потоки ввода-вывода, основные классы для многопоточности.

Кроме самой библиотеки в репозитории также находятся 4 демо-проекта:
- [MusicSynthesizer](Demos/MusicSynthesizer) - синтезатор MIDI. Этот проект представляет собой только консольный интерфейс к синтезатору, а основной код синтеза музыки находится в Intra/Audio/Synth.
- [Tests](Demos/Tests) - Тесты производительности контейнеров, алгоритмов и сериализации и сравнение с аналогами из STL.
- [UnitTests](Demos/UnitTests) - В исходных кодах этого проекта можно увидеть множество примеров использования библиотеки Intra.
- [Bin2C](Demos/Bin2C) - Утилита для преобразования файла в массив байт на C. В целях оптимизации её код достаточно низкоуровневый и демонстрирует мало преимуществ библиотеки.
 

### Поддерживаемые компиляторы:
- MSVC 2015+;
- g++ 4.8+;
- Clang 3.3+.
 

### Поддерживаемые платформы:
- Windows;
- Linux;
- FreeBSD;
- Web (Emscripten).


### Установка и компиляция на Debian-based ОС:
```bash
sudo apt-get install libopenal-dev git cmake
git clone https://github.com/gammaker/Intra
cd Intra
cmake -G"Unix Makefiles"
make -j4
```

На данный момент библиотека находится в нестабильном состоянии и недостаточно хорошо протестирована и документирована. На данном этапе она не рекомендуется для использования в production.
