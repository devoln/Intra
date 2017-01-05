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
#include "Imaging/ImageFormat.cpp"
#include "Imaging/ImageInfo.cpp"
#include "Imaging/FormatConversion.cpp"
#include "Imaging/Bindings/DXGI_Formats.cpp"
#include "Imaging/Bindings/GLenumFormats.cpp"
#include "Imaging/Loaders/Loader.cpp"
#include "Imaging/Loaders/LoaderPlatform.cpp"
#include "Imaging/Loaders/LoaderDDS.cpp"
#include "Imaging/Loaders/LoaderKTX.cpp"
#include "Imaging/Loaders/LoaderTGA.cpp"
#include "Imaging/Loaders/LoaderBMP.cpp"
#include "Imaging/Loaders/LoaderGIF.cpp"
#include "Imaging/Loaders/LoaderTIFF.cpp"
#include "Imaging/Loaders/LoaderJPEG.cpp"
#include "Imaging/Loaders/LoaderPNG.cpp"

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


#include "Audio/Sound.cpp"
#include "Audio/Music.cpp"
#include "Audio/Midi.cpp"
#include "Audio/AudioBuffer.cpp"
#include "Audio/AudioProcessing.cpp"
#include "Audio/AudioSource.cpp"

#include "Audio/Synth/DrumInstrument.cpp"
#include "Audio/Synth/SynthesizedInstrument.cpp"
#include "Audio/Synth/InstrumentLibrary.cpp"
#include "Audio/Synth/PostEffects.cpp"
#include "Audio/Synth/AttackDecayAttenuation.cpp"
#include "Audio/Synth/ExponentialAttenuation.cpp"
#include "Audio/Synth/TableAttenuation.cpp"
#include "Audio/Synth/HighLowPass.cpp"
#include "Audio/Synth/PeriodicSynth.cpp"
#include "Audio/Synth/SawtoothSynth.cpp"
#include "Audio/Synth/SineExpSynth.cpp"
#include "Audio/Synth/SineSynth.cpp"
#include "Audio/Synth/Generators/DrumPhysicalModel.cpp"

#include "Audio/Sources/MusicSynthSource.cpp"
#include "Audio/Sources/VorbisSource.cpp"
#include "Audio/Sources/WaveSource.cpp"

#include "Audio/Sound_DirectSound.cpp"
#include "Audio/Sound_OpenAL.cpp"
#include "Audio/Sound_Dummy.cpp"
#include "Audio/Sound_Emscripten.cpp"


#include "Test/PerformanceTest.cpp"
#include "Test/Unittest.cpp"

#include "Text/CharMap.cpp"

#include "Threading/Job.cpp"
#include "Threading/Thread.cpp"

#endif
