#pragma once

#include "IntraX/Unstable/Audio/MusicNote.h"
#include "IntraX/Unstable/Audio/Midi/MidiDeviceState.h"

INTRA_BEGIN
struct MidiNoteOn
{
	MidiTime Time;
	byte Channel;
	byte NoteOctaveOrDrumId;
	byte Velocity;
	byte Program;

	INTRA_FORCEINLINE MusicNote::Type Note() const {return MusicNote::Type(NoteOctaveOrDrumId % 12);}
	INTRA_FORCEINLINE byte Octave() const {return byte(NoteOctaveOrDrumId / 12);}
	INTRA_FORCEINLINE uint16 Id() const {return uint16((Channel << 8) | NoteOctaveOrDrumId);}
	INTRA_FORCEINLINE float Frequency() const {return MusicNote::BasicFrequencies[Note()]*0.5f*float(1 << Octave());}

};

struct MidiNoteOff
{
	MidiTime Time;
	byte Channel;
	byte NoteOctaveOrDrumId;
	byte Velocity;

	INTRA_FORCEINLINE uint16 Id() const {return uint16((Channel << 8) | NoteOctaveOrDrumId);}
};

struct MidiPitchBend
{
	MidiTime Time;
	byte Channel;
	short Pitch;
};

struct MidiChannelPanChange
{
	MidiTime Time;
	byte Channel;
	byte Pan;
};

struct MidiChannelVolumeChange
{
	MidiTime Time;
	byte Channel;
	byte Volume;
};

struct MidiChannelReverbChange
{
	MidiTime Time;
	byte Channel;
	byte ReverbCoeff;
};

struct MidiChannelProgramChange
{
	MidiTime Time;
	byte Channel;
	byte Instrument;
};

class IMidiDevice
{
public:
	virtual ~IMidiDevice() {}
	virtual void OnNoteOn(const MidiNoteOn& noteOn) {(void)noteOn;}
	virtual void OnNoteOff(const MidiNoteOff& noteOff) {(void)noteOff;}
	virtual void OnPitchBend(const MidiPitchBend& pitchBend) {(void)pitchBend;}
	virtual void OnAllNotesOff(byte channel) {(void)channel;}
	virtual void OnChannelPanChange(const MidiChannelPanChange& panChange) {(void)panChange;}
	virtual void OnChannelVolumeChange(const MidiChannelVolumeChange& volumeChange) {(void)volumeChange;}
	virtual void OnChannelReverbChange(const MidiChannelReverbChange& reverbChange) {(void)reverbChange;}
	virtual void OnChannelProgramChange(const MidiChannelProgramChange& programChange) {(void)programChange;}
};

INTRA_END
