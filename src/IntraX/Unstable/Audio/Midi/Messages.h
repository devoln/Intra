#pragma once

#include "IntraX/Unstable/Audio/MusicNote.h"
#include "IntraX/Unstable/Audio/Midi/MidiDeviceState.h"

namespace Intra { INTRA_BEGIN
struct MidiNoteOn
{
	MidiTime Time;
	uint8 Channel;
	uint8 NoteOctaveOrDrumId;
	uint8 Velocity;
	uint8 Program;

	INTRA_FORCEINLINE MusicNote::Type Note() const {return MusicNote::Type(NoteOctaveOrDrumId % 12);}
	INTRA_FORCEINLINE uint8 Octave() const {return uint8(NoteOctaveOrDrumId / 12);}
	INTRA_FORCEINLINE uint16 Id() const {return uint16((Channel << 8) | NoteOctaveOrDrumId);}
	INTRA_FORCEINLINE float Frequency() const {return MusicNote::BasicFrequencies[Note()]*0.5f*float(1 << Octave());}

};

struct MidiNoteOff
{
	MidiTime Time;
	uint8 Channel;
	uint8 NoteOctaveOrDrumId;
	uint8 Velocity;

	INTRA_FORCEINLINE uint16 Id() const {return uint16((Channel << 8) | NoteOctaveOrDrumId);}
};

struct MidiPitchBend
{
	MidiTime Time;
	uint8 Channel;
	short Pitch;
};

struct MidiChannelPanChange
{
	MidiTime Time;
	uint8 Channel;
	uint8 Pan;
};

struct MidiChannelVolumeChange
{
	MidiTime Time;
	uint8 Channel;
	uint8 Volume;
};

struct MidiChannelReverbChange
{
	MidiTime Time;
	uint8 Channel;
	uint8 ReverbCoeff;
};

struct MidiChannelProgramChange
{
	MidiTime Time;
	uint8 Channel;
	uint8 Instrument;
};

class IMidiDevice
{
public:
	virtual ~IMidiDevice() {}
	virtual void OnNoteOn(const MidiNoteOn& noteOn) {(void)noteOn;}
	virtual void OnNoteOff(const MidiNoteOff& noteOff) {(void)noteOff;}
	virtual void OnPitchBend(const MidiPitchBend& pitchBend) {(void)pitchBend;}
	virtual void OnAllNotesOff(uint8 channel) {(void)channel;}
	virtual void OnChannelPanChange(const MidiChannelPanChange& panChange) {(void)panChange;}
	virtual void OnChannelVolumeChange(const MidiChannelVolumeChange& volumeChange) {(void)volumeChange;}
	virtual void OnChannelReverbChange(const MidiChannelReverbChange& reverbChange) {(void)reverbChange;}
	virtual void OnChannelProgramChange(const MidiChannelProgramChange& programChange) {(void)programChange;}
};

} INTRA_END
