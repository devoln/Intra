#include "Core/Core.h"

#if(INTRA_LIBRARY_WINDOW_SYSTEM==INTRA_LIBRARY_WINDOW_SYSTEM_X11)

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#include "GUI/WindowSystemApi.h"
#include "IO/Stream.h"
#include "Containers/Array.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <GL/glx.h>

namespace Intra { namespace WindowAPI {

using namespace Math;

static Display* display=null;
static Cursor empty_cursor;
static Atom atom_fullscreen, atom_state, atom_delete_window, atom_max_horz, atom_max_vert, atom_name, atom_utf8, atom_add, atom_hidden;

struct Window
{
	Window(void* wnd, USVec2 position):
		glcontext(), window(), pos(position),
		is_fullscreen(false), is_cursor_visible(true), is_hidden(true),
		minmax(null),
		wndObj(wnd) {}

	GLXContext glcontext;
	::Window window;

	USVec2 pos;
	bool is_fullscreen, is_cursor_visible, is_hidden;
	XSizeHints* minmax;

	void* wndObj;
};

static void StaticWndProc(WindowHandle impl, XEvent event);

static Array<WindowHandle> windows;

WindowHandle WindowCreate(StringView caption, WindowType type, SVec2 pos, USVec2 size, void* wndObj)
{
	(void)type;

	static Display* const display = XOpenDisplay(null); WindowAPI::display=display;
	static const Atom atom_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", true),
		atom_state = XInternAtom(display, "_NET_WM_STATE", true),
		atom_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", true),
		atom_add = XInternAtom(display, "_NET_WM_STATE_ADD", true),
		atom_max_horz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", false),
		atom_max_vert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", false),
		atom_name = XInternAtom(display, "_NET_WM_NAME", false),
		atom_utf8 = XInternAtom(display, "UTF8_STRING", false),
		atom_hidden = XInternAtom(display, "_NET_WM_STATE_HIDDEN", false);
	WindowAPI::atom_delete_window = atom_delete_window;
	WindowAPI::atom_fullscreen = atom_fullscreen;
	WindowAPI::atom_state = atom_state;
	WindowAPI::atom_max_horz = atom_max_horz;
	WindowAPI::atom_max_vert = atom_max_vert;
	WindowAPI::atom_name = atom_name;
	WindowAPI::atom_utf8 = atom_utf8;
	WindowAPI::atom_add = atom_add;
	WindowAPI::atom_hidden = atom_hidden;


	static const int visual_attribs[]=
	{
		GLX_X_RENDERABLE, true,   GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,   GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_RED_SIZE, 8,   GLX_GREEN_SIZE, 8,   GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8, GLX_DEPTH_SIZE, 24,   GLX_DOUBLEBUFFER, true,
	    GLX_SAMPLE_BUFFERS, 0,     GLX_SAMPLES,        0,
		None
	};

	int fbcount=0;
	GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);

	if(fbc==null)
	{
		//IO::WarnLog.PrintLine("Error: cannot retrieve framebuffer config");
		return null;
	}

	/*int best_fbc=-1, worst_fbc=-1, best_num_samp=-1, worst_num_samp=999;
	for(int i=0; i<fbcount; i++)
	{
		XVisualInfo* vi=glXGetVisualFromFBConfig(display, fbc[i]);
		if(vi==null) continue;
		int samp_buf=0, samples=0;
		glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
		glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLES, &samples);
		if(best_fbc<0 || (samp_buf && samples>best_num_samp)) best_fbc=i, best_num_samp=samples;
		if(worst_fbc<0 || !samp_buf || samples<worst_num_samp) worst_fbc = i, worst_num_samp=samples;
		XFree(vi);
	}*/

	XVisualInfo* vi = glXGetVisualFromFBConfig(display, fbc[0/*best_fbc*/]);

	XSetWindowAttributes swa = 
	{
		None, 0, None, 0, 0, 0, 0, 0, 0, false,
		ExposureMask|VisibilityChangeMask|KeyPressMask|KeyReleaseMask|ButtonPressMask|
			ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|SubstructureNotifyMask|FocusChangeMask,
		0, false, XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone), None
	};

	WindowHandle impl = new Window(wndObj, pos);
	windows.AddLast(impl);
	impl->window = XCreateWindow(display, RootWindow(display, vi->screen), pos.x, pos.y, size.x, size.y,
			0, vi->depth, InputOutput, vi->visual, CWBorderPixel|CWColormap|CWEventMask, &swa);

