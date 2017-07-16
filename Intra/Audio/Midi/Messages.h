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

	forceinline MusicNote::Type Note() const {return MusicNote::Type(NoteOctaveOrDrumId % 12);}
	forceinline byte Octave() const {return byte(NoteOctaveOrDrumId / 12);}
	forceinline ushort Id() const {return ushort((Channel << 8) | NoteOctaveOrDrumId);}
	forceinline float Frequency() const {return MusicNote::BasicFrequencies[Note()]*0.5f*(1 << Octave());}
	forceinline float TotalVolume() const {return (Volume*Velocity)/(127.0f*127.0f);}

};

struct NoteOff
{
	double Time;
	byte Channel;
	byte NoteOctaveOrDrumId;
	byte Velocity;

	forceinline ushort Id() const {return ushort((Channel << 8) | NoteOctaveOrDrumId);}
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
	virtual ~IDevice() {}
	virtual void OnNoteOn(const NoteOn& noteOn) = 0;
	virtual void OnNoteOff(const NoteOff& noteOff) = 0;
	virtual void OnPitchBend(const PitchBend& pitchBend) = 0;
	virtual void OnAllNotesOff(byte channel) = 0;
};

}}}

INTRA_WARNING_POP
