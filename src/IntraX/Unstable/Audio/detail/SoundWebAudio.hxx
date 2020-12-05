


#include "Intra/Range/Mutation/Fill.h"
#include "Intra/Range/Take.h"

#include "IntraX/Container/Utility/IndexAllocator.h"

#include "IntraX/Unstable/Data/ValueType.h"

#include "SoundBasicData.hxx"
#include "IntraX/Utils/Shared.h"

#include <emscripten.h>


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_IGNORE_WARN(dollar-in-identifier-extension)


INTRA_BEGIN
namespace Audio {

using ValueType;

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

	void ReleaseAllSounds();

	void ReleaseAllStreamedSounds();

	IndexAllocator<uint16> BufferIdalloc;
	IndexAllocator<uint16> InstanceIdalloc;
	IndexAllocator<uint16> StreamedSoundIdalloc;
};

struct Sound::Data: detail::SoundBasicData
{
	Data(IAudioSource& src): SoundBasicData(src)
	{
		auto& context = SoundContext::Instance();
		Id = context.BufferIdalloc.Allocate();
		const size_t totalSampleCount = src.SamplesLeft();
		EM_ASM_({
			var buffer = Module.gWebAudioContext.createBuffer($1, $0, $2);
			Module.gWebAudioBufferArray[$3] = buffer;
		}, totalSampleCount, Info.Channels, Info.SampleRate, Id);

		size_t totalSamplesRead = 0;
		static const size_t tempBufferSamplesPerChannel = 16384;
		FixedArray<float> tempBuffer(tempBufferSamplesPerChannel * Info.Channels);
		Span<float> tempSpans[16];
		INTRA_DEBUG_ASSERT(Info.Channels <= 16);
		for(size_t c = 0; c < Info.Channels; c++)
			tempSpans[c] = Drop(tempBuffer, tempBufferSamplesPerChannel*c).Take(tempBufferSamplesPerChannel);
		auto tempSpanRange = Take(tempSpans, Info.Channels);
		while(totalSamplesRead < totalSampleCount)
		{
			size_t samplesRead = src.GetUninterleavedSamples(tempSpanRange);
			for(unsigned c = 0; c < Info.Channels; c++)
				SetChannelFloatSubData(tempSpans[c].Take(samplesRead), c, totalSamplesRead);
			totalSamplesRead += samplesRead;
		}

		context.AllSounds.AddLast(this);
	}

	~Data()
	{
		Release();
	}

	void SetDataInterleaved(const void* data, ValueType type)
	{
		if(Info.Channels == 1)
		{
			if(type == ValueType::Float) SetChannelsFloatSubData({CSpanOfRaw<float>(data, Info.GetBufferSize())});
			else if(type == ValueType::Short || type == ValueType::SNorm16) SetChannelsShortSubData({CSpanOfRaw<short>(data, Info.GetBufferSize())});
			return;
		}

		if(type == ValueType::Float)
		{
			EM_ASM_({
				var snd = Module.gWebAudioBufferArray[$3];
				if(snd == null) return;
				var bufferData = [];
				for(var c=0; c<$0; c++) bufferData[c] = snd.getChannelData(c);
				var j=0;
				for(var i=0; i<$2; i++)
					for(var c=0; c<$0; c++)
						bufferData[c][i] = Module.HEAPF32[$1 + j++];
			}, Info.Channels, reinterpret_cast<size_t>(data)/sizeof(float), Info.SampleCount, Id);
		}
		else if(type == ValueType::SNorm16 || type == ValueType::Short)
		{
			EM_ASM_({
				var snd = Module.gWebAudioBufferArray[$3];
				if(snd == null) return;
				var bufferData = [];
				for(var c=0; c<$0; c++) bufferData[c] = snd.getChannelData(c);
				var j=0;
				for(var i=0; i<$2; i++)
					for(var c=0; c<$0; c++)
						bufferData[c][i] = (Module.HEAP16[$1+i] + 0.5) / 32767.5;
			}, Info.Channels, reinterpret_cast<size_t>(data)/sizeof(short), Info.SampleCount, Id);
		}
	}

