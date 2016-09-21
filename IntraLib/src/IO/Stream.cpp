#include "IO/Stream.h"

#include <stdio.h>

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
#include <conio.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else
#include <termios.h>
#include <unistd.h>
#endif

#ifdef EOF
#undef EOF
#endif

static inline bool isSpace(char c) {return c==' ' || c=='\t' || c=='\r' || c=='\n';}

namespace Intra { namespace IO {

endl_t endl;

IInputStream& operator>>(IInputStream& stream, byte& n)
{
	long64 x = stream.ParseInteger();
	//if(x<byte_MIN || x>byte_MAX) throw InputRangeException(byte_MIN, byte_MAX);
	n = byte(x);
	return stream;
}

IInputStream& operator>>(IInputStream& stream, sbyte& n)
{
	long64 x = stream.ParseInteger();
	n = sbyte(x);
	return stream;
}

IInputStream& operator>>(IInputStream& stream, ushort& n)
{
	long64 x = stream.ParseInteger();
	n = ushort(x);
	return stream;
}

IInputStream& operator>>(IInputStream& stream, short& n)
{
	long64 x = stream.ParseInteger();
	n = short(x);
	return stream;
}

IInputStream& operator>>(IInputStream& stream, uint& n)
{
	long64 x = stream.ParseInteger();
	n = uint(x);
	return stream;
}

IInputStream& operator>>(IInputStream& stream, int& n)
{
	long64 x = stream.ParseInteger();
	n = int(x);
	return stream;
}

IInputStream& operator>>(IInputStream& stream, const char* r)
{
	for(auto p=r; *p!='\0'; p++)
	{
		//Читаем следующий символ, игнорируя все пробелы обеих строк
		char lastChar;
		do lastChar = stream.Read<char>(); while(isSpace(lastChar));
		while(*p!='\0' && isSpace(*p)) p++;

		if(stream.EndOfStream()) return stream;
		if(*p!=lastChar)
		{
			//String error=String::Format("Неверный ввод из потока. Ожидается \"<^>\", но введено \"<^>\".\nlastChar=<^>.")(r)(*p)(lastChar);
			return stream;
		}
	}
	return stream;
}

//Прочитать число. Если это не число, вернёт 0
long64 IInputStream::ParseInteger(bool* error)
{
	long64 result = 0;
	bool wasnumber = false;
	int sign = 1;
	for(;;)
	{
		int c = EndOfStream()? -1: Read<char>();
		if(!wasnumber)
		{
			if(c==-1)
			{
				if(error!=null) *error=true;
				return 0;
			}
			if(c=='-') sign = -sign;
			if(AsciiSet::Spaces.Contains(char(c)) || c=='+' || c=='-') continue;
		}
		if(unsigned(c-'0')<='9')
		{
			result = result*10+(c-'0');
			wasnumber = true;
			continue;
		}
		if(error!=null) *error = !wasnumber;
		if(c!=-1) {byte ch = byte(c); UnreadData(&ch, 1);}
		return result*sign;
	}
}

real IInputStream::ParseFloat(bool* error)
{
	real sign=1, result=0, pos=1;
	bool wasnumber=false, waspoint=false;
	for(;;)
	{
		int c = EndOfStream()? -1: Read<char>();
		if(!wasnumber)
		{
			if(c==-1)
			{
				if(error!=null) *error=true;
				return Math::NaN;
			}
			if(c=='-') sign=-sign;
			if(AsciiSet::Spaces.Contains(char(c)) || c=='+' || c=='-') continue;
		}
		if(c=='.' && !waspoint)
		{
			waspoint = true;
			continue;
		}
		if(unsigned(c-'0')<='9')
		{
			const int digit = c-'0';
			if(!waspoint) result = result*10+digit;
			else pos*=10, result += digit/pos;
			wasnumber = true;
			continue;
		}
		if(c!=-1) {byte ch = byte(c); UnreadData(&ch, 1);}
		if(error!=null) *error = !wasnumber;
		if(!wasnumber) return Math::NaN;
		return result*sign;
	}
}

String IInputStream::ReadToChar(const AsciiSet& stopCharset, char* oStopChar)
{
	String token;
	const AsciiSet stopCharsetAnd0 = stopCharset|'\0';
	for(;;)
	{
		const char readChar = Read<char>();
		if(EndOfStream() || stopCharsetAnd0.Contains(readChar))
		{
			if(oStopChar!=null) *oStopChar = readChar;
			return token;
		}
		token += readChar;
	}
}


void ConsoleStream::WriteData(const void* src, size_t bytes)
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	const auto hndl = HANDLE(_get_osfhandle(_fileno(reinterpret_cast<FILE*>(myfout))));
	wchar_t wbuf[512];
	wchar_t* wsrc = bytes<=core::numof(wbuf)? wbuf: new wchar_t[bytes];
	int wsrcLength = MultiByteToWideChar(CP_UTF8, 0, LPCSTR(src), int(bytes), wsrc, int(bytes));
	DWORD written;
	WriteConsoleW(hndl, wsrc, DWORD(wsrcLength), &written, null);
	if(bytes>core::numof(wbuf)) delete[] wsrc;
#else
	fwrite(src, 1, bytes, reinterpret_cast<FILE*>(myfout));
#endif
}

size_t ConsoleStream::ReadData(void* dst, size_t bytes)
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	char* pbegin = reinterpret_cast<char*>(dst);
	char* pdst = pbegin;
	char* pend = pbegin+bytes;
	byte unreadedBytesToRead = byte(Math::Min<size_t>(unread_buf_chars, bytes));
	core::memcpy(pdst, unread_buf, unreadedBytesToRead);
	pdst += unreadedBytesToRead;
	unread_buf_chars = byte(int(unread_buf_chars)-int(unreadedBytesToRead));
	core::memmove(unread_buf, unread_buf+unreadedBytesToRead, unread_buf_chars);

