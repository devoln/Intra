#pragma once

#include "Core/Range/Span.h"
#include "Core/Range/StringView.h"
#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Stream/ToString.h"

#include "Allocator/System.h"
#include "Allocator/Global.h"
#include "Allocator/Decorators.hh"
#include "Allocator/Basic.hh"
#include "Allocator/Compositors.hh"

#define INTRA_NEW(type, allocator) new((allocator).Allocate(sizeof(type), INTRA_SOURCE_INFO)) type
#define INTRA_DELETE(ptr, allocator) (::Intra::DestructObj(*ptr), allocator.Free(ptr))

#define INTRA_NEW_ARRAY(type, n, allocator) ::Intra::AllocateRange<type>((allocator), (n), INTRA_SOURCE_INFO)
#define INTRA_DELETE_ARRAY(range, allocator) ::Intra::FreeRange((allocator), (range))

