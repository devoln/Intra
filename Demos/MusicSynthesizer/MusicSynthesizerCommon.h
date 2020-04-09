#pragma once

#include "Intra/Range/StringView.h"
#include "Extra/System/Error.h"
#include "Extra/Utils/Unique.h"

#include "Extra/Container/ForwardDecls.h"

#include "Intra/Range/Polymorphic/ForwardRange.h"

#include "Extra/Unstable/Audio/Midi/MidiFileParser.h"

INTRA_BEGIN
namespace Audio {

struct Music;
class Sound;
class StreamedSound;
class IAudioSource;

namespace Midi {
struct MidiFileInfo;
}

}}

Intra::String GetMidiPath(Intra::StringView fileName);
Intra::Audio::Midi::MidiFileInfo PrintMidiInfo(Intra::InputStream midiFileStream, Intra::ErrorStatus& status);
bool PrintMidiFileInfo(Intra::StringView filePath);
Intra::Unique<Intra::Audio::IAudioSource> CreateMidiAudioSource(Intra::InputStream stream,
	double duration, float startingVolume, Intra::ErrorStatus& status, Intra::unsigned sampleRate = 0);
Intra::Audio::Sound CreateSoundFromMidi(Intra::ForwardStream midiFiletream, double duration, float startingVolume, bool printMessages);
Intra::Audio::StreamedSound CreateStreamedSoundFromMidi(Intra::ForwardStream midiFiletream, float startingVolume, bool printMessages);