	const auto hndl = HANDLE(_get_osfhandle(_fileno(reinterpret_cast<FILE*>(myfin))));
	while(pdst<pend)
	{
		DWORD read;
		wchar_t c[2];
		char u8[5];
		ReadConsoleW(hndl, c, 1, &read, null);
		//if(c[0]>) ReadConsoleW(hndl, c+1, 1, &read, null), read++;
		int bytesPerChar = WideCharToMultiByte(CP_UTF8, 0, c, int(read), u8, sizeof(u8), null, null);
		size_t bytesToRead = Math::Min(size_t(bytesPerChar), size_t(pend-pdst));
		core::memcpy(pdst, u8, bytesToRead);
		core::memcpy(unread_buf, u8+bytesToRead, size_t(bytesPerChar)-bytesToRead);
		unread_buf_chars = byte(size_t(bytesPerChar)-bytesToRead);
		pdst+=bytesToRead;
	}
	return bytes;
#else
	return fread(dst, 1, bytes, reinterpret_cast<FILE*>(myfin));
#endif
}

void ConsoleStream::UnreadData(const void* src, size_t bytes)
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	INTRA_ASSERT(bytes+unread_buf_chars<sizeof(unread_buf));
	core::memmove(unread_buf+bytes, unread_buf, unread_buf_chars);
	core::memcpy(unread_buf, src, bytes);
#else
	for(const byte* ptr = reinterpret_cast<const byte*>(src)+bytes; ptr>src;)
		ungetc(*--ptr, reinterpret_cast<FILE*>(myfin));
#endif
}

dchar ConsoleStream::GetChar()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	auto ch = _getwch();
	return ch=='\r'? '\n': dchar(ch);
#else
	termios oldt, newt;
	int ch;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~uint(ICANON|ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getc(reinterpret_cast<FILE*>(myfin)); //TODO: добавить поддержку ввода UTF-8 символа
	if(ch=='\r') ch='\n';
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return dchar(ch);
#endif
}

ConsoleStream::ConsoleStream(void* fout, void* fin):
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	unread_buf_chars(0),
#endif
	myfout(fout), myfin(fin) {}

ConsoleStream Console(stdout, stdin);
ConsoleStream ConsoleError(stderr, stdin);

}}
