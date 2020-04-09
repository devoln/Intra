
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
- MSVC 2017+;
- g++ 5.3+;
- Clang 3.5+.

### Supported platforms:
- Windows;
- Linux;
- FreeBSD;
- Web (Emscripten).


For now the library is unstable and lacks tests and documentation. At this stage it is not recommended to use in production.

### Setup and compilation on Debian based linux:
```bash
sudo apt-get install libopenal-dev git cmake
git clone https://github.com/devoln/Intra
cd Intra
cmake -G"Unix Makefiles"
make -j4
```
