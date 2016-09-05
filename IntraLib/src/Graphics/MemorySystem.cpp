#include "Graphics/MemorySystem.h"

namespace Graphics
{

MemorySystem::MemorySystem(Graphics::ContextRef gr):
	graphics(gr), block_idalloc(65535), buffer_idalloc(65535)
#if DISABLED
	, image_idalloc(65535)
#endif
    {}

MemorySystem::BlockId MemorySystem::AllocateBlock(uintptr size, MemoryFlags flags)
{
	BlockId result = block_idalloc.Allocate();
	if(result.value>=blocks.Count())
		blocks.SetCountUninitialized(blocks.Count());
	auto& b = blocks[result.value];
	b.data = graphics->MemoryAllocate(size, flags);
	b.size = size;
	b.buffer_count = 0;
	b.flags = flags;
	return result;
}

void MemorySystem::FreeBlock(BlockId id)
{
	if(!AssertWarning(BlockAlive(id))) return;
	auto& b = blocks[id.value];
	graphics->MemoryFree(b.data);
	block_idalloc.Deallocate(id);
}

bool MemorySystem::BlockAlive(BlockId block) const
{
	return block_idalloc.IsId(block);
}


MemorySystem::BufferViewId MemorySystem::CreateBufferView(BlockId block, uintptr startInBlock, uintptr size)
{
	if(!AssertWarning(BlockAlive(block))) return null;
	BufferViewId result = buffer_idalloc.Allocate();
	if(result.value>=buffer_views.Count())
		buffer_views.SetCountUninitialized(buffer_views.Count());
	auto& b = buffer_views[result.value];
	b.hndl = graphics->BufferViewCreate(blocks[block.value].data, startInBlock, size);
	b.startInBlock = startInBlock;
	b.size = size;
	return result;
}

void MemorySystem::DeleteBufferView(BufferViewId id)
{
	if(!AssertWarning(BufferViewAlive(id))) return;
	auto& b = buffer_views[id.value];
	graphics->BufferViewDelete(b.hndl);
	block_idalloc.Deallocate(id);
}

bool MemorySystem::BufferViewAlive(BufferViewId id) const
{
	return buffer_idalloc.IsId(id);
}

MemorySystem::BufferViewId MemorySystem::AllocateBuffer(MemoryFlags flags, uintptr size)
{

}

}