	void SetChannelFloatSubData(CSpan<float> data, unsigned channel, size_t bufferOffset=0)
	{
		EM_ASM_({
			var buffer = Module.gWebAudioBufferArray[$3];
			if(buffer == null) return;
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
		for(unsigned c = 0; c < Info.Channels; c++)
			SetChannelFloatSubData(data[c], bufferOffset);
	}

	void SetChannelShortSubData(CSpan<short> data, unsigned channel, size_t bufferOffset=0)
	{
		EM_ASM_({
			var buffer = Module.gWebAudioBufferArray[$3];
			if(buffer == null) return;
			var bufferData = buffer.getChannelData($0);
			for(var i = 0; i<$2; i++) bufferData[$4 + i] = (Module.HEAP16[$1 + i] + 0.5) / 32767.5;
		}, channel, reinterpret_cast<size_t>(data.Data())/sizeof(short), data.Length(), Id, bufferOffset);
	}

	void SetChannelsShortSubData(CSpan<CSpan<short>> data, size_t bufferOffset=0)
	{
		for(unsigned c = 0; c < Info.Channels; c++)
			SetChannelShortSubData(data[c], bufferOffset);
	}

	void Release()
	{
		if(Id == ~size_t()) return;
		auto& context = SoundContext::Instance();
		context.AllSounds.FindAndRemoveUnordered(this);
		context.BufferIdalloc.Deallocate(uint16(Id));
		EM_ASM_({
			Module.gWebAudioBufferArray[$0] = null;
		}, Id);
		Id = ~size_t();
	}

	size_t Id;
};

struct Sound::Instance::Data: SharedClass<Sound::Instance::Data>, detail::SoundInstanceBasicData
{
	INTRA_FORCEINLINE Data(Shared<Sound::Data> parent): SoundInstanceBasicData(Move(parent))
	{
		Id = SoundContext::Instance().InstanceIdalloc.Allocate();
		EM_ASM_({
			var source = Module.gWebAudioContext.createBufferSource();
			source.buffer = Module.gWebAudioBufferArray[$0];
			source.connect(Module.gWebAudioContext.destination);
			source.__is_playing = false;
			source.onended = function()
			{
				source.__is_playing = false;
				Module._Emscripten_OnSoundStopCallback($2);
			};
			Module.gWebAudioInstanceArray[$1] = source;
		}, Parent->Id, Id, this);
	}

	~Data()
	{
		Release();
	}

	void Release()
	{
		if(Id == ~size_t()) return;
		auto& context = SoundContext::Instance();
		auto ref = Move(SelfRef);
		stop();
		EM_ASM_({
			if(Module.gWebAudioInstanceArray[$0] != null) Module.gWebAudioInstanceArray[$0].disconnect(Module.gWebAudioContext.destination);
			Module.gWebAudioInstanceArray[$0] = null;
		}, Id);
		context.InstanceIdalloc.Deallocate(uint16(Id));
		Id = ~size_t();
	}

	void Play(bool loop)
	{
		SelfRef = SharedThis();
		EM_ASM_({
			var src = Module.gWebAudioInstanceArray[$0];
			if(src == null) return;
			src.loop = $1;
			src.start();
			src.__is_playing = true;
		}, Id, loop);
	}

	bool IsPlaying()
	{
		return bool(EM_ASM_INT({
			var src = Module.gWebAudioInstanceArray[$0];
			if(src == null) return false;
			return src.__is_playing;
		}, Id));
	}

	void Stop()
	{
		auto ref = Move(SelfRef);
		if(Id == ~size_t()) return;
		stop();
	}

private:
	void stop()
	{
		EM_ASM_({
			var src = Module.gWebAudioInstanceArray[$0];
			if(src == null) return;
			src.stop();
		}, Id);
	}

public:
	size_t Id;
};

struct StreamedSound::Data: SharedClass<StreamedSound::Data>, detail::StreamedSoundBasicData
{
	Data(Unique<IAudioSource> source, size_t bufferSampleCount, bool autoStreamingEnabled=true):
		StreamedSoundBasicData(Move(source), bufferSampleCount > 16384? 16384: bufferSampleCount)
	{
		(void)autoStreamingEnabled;
		auto& context = SoundContext::Instance();

		Id = context.StreamedSoundIdalloc.Allocate();
		TempBuffer.SetCount(BufferSampleCount*Source->ChannelCount());

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
		}, Id, BufferSampleCount, Source->ChannelCount(), this, reinterpret_cast<size_t>(TempBuffer.Data())/sizeof(float));
		context.AllStreamedSounds.AddLast(this);
	}