	if(impl->window==0)
	{
		//ErrorLog << "failed to create window";
		if(fbc!=null) XFree(fbc);
		if(vi!=null) XFree(vi);
		delete impl;
		return null;
	}

	WindowSetCaption(impl, caption);
	XSetWMProtocols(display, impl->window, &WindowAPI::atom_delete_window, 1);

	//Создание невидимого курсора
	XColor black = {0,  0,0,0,  0, 0};
	const char noData=0;
	Pixmap invPixmap = XCreateBitmapFromData(display, impl->window, &noData, 1, 1);
	WindowAPI::empty_cursor = XCreatePixmapCursor(display, invPixmap, invPixmap, &black, &black, 0, 0);

	impl->glcontext = glXCreateContext(display, vi, 0, true);
	glXMakeCurrent(display, impl->window, impl->glcontext);

	typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
	auto glXCreateContextAttribs = reinterpret_cast<glXCreateContextAttribsARBProc>(glXGetProcAddress(
			reinterpret_cast<const byte*>("glXCreateContextAttribsARB")));
	if(false)//if(glXCreateContextAttribs!=null)
	{
		auto version = glGetString(GL_VERSION);
		glXMakeCurrent(display, 0, 0);
		glXDestroyContext(display, impl->glcontext);

		static int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, version[0],
			GLX_CONTEXT_MINOR_VERSION_ARB, version[2], None};
		impl->glcontext = glXCreateContextAttribs(display, fbc[0/*best_fbc*/], 0, true, context_attribs);
		glXMakeCurrent(display, impl->window, impl->glcontext);
	}
	if(fbc!=null) XFree(fbc);
	if(vi!=null) XFree(vi);
	if(impl->glcontext==null) return null;

	//if(!glXIsDirect(display, glcontext)) WarningLog << "created context is not direct!";


	/*if(vsync)
	{
		auto glXSwapInterval=(PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddress((const GLubyte*)"glXSwapIntervalSGI");
		if(glXSwapInterval!=null) glXSwapInterval(1);
		//else WarningLog << "failed to use vertical sync");
	}*/
	return impl;
}

AnyPtr WindowGetHandle(WindowHandle wnd) {return reinterpret_cast<void*>(wnd->window);}

void WindowDelete(WindowHandle wnd)
{
	OnDestroy(wnd->wndObj);
	XDestroyWindow(display, wnd->window);
	if(wnd->minmax) XFree(wnd->minmax);
	windows.FindAndRemoveUnordered(wnd); delete wnd;
}

void WindowSetState(WindowHandle wnd, WindowState state)
{
	if((!wnd->is_fullscreen && state==WindowState::FullScreen) ||
			(wnd->is_fullscreen && state!=WindowState::FullScreen))
	{
		XClientMessageEvent event = {ClientMessage, 0, true, display, wnd->window, atom_state, 32, {}};
		event.data.l[0] = wnd->is_fullscreen = (state==WindowState::FullScreen);
		event.data.l[1] = long(atom_fullscreen);
		event.data.l[2] = 0;
		XSendEvent(display, DefaultRootWindow(display), false,
		SubstructureRedirectMask|SubstructureNotifyMask, (XEvent*)&event);
		if(!wnd->is_fullscreen) WindowSetPos(wnd, wnd->pos);
	}
	const WindowState curstate = WindowGetState(wnd);
	if((curstate==WindowState::Maximized)!=(state==WindowState::Maximized))
	{
		XClientMessageEvent event = {ClientMessage, 0, true, display, wnd->window, atom_state, 32, {}};
		event.data.l[0] = (state==WindowState::Maximized);
		event.data.l[1] = long(atom_max_horz);
		event.data.l[2] = long(atom_max_vert);
		XSendEvent(display, DefaultRootWindow(display), false,
			SubstructureRedirectMask|SubstructureNotifyMask, (XEvent*)&event);
		if(state!=WindowState::Maximized) WindowSetPos(wnd, wnd->pos);
	}
	if(state!=WindowState::Hidden)
	{
		if(state!=WindowState::Minimized)
		{
			XMapWindow(display, wnd->window);
			WindowSetPos(wnd, wnd->pos);
		}
	}
	else XUnmapWindow(display, wnd->window);
	if(state==WindowState::Minimized) XIconifyWindow(display, wnd->window, 0);
	XFlush(display);
	wnd->is_hidden=(state==WindowState::Hidden);
}

