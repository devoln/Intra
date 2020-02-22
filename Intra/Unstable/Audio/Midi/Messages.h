#pragma once

#include "Audio/MusicNote.h"
#include "Audio/Midi/MidiDeviceState.h"

INTRA_BEGIN
struct MidiNoteOn
{
	MidiTime Time;
	byte Channel;
	byte NoteOctaveOrDrumId;
	byte Velocity;
	byte Program;

	forceinline MusicNote::Type Note() const {return MusicNote::Type(NoteOctaveOrDrumId % 12);}
	forceinline byte Octave() const {return byte(NoteOctaveOrDrumId / 12);}
	forceinline ushort Id() const {return ushort((Channel << 8) | NoteOctaveOrDrumId);}
	forceinline float Frequency() const {return MusicNote::BasicFrequencies[Note()]*0.5f*float(1 << Octave());}

};

struct MidiNoteOff
{
	MidiTime Time;
	byte Channel;
	byte NoteOctaveOrDrumId;
	byte Velocity;

	forceinline ushort Id() const {return ushort((Channel << 8) | NoteOctaveOrDrumId);}
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
