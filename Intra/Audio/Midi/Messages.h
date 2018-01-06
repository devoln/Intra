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
	byte Program;

	forceinline MusicNote::Type Note() const {return MusicNote::Type(NoteOctaveOrDrumId % 12);}
	forceinline byte Octave() const {return byte(NoteOctaveOrDrumId / 12);}
	forceinline ushort Id() const {return ushort((Channel << 8) | NoteOctaveOrDrumId);}
	forceinline float Frequency() const {return MusicNote::BasicFrequencies[Note()]*0.5f*float(1 << Octave());}

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

struct ChannelPanChange
{
	double Time;
	byte Channel;
	byte Pan;
};

struct ChannelVolumeChange
{
	double Time;
	byte Channel;
	byte Volume;
};

struct ChannelReverbChange
{
	double Time;
	byte Channel;
	byte ReverbCoeff;
};

struct ChannelProgramChange
{
	double Time;
	byte Channel;
	byte Instrument;
};

class IDevice
{
public:
	virtual ~IDevice() {}
	virtual void OnNoteOn(const NoteOn& noteOn) {(void)noteOn;}
	virtual void OnNoteOff(const NoteOff& noteOff) {(void)noteOff;}
	virtual void OnPitchBend(const PitchBend& pitchBend) {(void)pitchBend;}
	virtual void OnAllNotesOff(byte channel) {(void)channel;}
	virtual void OnChannelPanChange(const ChannelPanChange& panChange) {(void)panChange;}
	virtual void OnChannelVolumeChange(const ChannelVolumeChange& volumeChange) {(void)volumeChange;}
	virtual void OnChannelReverbChange(const ChannelReverbChange& reverbChange) {(void)reverbChange;}
	virtual void OnChannelProgramChange(const ChannelProgramChange& programChange) {(void)programChange;}
};

}}}

INTRA_WARNING_POP
