#include "Audio/Sources/MidiSynth.h"

#include "Cpp/Warnings.h"

#include "Math/Math.h"

#include "Range/Mutation/Fill.h"
#include "Range/Reduction.h"
#include "Range/Mutation/Transform.h"

#include "IO/FileSystem.h"
#include "IO/FileReader.h"

#include "Audio/Synth/MusicalInstrument.h"
#include "Audio/Midi/MidiFileParser.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Sources {

#ifndef INTRA_NO_MUSIC_LOADER

MidiSynth::MidiSynth(Midi::TrackCombiner music, double duration, const Synth::MidiInstrumentSet& instruments, float maxVolume,
	OnCloseResourceCallback onClose, uint sampleRate, bool stereo, bool reverb):
	SeparateFloatAudioSource(Cpp::Move(onClose), sampleRate, ushort(stereo? 2: 1)),
	mInstruments(instruments),
	mMusic(Cpp::Move(music)),
	mTime(0),
	mSampleCount(duration == Cpp::Infinity? ~size_t(): size_t((duration+2)*sampleRate)),
	mMaxSample(maxVolume),
	mReverberator(size_t(reverb? 16384: 0), size_t(reverb? 32: 0), 1)
{
	Range::Fill(mChannelVolumes, byte(127));
}

bool MidiSynth::synthNote(Synth::NoteSampler& sampler, Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb, bool add)
{
	Span<float> dstLeftStart = dstLeft;
	size_t samplesProcessed;
	if(mChannelCount >= 2)
	{
		samplesProcessed = sampler(dstLeft, dstRight, dstReverb, add);
		dstRight.PopFirstExactly(samplesProcessed);
	}
	else
	{
		auto restPart = sampler(dstLeft, add);
		samplesProcessed = dstLeft.Length() - restPart.Length();
	}
	dstLeft.PopFirstExactly(samplesProcessed);
	if(!add)
	{
		FillZeros(dstLeft);
		FillZeros(dstRight);
	}

	return samplesProcessed < dstLeftStart.Length();
}

