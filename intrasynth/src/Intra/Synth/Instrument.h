#pragma once

#include "Sampler.h"
#include "Container/Utility/Blob.h"

class Instrument
{
public:
	virtual ~Instrument() {}
	virtual void MoveConstruct(void* dst) = 0;
	virtual Sampler& CreateSampler(float freq, float volume, unsigned sampleRate,
		SamplerContainer& dst, uint16* oIndex = nullptr) const = 0;

	/// Подготовить дорогие данные инструмента (волновые таблицы и т.п.)
	/// заранее, вне аудио-колбэка. Вызывается при создании источника
	/// (прелоад использованных в файле инструментов) и при смене программы
	/// (живой ввод). Рендер нот не должен строить таблицы в реальном времени:
	/// в браузере это происходит внутри ScriptProcessor-колбэка и вызывает
	/// выпадения звука («звук прерывается»).
	virtual void PreloadTables(unsigned sampleRate) {(void)sampleRate;}

	/// Подготовить волновую таблицу конкретной частоты заранее, вне
	/// аудио-колбэка (первая нота каждой клавиши не должна генерировать
	/// таблицу в реальном времени — это роняет звук на всех голосах).
	virtual void PreloadKey(float freq, unsigned sampleRate) {(void)freq; (void)sampleRate;}
};

typedef Intra::Container::DynamicBlob<Instrument, alignof(Instrument), uint16> InstrumentContainer;
