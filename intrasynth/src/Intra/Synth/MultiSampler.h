#pragma once

#include "Sampler.h"
#include "Container/Sequential/Array.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Семплер, объединяющий несколько семплеров в один. Владеет подсэмплерами.
class MultiSampler: public Sampler
{
	Array<Sampler*> mSubSamplers;
public:
	MultiSampler() {}
	~MultiSampler() {for(auto* s: mSubSamplers) delete s;}

	void MoveConstruct(void* dst) override
	{
		auto* result = new(dst) MultiSampler();
		result->mSubSamplers = Move(mSubSamplers);
		mSubSamplers = nullptr;
	}

	void AddSampler(Sampler* sampler) {mSubSamplers.AddLast(sampler);}
	size_t Count() const {return mSubSamplers.Length();}

	bool Generate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples) override
	{
		bool anyAlive = false;
		for(size_t i = 0; i < mSubSamplers.Length(); i++)
			if(mSubSamplers[i]->Generate(dstTasks, offsetInSamples, numSamples)) anyAlive = true;
		return anyAlive;
	}

	void MultiplyPitch(float freqMultiplier) override {for(auto* s: mSubSamplers) s->MultiplyPitch(freqMultiplier);}
	void MultiplyVolume(float volumeMultiplier) override {for(auto* s: mSubSamplers) s->MultiplyVolume(volumeMultiplier);}
	void SetPan(float newPan) override {for(auto* s: mSubSamplers) s->SetPan(newPan);}
	void SetRenderParams(const RenderParams& params) override {for(auto* s: mSubSamplers) s->SetRenderParams(params);}
	void NoteRelease() override {for(auto* s: mSubSamplers) s->NoteRelease();}
};

INTRA_WARNING_POP