size_t MidiSynth::GetUninterleavedSamples(CSpan<Span<float>> outFloatChannels)
{
	if(outFloatChannels.Empty()) return 0;
	Span<float> dstLeft = outFloatChannels.First();
	Span<float> dstRight = outFloatChannels.Get(1).Take(dstLeft.Length());
	if(outFloatChannels.Length() >= 2) dstLeft = dstLeft.Take(dstRight.Length());

	size_t totalSamplesProcessed = 0;
	while(!dstLeft.Empty())
	{
		const double nextTime = mMusic.Empty()? Cpp::Infinity: mMusic.NextEventTime();
		mPrevTime = nextTime;
		if(nextTime - mTime <= 0.0001)
		{
			mMusic.ProcessEvent(*this);
			continue;
		}
		size_t samplesBeforeNextEvent = (nextTime == Cpp::Infinity)? ~size_t():
			size_t(Math::Max((nextTime - mTime)*mSampleRate + 0.5, 1.0));
		const size_t samplesLeft = SamplesLeft();
		if(samplesLeft == 0) break;
		samplesBeforeNextEvent = Funal::Min(samplesBeforeNextEvent, samplesLeft);
		bool add = false;
		const auto dstLeftBeforeEvent = dstLeft.Take(samplesBeforeNextEvent);
		const auto dstRightBeforeEvent = dstRight.Take(samplesBeforeNextEvent);

		mReverbChannelBuffer.SetCount(dstLeftBeforeEvent.Length());
		Span<float> dstReverbBeforeEvent = null;
		Array<NoteMap::iterator> notesToRemove;
		for(size_t i=0; i<mOffPlayingNotes.Length(); i++)
		{
			auto& sampler = mOffPlayingNotes[i].Sampler;
			if(sampler.ReverbCoeff > 0.001f) dstReverbBeforeEvent = mReverbChannelBuffer;
			const bool removeThisNote = synthNote(sampler, dstLeftBeforeEvent, dstRightBeforeEvent, dstReverbBeforeEvent, add);
			if(removeThisNote) mOffPlayingNotes.RemoveUnordered(i--);
			add = true;
		}
		int i = 0;
		for(auto it = mPlayingNotes.begin(); it != mPlayingNotes.end(); ++it)
		{
			auto& sampler = it->Value.Sampler;
			if(sampler.ReverbCoeff > 0.001f) dstReverbBeforeEvent = mReverbChannelBuffer;
			const bool removeThisNote = synthNote(sampler, dstLeftBeforeEvent, dstRightBeforeEvent, dstReverbBeforeEvent, add);
			if(removeThisNote) notesToRemove.AddLast(it);
			add = true;
			i++;
		}
		for(auto it: notesToRemove) mPlayingNotes.Remove(it);
		if(!add)
		{
			FillZeros(dstLeftBeforeEvent);
			FillZeros(dstRightBeforeEvent);
		}
		else
		{
			//FillZeros(dstLeftBeforeEvent);
			//FillZeros(dstRightBeforeEvent);
			mReverberator(dstLeftBeforeEvent, dstRightBeforeEvent, dstReverbBeforeEvent);
			auto minimax1 = MiniMax(dstLeftBeforeEvent.AsConstRange());
			auto minimax2 = MiniMax(dstRightBeforeEvent.AsConstRange());
			auto maxSample = Math::Max(
				Math::Max(Math::Abs(minimax1.first), Math::Abs(minimax1.second)),
				Math::Max(Math::Abs(minimax2.first), Math::Abs(minimax2.second)));
			if(mMaxSample < maxSample) mMaxSample = maxSample;
			Multiply(dstLeftBeforeEvent, 1.0f/mMaxSample);
			Multiply(dstRightBeforeEvent, 1.0f/mMaxSample);
			/*if(mMaxSample > 10 && maxSample == mMaxSample)
			{
				printf("dstLeftBeforeEvent.Length() = %i, dstRightBeforeEvent.Length() = %i\n", dstLeftBeforeEvent.Length(), dstRightBeforeEvent.Length());
				printf("minimax1 = [%f; %f], minimax2 = [%f; %f]\n", minimax1.first, minimax1.second, minimax2.first, minimax2.second);
				printf("mTime = %f\n", mTime);
			}*/
		}
		dstLeft.PopFirstExactly(dstLeftBeforeEvent.Length());
		dstRight.PopFirstExactly(dstRightBeforeEvent.Length());
		totalSamplesProcessed += dstLeftBeforeEvent.Length();
		mTime += double(dstLeftBeforeEvent.Length())/mSampleRate;
		if(mPlayingNotes.Empty() && mOffPlayingNotes.Empty() && nextTime == Cpp::Infinity)
		{
			if(dstLeft.Empty() && totalSamplesProcessed != 0) totalSamplesProcessed--;
			break;
		}
	}

	return totalSamplesProcessed;
}

void MidiSynth::OnNoteOn(const Midi::NoteOn& noteOn)
{
	const float totalStartVolume = float(noteOn.Velocity*mChannelVolumes[noteOn.Channel])/(127.0f*127.0f);
	const ushort id = noteOn.Id();
	NoteEntry* newNote = null;
	auto found = mPlayingNotes.Find(id);
	if(!found.Empty())
	{
		if(Math::Abs(found.First().Value.Time - noteOn.Time) < 0.0001) return;
		found.First().Value.Sampler.NoteRelease();
		mOffPlayingNotes.AddLast(Cpp::Move(found.First().Value));
	}
	if(noteOn.Channel != 9)
	{
		auto instr = mInstruments.Instruments[mChannelPrograms[noteOn.Channel]];
		if(instr == null) return;
		newNote = &mPlayingNotes[id];
		newNote->Sampler = (*instr)(noteOn.Frequency(), totalStartVolume, mSampleRate);
	}
	else
	{
		auto instr = mInstruments.DrumInstruments[noteOn.NoteOctaveOrDrumId];
		if(instr == null) return;
		newNote = &mPlayingNotes[id];
		newNote->Sampler = Synth::NoteSampler();
		newNote->Sampler.GenericSamplers.AddLast((*instr)(totalStartVolume, mSampleRate));
	}
	if(newNote)
	{
		newNote->Channel = noteOn.Channel;
		newNote->Time = noteOn.Time;
		newNote->NoteOctaveOrDrumId = noteOn.NoteOctaveOrDrumId;
		newNote->Sampler.SetPan((mChannelPans[noteOn.Channel] + 0.5f)/63.5f);
		newNote->Sampler.SetReverbCoeff(mChannelReverbs[noteOn.Channel] / 127.0f);
		const float freqMult = pitchBendToFreqMultiplier(mChannelPitchBend[noteOn.Channel]);
		newNote->Sampler.MultiplyPitch(freqMult);
	}
}

