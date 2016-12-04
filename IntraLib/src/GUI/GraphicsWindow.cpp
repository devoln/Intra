#include "GUI/GraphicsWindow.h"
#include "Range/StringView.h"
#include "GUI/WindowSystemApi.h"

namespace Intra {

using namespace Math;

namespace WindowAPI {

void OnKeyRelease(void* wnd, Key key) {reinterpret_cast<GraphicsWindow*>(wnd)->impl_on_key_release(key);}
void OnKeyPress(void* wnd, Key key) {reinterpret_cast<GraphicsWindow*>(wnd)->impl_on_key_press(key);}

void OnMove(void* wnd, SVec2 newPos)
{
	auto window = reinterpret_cast<GraphicsWindow*>(wnd);
	window->position = newPos;
	window->OnMove();
}
	
void OnResize(void* wnd, USVec2 newSize)
{
	auto window = reinterpret_cast<GraphicsWindow*>(wnd);
	window->size = newSize;
	window->OnResize();
}

bool OnClose(void* wnd) {return reinterpret_cast<GraphicsWindow*>(wnd)->OnClose();}
void OnDestroy(void* wnd) {reinterpret_cast<GraphicsWindow*>(wnd)->impl_on_destroy();}

void OnMouseMove(void* wnd, Math::SVec2 mousepos) {reinterpret_cast<GraphicsWindow*>(wnd)->OnMouseMove(mousepos);}

}


//Создание окна и инициализация в нём OpenGL
GraphicsWindow::GraphicsWindow(StringView caption, WindowType type, SVec2 pos, USVec2 sizes, WindowState state):
	my_graphics(null), is_cursor_visible(true), position(pos), size(sizes)
{
	core::memset(was_pressed_keys, 0, sizeof(was_pressed_keys));
	core::memset(was_released_keys, 0, sizeof(was_released_keys));
	core::memset(pressed_keys, 0, sizeof(pressed_keys));
	hndl = WindowAPI::WindowCreate(caption, type, pos, size, this);
	SetState(state);
}

GraphicsWindow::~GraphicsWindow() {WindowAPI::WindowDelete(hndl);}
void GraphicsWindow::SetCaption(StringView caption) {WindowAPI::WindowSetCaption(hndl, caption);}
void GraphicsWindow::SetState(WindowState state) {WindowAPI::WindowSetState(hndl, state);}
WindowState GraphicsWindow::GetState() const {return WindowAPI::WindowGetState(hndl);}
bool GraphicsWindow::IsActive() const {return WindowAPI::WindowIsActive(hndl);}

void GraphicsWindow::SetCursorVisibility(bool visible)
{
	WindowAPI::WindowShowCursor(hndl, visible);
	is_cursor_visible = visible;
}

void GraphicsWindow::Close() {WindowAPI::WindowDelete(hndl); hndl=null;}
void GraphicsWindow::SetPosition(SVec2 newpos) {WindowAPI::WindowSetPos(hndl, newpos);}
void GraphicsWindow::SetSize(USVec2 newsize) {WindowAPI::WindowSetSize(hndl, newsize);}
SVec2 GraphicsWindow::GetCursorPos() {return WindowAPI::WindowGetCursorPos(hndl);}
void GraphicsWindow::SetCursorPos(SVec2 pos) {WindowAPI::WindowSetCursorPos(hndl, pos);}

void GraphicsWindow::swap_buffers() {WindowAPI::WindowGLSwapBuffers(hndl);}
void GraphicsWindow::make_current() {WindowAPI::WindowGLMakeCurrent(hndl);}

AnyPtr GraphicsWindow::GetNativeHandle() const {return WindowAPI::WindowGetHandle(hndl);}


USVec2 GetScreenResolution() {return WindowAPI::GetScreenResolution();}

}