	~Data()
	{
		Release();
	}

	void Play(bool loop)
	{
		SelfRef = SharedThis();
		Looping = loop;
		EM_ASM_({
			var snd = Module.gWebAudioStreamArray[$0];
			if(snd == null) return;
			snd.__is_playing = true;
			snd.__is_looping = $1;
			snd.connect(Module.gWebAudioContext.destination);
		}, Id, loop);
	}

	bool IsPlaying()
	{
		return bool(EM_ASM_INT({
			var snd = Module.gWebAudioStreamArray[$0];
			if(snd == null) return false;
			return snd.__is_playing;
		}, Id));
	}

	void Stop()
	{
		stop();
		SelfRef = null;
	}

	void stop()
	{
		EM_ASM_({
			var snd = Module.gWebAudioStreamArray[$0];
			if(snd == null) return;
			if(snd.__is_playing) snd.disconnect(Module.gWebAudioContext.destination);
			snd.__is_playing = false;
		}, Id);
	}

	void Release()
	{
		if(Id == ~size_t()) return;
		auto& context = SoundContext::Instance();
		context.AllStreamedSounds.FindAndRemoveUnordered(this);
		EM_ASM_({
			var snd = Module.gWebAudioStreamArray[$0];
			if(snd == null) return;
			if(snd.__is_playing) snd.disconnect(Module.gWebAudioContext.destination);
			snd.__is_playing = false;
			Module.gWebAudioStreamArray[$0] = null;
		}, Id, this);
		context.StreamedSoundIdalloc.Deallocate(uint16(Id));
		Id = ~size_t();
		SelfRef = null;
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
		size_t samplesRead = Source->GetUninterleavedSamples(Take(tempSpans, Source->ChannelCount()));
		if(samplesRead >= BufferSampleCount) return samplesRead;

		for(size_t i = 0; i<Source->ChannelCount(); i++)
			tempSpans[i].PopFirstCount(samplesRead);
		if(!Looping)
		{
			StopSoon = true;
			for(size_t i = 0; i<Source->ChannelCount(); i++)
				FillZeros(tempSpans[i]);
			return samplesRead;
		}
		samplesRead += Source->GetUninterleavedSamples(Take(tempSpans, Source->ChannelCount()));
		return samplesRead;
	}

	size_t Id;
	Array<float> TempBuffer;

	bool Looping = false;
	bool StopSoon = false;
};

unsigned Sound::DefaultSampleRate()
{
	SoundContext::Instance();
	return unsigned(EM_ASM_INT({
		return Module.gWebAudioContext.sampleRate;
	}, 0));
}


void SoundContext::ReleaseAllSounds()
{
	auto sounds = Move(AllSounds);
	for(auto sound: sounds) sound->Release();
}

void SoundContext::ReleaseAllStreamedSounds()
{
	auto sounds = Move(AllStreamedSounds);
	for(auto sound: sounds) sound->Release();
}



extern "C" size_t EMSCRIPTEN_KEEPALIVE Emscripten_StreamedSoundLoadCallback(StreamedSound::Data* snd)
{return snd->loadBuffer();}

extern "C" void EMSCRIPTEN_KEEPALIVE Emscripten_OnSoundStopCallback(Sound::Instance::Data* snd)
{snd->Stop();}

}}

INTRA_WARNING_POP
