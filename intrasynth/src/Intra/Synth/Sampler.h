#pragma once

#include "Container/Utility/SparseArray.h"
#include "Container/Sequential/Array.h"
#include "SamplerTask.h"
#include "Types.h"

class Sampler
{
	alignas(8) byte mInfo[8];
public:
	virtual ~Sampler() {}

	/// Виртульный конструктор перемещения из this в dst.
	/// Реализуется в каждом прозвольном типе по шаблону:
	/// {new(dst) DerivedSampler(Move(*this));}
	virtual void MoveConstruct(void* dst) = 0;

	/// Генерирует задачи для синтезатора и кладёт их в контейнер.
	/// @returns false, если известно, что этот семплер больше не будет генерировать задач, иначе true. false означает, что семплер можно удалить.
	virtual bool Generate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples) = 0;

	/// Умножить текущую частоту воспроизведения на указанное число, используется для note bend.
	virtual void MultiplyPitch(float freqMultiplier) {(void)freqMultiplier;}

	/// Умножить текущую громкость воспроизведения на указанное число, используется для MIDI событий смены громкости.
	/// Изменение громкости должно происходить плавно, иначе будет щелчок.
	virtual void MultiplyVolume(float volumeMultiplier) {(void)volumeMultiplier;}

	/// Установить текущую панораму - баланс между левым и правым каналами в интервале [-1; 1]
	/// Изменение должно происходить плавно, иначе будет щелчок.
	virtual void SetPan(float newPan) {(void)newPan;}

	/// Ready linear dynamic scale used only for irreversible pruning decisions.
	/// Raw MIDI controllers never cross this interface.
	virtual void SetPruneGain(float gain) {(void)gain;}

	/// Pass source-level render parameters to note samplers. Master effects are
	/// handled by MidiSynth; only note-local parameters are forwarded further.
	virtual void SetRenderParams(const RenderParams& params) {(void)params;}

	/// Нота отпущена, что означает, что пора начать её затухание.
	virtual void NoteRelease() {}

#ifdef INTRA_UI_METERS
	/// Текущий уровень огибающей ноты (0..1) для индикатора громкости в веб-UI.
	/// JS спрашивает его раз в ~100 мс (SourceGetNoteLevels), на семпл расходов
	/// нет. По умолчанию 1: семплеры без огибающей звучат ровно. Собирается
	/// только с -DINTRA_UI_METERS — минимальные сборки не тянут ни виртуальную
	/// функцию, ни её реализации.
	///
	/// Замерено (Update 82): поле вместо этого виртуального геттера НЕ экономит:
	/// слот в таблице виртуалов наследника стоит 4 байта, а поле требует записи
	/// уровня из горячего цикла рендера (в шаблонных циклах струн она
	/// инстанцируется по нескольку раз) — в сумме поле вышло на ~200 байт
	/// дороже. Поэтому геттер остаётся виртуальным.
	virtual float GetLevel() const {return 1.0f;}
#endif

	/// Готовый линейный динамический множитель поверх тела ноты. Верхний MIDI-
	/// слой переводит контроллеры в amplitude-domain; sampler не знает их номера
	/// или диапазоны. 0 может временно замьютить ноту без остановки её состояния.
	float OutputGain = 1.0f;


	/// Получить ссылку на метаинформацию о семплере, которую в него записывает синтезатор.
	template<typename T> INTRA_FORCEINLINE Requires<
		sizeof(T) <= 8,
	T&> GetInfo() {return *reinterpret_cast<T*>(mInfo);}
};

typedef Intra::Container::DynamicBlob<Sampler> SamplerContainer;
