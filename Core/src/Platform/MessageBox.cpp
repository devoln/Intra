#include "Platform/PlatformInfo.h"
#include "Platform/MessageBox.h"
#include "Range/Generators/StringView.h"


#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

#ifdef _MSC_VER
#pragma warning(disable: 4668)
INTRA_DISABLE_REDUNDANT_WARNINGS
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#endif

namespace Intra {

void ShowMessageBox(StringView message, StringView caption, MessageIcon icon)
{
	const ushort iconTable[] = {0, MB_ICONINFORMATION, MB_ICONQUESTION, MB_ICONERROR, MB_ICONWARNING};
	wchar_t* buffer = new wchar_t[message.Length()+1+caption.Length()+1];
	LPWSTR wmessage = buffer;
	LPWSTR wcaption = buffer+message.Length()+1;
	int wmessageLength = MultiByteToWideChar(CP_UTF8, 0, message.Data(), int(message.Length()), wmessage, int(message.Length()));
	int wcaptionLength = MultiByteToWideChar(CP_UTF8, 0, caption.Data(), int(caption.Length()), wcaption, int(caption.Length()));
	wmessage[wmessageLength] = L'\0';
	wcaption[wcaptionLength] = L'\0';
	MessageBoxW(null, wmessage, wcaption, iconTable[byte(icon)]);
	delete[] buffer;
}

}

#elif(INTRA_LIBRARY_WINDOW_SYSTEM==INTRA_LIBRARY_WINDOW_SYSTEM_Qt)

#include "Range/StringView.h"
#include "Containers/String.h"
#include <QtGui/QMessageBox>

namespace Intra {

void ShowMessageBox(StringView message, StringView caption, MessageIcon icon)
{
	static const QMessageBox::Icon qticons[] = {
		QMessageBox::NoIcon, QMessageBox::Information,
		QMessageBox::Question, QMessageBox::Critical, QMessageBox::Warning
	};
	QMessageBox box(qticons[(ushort)icon], String(caption).CStr(), String(message).CStr(), QMessageBox::Ok, (QWidget*)parentWnd.ptr);
	box.exec();
}

}

#else

#include "IO/Stream.h"

namespace Intra {

void ShowMessageBox(StringView message, StringView caption, MessageIcon icon)
{
	IO::Console << IO::endl << "  --- " <<
		(icon==MessageIcon::Error? "Error! ": icon==MessageIcon::Warning? "Warning! ": "") <<
		caption << " ---" << IO::endl << message << IO::endl;
}

}

#endif
