#include "IO/ConsoleOutput.h"
#include "Range/Special/Unicode.h"
#include "Cpp/Warnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif

#include <io.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <Windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else
#include <unistd.h>
#endif

namespace Intra { namespace IO {

struct ConsoleOutStream
{
	ConsoleOutStream()
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
		: mOutputHandle(GetStdHandle(STD_OUTPUT_HANDLE))
#endif
	{}

	forceinline bool Empty() const {return false;}

	void Put(char c)
	{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
		mWriteBuf[mWriteBufCount++] = c;
		size_t charLen = UTF8::SequenceBytes(mWriteBuf[0]);
		if(mWriteBufCount < charLen) return;

		wchar_t wbuf[2];
		int wsrcLength = MultiByteToWideChar(CP_UTF8, 0, mWriteBuf, int(charLen), wbuf, 2);
		DWORD written;
		WriteConsoleW(HANDLE(mOutputHandle), wbuf, DWORD(wsrcLength), &written, null);
		mWriteBufCount = 0;
#else
		const auto charsWritten = write(STDOUT_FILENO, &c, 1);
		(void)charsWritten;
#endif
	}

	size_t CopyAdvanceFromAdvance(CSpan<char>& src)
	{
		if(src.Empty()) return 0;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
		const size_t totalBytesToWrite = src.Length();
		while(mWriteBufCount != 0)
		{
			Put(src.First());
			src.PopFirst();
			if(src.Empty()) return totalBytesToWrite;
		}

		const int wlen = MultiByteToWideChar(CP_UTF8, 0, src.Data(), int(src.Length()), null, 0);
		wchar_t wbuf[512];
		GenericString<wchar_t> wsrc;
		if(wlen>512) wsrc.SetCountUninitialized(size_t(wlen));
		auto wsrcPtr = wlen>512? wsrc.Data(): wbuf;
		int wsrcLength = MultiByteToWideChar(CP_UTF8, 0, src.Data(), int(src.Length()), wsrcPtr, wlen);
		DWORD written;
		WriteConsoleW(HANDLE(mOutputHandle), wsrcPtr, DWORD(wsrcLength), &written, null);
		src.Begin = src.End;

		return totalBytesToWrite;
#else
		size_t n = size_t(write(STDOUT_FILENO, src.Data(), src.Length()));
		src.Begin += n;
		return n;
#endif
	}

private:
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	byte mWriteBufCount = 0;
	char mWriteBuf[6];
	void* mOutputHandle;
#endif
};

class ConsoleFormatter final: public BasicFormatter
{
public:
	using BasicFormatter::PushFont;
	using BasicFormatter::PopFont;

	void PushFont(IOutputStream<char>& s, const FontDesc& newFont, const FontDesc& curFont) override
	{
#if INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows
		(void)s;
		if(curFont.Color != newFont.Color ||
			curFont.Underline != newFont.Underline)
		{
			ushort consoleCode = 0;
			if(newFont.Color!=Math::Vec3(-1))
			{
				const float maxColorChannel = Op::Max(Op::Max(newFont.Color.x, newFont.Color.y), newFont.Color.z);
				if(maxColorChannel >= 0.25f)
				{
					if(newFont.Color.z >= maxColorChannel / 2) consoleCode |= FOREGROUND_BLUE;
					if(newFont.Color.y >= maxColorChannel / 2) consoleCode |= FOREGROUND_GREEN;
					if(newFont.Color.x >= maxColorChannel / 2) consoleCode |= FOREGROUND_RED;
					if(maxColorChannel >= 0.5f) consoleCode |= FOREGROUND_INTENSITY;
				}
			}
			else consoleCode |= FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED;
			if(newFont.Underline) consoleCode |= COMMON_LVB_UNDERSCORE;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), consoleCode);
		}
#else
		if(curFont.Color != newFont.Color ||
			curFont.Bold != newFont.Bold ||
			curFont.Underline != newFont.Underline)
		{
			s << "\x1B[0m";
			if(newFont.Color != Math::Vec3(-1))
			{
				const float maxColorChannel = Op::Max(Op::Max(newFont.Color.x, newFont.Color.y), newFont.Color.z);
				int code = 0;
				int colorCode = 30;
				if(maxColorChannel >= 0.25f)
				{
					if(newFont.Color.x >= maxColorChannel / 2) colorCode += 1;
					if(newFont.Color.y >= maxColorChannel / 2) colorCode += 2;
					if(newFont.Color.z >= maxColorChannel / 2) colorCode += 4;
				}
				if(maxColorChannel < 0.5f) code = 2;
				s << "\x1B[" << code << ';' << colorCode << 'm';
			}
			if(newFont.Bold) s <<"\x1B[1m";
			if(newFont.Underline) s << "\x1B[4m";
		}
#endif
	}

	void PopFont(IOutputStream<char>& s, const FontDesc& curFont, const FontDesc& prevFont) override
	{PushFont(s, prevFont, curFont);}
};

FormattedWriter ConsoleOutput() {return FormattedWriter(ConsoleOutStream(), new ConsoleFormatter);}
FormattedWriter ConsoleOut = ConsoleOutput();

}}

INTRA_WARNING_POP
