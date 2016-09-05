#pragma once

#include "Graphics.h"
#include "Containers/IdAllocator.h"

namespace Intra { namespace Graphics {

class MemorySystem
{
#if INTRA_DISABLED
	typedef CheckedIdAllocator<ushort, ushort, DeviceMemorySystem> ImageViewAllocator;
	typedef ImageViewAllocator::Id ImageViewId;
#endif

public:
	typedef CheckedIdAllocator<ushort, ushort, MemorySystem> MemoryBlockAllocator;
	typedef MemoryBlockAllocator::Id BlockId;
	typedef CheckedIdAllocator<ushort, ushort, MemorySystem> BufferViewAllocator;
	typedef BufferViewAllocator::Id BufferViewId;


	MemorySystem(Graphics::ContextRef gr);


	BlockId AllocateBlock(size_t size, MemoryFlags flags);


	void FreeBlock(BlockId id);


	bool BlockAlive(BlockId block) const;



	BufferViewId CreateBufferView(BlockId block, uintptr startInBlock, uintptr size);


	void DeleteBufferView(BufferViewId id);


	bool BufferViewAlive(BufferViewId id) const;


	BufferViewId AllocateBuffer(MemoryFlags flags, uintptr size);


private:
	Graphics::ContextRef graphics;

	struct BlockEntry
	{
		AGraphics::MemoryBlock* data;
		uintptr size;
		ushort buffer_count;
		MemoryFlags flags;
	};
	Array<BlockEntry> blocks;
	MemoryBlockAllocator block_idalloc;

	struct BufferViewEntry
	{
		BlockId block;
		uintptr startInBlock, size;
		AGraphics::BufferView* hndl;
	};
	Array<BufferViewEntry> buffer_views;
	BufferViewAllocator buffer_idalloc;

#if INTRA_DISABLED
	struct ImageViewEntry
	{
		BlockId block;
		uintptr startInBlock;
		AGraphics::Texture* hndl;
	};
	Array<ImageViewEntry> image_views;
	ImageViewAllocator image_idalloc;
#endif


};

}}
