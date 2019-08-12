#pragma once



#include "Utils/FixedArray.h"
#include "System/Error.h"

#include "Container/Associative/HashMap.h"

#include "Audio/AudioSource.h"
#include "Types.h"
#include "Audio/Midi/Messages.h"
#include "Audio/Midi/MidiFileParser.h"
#include "NoteSampler.h"
#include "InstrumentSet.h"
#include "PostEffects.hh"
#include "Sampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct MidiState
{
	short ChannelPitchBend[16]{};
	sbyte ChannelPans[16]{};
	byte ChannelVolumes[16];
	byte ChannelReverbs[16]{};
	byte ChannelPrograms[16]{};
	ushort PitchBendRangeInSemitones = 2;

	MidiState();
};

class MidiSynth: public Audio::SeparateFloatAudioSource, public Audio::Midi::IDevice
{
	MidiInstrumentSet mInstruments;
	Audio::Midi::TrackCombiner mMusic;
	Audio::Midi::MidiTime mTime = 0, mPrevTime = 0;
	size_t mSampleCount;
	float mMaxSample;

	MidiState mMidiState;

	SamplerContainer mNoteSamplers;

	struct NoteInfo
	{
		Audio::Midi::MidiTime Time;
		byte Channel;
		byte NoteOctaveOrDrumId;

		forceinline ushort Key() const {return ushort((Channel << 8) | NoteOctaveOrDrumId);}
	};

	typedef Container::HashMap<ushort, ushort> NoteSamplerMap;
	NoteSamplerMap mPlayingNoteMap;
	PostEffects::HallReverb mReverberator;
	Array<float> mReverbChannelBuffer;

	forceinline MusicalInstrument* getInstrument(byte channel) const
	{return mInstruments.Instruments[mMidiState.ChannelPrograms[channel]];}

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
	bool synthNote(Sampler& sampler, Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb);
	float pitchBendToFreqMultiplier(short relativePitchBend) const;
};


INTRA_WARNING_POP
