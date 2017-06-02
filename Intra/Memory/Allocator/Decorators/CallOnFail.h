#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Warnings.h"
#include "Utils/Debug.h"
#include "Utils/StringView.h"
#include "Range/Stream/ToString.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Memory {

inline void NoMemoryBreakpoint(size_t bytes, Utils::SourceInfo sourceInfo)
{
	(void)bytes; (void)sourceInfo;
	INTRA_DEBUGGER_BREAKPOINT;
}

inline void NoMemoryAbort(size_t bytes, Utils::SourceInfo sourceInfo)
{
	char errorMsg[512];
	GenericStringView<char>(errorMsg) << StringView(sourceInfo.File).Tail(400) << '(' << sourceInfo.Line << "): " <<
		"Out of memory!\n" << "Couldn't allocate " << bytes << " byte memory block." << '\0';
	INTRA_FATAL_ERROR(errorMsg);
}

template<typename A, void(*F)(size_t, Utils::SourceInfo) = NoMemoryBreakpoint> struct ACallOnFail: A
{
	ACallOnFail() = default;
	ACallOnFail(A&& allocator): A(Cpp::Move(allocator)) {}

	AnyPtr Allocate(size_t& bytes, Utils::SourceInfo sourceInfo)
	{
		auto result = A::Allocate(bytes, sourceInfo);
		if(result==null) F(bytes, sourceInfo);
		return result;
	}
};

}}

INTRA_WARNING_POP
