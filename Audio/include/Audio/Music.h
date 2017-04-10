#pragma once

#include "Platform/CppWarnings.h"
#include "AudioSource.h"
#include "Synth/Types.h"
#include "MusicNote.h"


namespace Intra { namespace Audio {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct AudioBuffer;

struct MusicTrack
{
	struct NoteEntry
	{
		NoteEntry(MusicNote note, ushort offset=0xFFFF, float volume=1):
			Note(note), TimeOffset(offset), Volume(volume) {}
		NoteEntry(byte octave, MusicNote::NoteType note, ushort duration, ushort offset=0xFFFF, float volume=1):
			Note(octave, note, duration), TimeOffset(offset), Volume(volume) {}

		MusicNote Note;
		ushort TimeOffset;
		float Volume;
	};


	MusicTrack(null_t=null): MusicTrack(null, null, 1, 0) {}
	MusicTrack(CSpan<NoteEntry> notes,
		Synth::IMusicalInstrument* instrument=null, float tempo=1, short toneOffset=0):
		Notes(notes), Instrument(instrument), Tempo(tempo), ToneOffset(toneOffset) {}

	double Duration() const;
	MusicNote operator[](size_t index) const;

	AudioBuffer GetSamples(uint sampleRate=44100) const;

	double GetNoteTimeOffset(uint index) const
	{
		if(Notes[index].TimeOffset==0xFFFF)
		{
			if(index==0) return 0;
			return Notes[index-1].Note.AbsDuration(Tempo);
		}
		return Notes[index].TimeOffset*Tempo/2048;
	}

	Array<NoteEntry> Notes;
	Synth::IMusicalInstrument* Instrument=null;
	float Tempo=1; //Совпадает с длительностью ноты с Duration=2048 в секундах
	float Volume=1;
	short ToneOffset=0;
};

struct Music
{
	Music(null_t=null): Tracks() {}
	Music(CSpan<MusicTrack> tracks): Tracks(tracks) {}

	double Duration() const;
	AudioBuffer GetSamples(uint sampleRate=44100) const;

	bool operator==(null_t) const {return Tracks==null;}

	Array<MusicTrack> Tracks;
};

INTRA_WARNING_POP

}}
