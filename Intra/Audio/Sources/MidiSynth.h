#pragma once

#include "Cpp/Warnings.h"

#include "Utils/FixedArray.h"

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
	Array<float> mBuffer;
	float mTime = 0;
	size_t mSampleCount;
	float mMinSample, mMaxSample;

	Container::HashMap<ushort, Synth::NoteSampler> mPlayingNotes;

public:
	MidiSynth(Midi::TrackCombiner music, OnCloseResourceCallback onClose=null, uint sampleRate=48000, bool stereo=true);
	~MidiSynth() {}

	MidiSynth(const MidiSynth&) = delete;
	MidiSynth& operator=(const MidiSynth&) = delete;

	size_t SampleCount() const final {return mSampleCount;}
	size_t SamplePosition() const final {return size_t(mTime*mSampleRate);}

	size_t GetUninterleavedSamples(CSpan<Span<float>> outFloats) final;

	void OnNoteOn(const Midi::NoteOn& noteOn) final;
	void OnNoteOff(const Midi::NoteOff& noteOff) final;
	void OnPitchBend(const Midi::PitchBend& pitchBend) final;
	void OnAllNotesOff(byte channel) final;
};

INTRA_WARNING_POP

#endif

}}}
