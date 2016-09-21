#include "Core/Core.h"

#if(INTRA_LIBRARY_WINDOW_SYSTEM==INTRA_LIBRARY_WINDOW_SYSTEM_Windows)


#include "GUI/WindowSystemApi.h"
#include "Graphics/OpenGL/GLExtensions.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4191)
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <windows.h>
#include <GL/GL.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace Intra { namespace WindowAPI {

using namespace Math;

static LRESULT WINAPI StaticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam); //Определена ниже

struct Window
{
	HWND hwnd;
	uint id;
	HDC dc;
	HGLRC gl;

	uint winstyle;
	SVec2 preFSpos;
	USVec2 preFSsizes;
	HCURSOR prev_cursor; //Хендл курсора, который был перед отключением, и null, если курсор включён
	bool is_fullscreen;

	USVec2 min_size, max_size;

	void* wndObj;
};

static HCURSOR empty_cursor=null;

struct GLInit
{
	typedef HGLRC(GLCALL *WGLCCA)(HDC, HGLRC, const int*);
	typedef BOOL(GLCALL *WGLCPF)(HDC, const int*, const float*, uint, int*, uint*);

	GLInit();
	~GLInit();
	ushort version, MaxSamples;
	WGLCCA wglCreateContextAttribsARB;
	WGLCPF wglChoosePixelFormatARB;

	const wchar_t* cname=L"OpenGL_Initializer";
	HDC dc;
	HWND hwnd;
	HGLRC gl;
};


GLInit::GLInit()
{
	version=0; MaxSamples=0;
	wglCreateContextAttribsARB=null; wglChoosePixelFormatARB=null;

	WNDCLASSW wc = {
		CS_HREDRAW|CS_VREDRAW, DefWindowProcW, 0, 0,
		GetModuleHandle(null), LoadIcon(null, IDI_APPLICATION),
		LoadCursor(null, IDC_ARROW), null, 0, cname
	};
	RegisterClassW(&wc);
	hwnd = CreateWindowExW(0, cname, L"",
		WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_OVERLAPPEDWINDOW,
		0, 0, 1, 1,
		null, null, wc.hInstance, null);
	dc = GetDC(hwnd);
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL,
		PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0
	};
	SetPixelFormat(dc, ChoosePixelFormat(dc, &pfd), &pfd);
	gl = wglCreateContext(dc);
	wglMakeCurrent(dc, gl);

	bool gles;
	version = GetGLVersion(&gles);

	wglCreateContextAttribsARB = reinterpret_cast<WGLCCA>(wglGetProcAddress("wglCreateContextAttribsARB"));
	wglChoosePixelFormatARB = reinterpret_cast<WGLCPF>(wglGetProcAddress("wglChoosePixelFormatARB"));
}

GLInit::~GLInit()
{
	if(gl!=null) wglDeleteContext(gl);
	if(dc!=null) ReleaseDC(hwnd, dc);
	if(hwnd!=null) DestroyWindow(hwnd);
	UnregisterClassW(cname, GetModuleHandle(null));
}

extern bool gl_version3;
extern bool no_glext;