WindowState WindowGetState(WindowHandle wnd)
{
	XWindowAttributes wa; XGetWindowAttributes(display, wnd->window, &wa);
	if(wa.map_state==IsUnviewable) return WindowState::Hidden;
	if(wnd->is_fullscreen) return WindowState::FullScreen;

	Atom actual_type;
	int actual_format;
	unsigned long num_items, bytes_after;
	Atom* atoms;
	XGetWindowProperty(display, wnd->window, atom_state, 0, 1024, false, XA_ATOM,
		&actual_type, &actual_format, &num_items, &bytes_after, reinterpret_cast<unsigned char**>(&atoms));

	WindowState state=WindowState::Normal;
	for(ushort i=0; i<num_items; i++)
	{
		if(atoms[i]==atom_hidden)
		{
			state=WindowState::Minimized;
			break;
		}
	    if(atoms[i]==atom_max_horz || atoms[i]==atom_max_vert)
		{
			state=WindowState::Maximized;
			break;
		}
	}
	XFree(atoms);

	return state;
}

void WindowSetCaption(WindowHandle wnd, StringView caption)
{
	XChangeProperty(display, wnd->window, atom_name, atom_utf8,
		8, PropModeReplace, reinterpret_cast<const byte*>(caption.Data()), int(caption.Length()));
}

bool WindowIsActive(WindowHandle wnd)
{
	int unused;
	::Window focused;
	XGetInputFocus(display, &focused, &unused);
	return focused==wnd->window;
}

void WindowShowCursor(WindowHandle wnd, bool visible)
{
	XDefineCursor(display, wnd->window, visible? None: WindowAPI::empty_cursor);
}

void WindowSetPos(WindowHandle wnd, SVec2 newpos)
{
	XMoveWindow(display, wnd->window, newpos.x, newpos.y);
	XFlush(display);
}

void WindowSetSize(WindowHandle wnd, USVec2 newsize)
{
	XResizeWindow(display, wnd->window, newsize.x, newsize.y);
	XFlush(display);
}

SVec2 WindowGetCursorPos(WindowHandle wnd)
{
	::Window root_ret, child_ret;
	int global_x, global_y;
	uint mask; //unused
	int rel_x, rel_y;
	XQueryPointer(display, wnd->window, &root_ret, &child_ret, &global_x, &global_y, &rel_x, &rel_y, &mask);
	return SVec2(short(rel_x), short(rel_y));
}

void WindowSetCursorPos(WindowHandle wnd, SVec2 pos)
{
	XWarpPointer(display, None, wnd->window, 0,0, 10000, 10000, pos.x, pos.y);
}

void WindowSetSizeLimits(WindowHandle wnd, USVec2 minsize, USVec2 maxsize)
{
	if(wnd->minmax==null) wnd->minmax = XAllocSizeHints();
	wnd->minmax->flags = PMinSize|PMaxSize;
	wnd->minmax->min_width = minsize.x;
	wnd->minmax->min_height = minsize.y;
	wnd->minmax->max_width = maxsize.x;
	wnd->minmax->max_height = maxsize.y;
}

void WindowGLSwapBuffers(WindowHandle wnd) {/*glFinish(); */glXSwapBuffers(display, wnd->window);}
void WindowGLMakeCurrent(WindowHandle wnd) {glXMakeCurrent(display, wnd->window, wnd->glcontext);}

USVec2 GetScreenResolution()
{
	Screen* screen = XScreenOfDisplay(display, 0);
	return USVec2(ushort(XWidthOfScreen(screen)), ushort(XHeightOfScreen(screen)));
}

bool AppIsRunning=true;

void AppProcessMessages()
{
	//while(XPending(display))
	while(XEventsQueued(display, QueuedAlready))
	{
		XEvent event; XNextEvent(display, &event);

		//Ищем окно, которому предназначено это событие
		WindowHandle impl=null;
		for(uint i=0; i<windows.Count(); i++)
		{
			if(windows[i]->window!=event.xany.window) continue;
			impl = windows[i];
			break;
		}
		if(impl==null) continue; //Все сообщения чужих окон будут потеряны!

		StaticWndProc(impl, event);
	}

}

}}

#include <X11/XKBlib.h>

