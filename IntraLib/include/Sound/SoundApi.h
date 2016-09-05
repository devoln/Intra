#pragma once

#include "Data/ValueType.h"
#include "SoundApiDeclarations.h"

namespace Intra { namespace SoundAPI {

extern const ValueType::I InternalBufferType;
extern const int InternalChannelsInterleaved;

BufferHandle BufferCreate(size_t sampleCount, uint channels, uint sampleRate);
void BufferSetDataInterleaved(BufferHandle snd, const void* data, ValueType type);
void BufferSetDataChannels(BufferHandle snd, const void* const* data, ValueType type);
void* BufferLock(BufferHandle snd);
void BufferUnlock(BufferHandle snd);
void BufferDelete(BufferHandle snd);
InstanceHandle InstanceCreate(BufferHandle snd);
void InstanceDelete(InstanceHandle si);
void InstancePlay(InstanceHandle si, bool loop);
bool InstanceIsPlaying(InstanceHandle si);
void InstanceStop(InstanceHandle si);
void InstanceSetDeleteOnStop(InstanceHandle snd, bool del);


struct StreamingCallback
{
	typedef size_t(*CallbackFunctionType)(void** dstSamples, uint channels,
		ValueType::I type, bool interleaved, size_t sampleCount, void* additionalData);

	CallbackFunctionType CallbackFunction;
	void* CallbackData;
};

StreamedBufferHandle StreamedBufferCreate(size_t sampleCount,
	uint channels, uint sampleRate, StreamingCallback callback);
void StreamedBufferSetDeleteOnStop(StreamedBufferHandle snd, bool del);
void StreamedBufferDelete(StreamedBufferHandle snd);
void StreamedSoundPlay(StreamedBufferHandle snd, bool loop);
bool StreamedSoundIsPlaying(StreamedBufferHandle snd);
void StreamedSoundStop(StreamedBufferHandle snd);
void StreamedSoundUpdate(StreamedBufferHandle snd);

}}

