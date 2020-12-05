#pragma once

#include "Intra/Assert.h"
#include "Intra/Range/StringView.h"
#include "Intra/Range/Stream/ToString.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED
inline void NoMemoryBreakpoint(size_t bytes, SourceInfo sourceInfo = SourceInfo())
{
	(void)bytes; (void)sourceInfo;
	INTRA_DEBUGGER_BREAKPOINT;
}

inline void NoMemoryAbort(size_t bytes, SourceInfo sourceInfo = SourceInfo())
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

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{
		auto result = A::Allocate(bytes, sourceInfo);
		if(result == null) F(bytes, sourceInfo);
		return result;
	}
};
INTRA_END
