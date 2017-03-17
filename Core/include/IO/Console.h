#pragma once

#include "Range/Stream.hh"
#include "IO/FormattedWriter.h"

namespace Intra { namespace IO {

class ConsoleInOut: public Range::InputStreamMixin<ConsoleInOut, char>,
	public Range::OutputStreamMixin<ConsoleInOut, char>,
	public APlainTextFormattedWriter
{
public:
	ConsoleInOut();

	char First() const;

	forceinline bool Empty() const {return false;}

	void PopFirst();
	
	dchar GetChar();

	void Put(char c);
	size_t CopyAdvanceFromAdvance(ArrayRange<const char>& src);
	size_t CopyAdvanceToAdvance(ArrayRange<char>& dst);

protected:
	void printRaw(StringView str) override final {Print(str);}

	void pushFont(const FontDesc& fontDesc) override final;
	void popFont() override final;

private:
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	byte mWriteBufCount;
	char mWriteBuf[6];
	mutable byte mReadBufStart, mReadBufCount;
	char mReadBuf[6];
	void* mOutputHandle;
	void* mInputHandle;
#else
	char mFirst;
	byte mFirstInited;
#endif

	ConsoleInOut(const ConsoleInOut&) = delete;
	ConsoleInOut& operator=(const ConsoleInOut&) = delete;
};

extern ConsoleInOut Console;

}}