WindowHandle WindowCreate(StringView caption, WindowType type, SVec2 pos, USVec2 size, void* wndObj)
{
	static uint windowsCreated=0;
	const String className = "P_Window"+ToString(windowsCreated);

	wchar_t wclassName[20];
	int wclassNameLength = MultiByteToWideChar(CP_UTF8, 0, className.Data(), int(className.Length()), wclassName, int(core::numof(wclassName)));
	wclassName[size_t(wclassNameLength)] = L'\0';
	WString wcaption;
	wcaption.Reserve(caption.Length()+1);
	int wcaptionLength = MultiByteToWideChar(CP_UTF8, 0, caption.Data(),
			int(caption.Length()), reinterpret_cast<LPWSTR>(wcaption.Data()), int(caption.Length()));
	wcaption.SetLengthUninitialized(size_t(wcaptionLength));

    WNDCLASSW wc = {
		CS_HREDRAW|CS_VREDRAW, StaticWndProc, 0, 0,
		GetModuleHandle(null), LoadIcon(null, IDI_APPLICATION),
		LoadCursor(null, IDC_ARROW), null, 0, wclassName
	};
	RegisterClassW(&wc);

	WindowHandle impl = new Window;
	impl->min_size = USVec2(0, 0);
	impl->max_size = USVec2(65535, 65535);
	impl->is_fullscreen = false;
	impl->wndObj = wndObj;
	impl->id = windowsCreated++;

	impl->winstyle = WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_OVERLAPPEDWINDOW;
	if(type & WindowType_Popup)
	{
		impl->winstyle &= ~uint(WS_OVERLAPPEDWINDOW);
		impl->winstyle|=WS_POPUP;
	}
	if(type & WindowType_NoMaximize)
		impl->winstyle &= ~uint(WS_MAXIMIZEBOX);
	if(type & WindowType_NoMinimize)
		impl->winstyle &= ~uint(WS_MINIMIZEBOX);
	if(type & WindowType_NoResize)
		impl->winstyle &= ~uint(WS_THICKFRAME);
	if(type & WindowType_NoCaption)
		impl->winstyle &= ~uint(WS_CAPTION);

	impl->hwnd=CreateWindowExW(0, wclassName, reinterpret_cast<LPCWSTR>(wcaption.CStr()),
		impl->winstyle, pos.x, pos.y, size.x, size.y, null, null, wc.hInstance, impl);

	impl->prev_cursor=null;
	const int andMask=-1, xorMask=0;
	static const HCURSOR emptyCursor = CreateCursor(null, 0, 0, 1, 1, &andMask, &xorMask);
	Intra::WindowAPI::empty_cursor = emptyCursor;

	impl->dc = GetDC(impl->hwnd);
	impl->gl=null;

	return impl;
}

AnyPtr WindowGetHandle(WindowHandle wnd) {return wnd->hwnd;}

GLContextHandle WindowCreateGLContext(WindowHandle wnd, uint msaaSamples, bool vsync, bool coreProfile, bool debugContext, OpenGL* outGL)
{
	static GLInit glInit;

	//Установка пиксельного формата
	const PIXELFORMATDESCRIPTOR pfd={sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0};
	int format=0;
	if(glInit.wglChoosePixelFormatARB==null) format=ChoosePixelFormat(wnd->dc, &pfd);
	else
	{
		int attribList[]={
			WGL_DRAW_TO_WINDOW_ARB, true, WGL_SUPPORT_OPENGL_ARB, true, WGL_DOUBLE_BUFFER_ARB, true,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,  WGL_COLOR_BITS_ARB, 32,  WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			(msaaSamples>1? WGL_SAMPLE_BUFFERS_ARB: 0 /*end*/), 1,
			WGL_SAMPLES_ARB, int(msaaSamples),
			0
		};

		uint numFormats;
		for(int i = int(msaaSamples); i>=0; i--)
		{
			attribList[17]=i;
			if(i<=1) attribList[14]=0;
			glInit.wglChoosePixelFormatARB(wnd->dc, attribList, null, 1, &format, &numFormats);
			if(format!=0) break;
		}
	}
	if(format==0) return null;
	SetPixelFormat(wnd->dc, format, &pfd);

	ushort glver = glInit.version; //На Mesa такой трюк не работает: она поддерживает compatibility (у контекста по умолчанию) только до версии 3.0.
	if(glver<30) coreProfile=false;
	if(gl_version3) {coreProfile=false; glver=30;}

	//Создание контекста OpenGL
	if(glInit.wglCreateContextAttribsARB==null)
	{
		wnd->gl = wglCreateContext(wnd->dc);
		coreProfile = false;
		debugContext = false;
	}
	else
	{
		int attribs[]={WGL_CONTEXT_MAJOR_VERSION, glver/10, WGL_CONTEXT_MINOR_VERSION, glver%10,
			WGL_CONTEXT_PROFILE_MASK, coreProfile? WGL_CONTEXT_CORE_PROFILE_BIT: WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT,
			WGL_CONTEXT_FLAGS, (debugContext? WGL_CONTEXT_DEBUG_BIT: 0)|/*WGL_CONTEXT_FORWARD_COMPATIBLE_BIT*/0, 0
		};

		wnd->gl = glInit.wglCreateContextAttribsARB(wnd->dc, 0, attribs);
	}
	wglMakeCurrent(wnd->dc, wnd->gl);

	auto& gl = *outGL;
	InitExtensions(glver, no_glext, gl);
	gl.IsCoreContext=coreProfile;
	gl.IsDebugContext=debugContext;

	if(glver<33) gl.GLSLVersion = ushort( 10*( (glver>=20)*11+(glver>=21)+(glver>=30)+(glver>=31)+(glver>=32) ) );
	else gl.GLSLVersion = ushort(glver*10);

	if(gl.SwapInterval!=null) gl.SwapInterval(vsync);
	if(gl.Caps.multisampling && msaaSamples>1) glEnable(OpenGL::MULTISAMPLE);

	return reinterpret_cast<GLContextHandle>(wnd->gl);
}

