﻿//#define INTRA_UNITY_BUILD

#include "IntraCore.hh"

#ifdef INTRA_UNITY_BUILD

#include "Algo/Hash/Murmur.cpp"
#include "Algo/Mutation/Copy.cpp"
#include "Algo/Mutation/Transform.cpp"
#include "Algo/String/Ascii.cpp"
#include "Algo/String/Path.cpp"
#include "Algo/Reduction.cpp"


#include "Platform/Debug.cpp"
#include "Platform/Errors.cpp"

#include "Data/Reflection.cpp"
#include "Data/BinarySerialization.cpp"
#include "Data/TextSerialization.cpp"
#include "Data/Variable.cpp"
#include "Data/ValueType.cpp"
#include "Data/Object.cpp"

#include "IO/DocumentWriter.cpp"
#include "IO/File.cpp"
#include "IO/LogSystem.cpp"
#include "IO/Stream.cpp"
#include "IO/Networking.cpp"

#include "Math/Math.cpp"
#include "Math/Shapes.cpp"
#include "Math/Random.cpp"

#include "Memory/Memory.cpp"
#include "Memory/VirtualMemory.cpp"
#include "Memory/Allocator.cpp"
#include "Memory/Allocator/System.cpp"
#include "Memory/Allocator/Basic/Pool.cpp"
#include "Memory/Allocator/Basic/Stack.cpp"

#include "Platform/HardwareInfo.cpp"
#include "Platform/Runtime.cpp"
#include "Platform/Environment.cpp"
#include "Platform/Time.cpp"
#include "Platform/MessageBox.cpp"

#include "Range/Special/Unicode.cpp"


#include "Test/PerformanceTest.cpp"
#include "Test/Unittest.cpp"

#include "Thread/Job.cpp"
#include "Thread/Thread.cpp"

#include "Utils/AsciiSet.cpp"

#endif