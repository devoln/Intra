#pragma once

#include "Extra/Container/Utility/SparseArray.h"
#include "Extra/Container/Sequential/Array.h"
#include "SamplerTask.h"

class Sampler
{
	alignas(8) byte mInfo[8];
public:
	virtual ~Sampler() {}

	//! Виртульный конструктор перемещения из this в dst.
	//! Реализуется в каждом прозвольном типе по шаблону:
	//! {new(dst) DerivedSampler(Move(*this));}
	virtual void MoveConstruct(void* dst) = 0;

	//! Генерирует задачи для синтезатора и кладёт их в контейнер.
	//! @returns false, если известно, что этот семплер больше не будет генерировать задач, иначе true. false означает, что семплер можно удалить.
	virtual bool Generate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples) = 0;

	//! Умножить текущую частоту воспроизведения на указанное число, используется для note bend.
	virtual void MultiplyPitch(float freqMultiplier) {(void)freqMultiplier;}

	//! Умножить текущую громкость воспроизведения на указанное число, используется для MIDI событий смены громкости.
	//! Изменение громкости должно происходить плавно, иначе будет щелчок.
	virtual void MultiplyVolume(float volumeMultiplier) {(void)volumeMultiplier;}

	//! Установить текущую панораму - баланс между левым и правым каналами в интервале [-1; 1]
	//! Изменение должно происходить плавно, иначе будет щелчок.
	virtual void SetPan(float newPan) {(void)newPan;}

	//! Установить коэффициент канала реверберации.
	//! Изменение должно происходить плавно, иначе будет щелчок.
	virtual void SetReverbCoeff(float newCoeff) {(void)newCoeff;}

	//! Нота отпущена, что означает, что пора начать её затухание.
	virtual void NoteRelease() {}
	
	//! Получить ссылку на метаинформацию о семплере, которую в него записывает синтезатор.
	template<typename T> INTRA_FORCEINLINE Requires<
		sizeof(T) <= 8,
	T&> GetInfo() {return *reinterpret_cast<T*>(mInfo);}
};

typedef Intra::Container::DynamicBlob<Sampler> SamplerContainer;
