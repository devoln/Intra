//#define INTRA_UNITY_BUILD

#include "Intra.h"

#ifdef INTRA_UNITY_BUILD

#include "Algo/Hash/Murmur.cpp"
#include "Algo/Mutation/Copy.cpp"
#include "Algo/Mutation/Transform.cpp"
#include "Algo/String/Ascii.cpp"
#include "Algo/String/Path.cpp"
#include "Algo/Reduction.cpp"

#include "Containers/AsciiSet.cpp"

#include "Core/Debug.cpp"
#include "Core/Errors.cpp"

#include "Data/Reflection.cpp"
#include "Data/BinarySerialization.cpp"
#include "Data/TextSerialization.cpp"
#include "Data/Variable.cpp"
#include "Data/ValueType.cpp"
#include "Data/Object.cpp"

#include "Graphics/OpenGL/GLEnum.cpp"
#include "Graphics/OpenGL/GLExtensions.cpp"
#include "Graphics/OpenGL/GLSL.cpp"
#include "Graphics/ObjectDescs.cpp"
#include "Graphics/States.cpp"
#include "Graphics/UniformType.cpp"

#include "GUI/FontLoading.cpp"
#if(INTRA_LIBRARY_WINDOW_SYSTEM!=INTRA_LIBRARY_WINDOW_SYSTEM_Console)
#include "GUI/GraphicsWindow.cpp"
#endif
#include "GUI/MessageBox.cpp"

#if(INTRA_LIBRARY_WINDOW_SYSTEM!=INTRA_LIBRARY_WINDOW_SYSTEM_Console)
#include "GUI/GraphicsWindow_Android.cpp"
#include "GUI/GraphicsWindow_Qt.cpp"
#include "GUI/GraphicsWindow_X11.cpp"
#include "GUI/GraphicsWindow_WinAPI.cpp"
#endif


#include "Imaging/Image.cpp"
#include "Imaging/ImagingTypes.cpp"
#include "Imaging/Bindings/DXGI_Formats.cpp"
#include "Imaging/Bindings/GLenumFormats.cpp"
#include "Imaging/Loaders/LoaderDDS.cpp"
#include "Imaging/Loaders/LoaderKTX.cpp"

#include "IO/DocumentWriter.cpp"
#include "IO/File.cpp"
#include "IO/LogSystem.cpp"
#include "IO/Stream.cpp"
#include "IO/Networking.cpp"

#include "Math/MathEx.cpp"
#include "Math/Shapes.cpp"
#include "Math/Random.cpp"

#include "Memory/Memory.cpp"
#include "Memory/Allocator.cpp"
#include "Memory/SystemAllocators.cpp"
#include "Memory/VirtualMemory.cpp"

#include "Platform/HardwareInfo.cpp"
#include "Platform/Runtime.cpp"
#include "Platform/Environment.cpp"
#include "Platform/Time.cpp"

#include "Range/Unicode.cpp"

#include "Sound/DrumPhysicalModel.cpp"
#include "Sound/Sound.cpp"
#include "Sound/Music.cpp"
#include "Sound/Midi.cpp"
#include "Sound/SoundBuilder.cpp"
#include "Sound/SoundProcessing.cpp"
#include "Sound/SoundSource.cpp"
#include "Sound/SynthesizedInstrument.cpp"
#include "Sound/InstrumentLibrary.cpp"
#include "Sound/Sound_DirectSound.cpp"
#include "Sound/Sound_OpenAL.cpp"
#include "Sound/Sound_Dummy.cpp"
#include "Sound/Sound_Emscripten.cpp"

#include "Test/PerformanceTest.cpp"
#include "Test/UnitTest.cpp"

#include "Text/CharMap.cpp"

#include "Threading/Job.cpp"
#include "Threading/Thread.cpp"

#endif
