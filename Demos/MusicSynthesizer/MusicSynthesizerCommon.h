#pragma once

#include "Utils/StringView.h"
#include "Utils/ErrorStatus.h"
#include "Utils/Unique.h"

#include "Container/ForwardDecls.h"

#include "Range/Polymorphic/InputRange.h"

#include "Audio/Midi/MidiFileParser.h"

namespace Intra { namespace Audio {

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
Intra::Unique<Intra::Audio::IAudioSource> CreateMidiAudioSource(Intra::InputStream stream, double duration, float startingVolume, Intra::ErrorStatus& status);
Intra::Audio::Sound CreateSoundFromMidi(Intra::InputStream midiFiletream, double duration, float startingVolume, bool printMessages);
Intra::Audio::StreamedSound CreateStreamedSoundFromMidi(Intra::InputStream midiFiletream, float startingVolume, bool printMessages);
