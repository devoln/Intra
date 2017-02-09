#ifdef INTRA_UNITY_BUILD

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

#endif
