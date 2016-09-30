#pragma once

#include "Containers/ForwardDeclarations.h"

namespace Intra {struct Music; class Sound;}

Intra::String GetMidiPath(Intra::StringView fileName);
void PrintMusicInfo(const Intra::Music& music);
bool PrintMidiFileInfo(Intra::StringView filePath);
Intra::Sound SynthSoundFromMidi(Intra::StringView filePath, bool printMessages);
