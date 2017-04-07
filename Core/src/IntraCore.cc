//#define INTRA_UNITY_BUILD

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
#include "Data/Serialization/TextSerializerParams.cpp"
#include "Data/Serialization/LanguageParams.cpp"
#include "Data/Variable.cpp"
#include "Data/ValueType.cpp"
#include "Data/Object.cpp"

#include "IO/ConsoleInput.cpp"
#include "IO/ConsoleOutput.cpp"
#include "IO/Std.cpp"
#include "IO/OsFile.cpp"
#include "IO/FileSystem.cpp"
#include "IO/FileMapping.cpp"
#include "IO/CompositeFormattedWriter.cpp"
#include "IO/ReferenceFormattedWriter.cpp"
#include "IO/HtmlWriter.cpp"
#include "IO/LogSystem.cpp"
#include "IO/Networking.cpp"
#include "IO/Socket.cpp"


#include "Math/Math.cpp"
#include "Math/Shapes.cpp"
#include "Math/Random.cpp"

#include "Memory/Memory.cpp"
#include "Memory/VirtualMemory.cpp"
#include "Memory/Allocator/Global.cpp"
#include "Memory/Allocator/System.cpp"
#include "Memory/Allocator/Basic/Pool.cpp"
#include "Memory/Allocator/Basic/Stack.cpp"

#include "Platform/HardwareInfo.cpp"
#include "Platform/Runtime.cpp"
#include "Platform/Environment.cpp"
#include "Platform/Time.cpp"
#include "Platform/MessageBox.cpp"

#include "Range/Special/Unicode.cpp"


#include "Test/PerfSummary.cpp"
#include "Test/TestGroup.cpp"

#include "Thread/Job.cpp"
#include "Thread/Thread.cpp"

#include "Utils/AsciiSet.cpp"

#endif
