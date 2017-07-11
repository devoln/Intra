#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"

#include "Audio/MusicNote.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Midi {

struct NoteOn
{
	double Time;
	byte Channel;
	byte NoteOctaveOrDrumId;
	byte Velocity;
	byte Instrument;
	byte Volume;
	sbyte Pan;

	forceinline MusicNote::Type Note() const {MusicNote::Type(NoteOctaveOrDrumId % 12);}
	forceinline byte Octave() const {return byte(NoteOctaveOrDrumId / 12);}
};

struct NoteOff
{
	double Time;
	byte Channel;
	byte NoteOctaveOrDrumId;
	byte Velocity;
};

struct PitchBend
{
	double Time;
	byte Channel;
	short Pitch;
};

class IDevice
{
public:
	virtual void OnNoteOn(const NoteOn& noteOn) = 0;
	virtual void OnNoteOff(const NoteOff& noteOff) = 0;
	virtual void OnPitchBend(const PitchBend& pitchBend) = 0;
	virtual void OnAllNotesOff(byte channel) = 0;
};

}}}

INTRA_WARNING_POP
