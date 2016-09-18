#pragma once

#include "SoundSource.h"

namespace Intra {

struct MusicNote
{
	enum NoteType: byte {C, CSharp, D, DSharp, E, F, FSharp, G, GSharp, A, ASharp, H};
	static const float BasicFrequencies[12]; //Таблица соответствия нот субконтроктавы частотам

	MusicNote(byte octave, NoteType note, ushort duration): Octave(octave), Note(note), Duration(duration) {}
	MusicNote(null_t=null): Octave(255), Note(NoteType(255)), Duration(0) {}

	static MusicNote Pause(ushort duration)
	{
		MusicNote result;
		result.Duration = duration;
		return result;
	}

	bool IsPause() const {return Octave==255 && Duration!=0;}

	operator NoteType() const {return Note;}
	bool operator==(const MusicNote& rhs) const {return Octave==rhs.Octave && Note==rhs.Note && Duration==rhs.Duration;}
	bool operator!=(const MusicNote& rhs) const {return !operator==(rhs);}
	bool operator==(null_t) const {return Duration==0 || (Octave!=255 && (Octave>=8 || Note>=12));}
	bool operator!=(null_t) const {return !operator==(null);}

	float Frequency() const {return BasicFrequencies[Note]*float(1 << Octave);}
	float AbsDuration(float tempo) const {return Duration*tempo/2048;}

	byte Octave; //0 - субконтроктава, дальше по порядку. Если Octave==255, то это не нота, а пауза
	NoteType Note;
	ushort Duration; //Относительная длительность ноты в 1/2048 долях
};

struct SoundBuffer;

class IMusicalInstrument
{
public:
	virtual ~IMusicalInstrument() {}
	virtual void GetNoteSamples(ArrayRange<float> dst, MusicNote note, float tempo, float volume=1, uint sampleRate=44100, bool add=false) const = 0;
	virtual uint GetNoteSampleCount(MusicNote note, float tempo, uint sampleRate=44100) const = 0;
};


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
	MusicTrack(ArrayRange<const NoteEntry> notes, IMusicalInstrument* instrument=null, float tempo=1, short toneOffset=0):
		Notes(notes), Instrument(instrument), Tempo(tempo), ToneOffset(toneOffset) {}

	double Duration() const;
	MusicNote operator[](uint index) const;

	SoundBuffer GetSamples(uint sampleRate=44100) const;

	double GetNoteTimeOffset(uint index) const
	{
		if(Notes[index].TimeOffset==0xFFFF) {if(index==0) return 0; return Notes[index-1].Note.AbsDuration(Tempo);}
		return Notes[index].TimeOffset*Tempo/2048;
	}

	Array<NoteEntry> Notes;
	IMusicalInstrument* Instrument=null;
	float Tempo=1; //Совпадает с длительностью ноты с Duration=2048 в секундах
	float Volume=1;
	short ToneOffset=0;
};

struct Music
{
	Music(null_t=null): Tracks() {}
	Music(ArrayRange<const MusicTrack> tracks): Tracks(tracks) {}

	double Duration() const;
	SoundBuffer GetSamples(uint sampleRate=44100) const;

	bool operator==(null_t) const {return Tracks==null;}

	Array<MusicTrack> Tracks;
};




#ifndef INTRA_NO_MUSIC_LOADER

class MusicSoundSampleSource: public ASoundSampleSource
{
	Music data;
	SoundBuffer buffer;
	size_t current_sample_pos, sample_count;

	struct Position { uint samplePos; uint noteId; };

	Array<Position> currentPositions;
	float maxVolume;
	size_t processedSamplesToFlush;

public:
	MusicSoundSampleSource(const Music& mydata, uint sampleRate=11025);
	~MusicSoundSampleSource() {}

	size_t SampleCount() const override { return sample_count; }
	size_t CurrentSamplePosition() const override { return current_sample_pos; }

	//! Загрузить следующие maxFloatsToGet/ChannelCount семплов в текущий буфер
	//! Если в семплов осталось меньше, то загрузится столько семплов, сколько осталось.
	//! \returns Количество прочитанных float'ов, то есть прочитанное количество семплов, умноженное на ChannelCount
	size_t LoadNextNonNormalizedSamples(uint maxFloatsToGet);


	size_t LoadNextNormalizedSamples(uint maxFloatsToGet);

	//! Удалить уже обработанные семплы из буфера
	void FlushProcessedSamples();


	size_t GetInterleavedSamples(ArrayRange<short> outShorts) override;
	size_t GetInterleavedSamples(ArrayRange<float> outFloats) override;
	size_t GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats) override;

	Array<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		ValueType* outType, bool* outInterleaved, size_t* outSamplesRead) override;


	MusicSoundSampleSource& operator=(const MusicSoundSampleSource&) = delete;
};

#endif

}


