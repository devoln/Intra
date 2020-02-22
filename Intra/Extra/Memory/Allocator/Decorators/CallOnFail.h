#pragma once

#include "Core/Assert.h"
#include "Core/Range/StringView.h"
#include "Core/Range/Stream/ToString.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
inline void NoMemoryBreakpoint(size_t bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
{
	(void)bytes; (void)sourceInfo;
	INTRA_DEBUGGER_BREAKPOINT;
}

inline void NoMemoryAbort(size_t bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
{
	char errorMsg[512];
	GenericStringView<char>(errorMsg) << StringView(sourceInfo.File).Tail(400) << '(' << sourceInfo.Line << "): " <<
		"Out of memory!\n" << "Couldn't allocate " << bytes << " byte memory block." << '\0';
	INTRA_FATAL_ERROR(errorMsg);
}

template<typename A, void(*F)(size_t, SourceInfo) = NoMemoryBreakpoint> struct ACallOnFail: A
{
	ACallOnFail() = default;
	ACallOnFail(A&& allocator): A(Move(allocator)) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{
		auto result = A::Allocate(bytes, sourceInfo);
		if(result == null) F(bytes, sourceInfo);
		return result;
	}
};
INTRA_END
