#pragma once

#include "Cpp/Warnings.h"

#include "Utils/FixedArray.h"
#include "Utils/ErrorStatus.h"

#include "Container/Associative/HashMap.h"

#include "Audio/AudioSource.h"
#include "Types.h"
#include "Audio/Midi/Messages.h"
#include "Audio/Midi/MidiFileParser.h"
#include "NoteSampler.h"
#include "InstrumentSet.h"
#include "PostEffects.hh"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS


class MidiSynth: public Audio::SeparateFloatAudioSource, public Audio::Midi::IDevice
{
	MidiInstrumentSet mInstruments;
	Audio::Midi::TrackCombiner mMusic;
	double mTime = 0, mPrevTime = 0;
	size_t mSampleCount;
	float mMaxSample;
	short mChannelPitchBend[16]{};
	sbyte mChannelPans[16]{};
	byte mChannelVolumes[16];
	byte mChannelReverbs[16]{};
	byte mChannelPrograms[16]{};
	ushort mPitchBendRangeInSemitones = 2;

	struct NoteEntry
	{
		double Time;
		byte Channel;
		byte NoteOctaveOrDrumId;
		NoteSampler Sampler;
	};

	typedef Container::HashMap<ushort, NoteEntry> NoteMap;
	NoteMap mPlayingNotes;
	Array<NoteEntry> mOffPlayingNotes;
	PostEffects::HallReverb mReverberator;
	Array<float> mReverbChannelBuffer;

public:
	MidiSynth(Audio::Midi::TrackCombiner music, double duration, const MidiInstrumentSet& instruments, float maxVolume=1,
		OnCloseResourceCallback onClose=null, uint sampleRate=48000, bool stereo=true, bool reverb=true);
	~MidiSynth() {}

	MidiSynth(const MidiSynth&) = delete;
	MidiSynth& operator=(const MidiSynth&) = delete;

	static Unique<MidiSynth> FromFile(StringView path, double duration, const MidiInstrumentSet& instruments,
		float maxVolume=1, uint sampleRate=48000, bool stereo=true, ErrorStatus& status=Error::Skip());

	size_t SampleCount() const final {return mSampleCount;}
	size_t SamplePosition() const final {return size_t(mTime*mSampleRate);}

	size_t GetUninterleavedSamplesAdd(CSpan<Span<float>> outFloats);
	size_t GetUninterleavedSamples(CSpan<Span<float>> outFloats) final;

	void OnNoteOn(const Audio::Midi::NoteOn& noteOn) final;
	void OnNoteOff(const Audio::Midi::NoteOff& noteOff) final;
	void OnPitchBend(const Audio::Midi::PitchBend& pitchBend) final;
	void OnChannelPanChange(const Audio::Midi::ChannelPanChange& panChange) final;
	void OnChannelVolumeChange(const Audio::Midi::ChannelVolumeChange& volumeChange) final;
	void OnChannelReverbChange(const Audio::Midi::ChannelReverbChange& reverbChange) final;
	void OnAllNotesOff(byte channel) final;
	void OnChannelProgramChange(const Audio::Midi::ChannelProgramChange& programChange) final;

private:
	bool synthNote(NoteSampler& sampler, Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb);
	float pitchBendToFreqMultiplier(short relativePitchBend) const;
};


INTRA_WARNING_POP
