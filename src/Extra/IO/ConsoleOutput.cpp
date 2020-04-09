#include "Extra/IO/ConsoleOutput.h"

#include "Intra/Range/Special/Unicode.h"

INTRA_PUSH_DISABLE_ALL_WARNINGS

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif
#include <io.h>
#include <Windows.h>

#else
#include <unistd.h>
#endif
INTRA_WARNING_POP

INTRA_BEGIN
struct ConsoleOutStream
{
	ConsoleOutStream()
#ifdef _WIN32
		: mOutputHandle(GetStdHandle(STD_OUTPUT_HANDLE))
#endif
	{}

	INTRA_FORCEINLINE bool Empty() const {return false;}

	void Put(char c)
	{
#ifdef _WIN32
		mWriteBuf[mWriteBufCount++] = c;
		const auto charLen = 1 + Unicode::ValidContinuationBytes(unsigned(mWriteBuf[0]));
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

	index_t ReadFromAdvance(CSpan<char>& src)
	{
		if(src.Empty()) return 0;
#ifdef _WIN32
		const index_t totalBytesToWrite = src.Length();
		while(mWriteBufCount != 0)
		{
			Put(src.First());
			src.PopFirst();
			if(src.Empty()) return totalBytesToWrite;
		}

		const int wlen = MultiByteToWideChar(CP_UTF8, 0, src.Data(), int(src.Length()), null, 0);
		wchar_t wbuf[512];
		GenericString<wchar_t> wsrc;
		if(wlen > 512) wsrc.SetCountUninitialized(wlen);
		auto wsrcPtr = wlen>512? wsrc.Data(): wbuf;
		int wsrcLength = MultiByteToWideChar(CP_UTF8, 0, src.Data(), int(src.Length()), wsrcPtr, wlen);
		DWORD written;
		WriteConsoleW(HANDLE(mOutputHandle), wsrcPtr, DWORD(wsrcLength), &written, null);
		src.Begin = src.End;

		return totalBytesToWrite;
#else
		index_t n = index_t(write(STDOUT_FILENO, src.Data(), src.Length()));
		src.Begin += n;
		return n;
#endif
	}

private:
#ifdef _WIN32
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

	void PushFont(IOutputStream& s, const FontDesc& newFont, const FontDesc& curFont) override
	{
#ifdef _WIN32
		(void)s;
		if(curFont.Color != newFont.Color ||
			curFont.Underline != newFont.Underline)
		{
			uint16 consoleCode = 0;
			if(newFont.Color != Vec3(-1))
			{
				const float maxColorChannel = FMax(FMax(newFont.Color.x, newFont.Color.y), newFont.Color.z);
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
			if(newFont.Color != Vec3(-1))
			{
				const float maxColorChannel = FMax(FMax(newFont.Color.x, newFont.Color.y), newFont.Color.z);
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

	void PopFont(IOutputStream& s, const FontDesc& curFont, const FontDesc& prevFont) override
	{PushFont(s, prevFont, curFont);}
};

FormattedWriter ConsoleOutput() {return FormattedWriter(ConsoleOutStream(), new ConsoleFormatter);}

INTRA_IGNORE_WARNING_GLOBAL_CONSTRUCTION
FormattedWriter ConsoleOut = ConsoleOutput();

INTRA_END
