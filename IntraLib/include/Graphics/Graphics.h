#pragma once

#include "Memory/SmartRef.h"
#include "Imaging/ImagingTypes.h"
#include "States.h"
#include "ObjectDescs.h"
#include "IO/LogSystem.h"
#include "Math/Vector.h"

namespace Intra {

class GraphicsWindow;

namespace Graphics {

struct DeviceCaps;

//! Абстрактный класс графики, реализации которого полностью инкапсулируют особенности графического API
class AGraphics: public Memory::IntrusiveRefCounted<AGraphics>
{
public:
	AGraphics(GraphicsWindow* wnd);
	virtual ~AGraphics() {}

	static Memory::IntrusiveReference<AGraphics> Current;

	GraphicsWindow* OwnerWindow() const {return owner_window;};

	GraphicsAPI GAPI() const {return gapi;};

	const DeviceCaps& Caps() const {return caps;};


	Math::Vec3 ErrorColor, WarnColor, PerfWarnColor, PerfClockColor, InfoColor; //< Цвет, которым выводится сообщение в соответветствующий лог
	IO::AppLogger ErrorLog; //< Лог, в который будут выводиться сообщения об ошибках
	IO::AppLogger WarnLog; //< Лог, в который будут выводиться предупреждения
	IO::AppLogger PerfWarnLog; //Лог, в который будут выводиться предупреждения, связанные с производительностью
	IO::AppLogger PerfClockLog; //Лог, в который будут выводиться результаты замера производительности кода
	IO::AppLogger InfoLog; //Лог, в который будет выводиться дополнительная информация
	IO::LogDetail ShaderCodeLogging = IO::LogDetail::OnWarning;

	uint FrameNumber;

public: //Далее идёт обёртка-абстракция от API

	virtual MemoryBlockId MemoryAllocate(uint bytes, MemoryFlags flags) = 0;


	virtual void MemoryFree(MemoryBlockId block) = 0;


	virtual const MemoryBlockDesc& MemoryGetDesc(MemoryBlockId block) = 0;

	//! Создать текстуру
	virtual TextureId TextureAllocate(const TextureDesc& desc) = 0;

	//! Получить информацию о текстуре
	virtual const TextureDesc& TextureGetDesc(TextureId tex) = 0;
	
	//! Заполнить указанную прямоугольную область текстуры данными
    virtual void TextureSetData(TextureId tex, uint mipLevel, math::USVec3 offset, math::USVec3 size,
		ImageFormat format, const void* data, bool swapRB, uint lineAlignment) = 0;

	//! Сообщить о том, что mipmaps стали невалидны и их надо перегенерировать перед использованием
	virtual void TextureGenerateMipmaps(TextureId tex) = 0;

	//! Удалить текстуру
	virtual void TextureDelete(TextureId tex) = 0;

	//! Копирует данные текселей в dstBuffer
    virtual void TextureGetData(TextureId src, uint mipmapLevel, math::USVec3 offset, math::USVec3 size,
		ImageFormat dstFormat, void* dstBuffer, bool swapRB, uint lineAlignment) = 0;

	//! Побитовое копирование прямоугольника текстуры src в прямоугольник текстуры dst. Размеры в байтах должны совпадать
	virtual void TextureCopyBitData(TextureId dst, uint dstMipLevel, math::USVec3 dstOffset, math::USVec3 dstSize,
		TextureId src, uint srcMipLevel, math::USVec3 srcOffset, math::USVec3 srcSize) = 0;

	//! Копировать прямоугольник src в dst с преобразованием форматов и растягиванием, если необходимо
    virtual void TextureCopyData(TextureId dst, uint dstMipLevel, math::USVec3 dstOffset, math::USVec3 dstSize,
		TextureId src, uint srcMipLevel, math::USVec3 srcOffset, math::USVec3 srcSize) = 0;

	//! Создать копию текстуры
    virtual TextureId TextureCopy(TextureId src)=0;

	//! Создание битовой копии данных текстуры, которые интерпретируются с указанными форматом и размерами
	virtual TextureId TextureBitCopy(TextureId src, const ImageInfo& desc) = 0;

	//! Залить указанную область текстуры цветом color
	virtual void Texture2DClear(TextureId tex, const math::Vec4& color,
		math::USVec3 offset={0,0,0}, math::USVec2 size={65535, 65535}) = 0;


	//! Создать семплер
	virtual SamplerId SamplerCreate(const SamplerDesc& desc) = 0;

	//! Удалить семплер
	virtual void SamplerDelete(SamplerId samp) = 0;

	

