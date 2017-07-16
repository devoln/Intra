#pragma once

#include "Cpp/Warnings.h"

#include "Utils/FixedArray.h"
#include "Utils/ErrorStatus.h"

#include "Container/Associative/HashMap.h"

#include "Audio/AudioSource.h"
#include "Audio/Synth/Types.h"
#include "Audio/Midi/Messages.h"
#include "Audio/Midi/MidiFileParser.h"
#include "Audio/Synth/NoteSampler.h"

namespace Intra { namespace Audio { namespace Sources {

#ifndef INTRA_NO_MUSIC_LOADER

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS


class MidiSynth: public SeparateFloatAudioSource, public Midi::IDevice
{
	Synth::MidiInstrumentSet mInstruments;
	Midi::TrackCombiner mMusic;
	double mTime = 0, mPrevTime = 0;
	size_t mSampleCount;
	float mMaxSample;
	short mCurrentPitchBend[16]{};
	ushort mPitchBendRangeInSemitones = 2;

	struct NoteEntry
	{
		double Time;
		byte Channel;
		byte NoteOctaveOrDrumId;
		Synth::NoteSampler Sampler;
	};

	typedef Container::HashMap<ushort, NoteEntry> NoteMap;
	NoteMap mPlayingNotes;
	Array<NoteEntry> mOffPlayingNotes;

public:
	MidiSynth(Midi::TrackCombiner music, double duration, const Synth::MidiInstrumentSet& instruments, float maxVolume=1,
		OnCloseResourceCallback onClose=null, uint sampleRate=48000, bool stereo=true);
	~MidiSynth() {}

	MidiSynth(const MidiSynth&) = delete;
	MidiSynth& operator=(const MidiSynth&) = delete;

	static Unique<MidiSynth> FromFile(StringView path, double duration, const Synth::MidiInstrumentSet& instruments,
		float maxVolume=1, uint sampleRate=48000, bool stereo=true, ErrorStatus& status=Error::Skip());

	size_t SampleCount() const final {return mSampleCount;}
	size_t SamplePosition() const final {return size_t(mTime*mSampleRate);}

	size_t GetUninterleavedSamples(CSpan<Span<float>> outFloats) final;

	void OnNoteOn(const Midi::NoteOn& noteOn) final;
	void OnNoteOff(const Midi::NoteOff& noteOff) final;
	void OnPitchBend(const Midi::PitchBend& pitchBend) final;
	void OnAllNotesOff(byte channel) final;

private:
	bool synthNote(Synth::NoteSampler& sampler, Span<float> dstLeft, Span<float> dstRight, bool add);
	float pitchBendToFreqMultiplier(short relativePitchBend) const;
};


INTRA_WARNING_POP

#endif

}}}
