#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"

#include "Range/Mutation/Fill.h"
#include "Range/Decorators/Take.h"

#include "Container/Utility/IndexAllocator.h"

#include "Data/ValueType.h"

#include "SoundBasicData.hxx"

#include <emscripten.h>


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio {

using Data::ValueType;

struct SoundContext: detail::SoundBasicContext
{
	SoundContext()
	{
		EM_ASM(
			Module.gWebAudioContext = new (window.AudioContext || window.webkitAudioContext)();
			Module.gWebAudioBufferArray = [];
			Module.gWebAudioInstanceArray = [];
			Module.gWebAudioStreamArray = [];
		);
	}

	SoundContext(const SoundContext&) = delete;
	SoundContext& operator=(const SoundContext&) = delete;

	static SoundContext& Instance()
	{
		static SoundContext context;
		return context;
	}

	IndexAllocator<ushort> BufferIdalloc;
	IndexAllocator<ushort> InstanceIdalloc;
	IndexAllocator<ushort> StreamedSoundIdalloc;
};

struct Sound::Data: detail::SoundBasicData
{
	Data(IAudioSource& src): SoundBasicData(src),
		Id(SoundContext::Instance().BufferIdalloc.Allocate())
	{
		const size_t totalSampleCount = src.SamplesLeft();
		EM_ASM_({
			var frameCount = $0;
			var buffer = Module.gWebAudioContext.createBuffer($1, frameCount, $2);
			Module.gWebAudioBufferArray[$3] = buffer;
		}, totalSampleCount, src.ChannelCount(), src.SampleRate(), Id);

		size_t totalSamplesRead = 0;
		FixedArray<float> tempBuffer(16384 * src.ChannelCount());
		Span<float> tempSpans[16];
		INTRA_DEBUG_ASSERT(Source->ChannelCount() <= 16);
		for(size_t c = 0; c < Source->ChannelCount(); c++)
			tempSpans[c] = Range::Drop(tempBuffer, BufferSampleCount*c).Take(BufferSampleCount);
		auto tempSpanRange = Range::Take(tempSpans, Source->ChannelCount());
		while(src.SamplesLeft() > 0)
		{
			size_t samplesRead = src.GetUninterleavedSamples(tempSpanRange);
			for(uint c = 0; c < Info.Channels; c++)
				SetChannelSubData(tempSpans[c].Take(samplesRead), totalSamplesRead);
			totalSamplesRead += samplesRead;
		}
	}

	void SetDataInterleaved(const void* data, ValueType type)
	{
		if(Info.Channels == 1)
		{
			SetDataChannels(&data, type);
			return;
		}

		if(type == ValueType::Float)
		{
			EM_ASM_({
				var bufferData = [];
				for(var c=0; c<$0; c++) bufferData[c] = Module.gWebAudioBufferArray[$3].getChannelData(c);
				var j=0;
				for(var i=0; i<$2; i++)
					for(var c=0; c<$0; c++)
						bufferData[c][i] = Module.HEAPF32[$1 + j++];
			}, Info.Channels, reinterpret_cast<size_t>(data)/sizeof(float), Info.SampleCount, Id);
		}
		else if(type == ValueType::SNorm16 || type == ValueType::Short)
		{
			EM_ASM_({
				var bufferData = [];
				for(var c=0; c<$0; c++) bufferData[c] = Module.gWebAudioBufferArray[$3].getChannelData(c);
				var j=0;
				for(var i=0; i<$2; i++)
					for(var c=0; c<$0; c++)
						bufferData[c][i] = (Module.HEAP16[$1+i] + 0.5) / 32767.5;
			}, Info.Channels, reinterpret_cast<size_t>(data)/sizeof(short), Info.SampleCount, Id);
		}
	}

	void SetChannelFloatSubData(CSpan<float> data, uint channel, size_t bufferOffset=0)
	{
		EM_ASM_({
			var buffer = Module.gWebAudioBufferArray[$3];
			if(buffer.copyToChannel === undefined)
			{
				var bufferData = buffer.getChannelData($0);
				bufferData.set(Module.HEAPF32.subarray($1, $1+$2), $4);
			}
			else buffer.copyToChannel(Module.HEAPF32.subarray($1, $1+$2), $0, $4);
		}, channel, reinterpret_cast<size_t>(data.Data())/sizeof(float), data.Length(), Id, bufferOffset);
	}

	void SetChannelsFloatSubData(CSpan<CSpan<float>> data, size_t bufferOffset=0)
	{
		for(uint c = 0; c < Info.Channels; c++)
			SetChannelFloatSubData(data[c], bufferOffset);
	}

	void SetChannelShortSubData(CSpan<short> data, uint channel, size_t bufferOffset=0)
	{
		EM_ASM_({
			var buffer = Module.gWebAudioBufferArray[$3];
			var bufferData = buffer.getChannelData($0);
			for(var i = 0; i<$2; i++) bufferData[$4 + i] = (Module.HEAP16[$1 + i] + 0.5) / 32767.5;
		}, channel, reinterpret_cast<size_t>(data.Data())/sizeof(short), data.Length(), Id, bufferOffset);
	}

	void SetChannelsShortSubData(CSpan<CSpan<short>> data, size_t bufferOffset=0)
	{
		for(uint c = 0; c < Info.Channels; c++)
			SetChannelShortSubData(data[c], bufferOffset);
	}

	void Release()
	{
		SoundContext::Instance().BufferIdalloc.Deallocate(ushort(Id));
		EM_ASM_({
			Module.gWebAudioBufferArray[$0] = null;
		}, Id);
	}

	size_t Id;
};

struct Sound::Instance::Data: detail::SoundInstanceBasicData
{
	forceinline Data(Shared<Sound::Data> parent): SoundInstanceBasicData(Cpp::Move(parent))
	{
		Id = SoundContext::Instance().InstanceIdalloc.Allocate();
		EM_ASM_({
			var source = Module.gWebAudioContext.createBufferSource();
			source.buffer = Module.gWebAudioBufferArray[$0];
			source.connect(Module.gWebAudioContext.destination);
			source.__is_playing = false;
			source.onended = function() { source.__is_playing = false; };
			Module.gWebAudioInstanceArray[$1] = source;
		}, Parent->Id, Id);
	}

	void Release()
	{
		Stop();
		EM_ASM_({
			Module.gWebAudioInstanceArray[$0] = null;
		}, Id);
		SoundContext::Instance().InstanceIdalloc.Deallocate(ushort(Id));
	}

	void Play(bool loop)
	{
		EM_ASM_({
			var src = Module.gWebAudioInstanceArray[$0];
			src.loop = $1;
			src.start();
			src.__is_playing = true;
		}, Id, loop);
	}

	bool IsPlaying()
	{
		return bool(EM_ASM_INT({
			return Module.gWebAudioInstanceArray[$0].__is_playing;
		}, Id));
	}

	void Stop()
	{
		EM_ASM_({
			Module.gWebAudioInstanceArray[$0].stop();
		}, Id);
	}

	size_t Id;
};

struct StreamedSound::Data: detail::StreamedSoundBasicData
{
	Data(Unique<IAudioSource> source, size_t bufferSampleCount, bool autoStreamingEnabled=true):
		StreamedSoundBasicData(Cpp::Move(source), bufferSampleCount > 16384? 16384: bufferSampleCount)
	{
		(void)autoStreamingEnabled;
		SoundContext::Instance();

		TempBuffer.SetCount(Source->SampleCount()*Source->ChannelCount());

		EM_ASM_({
			var result = Module.gWebAudioContext.createScriptProcessor($1, 0, $2);
			result.__is_playing = false;
			Module.gWebAudioStreamArray[$0] = result;
			result.onaudioprocess = function(audioProcessingEvent)
			{
				Module._Emscripten_StreamedSoundLoadCallback($3);
				var outputBuffer = audioProcessingEvent.outputBuffer;
				for(var ch = 0; ch < $2; ch++)
				{
					if(outputBuffer.copyToChannel === undefined)
					{
						var outputData = outputBuffer.getChannelData(ch);
						outputData.set(Module.HEAPF32.subarray($4+ch*$1, $4+ch*$1+$1));
					}
					else outputBuffer.copyToChannel(Module.HEAPF32.subarray($4+ch*$1, $4+ch*$1+$1), ch);
				}
			}
		}, Id, Source->SampleCount(), Source->ChannelCount(), this, reinterpret_cast<size_t>(TempBuffer.Data())/sizeof(float));
	}

	void Play(bool loop)
	{
		Looping = loop;
		EM_ASM_({
			var snd = Module.gWebAudioStreamArray[$0];
			snd.__is_playing = true;
			snd.__is_looping = $1;
			snd.connect(Module.gWebAudioContext.destination);
		}, Id, loop);
	}

	bool IsPlaying()
	{
		return bool(EM_ASM_INT({
			return Module.gWebAudioStreamArray[$0].__is_playing;
		}, Id));
	}

	void Stop()
	{
		EM_ASM_({
			var snd = Module.gWebAudioStreamArray[$0];
			if(snd.__is_playing) snd.disconnect(Module.gWebAudioContext.destination);
			snd.__is_playing = false;
		}, Id);
	}

	void Release()
	{
		Stop();
		EM_ASM_({
			Module.gWebAudioStreamArray[$0] = null;
		}, Id);
		SoundContext::Instance().StreamedSoundIdalloc.Deallocate(ushort(Id));
	}

	size_t loadBuffer()
	{
		if(StopSoon)
		{
			StopSoon = false;
			Stop();
			return 0;
		}

		Span<float> tempSpans[16];
		INTRA_DEBUG_ASSERT(Source->ChannelCount() <= 16);
		for(size_t c = 0; c < Source->ChannelCount(); c++)
			tempSpans[c] = TempBuffer.Drop(BufferSampleCount*c).Take(BufferSampleCount);
		size_t samplesRead = Source->GetUninterleavedSamples(Range::Take(tempSpans, Source->ChannelCount()));
		if(samplesRead >= BufferSampleCount) return samplesRead;

		for(size_t i = 0; i<Source->ChannelCount(); i++)
			tempSpans[i].PopFirstN(samplesRead);
		if(!Looping)
		{
			StopSoon = true;
			for(size_t i = 0; i<Source->ChannelCount(); i++)
				FillZeros(tempSpans[i]);
			return samplesRead;
		}
		samplesRead += Source->GetUninterleavedSamples(Range::Take(tempSpans, Source->ChannelCount()));
		return samplesRead;
	}

	size_t Id;
	Array<float> TempBuffer;

	bool Looping = false;
	bool StopSoon = false;
};

uint Sound::DefaultSampleRate()
{
	SoundContext::Instance();
	return uint(EM_ASM_INT({
		return Module.gWebAudioContext.sampleRate;
	}, 0));
}





extern "C" size_t EMSCRIPTEN_KEEPALIVE Emscripten_StreamedSoundLoadCallback(StreamedBufferHandle snd)
{return snd->loadBuffer();}

}}

INTRA_WARNING_POP
