#pragma once

#include "Math/Vector.h"
#include "WindowSystemApiDeclarations.h"

namespace Intra {

struct OpenGL;

namespace WindowAPI {

WindowHandle WindowCreate(StringView caption, WindowType type, Math::SVec2 pos, Math::USVec2 size, void* wndObj);
AnyPtr WindowGetHandle(WindowHandle wnd);
GLContextHandle WindowCreateGLContext(WindowHandle wnd, uint msaa, bool vsync, bool coreProfile, bool debugContext, OpenGL* caps);
void WindowDeleteGLContext(WindowHandle wnd);
void WindowDelete(WindowHandle wnd);
void WindowSetState(WindowHandle wnd, WindowState state);
WindowState WindowGetState(WindowHandle wnd);
void WindowSetCaption(WindowHandle wnd, StringView caption);
bool WindowIsActive(WindowHandle wnd);
void WindowShowCursor(WindowHandle wnd, bool visible);
void WindowSetPos(WindowHandle wnd, Math::SVec2 newpos);
void WindowSetSize(WindowHandle wnd, Math::USVec2 newsize);
Math::SVec2 WindowGetCursorPos(WindowHandle wnd); //Позиция мыши относительно окна
void WindowSetCursorPos(WindowHandle wnd, Math::SVec2 pos);
void WindowSetSizeLimits(WindowHandle wnd, Math::USVec2 minsize, Math::USVec2 maxsize);
void WindowGLSwapBuffers(WindowHandle wnd);
void WindowGLMakeCurrent(WindowHandle wnd);
Math::USVec2 GetScreenResolution();

extern bool AppIsRunning;

//Определяется снаружи
void OnKeyRelease(void* wnd, Key key);
void OnKeyPress(void* wnd, Key key);
void OnMousePress(void* wnd, MouseKey key, Math::SVec2 pos);
void OnMouseRelease(void* wnd, MouseKey key, Math::SVec2 pos);
void OnMouseMove(void* wnd, Math::SVec2 mousepos);

void OnMove(void* wnd, Math::SVec2 newPos);
void OnResize(void* wnd, Math::USVec2 newSize);
bool OnClose(void* wnd);
void OnDestroy(void* wnd);

}}