	//! Создать шейдерный объект.
	//! \param code Код шейдера.
	//! \param type Тип шейдерного объекта (вершинный, фрагментный, геометрический, тесселяции, вычислительный).
	//! \param language Язык, на котором написан шейдер (GLSL, HLSL, Cg, asm, SPIR-V, binary, ...).
	//! \param[out] Log Куда записать строку с логом компиляции.
	//! \returns Созданный шейдерный объект или null при ошибке.
	virtual ShaderObjectId ShaderObjectCreate(const ShaderObjectDesc& desc, string* Log) = 0;

	//! Удалить шейдерный объект
	virtual void ShaderObjectDelete(ShaderObjectId hndl) = 0;
	
	virtual intptr ShaderUniformGetId(ShaderProgramId sh, StringView uname) = 0;
	virtual const ShaderInfo& ShaderGetInfo(ShaderProgramId sh) = 0;

	//! Скомпоновать шейдер из объектов
	virtual ShaderProgramId ShaderProgramCreate(const ShaderProgramDesc& desc, string* Log) = 0;

	//! Удалить шейдер
	virtual void ShaderProgramDelete(ShaderProgramId hndl) = 0;


	//! Создать буфер внутри ранее выделенного блока памяти block
	//! \param block В каком блоке памяти создать буфер
	//! \param start Начало буфера в блоке памяти
	//! \param size Размер создаваемого буфера в байтах
	//! \returns Созданный буфер или null при ошибке
	virtual BufferViewId BufferViewCreate(const BufferViewDesc& desc) = 0;

	//! Прочитать данные из буфера
	//! \param[in] hndl Из какого буфера считать данные
	//! \param pos Начальное смещение в буфере
	//! \param data Куда записать данные
	//! \param Сколько байт прочитать и записать в data
	virtual void BufferRead(BufferViewId hndl, uint pos, void* data, uint bytes) = 0;

	//! Записать данные в буфер
	//! \param[out] hndl В какой буфер записать данные
	//! \param pos Начальное смещение в буфере
	//! \param[in] data Записываемые данные
	//! \param bytes Сколько байт записать из data в буфер
	virtual void BufferWrite(BufferViewId hndl, uint pos, const void* data, uint bytes) = 0;

	//! Скопировать данные из одного буфера в другой
	//! \param[in] hsrc Из какого буфера
	//! \param srcPos Смещение в буфере hsrc
	//! \param[out] hdst В какой буфер
	//! \param dstPos Смещение в буфере hdst
	//! \param bytes Сколько байт копировать
	virtual void BufferMemcpy(BufferViewId hsrc, uint srcPos, BufferViewId hdst, uint dstPos, uint bytes) = 0;

	//! Скопировать данные из буфера в пиксели текстуры
	virtual void BufferCopyToTexture(BufferViewId hsrc, uintptr srcPos, TextureId hdst,
		uint mipLevel, math::USVec3 dstOffset, math::USVec3 dstSize, uint lineAlignment=4) = 0;
	
	//! Скопировать данные из текстуры в буфер
	virtual void BufferCopyFromTexture(TextureId hsrc, uint mipLevel,
		math::USVec3 srcOffset, math::USVec3 srcSize, BufferViewId hdst, uintptr dstPos, uint lineAlignment=4) = 0;

	//! Заблокировать буфер для чтения и\или записи
	//! \returns Указатель на данные буфера
	virtual void* BufferLock(BufferViewId hndl, uint start, uint end, LockAccess access, bool unsynchronized) = 0;

	//! Разблокировать заблокированный буфер
	virtual bool  BufferUnlock(BufferViewId hndl) = 0;

	//! Удалить буфер
	virtual void  BufferViewDelete(BufferViewId hndl) = 0;


	virtual SyncId SyncFenceCreate() = 0;
	virtual void SyncDelete(SyncId sync) = 0;
	virtual void SyncWait(SyncId sync) = 0;
	virtual bool SyncClientWait(SyncId sync, ulong64 timeout) = 0;

	//! Создать render pass
	virtual RenderPassId RenderPassCreate(const RenderPassDesc& desc) = 0;

	//! Удалить render pass
	virtual void RenderPassDelete(RenderPassId rp) = 0;

	//! Создать фреймбуфер из присоединяемых текстур
	//! \param colorAttachments,depthAttachment,stencilAttachment В какие текстуры рендерить
	virtual FramebufferId FramebufferCreate(const FramebufferDesc& desc) = 0;

	//! Сделать указанный фреймбуфер текущим и установить область для рендеринга в него
	virtual void FramebufferBind(FramebufferId fbo, math::USVec2 viewportPos, math::USVec2 viewportSize) = 0;

