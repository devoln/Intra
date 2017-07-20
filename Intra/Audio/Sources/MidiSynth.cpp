#include "Audio/Sources/MidiSynth.h"

#include "Cpp/Warnings.h"

#include "Math/Math.h"

#include "Range/Mutation/Fill.h"
#include "Range/Reduction.h"
#include "Range/Mutation/Transform.h"

#include "IO/FileSystem.h"
#include "IO/FileReader.h"

#include "IO/Std.h"

#include "Audio/Synth/MusicalInstrument.h"
#include "Audio/Midi/MidiFileParser.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Sources {

#ifndef INTRA_NO_MUSIC_LOADER


MidiSynth::MidiSynth(Midi::TrackCombiner music, double duration, const Synth::MidiInstrumentSet& instruments, float maxVolume,
	OnCloseResourceCallback onClose, uint sampleRate, bool stereo):
	SeparateFloatAudioSource(Cpp::Move(onClose), sampleRate, ushort(stereo? 2: 1)),
	mInstruments(instruments),
	mMusic(Cpp::Move(music)),
	mTime(0),
	mSampleCount(duration == Cpp::Infinity? ~size_t(): size_t((duration+1)*sampleRate)),
	mMaxSample(maxVolume)
{}

bool MidiSynth::synthNote(Synth::NoteSampler& sampler, Span<float> dstLeft, Span<float> dstRight, bool add)
{
	Span<float> dstLeftStart = dstLeft;
	size_t samplesProcessed;
	if(mChannelCount >= 2)
	{
		samplesProcessed = sampler(dstLeft, dstRight, add);
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
		const size_t samplesBeforeNextEvent = (nextTime == Cpp::Infinity)? ~size_t(): size_t(Math::Max((nextTime - mTime)*mSampleRate + 0.5, 1.0));
		bool add = false;
		const auto dstLeftBeforeEvent = dstLeft.Take(samplesBeforeNextEvent);
		const auto dstRightBeforeEvent = dstRight.Take(samplesBeforeNextEvent);
		Array<NoteMap::iterator> notesToRemove;
		for(size_t i=0; i<mOffPlayingNotes.Length(); i++)
		{
			const bool removeThisNote = synthNote(mOffPlayingNotes[i].Sampler, dstLeftBeforeEvent, dstRightBeforeEvent, add);
			if(removeThisNote) mOffPlayingNotes.RemoveUnordered(i--);
			add = true;
		}
		for(auto it = mPlayingNotes.begin(); it != mPlayingNotes.end(); ++it)
		{
			const bool removeThisNote = synthNote(it->Value.Sampler, dstLeftBeforeEvent, dstRightBeforeEvent, add);
			if(removeThisNote) notesToRemove.AddLast(it);
			add = true;
		}
		for(auto it: notesToRemove) mPlayingNotes.Remove(it);
		if(!add)
		{
			FillZeros(dstLeftBeforeEvent);
			FillZeros(dstRightBeforeEvent);
		}
		else
		{
			auto minimax1 = MiniMax(dstLeftBeforeEvent.AsConstRange());
			auto minimax2 = MiniMax(dstRightBeforeEvent.AsConstRange());
			auto maxSample = Math::Max(
				Math::Max(Math::Abs(minimax1.first), Math::Abs(minimax1.second)),
				Math::Max(Math::Abs(minimax2.first), Math::Abs(minimax2.second)));
			if(mMaxSample < maxSample) mMaxSample = maxSample;
			Multiply(dstLeftBeforeEvent, 1.0f/mMaxSample);
			Multiply(dstRightBeforeEvent, 1.0f/mMaxSample);
		}
		dstLeft.PopFirstExactly(dstLeftBeforeEvent.Length());
		dstRight.PopFirstExactly(dstRightBeforeEvent.Length());
		totalSamplesProcessed += dstLeftBeforeEvent.Length();
		mTime += double(dstLeftBeforeEvent.Length())/mSampleRate;
		if(mPlayingNotes.Empty() && nextTime == Cpp::Infinity)
		{
			totalSamplesProcessed--;
			break;
		}
	}

	return totalSamplesProcessed;
}

void MidiSynth::OnNoteOn(const Midi::NoteOn& noteOn)
{
	if(noteOn.Volume == 0) return;
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
		auto instr = mInstruments.Instruments[noteOn.Instrument];
		if(instr == null) return;
		newNote = &mPlayingNotes[id];
		newNote->Sampler = (*instr)(noteOn.Frequency(), noteOn.TotalVolume(), mSampleRate);
	}
	else
	{
		auto instr = mInstruments.DrumInstruments[noteOn.NoteOctaveOrDrumId];
		if(instr == null) return;
		newNote = &mPlayingNotes[id];
		newNote->Sampler = Synth::NoteSampler();
		newNote->Sampler.GenericSamplers.AddLast((*instr)(noteOn.TotalVolume(), mSampleRate));
	}
	if(newNote)
	{
		newNote->Channel = noteOn.Channel;
		newNote->Time = noteOn.Time;
		newNote->NoteOctaveOrDrumId = noteOn.NoteOctaveOrDrumId;
		newNote->Sampler.Pan = noteOn.Pan/64.0f;
		const float freqMult = pitchBendToFreqMultiplier(mCurrentPitchBend[noteOn.Channel]);
		newNote->Sampler.MultiplyPitch(freqMult);
	}
}

float MidiSynth::pitchBendToFreqMultiplier(short relativePitchBend) const
{
	return Math::Pow(2.0f, relativePitchBend / 8192.0f * mPitchBendRangeInSemitones / 12);
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
	const short shift = pitchBend.Pitch - mCurrentPitchBend[pitchBend.Channel];
	mCurrentPitchBend[pitchBend.Channel] = pitchBend.Pitch;
	const float freqMult = pitchBendToFreqMultiplier(shift);
	for(auto& note: mPlayingNotes)
	{
		if((note.Key >> 8) != pitchBend.Channel) continue;
		note.Value.Sampler.MultiplyPitch(freqMult);
	}
}

void MidiSynth::OnAllNotesOff(byte channel)
{
	for(auto& note: mPlayingNotes)
	{
		if((note.Key >> 8) != channel) continue;
		note.Value.Sampler.NoteRelease();
		mOffPlayingNotes.AddLast(Cpp::Move(note.Value));
	}
	mPlayingNotes.Clear();
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
