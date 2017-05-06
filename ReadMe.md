
# Intra

[![Build Status (master)](https://travis-ci.org/gammaker/Intra.svg?branch=master)](https://travis-ci.org/gammaker/Intra)

## English description

This library is my attempt to create general purpose library for C++ which is fast and convenient to use.
In the distant future it is planned to replace STL and Boost with it.
It is based on low level libraries to provide more features and more performance than it could provide, if it was based on C++ standard library or Boost.
It doesn't require C++ standard library or other libraries, but may optionally use them in some parts depending on configuration.

This library is very modular. To learn more about its modules see [Modules](Modules).
This repository contains all Intra [modules](Modules) and [demos](Demos).

This library contains the following functionality:
- Containers: Array, BList, HashMap, LinearMap, String and others. Array is different from existing implementations of dynamic arrays by fast O(1) appending to the beginning of array.
- Automatic recursive structure serialization: binary and text. See examples [here](Demos/Tests/src/PerfTestSerialization.cpp).
- Math functions and classes: FixedPoint, vectors, matrices, quaternions, geometric primitives.
- Multiple image format loading.
- Sound system and music intrument synthesis.
- etc: timer, IO streams, basic classes for multithreading.

In addition to the library this repository also contains 3 demo projects:
- [MusicSynthesizer](Demos/MusicSynthesizer) - MIDI synthesizer. This project consist only of one file with main function. Most of the synthesizer code is located at IntraLib/Sound.
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
- [Intra Core](Core)
- [Intra Audio](Audio)
- [Intra Image](Image)

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
- [MusicSynthesizer](Demos/MusicSynthesizer) - синтезатор MIDI. Этот проект состоит только из одного файла с функцией main, а основной код синтезатора находится в IntraLib/Sound.
- [Tests](Demos/Tests) - Тесты производительности контейнеров, алгоритмов и сериализации и сравнение с аналогами из STL.
- [UnitTests](Demos/UnitTests) - В исходных кодах этого проекта можно увидеть множество примеров использования библиотеки Intra.
- [Bin2C](Demos/Bin2C) - Утилита для преобразования файла в массив байт на C.
 

### Поддерживаемые компиляторы:
- MSVC 2015+;
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