	//! Удалить фреймбуфер
	virtual void FramebufferDelete(FramebufferId fbo) = 0;


	
	virtual VertexAttribStateId VertexAttribStateCreate(const VertexAttribStateDesc& desc) = 0;
	virtual void VertexAttribStateDelete(VertexAttribStateId vas) = 0;


	//! Создать графический конвейер
	virtual GraphicsPipelineId GraphicsPipelineCreate(const GraphicsPipelineDesc& desc) = 0;

	//! Удалить графический конвейер
	virtual void GraphicsPipelineDelete(GraphicsPipelineId gp) = 0;

	virtual DescriptorSetLayoutId DescriptorSetLayoutCreate(const DescriptorSetLayoutDesc& desc) = 0;
	virtual void DescriptorSetLayoutDelete(DescriptorSetLayoutId dsl) = 0;

	virtual DescriptorSetPoolId DescriptorSetPoolCreate(const DescriptorSetLayoutDesc& desc) = 0;
	virtual void DescriptorSetPoolDelete(DescriptorSetPoolId dsp) = 0;

	virtual void SetupStates(const RenderState* states) = 0;
	virtual void TextureBind(TextureId tex, Sampler* samp, uint slot) = 0;
	virtual void SetupTextures(ArrayRange<Texture* const> textures, ArrayRange<Sampler* const> samplers) = 0;
	
	
	struct UniformBinding
	{
		BufferViewId buffer;
		uint index;
		//uint size, offset;
	};

	//! Нарисовать указанный объект указанным шейдером
	virtual void Draw(const DrawCall& dc, ShaderProgramId sh, ArrayRange<const UniformBinding> ubindings) = 0;

	//! Очистить текущий фреймбуфер
	// \param color Цвет очистки
	// \param flags Очистить цвет и\или глубину и\или буфер трафарета
	virtual void Clear(const math::Vec4& color, ClearFlags flags=ClearFlags_Color|ClearFlags_Depth) = 0;

	//! Завершить кадр. Приводит к смене переднего и заднего буферов при двойной буферизации
	virtual void EndFrame() = 0;

	//Удалить все загруженные ресурсы
	virtual void CleanUp() = 0;

	//Проверить, не удалён ли объект данного класса
	virtual bool IsAlive() const = 0;

	//Нарисовать экранный прямоугольник с текстурой
	// \param pos Позиция правого верхнего угла (??? не проверено, так ли это)
	// \param size Размер квада
	// \param leftTopTexCoord Текстурные координаты левого верхнего угла
	// \param rightBottomTexCoord Текстурные координаты правого нижнего угла
	void DrawScreenQuad(math::Vec2 pos, math::Vec2 size, math::Vec2 leftTopTexCoord, math::Vec2 rightBottomTexCoord, Texture* tex);

	//////////////////////////

	struct State
	{
		RenderState render_states;
		Texture* textures[32]={null};
		Sampler* samplers[32]={};
		Framebuffer* framebuffer=null;
		math::USVec2 viewport_pos={0,0}, viewport_size={0,0};
		VertexAttribState* vertex_attrib_state=null;
		uint enabled_attribs=0;
	};

	const State& GetState() const {return current_state;}

	void SetState(const State& state)
	{
		SetupStates(&state.render_states);
		SetupTextures(state.textures, state.samplers);
		FramebufferBind(state.framebuffer, state.viewport_pos, state.viewport_size);
		VertexAttribStateBind(state.vertex_attrib_state, state.enabled_attribs);
	}

	//! Возвращает поставщика GPU
	GpuVendor Vendor() const {return vendor;}

	uintptr BuffersCount() const {return created_buffers.Count();}

	GenericEvent<AGraphics*> OnDestroy;

	//! Возвращает белую текстуру 1x1 в формате Luminance
	Texture* GetWhiteTexture();

protected:
	DeviceCaps caps;
	GraphicsWindow* owner_window;
	GraphicsAPI gapi;
	GpuVendor vendor;


	ShaderProgram* screen_quad_sh;

	//Текущие состояния
	State current_state;

	PodArray<Framebuffer*> created_framebuffers;
	PodArray<Texture*> created_textures;
	Map<SamplerCompactDesc, Sampler*> created_samplers;
	PodArray<ShaderObject*> created_shader_objects;
	PodArray<ShaderProgram*> created_shaders;
	PodArray<VertexAttribState*> created_vertex_attrib_states;
	PodArray<BufferView*> created_buffers;

	Sampler* clamp_sampler;
	mutable Texture* white2D1x1;

private:
	AGraphics& operator=(const AGraphics&) = delete;
	AGraphics(const AGraphics&) = delete;
};

typedef Memory::IntrusiveReference<AGraphics> ContextRef;

}

}