namespace Intra { namespace WindowAPI {

static Key convert_from_xkey(KeySym key)
{
	if(key>=XK_BackSpace && key<=XK_Escape) return Key(key & 0xFF);
	if(key>=XK_Home && key<=XK_Down) return Key(byte(Key::Home)+(key-XK_Home));
	if(key>=XK_Page_Up && key<=XK_End) return Key(byte(Key::PageUp)+(key-XK_Page_Up));
	if(key>=XK_KP_Multiply && key<=XK_KP_Divide) return Key(key-0xFF40);
	if(key>=XK_KP_0 && key<=XK_KP_9) return Key(byte(Key::Numpad0)+(key-XK_KP_0));
	if(key>=XK_F1 && key<=XK_F12) return Key(byte(Key::F1)+key-XK_F1);
	if((key>='0' && key<='9') || (key>='A' && key<='Z')) return Key(key);

	switch(key)
	{
	case XK_KP_Space: case ' ': return Key::Space;
	case ';': return Key::Semicolon;
	case '\'': return Key::Quote;
	case ',': return Key::Comma;
	case '[': return Key::QuadBracketLeft;
	case ']': return Key::QuadBracketRight;
	case '=': return Key::Equal;
	case '/': return Key::Slash;
	case '\\': return Key::BackSlash;
	case XK_Delete: return Key::Delete;
	case XK_Menu: return Key::Menu;
	case XK_Print: return Key::PrintScreen;
	case XK_Shift_L: case XK_Shift_R: return Key::Shift;
	case XK_Alt_L: case XK_Alt_R: return Key::Alt;
	case XK_Control_L: case XK_Control_R: return Key::Control;
	}
	key = XkbKeycodeToKeysym(display, XKeysymToKeycode(display, key), 0, 1);
	if((key>='0' && key<='9') || (key>='A' && key<='Z')) return Key(key);
	return Key::Unknown;
}

static void StaticWndProc(WindowHandle impl, XEvent event)
{
	static const Key mouseKeys[]={Key::MouseLButton, Key::MouseMButton, Key::MouseRButton, Key::MouseXButton1, Key::MouseXButton2};
	switch(event.type)
	{
	case ClientMessage:
		if(event.xclient.format==32 && Atom(event.xclient.data.l[0])==atom_delete_window) //Сообщение закрытия окна (кнопка "X")
			if(OnClose(impl->wndObj)) WindowDelete(impl); //Если закрытие разрешено, то закрываем окно
	return;

	case ConfigureNotify:
		if(WindowGetState(impl)==WindowState::Normal)
			impl->pos = SVec2(short(event.xconfigure.x), short(event.xconfigure.y));
		OnMove(impl->wndObj, SVec2(short(event.xconfigure.x), short(event.xconfigure.y)));
		OnResize(impl->wndObj, USVec2(ushort(event.xconfigure.width), ushort(event.xconfigure.height)));
	return;

	case KeyPress:
	{
		KeySym xkey;
		XLookupString(&event.xkey, null, 0, &xkey, null);
		const Key key = convert_from_xkey(xkey);
		OnKeyPress(impl->wndObj, key);
	}
	return;

	case KeyRelease:
	{
		//Определяем, является ли сообщение повторным или клавиша была в самом деле отпущена
		if(XEventsQueued(display, QueuedAfterReading))
		{
			XEvent nev;
			XPeekEvent(display, &nev);
			if(nev.type==KeyPress && nev.xkey.time==event.xkey.time && nev.xkey.keycode==event.xkey.keycode)
			{
				//Игнорируем это сообщение KeyRelease и следующее за ним сообщение KeyPress
				XNextEvent(display, &nev);
				return;
			}
		}

		KeySym xkey;
		XLookupString(&event.xkey, null, 0, &xkey, null);
		const Key key = convert_from_xkey(xkey);
		OnKeyRelease(impl->wndObj, key);
	}
	return;

	case ButtonPress:
		OnKeyPress(impl->wndObj, mouseKeys[event.xbutton.button-1]);
	return;

	case ButtonRelease:
		OnKeyRelease(impl->wndObj, mouseKeys[event.xbutton.button-1]);
	return;

	case MotionNotify:
		OnMouseMove(impl->wndObj, SVec2(short(event.xmotion.x), short(event.xmotion.y)));
	return;
	}
}

}}

#endif
