#pragma once

#include "Cpp/Warnings.h"

#include "Utils/Span.h"

#include "Funal/Delegate.h"

#include "Audio/MusicNote.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio {

struct MusicTrack;
namespace Synth {

class IGenericSampler: public Funal::ICopyableMutableFunctor<Span<float>(Span<float> dst, bool add)>
{
public:
	virtual void NoteRelease() {}
	virtual void MultiplyPitch(float freqMultiplier) {(void)freqMultiplier;}
	IGenericSampler* Clone() const override = 0;
};

INTRA_DEFINE_EXPRESSION_CHECKER(HasNoteRelease, Meta::Val<T>().NoteRelease());
INTRA_DEFINE_EXPRESSION_CHECKER(HasMultiplyPitch, Meta::Val<T>().MultiplyPitch(float()));

template<typename OBJ> class GenericSamplerImpl: public IGenericSampler
{
public:
	GenericSamplerImpl(OBJ&& obj): Obj(Cpp::Move(obj)) {}
	GenericSamplerImpl(const OBJ& obj): Obj(obj) {}
	GenericSamplerImpl* Clone() const final { return new GenericSamplerImpl(Obj); }
	forceinline Span<float> operator()(Span<float> dst, bool add) final { return Obj(dst, add); }

	forceinline void NoteRelease() final {noteRelease();}
	forceinline void MultiplyPitch(float freqMultiplier) final {multiplyPitch(freqMultiplier);}

	OBJ Obj;

private:
	template<typename U = OBJ> forceinline Meta::EnableIf<
		HasNoteRelease<U>::_
	> noteRelease() {Obj.NoteRelease();}

	template<typename U = OBJ> forceinline Meta::EnableIf<
		!HasNoteRelease<U>::_
	> noteRelease() {}

	template<typename U = OBJ> forceinline Meta::EnableIf<
		HasMultiplyPitch<U>::_
	> multiplyPitch(float freqMultiplier) {Obj.MultiplyPitch(freqMultiplier);}

	template<typename U = OBJ> forceinline Meta::EnableIf<
		!HasMultiplyPitch<U>::_
	> multiplyPitch(float freqMultiplier) {(void)freqMultiplier;}
};

class GenericSampler
{
	Unique<IGenericSampler> mSampler;

	typedef Span<float>(*FunctionPtr)(Span<float> dst, bool add);

	
public:
	forceinline GenericSampler(null_t=null): mSampler(null) {}

	template<typename T, typename = Meta::EnableIf<
		!Meta::IsFunction<Meta::RemovePointer<Meta::RemoveConstRef<T>>>::_ &&
		Meta::IsCallable<T, Span<float>, bool>::_
	>> GenericSampler(T&& obj): mSampler(new GenericSamplerImpl<Meta::RemoveConstRef<T>>(Cpp::Forward<T>(obj))) {}

	GenericSampler(FunctionPtr freeFunction)
	{if(freeFunction) mSampler = new GenericSamplerImpl<FunctionPtr>(freeFunction);}

	forceinline GenericSampler(Unique<IGenericSampler> sampler):
		mSampler(Cpp::Move(sampler)) {}

	forceinline GenericSampler(const GenericSampler& rhs)
	{if(rhs) mSampler = rhs.mSampler->Clone();}

	forceinline GenericSampler(GenericSampler&& rhs) = default;

	//! Генератор семплов.
	//! @param[out] dst Массив в который записываются или складываются семплы. Считывается семплов не больше, чем dst.Length().
	//! @param add Если true, семплы будут суммироваться в dst, иначе dst будет перезаписываться.
	//! @return Возвращает часть dst, которая осталась не заполненной.
	forceinline Span<float> operator()(Span<float> dst, bool add) {return (*mSampler)(dst, add);}

	forceinline void NoteRelease() {mSampler->NoteRelease();}
	forceinline void MultiplyPitch(float freqMultiplier) {mSampler->MultiplyPitch(freqMultiplier);}

	forceinline bool operator==(null_t) const noexcept {return mSampler == null;}
	forceinline bool operator!=(null_t) const noexcept {return mSampler != null;}
	forceinline bool operator!() const noexcept {return operator==(null);}


	GenericSampler& operator=(const GenericSampler& rhs)
	{
		if(!rhs.mSampler) mSampler = null;
		else mSampler = rhs.mSampler->Clone();
		return *this;
	}

	forceinline GenericSampler& operator=(GenericSampler&&) = default;

	forceinline Unique<IGenericSampler> TakeAwaySampler() noexcept {return Cpp::Move(mSampler);}
	forceinline IGenericSampler& MySampler() const {return *mSampler;}
	forceinline IGenericSampler* ReleaseSampler() noexcept {return mSampler.Release();}

	forceinline explicit operator bool() const {return mSampler != null;}
};

//! Модификатор семплов.
//! @param[in,out] inOutSamples Массив, содержащий обрабатываемые семплы.
typedef Funal::CopyableMutableDelegate<void(
	Span<float> inOutSamples
)> GenericModifier;

//! Инструмент - источник семплеров нот.
typedef Funal::CopyableDelegate<GenericSampler(
	float freq, float volume, uint sampleRate
)> GenericInstrument;

//! Ударный инструмент - источник семплеров нот.
typedef Funal::CopyableDelegate<GenericSampler(
	float volume, uint sampleRate
)> GenericDrumInstrument;

//! Фабрика модификаторов - источник модификаторов семплов.
typedef Funal::CopyableDelegate<GenericModifier(
	float freq, float volume, uint sampleRate
)> GenericModifierFactory;


struct MusicalInstrument;

struct MidiInstrumentSet
{
	MusicalInstrument* Instruments[128]{null};
	GenericDrumInstrument* DrumInstruments[128]{null};
};

}}}

INTRA_WARNING_POP
