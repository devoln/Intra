#include "MusicalInstrument.h"
#include "Intra/Range/ForEach.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

NoteSampler MusicalInstrument::BuildNoteSampler(float freq, float volume, unsigned sampleRate,
	const NoteOnParams& noteParams) const
{
	NoteSampler result;

	for(auto& wave: Waves) result.WaveFormSamplers.AddLast(wave(freq, volume, sampleRate));
	for(auto& wave: WaveTables) result.WaveTableSamplers.AddLast(wave(freq, volume, sampleRate));
	if(WhiteNoise) result.WhiteNoiseSamplers.AddLast(WhiteNoise(freq, volume, sampleRate));
	for(auto& instrument: GenericInstruments) result.GenericSamplers.AddLast(instrument(freq, volume, sampleRate));
	if(DynamicGeneric) result.GenericSamplers.AddLast(DynamicGeneric(freq, volume, sampleRate, noteParams));

	// Модификаторы создаём парами: экземпляр для левого канала и экземпляр для
	// правого (см. NoteSampler::applyModifiersStereo). Обе копии рождаются из
	// одной фабрики и обрабатывают одинаковые по длине куски, поэтому их
	// расписание во времени совпадает, а фильтровая память у каналов своя.
	auto addModifierPair = [&](auto& factory)
	{
		result.Modifiers.AddLast(factory(freq, volume, sampleRate));
		result.ModifiersRight.AddLast(factory(freq, volume, sampleRate));
	};
	if(ExponentAttenuation) addModifierPair(ExponentAttenuation);
	if(Chorus) addModifierPair(Chorus);
	for(auto& mod: GenericModifiers) addModifierPair(mod);
	if(Envelope) result.ADSR = AdsrAttenuator(Envelope(sampleRate));

	// Static InstrumentLibrary calibration is baked into already-created
	// sampler amplitudes. The original input volume above remains untouched
	// during seed/timbre initialization.
	if(VolumeScale != 1.0f) result.MultiplyVolume(VolumeScale);
	return result;
}

Sampler& MusicalInstrument::CreateSampler(float freq, float volume, unsigned sampleRate,
	const NoteOnParams& noteParams, SamplerContainer& dst, uint16* oIndex) const
{
	auto& stored = dst.Add<NoteSampler>(BuildNoteSampler(freq, volume, sampleRate, noteParams));
	if(oIndex) *oIndex = uint16(dst.Length() - 1);
	return stored;
}

void MusicalInstrument::PreloadKey(float freq, unsigned sampleRate)
{
	// Таблицы кешируются в WaveTableCache::Get по точной частоте: вызов просто
	// заполняет кеш, повторные вызовы (перезагрузка файла, перемотка) ничего
	// не делают.
	for(auto& wave: WaveTables)
	{
		if(!wave.Tables) continue;
		wave.Tables->Get(freq, sampleRate);
	}
}

void MusicalInstrument::PreloadTables(unsigned sampleRate)
{
	// Волновые таблицы инструмента генерируются лениво в WaveTableCache::Get
	// при создании ноты. Для файлового рендера это происходит ВНУТРИ
	// аудио-колбэка (ProcessEvent → OnNoteOn → CreateSampler), и первая нота
	// каждого нового инструмента/регистра может занять 3-20 мс — при бюджете
	// колбэка 256/44100 ≈ 5.8 мс это даёт выпадения звука. Строим таблицы
	// заранее на октавной сетке; точные клавиши файла добирает
	// MidiInstrumentSet::Preload через PreloadKey.
	const float kPreloadFreqs[] = {55.0f, 110.0f, 220.0f, 440.0f, 880.0f, 1760.0f, 3520.0f};
	for(float f: kPreloadFreqs) PreloadKey(f, sampleRate);
}

INTRA_WARNING_POP