void WindowDeleteGLContext(WindowHandle wnd)
{
	if(wnd==null || wnd->gl==null) return;
	wglMakeCurrent(wnd->dc, null);
	wglDeleteContext(wnd->gl);
}

void WindowDelete(WindowHandle wnd)
{
	if(wnd==null) return;
	WindowDeleteGLContext(wnd);
	if(wnd->dc!=null) ReleaseDC(wnd->hwnd, wnd->dc);

	if(wnd->hwnd!=null) DestroyWindow(wnd->hwnd);
	const String className = "P_Window"+ToString(wnd->id);
	wchar_t wclassName[20];
	int wclassNameLength = MultiByteToWideChar(CP_UTF8, 0, className.Data(), int(className.Length()), wclassName, int(core::numof(wclassName)));
	wclassName[wclassNameLength]=L'\0';
	UnregisterClassW(wclassName, GetModuleHandleW(null));
	delete wnd;
}

void WindowSetState(WindowHandle wnd, WindowState state)
{
	INTRA_ASSERT(wnd!=null);
	if(state==WindowState::FullScreen && !wnd->is_fullscreen)	//Из оконного в полноэкранный
	{
		//Отключим WS_OVERLAPPEDWINDOW и добавим WS_POPUP
		SetWindowLong(wnd->hwnd, GWL_STYLE, (LONG(wnd->winstyle) & ~LONG(WS_OVERLAPPEDWINDOW))|LONG(WS_POPUP));
		RECT rect; GetWindowRect(wnd->hwnd, &rect);
		wnd->preFSpos = SVec2(short(rect.left), short(rect.top));
		wnd->preFSsizes = USVec2(ushort(rect.right-rect.left), ushort(rect.bottom-rect.top)); //Сохраняем размеры для возврата из полноэкранного режима
		
		//Увеличим окно во весь экран
		WindowSetPos(wnd, SVec2(0,0));
		WindowSetSize(wnd, GetScreenResolution());
		wnd->is_fullscreen=true;
	}
	else if(state!=WindowState::FullScreen && wnd->is_fullscreen)	//Из полноэкранного в оконный
	{
		//Восстановим размеры окна, которые были до перехода в полноэкранный режим
		WindowSetPos(wnd, wnd->preFSpos);
		WindowSetSize(wnd, wnd->preFSsizes);

		SetWindowLong(wnd->hwnd, GWL_STYLE, LONG(wnd->winstyle)); //Вернём стиль окна, с которым оно создавалось
		wnd->is_fullscreen=false;
	}

	static const int winapi_states[] = {SW_MAXIMIZE, SW_MINIMIZE, SW_SHOW, SW_SHOW, SW_HIDE};
	ShowWindow(wnd->hwnd, winapi_states[byte(state)]);
}

WindowState WindowGetState(WindowHandle wnd)
{
	INTRA_ASSERT(wnd!=null);
	if(wnd->is_fullscreen) return WindowState::FullScreen;
	if(IsZoomed(wnd->hwnd)) return WindowState::Maximized;
	if(IsIconic(wnd->hwnd)) return WindowState::Minimized;
	if(!IsWindowVisible(wnd->hwnd)) return WindowState::Hidden;
	return WindowState::Normal;
}

void WindowSetCaption(WindowHandle wnd, StringView caption)
{
	WString wcaption;
	wcaption.Reserve(caption.Length()+1);
	MultiByteToWideChar(CP_UTF8, 0, caption.Data(), int(caption.Length()),
		reinterpret_cast<LPWSTR>(wcaption.Data()), int(wcaption.Capacity()));

	SetWindowTextW(wnd->hwnd, reinterpret_cast<LPCWSTR>(wcaption.CStr()));
}

bool WindowIsActive(WindowHandle wnd) {return GetActiveWindow()==wnd->hwnd;}