float MidiSynth::pitchBendToFreqMultiplier(short relativePitchBend) const
{
	return Math::Pow2(relativePitchBend / 8192.0f * mPitchBendRangeInSemitones / 12);
}

void MidiSynth::OnNoteOff(const Midi::NoteOff& noteOff)
{
	auto found = mPlayingNotes.Find(noteOff.Id());
	if(!found.Empty())
	{
		auto& note = mOffPlayingNotes.AddLast(Cpp::Move(found.First().Value));
		note.Sampler.NoteRelease();
		mPlayingNotes.Remove(NoteMap::const_iterator(found));
	}
}

void MidiSynth::OnPitchBend(const Midi::PitchBend& pitchBend)
{
	const short shift = short(pitchBend.Pitch - mChannelPitchBend[pitchBend.Channel]);
	mChannelPitchBend[pitchBend.Channel] = pitchBend.Pitch;
	const float freqMult = pitchBendToFreqMultiplier(shift);
	for(auto& note: mPlayingNotes)
	{
		if((note.Key >> 8) == pitchBend.Channel)
			note.Value.Sampler.MultiplyPitch(freqMult);
	}
}

void MidiSynth::OnChannelPanChange(const Midi::ChannelPanChange& panChange)
{
	const float pan = panChange.Pan/63.5f - 1;
	/*for(auto& note: mPlayingNotes)
	{
		if((note.Key >> 8) == panChange.Channel)
			note.Value.Sampler.SetPan(pan);
	}*/
	mChannelPans[panChange.Channel] = sbyte(panChange.Pan - 64);
}

void MidiSynth::OnChannelVolumeChange(const Midi::ChannelVolumeChange& volumeChange)
{
	const float volumeMult = Math::Max(float(volumeChange.Volume), 0.001f) / Math::Max(float(mChannelVolumes[volumeChange.Channel]), 0.001f);
	/*for(auto& note: mPlayingNotes)
	{
		if((note.Key >> 8) == volumeChange.Channel)
			note.Value.Sampler.MultiplyVolume(volumeMult);
	}*/
	mChannelVolumes[volumeChange.Channel] = volumeChange.Volume;
}

void MidiSynth::OnChannelReverbChange(const Midi::ChannelReverbChange& reverbChange)
{
	if(!mReverberator) return;
	const float reverbCoeff = reverbChange.ReverbCoeff / 127.0f;
	for(auto& note: mPlayingNotes)
	{
		if((note.Key >> 8) == reverbChange.Channel)
			note.Value.Sampler.SetReverbCoeff(reverbCoeff);
	}
	mChannelReverbs[reverbChange.Channel] = reverbChange.ReverbCoeff;
}

void MidiSynth::OnAllNotesOff(byte channel)
{
	Array<NoteMap::iterator> notesToRemove;
	for(auto it = mPlayingNotes.begin(); it != mPlayingNotes.end(); ++it)
	{
		if((it->Key >> 8) != channel) continue;
		it->Value.Sampler.NoteRelease();
		mOffPlayingNotes.AddLast(Cpp::Move(it->Value));
		notesToRemove.AddLast(it);
	}
	for(auto it: notesToRemove) mPlayingNotes.Remove(it);
}

void MidiSynth::OnChannelProgramChange(const Midi::ChannelProgramChange& programChange)
{
	mChannelPrograms[programChange.Channel] = programChange.Instrument;
}

Unique<MidiSynth> MidiSynth::FromFile(StringView path, double duration, const Synth::MidiInstrumentSet& instruments,
	float maxVolume, uint sampleRate, bool stereo, ErrorStatus& status)
{
	auto file = IO::OS.FileOpen(path, status);
	return new MidiSynth(
		Midi::MidiFileParser::CreateSingleOrderedMessageStream(Cpp::Move(file), status),
		duration, instruments, maxVolume, null, sampleRate, stereo);
}

#endif

}}}

INTRA_WARNING_POP
