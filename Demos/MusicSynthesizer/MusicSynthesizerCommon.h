#pragma once

#include "Range/ForwardDecls.h"
#include "Container/ForwardDecls.h"

namespace Intra { namespace Audio {

struct Music;
class Sound;

}}

Intra::String GetMidiPath(Intra::StringView fileName);
void PrintMusicInfo(const Intra::Audio::Music& music);
bool PrintMidiFileInfo(Intra::StringView filePath);
Intra::Audio::Sound SynthSoundFromMidi(Intra::StringView filePath, bool printMessages);
