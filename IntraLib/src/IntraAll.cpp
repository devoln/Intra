//#define INTRA_UNITY_BUILD

#ifdef INTRA_UNITY_BUILD

#include "IntraAll.h"

#include "Algorithms/Algorithms.cpp"
#include "Algorithms/Hash.cpp"
#include "Algorithms/AsciiString.cpp"

#include "CompilerSpecific/CompilerSpecific.cpp"

#include "Containers/AsciiSet.cpp"

#include "Core/Debug.cpp"
#include "Core/Errors.cpp"
#include "Core/Time.cpp"

#include "Data/Reflection.cpp"
#include "Data/BinarySerialization.cpp"
#include "Data/TextSerialization.cpp"
#include "Data/Variable.cpp"
#include "Data/ValueType.cpp"

#include "Graphics/OpenGL/GLEnum.cpp"
#include "Graphics/OpenGL/GLExtensions.cpp"
#include "Graphics/OpenGL/GLSL.cpp"
#include "Graphics/ObjectDescs.cpp"
#include "Graphics/States.cpp"
#include "Graphics/UniformType.cpp"

#include "GUI/FontLoading.cpp"
#include "GUI/GraphicsWindow.cpp"
#include "GUI/MessageBox.cpp"

#include "Imaging/Image.cpp"
#include "Imaging/ImagingTypes.cpp"
#include "Imaging/Bindings/DXGI_Formats.cpp"
#include "Imaging/Bindings/GLenumFormats.cpp"

#include "IO/DocumentWriter.cpp"
#include "IO/File.cpp"
#include "IO/LogSystem.cpp"
#include "IO/Stream.cpp"

#include "Math/MathEx.cpp"
#include "Math/Shapes.cpp"
#include "Math/Random.cpp"

#include "Memory/Memory.cpp"
#include "Memory/Allocator.cpp"
#include "Memory/SystemAllocators.cpp"
#include "Memory/VirtualMemory.cpp"

#include "Platform/HardwareInfo.cpp"
#include "Platform/Platform.cpp"

#include "Sound/Sound.cpp"
#include "Sound/Music.cpp"
#include "Sound/SoundBuilder.cpp"
#include "Sound/SoundProcessing.cpp"
#include "Sound/SoundSource.cpp"
#include "Sound/SynthesizedInstrument.cpp"
#include "Sound/InstrumentLibrary.cpp"

#include "Test/PerformanceTest.cpp"
#include "Test/UnitTest.cpp"

#include "Text/CharMap.cpp"
#include "Text/UtfConversion.cpp"

#include "Threading/Job.cpp"
#include "Threading/Thread.cpp"

#endif
