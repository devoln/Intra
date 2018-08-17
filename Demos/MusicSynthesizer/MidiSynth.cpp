#include "MidiSynth.h"

#include <Cpp/Warnings.h>

#include <Math/Math.h>

#include <Range/Mutation/Fill.h>
#include <Range/Reduction.h>
#include <Range/Mutation/Transform.h>

#include <IO/FileSystem.h>
#include <IO/FileReader.h>

#include <Audio/Midi/MidiFileParser.h>

#include "MusicalInstrument.h"

using namespace Audio;
using namespace Midi;

using Concepts::begin;
using Concepts::end;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

MidiState::MidiState()
{
	Range::Fill(ChannelVolumes, byte(127));
}

MidiSynth::MidiSynth(Midi::TrackCombiner music, double duration, const MidiInstrumentSet& instruments, float maxVolume,
	OnCloseResourceCallback onClose, uint sampleRate, bool stereo, bool reverb):
	SeparateFloatAudioSource(Cpp::Move(onClose), sampleRate, ushort(stereo? 2: 1)),
	mInstruments(instruments),
	mMusic(Cpp::Move(music)),
	mTime(0),
	mSampleCount(duration == Cpp::Infinity? ~size_t(): size_t((duration+2)*sampleRate)),
	mMaxSample(maxVolume),
	mReverberator(size_t(reverb? 16384: 0), size_t(reverb? 32: 0), 1)
{
	
}

bool MidiSynth::synthNote(Sampler& sampler, Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb)
{
	Span<float> dstLeftStart = ioDstLeft;
	size_t samplesProcessed;
	if(mChannelCount >= 2)
	{
		samplesProcessed = sampler.GenerateStereo(ioDstLeft, ioDstRight, ioDstReverb);
		ioDstRight.PopFirstExactly(samplesProcessed);
	}
	else samplesProcessed = sampler.GenerateMono(ioDstLeft);
	ioDstLeft.PopFirstExactly(samplesProcessed);
	return samplesProcessed < dstLeftStart.Length();
}

size_t MidiSynth::GetUninterleavedSamplesAdd(CSpan<Span<float>> outFloatChannels)
{
	if(outFloatChannels.Empty()) return 0;
	Span<float> dstLeft = outFloatChannels.First();
	Span<float> dstRight = outFloatChannels.Get(1).Take(dstLeft.Length());
	if(outFloatChannels.Length() >= 2) dstLeft = dstLeft.Take(dstRight.Length());

	size_t totalSamplesProcessed = 0;
	while(!dstLeft.Empty())
	{
		const auto nextTime = mMusic.Empty()? MidiTime::Max: mMusic.NextEventTime();
		mPrevTime = nextTime;
		if(nextTime == mTime)
		{
			mMusic.ProcessEvent(*this);
			continue;
		}
		size_t samplesBeforeNextEvent = (nextTime == MidiTime::Max)? ~size_t():
			size_t(Math::Max((nextTime - mTime)*mSampleRate + MidiTime(0.5), MidiTime(1)));
		const size_t samplesLeft = SamplesLeft();
		if(samplesLeft == 0) break;
		samplesBeforeNextEvent = Funal::Min(samplesBeforeNextEvent, samplesLeft);
		bool add = false;
		const auto dstLeftBeforeEvent = dstLeft.Take(samplesBeforeNextEvent);
		const auto dstRightBeforeEvent = dstRight.Take(samplesBeforeNextEvent);

		if(mReverberator) mReverbChannelBuffer.SetCount(dstLeftBeforeEvent.Length());
		else mReverbChannelBuffer.Clear();
		Span<float> dstReverbBeforeEvent = null;
		
		for(auto noteSamplers = mNoteSamplers.AsRange(); !noteSamplers.Empty();)
		{
			const auto samplerIndex = noteSamplers.Index;
			auto& sampler = noteSamplers.Next();
			const auto& info = sampler.GetInfo<const NoteInfo&>();
			const byte reverbCoeff = mMidiState.ChannelReverbs[info.Channel];
			if(reverbCoeff != 0) dstReverbBeforeEvent = mReverbChannelBuffer;
			const bool removeThisNote = synthNote(sampler, dstLeftBeforeEvent, dstRightBeforeEvent, dstReverbBeforeEvent);
			if(removeThisNote)
			{
				mNoteSamplers.Delete(samplerIndex);
				mPlayingNoteMap.Remove(info.Key());
			}
		}

		//FillZeros(dstLeftBeforeEvent);
		//FillZeros(dstRightBeforeEvent);
		mReverberator(dstLeftBeforeEvent, dstRightBeforeEvent, dstReverbBeforeEvent);
		FillZeros(dstReverbBeforeEvent);

		//TODO: заменить на нормальную компрессию и вынести в отдельный эффект.
		//Этот код нормирует не только музыку сгенерированную этим классом, но и вместе с прежним состоянием буфера.
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
		dstLeft.PopFirstExactly(dstLeftBeforeEvent.Length());
		dstRight.PopFirstExactly(dstRightBeforeEvent.Length());
		totalSamplesProcessed += dstLeftBeforeEvent.Length();
		mTime += double(dstLeftBeforeEvent.Length())/mSampleRate;
		if(mNoteSamplers.Empty() && nextTime == MidiTime::Max)
		{
			if(dstLeft.Empty() && totalSamplesProcessed != 0) totalSamplesProcessed--;
			break;
		}
	}

	return totalSamplesProcessed;
}

size_t MidiSynth::GetUninterleavedSamples(CSpan<Span<float>> outFloatChannels)
{
	for(auto channel: outFloatChannels) FillZeros(channel);
	GetUninterleavedSamplesAdd(outFloatChannels);
}

void MidiSynth::OnNoteOn(const Midi::NoteOn& noteOn)
{
	const float totalStartVolume = float(noteOn.Velocity*mMidiState.ChannelVolumes[noteOn.Channel])/(127.0f*127.0f);
	const ushort key = noteOn.Id();
	Sampler* newNote = null;
	auto found = mPlayingNoteMap.Find(key);
	if(!found.Empty())
	{
		const auto samplerIndex = found.First().Value;
		//TODO: что если sampler уже удалён? Обращение по невалидному индексу!
		//Надо, чтобы SamplerContainer сообщал об удалении этому классу, чтобы он мог удалить невалидный индекс
		auto& sampler = mNoteSamplers.Get(samplerIndex);
		const auto& samplerInfo = sampler.GetInfo<NoteInfo>();
		if(samplerInfo.Time == noteOn.Time) return;
		sampler.NoteRelease();
	}
	if(noteOn.Channel != 9)
	{
		auto instr = getInstrument(noteOn.Channel);
		if(instr == null) return;
		newNote = mNoteSamplers.Add(, &mPlayingNoteMap[key]);
		newNote->Sampler = (*instr)(noteOn.Frequency(), totalStartVolume, mSampleRate);
	}
	else
	{
		auto instr = mInstruments.DrumInstruments[noteOn.NoteOctaveOrDrumId];
		if(instr == null) return;
		newNote = &mPlayingNotes[id];
		newNote->Sampler = NoteSampler();
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

Unique<MidiSynth> MidiSynth::FromFile(StringView path, double duration, const MidiInstrumentSet& instruments,
	float maxVolume, uint sampleRate, bool stereo, ErrorStatus& status)
{
	auto file = IO::OS.FileOpen(path, status);
	return new MidiSynth(
		Midi::MidiFileParser::CreateSingleOrderedMessageStream(Cpp::Move(file), status),
		duration, instruments, maxVolume, null, sampleRate, stereo);
}

INTRA_WARNING_POP
