#include "InstrumentSet.h"
#include "MusicalInstrument.h"
#include "Audio/Midi/MidiFileParser.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void MidiInstrumentSet::Preload(const Audio::Midi::MidiFileInfo& info, unsigned sampleRate)
{
	for(size_t i = 0; i < 128; i++)
	{
		if(info.UsedDrumInstrumentsFlags[i]) (*DrumInstruments[i])(1, sampleRate);
		// Мелодические инструменты файла: строим волновые таблицы заранее
		// (вне аудио-колбэка). Иначе первая нота каждого инструмента/регистра
		// генерирует таблицу прямо в колбэке рендера (3-20 мс при бюджете
		// ~5.8 мс) — слышимые выпадения звука в браузере.
		if(!(info.UsedInstrumentsFlags[i] && Instruments[i])) continue;
		MusicalInstrument* instr = Instruments[i];
		instr->PreloadTables(sampleRate);
		// Октавная сетка покрывает только ноты A — добираем точные клавиши,
		// которые реально звучат в файле (та же формула частоты, что и в
		// NoteOn::Frequency, — ключ WaveTableCache::Get при создании ноты).
		const auto& usedKeys = info.UsedKeysPerInstrument[i];
		for(size_t key = 0; key < 128; key++)
		{
			if(!usedKeys[key]) continue;
			const float freq = Intra::Audio::MusicNote::BasicFrequencies[byte(key % 12)]
				* 0.5f * float(1 << byte(key / 12));
			instr->PreloadKey(freq, sampleRate);
		}
	}
}

INTRA_WARNING_POP
