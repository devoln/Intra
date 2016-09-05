#pragma once

#include "Core/Core.h"

namespace Intra {

typedef flag8 WindowType;
const WindowType WindowType_NoMinimize=1, WindowType_NoMaximize=2,
	WindowType_NoCaption=4, WindowType_NoResize=8, WindowType_Popup=16;

enum class WindowState: byte {Maximized, Minimized, FullScreen, Normal, Hidden};

typedef void(*ApplicationRunCallback)();

enum class Key: byte //Совпадают с VK_* кодами WinAPI
{
	Unknown,
	MouseLButton=0x01, MouseRButton, MouseMButton=0x04, MouseXButton1, MouseXButton2,

	Backspace=0x08, Tab, Enter=0x0D, MainEnter, NumpadEnter,
	Shift=0x10, Control, Alt, Pause, CapsLock, Escape=0x1B,
	Space=0x20, PageUp, PageDown, End, Home,  Left, Up, Right, Down,
	PrintScreen=0x2C, Insert, Delete, Menu=0x5D,

	Numpad0=0x60, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
	Star, Plus, Minus=0x6D, Point, Divide,

	F1=0x70, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,


	//TODO: Следующим константам присвоить правильные коды WinAPI:
	Comma, Semicolon, Equal, Slash, BackSlash,
	QuadBracketLeft, QuadBracketRight, Quote
};

namespace WindowAPI {

struct Window;
struct GLContext;
typedef Window* WindowHandle;
typedef GLContext* GLContextHandle;

}

enum class MouseKey: byte {Left=1, Middle, Right, WheelUp, WheelDown}; //Совпадают с XEvent::button.xbutton из X11

}