void WindowShowCursor(WindowHandle wnd, bool visible)
{
	if(!visible) wnd->prev_cursor = reinterpret_cast<HCURSOR>(SetClassLongPtrW(wnd->hwnd,
			GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(empty_cursor)));
	else SetClassLongPtrW(wnd->hwnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(wnd->prev_cursor));
}

void WindowSetPos(WindowHandle wnd, SVec2 newpos)
{
	RECT rect; GetWindowRect(wnd->hwnd, &rect);
	MoveWindow(wnd->hwnd, newpos.x, newpos.y, rect.right-rect.left, rect.bottom-rect.top, true);
}

void WindowSetSize(WindowHandle wnd, USVec2 newsize)
{
	RECT rect; GetWindowRect(wnd->hwnd, &rect);
	MoveWindow(wnd->hwnd, rect.left, rect.top, newsize.x, newsize.y, true);
}

SVec2 WindowGetCursorPos(WindowHandle wnd)
{
	POINT pos; GetCursorPos(&pos);
	ScreenToClient(wnd->hwnd, &pos);
	return SVec2(pos.x, pos.y);
}

void WindowSetCursorPos(WindowHandle wnd, SVec2 pos)
{
	POINT scrpos = {pos.x, pos.y};
	ClientToScreen(wnd->hwnd, &scrpos);
	SetCursorPos(scrpos.x, scrpos.y);
}

void WindowSetSizeLimits(WindowHandle wnd, USVec2 minsize, USVec2 maxsize)
{
	wnd->min_size = minsize;
	wnd->max_size = maxsize;
}
void WindowGLSwapBuffers(WindowHandle wnd) {SwapBuffers(wnd->dc);}
void WindowGLMakeCurrent(WindowHandle wnd) {wglMakeCurrent(wnd->dc, wnd->gl);}

USVec2 WindowGetScreenResolution()
{
	return USVec2(
		ushort(GetSystemMetrics(SM_CXSCREEN)),
		ushort(GetSystemMetrics(SM_CYSCREEN))
	);
}

bool AppIsRunning = true;
void AppProcessMessages()
{
	for(;;)
	{
		MSG msg;
		if(!PeekMessageW(&msg, null, 0, 0, PM_REMOVE)) return;
		if(msg.message==WM_QUIT) {AppIsRunning=false; return;}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

//Оконная процедура для всех окон
static LRESULT WINAPI StaticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg==WM_NCCREATE)
	{
		WindowHandle wnd = reinterpret_cast<WindowHandle>(
				reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
		INTRA_ASSERT(wnd!=null);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<intptr>(wnd));
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	WindowHandle wnd = reinterpret_cast<WindowHandle>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	if(wnd==null) return DefWindowProcW(hWnd, uMsg, wParam, lParam);

	switch(uMsg)
	{
		case WM_KEYUP:
			OnKeyRelease(wnd->wndObj, Key(wParam));
			return 0;

		case WM_KEYDOWN:
			if((lParam&(1<<30))==0) OnKeyPress(wnd->wndObj, Key(wParam)); //Не обрабатываем повторные сообщения после удержания клавиши
			return 0;

		case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
			OnKeyPress(wnd->wndObj, uMsg==WM_LBUTTONDOWN? Key::MouseLButton: uMsg==WM_MBUTTONDOWN? Key::MouseMButton: Key::MouseRButton);
			return 0;

		case WM_CLOSE:
			if(OnClose(wnd->wndObj)) break; //Если закрытие подтверждено, действуем по умолчанию (закрываем окно)
			return 0;

		case WM_DESTROY:
			OnDestroy(wnd->wndObj);
			return 0;

		case WM_GETMINMAXINFO:
		{
			MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
			info->ptMinTrackSize = {wnd->min_size.x, wnd->min_size.y};
			info->ptMaxTrackSize = {wnd->max_size.x, wnd->max_size.y};
		}
			return 0;

		case WM_MOVE: case WM_SIZE:
		{
			RECT rect; GetClientRect(hWnd, &rect);
			if(uMsg==WM_MOVE) OnMove(wnd->wndObj, SVec2(ushort(rect.left), ushort(rect.top)));
			else OnResize(wnd->wndObj, USVec2(ushort(rect.right-rect.left), ushort(rect.bottom-rect.top) ));
		}
		return 0;

		default: return 0;
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

}}

#endif
