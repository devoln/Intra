
# IntraLib

## English description

This library contains the following functionality:
- Containers: Array, BList, HashMap, LinearMap, String and others. Array is different from existing implementations of dynamic arrays by fast O(1) appending to the beginning of array.
- Ranges and algorithms working with them. Range are implemented in the style of D language standard library. Range concept replaces iterator concept. Unlike iterators ranges can be combined forming complex ranges. It let us to write in functional style. See examples [here](PerfTesting/src/PerfTestRanges.cpp).
- Serialization. See examples [here](PerfTesting/src/PerfTestSerialization.cpp).
- Math functions and classes: FixedPoint, vectors, matrices, quaternions, geometric primitives.
- Window creation and unified OpenGL core\extension loading.
- Multiple image format loading.
- Sound system and music intrument synthesis.
- etc: timer, IO streams, multithreading.

In addition to the [library](IntraLib) this repository also contains:
- [MusicSynthesizer](MusicSynthesizer) - MIDI synthesizer. This project consist only of one file with main function. Most of the synthesizer code is located at IntraLib/Sound.
- [PerfTesting](PerfTesting) - Performance tests of containers, algorithms and serialization with comparison to their counterparts in STL. This project's source code contains many examples of IntraLib usage.
- [Bin2C](Bin2C) - Utility to convert any file into byte array in C code.
 
### Supported compilers:
- MSVC 2013+;
- g++ 4.8+;
- Clang 3.3+.

### Supported platforms:
- Windows;
- Linux;
- FreeBSD;
- Emscripten;
- Expected: Android.


For now the library is unstable and lacks tests and documentation. It is not recommended to use in production.

### Setup and compilation on Debian based linux:
```bash
sudo apt-get install libopenal-dev git cmake
git clone https://github.com/gammaker/Intra
cd Intra
cmake -G"Unix Makefiles"
make -j4
```


## Описание на русском языке

Библиотека содержит в себе следующий функционал:
- Контейнеры: Array, BList, HashMap, LinearMap, String и другие. Отличительная черта Array от существующих реализаций динамических массивов - быстрое O(1) добавление элементов в начало массива. При этом массив не теряет никаких преимуществ перед другими контейнерами, имея во всех остальных случаях производительность, схожую с std::vector.
- Диапазоны и алгоритмы для работы с ними. Диапазоны реализованы в стиле стандартной библиотеки Phobos языка D. Концепция диапазонов заменяет собой итераторы. В отличие от итераторов, диапазоны могут комбинироваться, образуя сложные диапазоны. Это позволяет писать в функциональном стиле. Примеры [здесь](PerfTesting/src/PerfTestRanges.cpp).
- Сериализация. Примеры [здесь](PerfTesting/src/PerfTestSerialization.cpp).
- Математика: FixedPoint, векторы, матрицы, кватернионы, геометрические примитивы
- Создание окна и унифицированная загрузка расширений\ядра OpenGL.
- Загрузка множества форматов изображений.
- Звук и синтез различных музыкальных инструментов.
- Другое: таймер, потоки ввода-вывода, многопоточность.

Кроме [самой библиотеки](IntraLib) в репозитории также находятся:
- [MusicSynthesizer](MusicSynthesizer) - синтезатор MIDI. Этот проект состоит только из одного файла с функцией main, а основной код синтезатора находится в IntraLib/Sound.
- [PerfTesting](PerfTesting) - Тесты производительности контейнеров, алгоритмов и сериализации и сравнение с аналогами из STL. В исходных кодах этого проекта можно увидеть множество примеров использования библиотеки IntraLib.
- [Bin2C](Bin2C) - Утилита для преобразования файла в массив байт на C.
 

### Поддерживаемые компиляторы:
- MSVC 2013+;
- g++ 4.8+;
- Clang 3.3+.
 

### Поддерживаемые платформы:
- Windows;
- Linux;
- FreeBSD;
- Emscripten;
- Ожидается Android.


### Установка и компиляция на Debian-based ОС:
```bash
sudo apt-get install libopenal-dev git cmake
git clone https://github.com/gammaker/Intra
cd Intra
cmake -G"Unix Makefiles"
make -j4
```

На данный момент библиотека находится в нестабильном состоянии и недостаточно хорошо протестирована и документирована. На данном этапе она не рекомендуется для использования в production.
