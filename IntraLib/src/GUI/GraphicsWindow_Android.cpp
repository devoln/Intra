#include "Core/Core.h"

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)

#error "Android platform support is not ready!"

#include "GUI/WindowSystemApi.h"
#include "Graphics/GLExtensions.h"
#include "IO/LogSystem.h"

#include <stdio.h>


#ifdef _MSC_VER
#pragma comment(lib, "libEGL.lib")
#pragma comment(lib, "libGLESv2.lib")
#pragma comment(lib, "libMaliEmulator.lib")
#endif

#include <EGL/egl.h>
#include <jni.h>
#include <android_native_app_glue.c>

extern android_app* g_GlobalAndroidApp;

namespace Intra { namespace WindowAPI {
using namespace Math;

struct Window
{
	android_app* app;

	ASensorManager* sensorManager;
	const ASensor* accelerometerSensor;
	ASensorEventQueue* sensorEventQueue;

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	USVec2 size;
	void* wndObj;
};

static Window* g_GlobalAndroidWnd=null;


WindowHandle WindowCreate(const char* caption, WindowType type, SVec2 pos, USVec2 size, void* wndObj)
{
	(void)(caption, type, pos, size);
	WindowHandle wnd = new Window;
	wnd->app = g_GlobalAndroidApp;
	wnd->wndObj=wndObj;

	int w, h;
	eglQuerySurface(wnd->display, wnd->surface, EGL_WIDTH, &w);
	eglQuerySurface(wnd->display, wnd->surface, EGL_HEIGHT, &h);
	wnd->size = {ushort(w), ushort(h)};

	g_GlobalAndroidWnd=wnd;
	return wnd;
}


AnyPtr WindowGetHandle(WindowHandle wnd) {return wnd->app;}

GLContextHandle WindowCreateGLContext(WindowHandle wnd, uint msaaSamples, bool vsync, bool coreProfile, bool debugContext, OpenGL* outGL)
{
	bool fullOpenGL = eglBindAPI(EGL_OPENGL_API);
	if(!fullOpenGL) eglBindAPI(EGL_OPENGL_ES_API);

	int glver=30;
	const int attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,  EGL_GREEN_SIZE, 8,  EGL_RED_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_BACK_BUFFER, true,
		//EGL_CONTEXT_MAJOR_VERSION, glver/10,
		//EGL_CONTEXT_MINOR_VERSION, glver%10,
		EGL_CONTEXT_CLIENT_VERSION, glver/10,
		//EGL_CONTEXT_OPENGL_DEBUG, debugContext,
		EGL_RENDERABLE_TYPE, fullOpenGL? EGL_OPENGL_BIT: EGL_OPENGL_ES_BIT,
		(msaaSamples>1? EGL_SAMPLE_BUFFERS: EGL_NONE /*end*/), 1,
		EGL_SAMPLES, (int)msaaSamples,
		EGL_NONE
	};

	wnd->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(wnd->display, 0, 0);

	EGLConfig config;
	int numConfigs;
	eglChooseConfig(wnd->display, attribs, &config, 1, &numConfigs);

	int format;
	eglGetConfigAttrib(wnd->display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(wnd->app->window, 0, 0, format);

	wnd->surface = eglCreateWindowSurface(wnd->display, config, wnd->app->window, null);
	wnd->context = eglCreateContext(wnd->display, config, null, null);

	if(!eglMakeCurrent(wnd->display, wnd->surface, wnd->surface, wnd->context))
	{
		IO::WarnLog << "Unable to eglMakeCurrent" << IO::endl;
		return null;
	}

	auto& gl = *outGL ;//= InitExtensions(glver, false);
	gl.IsCoreContext=coreProfile;
	gl.IsDebugContext=debugContext;

	gl.GLSLVersion=glver*10;

	if(gl.SwapInterval!=null) gl.SwapInterval(vsync);
	if(gl.Caps.multisampling && msaaSamples>1) glEnable(GL_MULTISAMPLE);

	return (handle)wnd->context;
}

void WindowGLContextDelete(WindowHandle wnd)
{
	if(wnd==null || wnd->context==null) return;
	if(wnd->display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(wnd->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if(wnd->context != EGL_NO_CONTEXT)
			eglDestroyContext(wnd->display, wnd->context);
		if(wnd->surface != EGL_NO_SURFACE)
			eglDestroySurface(wnd->display, wnd->surface);
		eglTerminate(wnd->display);
	}
	wnd->display = EGL_NO_DISPLAY;
	wnd->context = EGL_NO_CONTEXT;
	wnd->surface = EGL_NO_SURFACE;
}

void WindowDelete(WindowHandle wnd)
{
	if(wnd==null) return;
	WindowGLContextDelete(wnd);
	ANativeActivity_finish(wnd->app->activity);
	delete wnd;
}

void WindowSetState(WindowHandle wnd, WindowState state)
{
	(void)(wnd, state);
}

WindowState WindowGetState(WindowHandle wnd)
{
	INTRA_ASSERT(wnd!=null);
	return WindowState::Normal;
}

void WindowSetCaption(WindowHandle wnd, const char* caption)
{
	(void)(wnd, caption);
}

bool WindowIsActive(WindowHandle wnd) {(void)wnd; return true;}

void WindowShowCursor(WindowHandle wnd, bool visible)
{
	(void)(wnd, visible);
}

void WindowSetPos(WindowHandle wnd, SVec2 newpos)
{
	(void)(wnd, newpos);
}

void WindowSetSize(WindowHandle wnd, USVec2 newsize)
{
	(void)(wnd, newsize);
}

SVec2 WindowGetCursorPos(WindowHandle wnd)
{
	(void)wnd;
	return SVec2(wnd->size/2);
}

void WindowSetCursorPos(WindowHandle wnd, SVec2 pos)
{
	(void)(wnd, pos);
}

void WindowSetSizeLimits(WindowHandle wnd, USVec2 minsize, USVec2 maxsize)
{
	(void)(wnd, minsize, maxsize);
}

void WindowGLSwapBuffers(WindowHandle wnd)
{
	eglSwapBuffers(wnd->display, wnd->surface);
}

void WindowGLMakeCurrent(WindowHandle wnd)
{
	eglMakeCurrent(wnd->display, wnd->surface, wnd->surface, wnd->context);
}

USVec2 WindowGetScreenResolution()
{
	return g_GlobalAndroidWnd->size;
}

int GetUnicodeChar(struct android_app* app, int eventType, int keyCode, int metaState)
{
	JavaVM* javaVM = app->activity->vm;
	JNIEnv* jniEnv = app->activity->env;

	JavaVMAttachArgs attachArgs;
	attachArgs.version = JNI_VERSION_1_6;
	attachArgs.name = "NativeThread";
	attachArgs.group = nullptr;

	jint result = javaVM->AttachCurrentThread(&jniEnv, &attachArgs);
	if(result == JNI_ERR)
	{
		return 0;
	}

	jclass class_key_event = jniEnv->FindClass("android/view/KeyEvent");
	int unicodeKey;

	if(metaState == 0)
	{
		jmethodID method_get_unicode_char = jniEnv->GetMethodID(class_key_event, "getUnicodeChar", "()I");
		jmethodID eventConstructor = jniEnv->GetMethodID(class_key_event, "<init>", "(II)V");
		jobject eventObj = jniEnv->NewObject(class_key_event, eventConstructor, eventType, keyCode);

		unicodeKey = jniEnv->CallIntMethod(eventObj, method_get_unicode_char);
	}
	else
	{
		jmethodID method_get_unicode_char = jniEnv->GetMethodID(class_key_event, "getUnicodeChar", "(I)I");
		jmethodID eventConstructor = jniEnv->GetMethodID(class_key_event, "<init>", "(II)V");
		jobject eventObj = jniEnv->NewObject(class_key_event, eventConstructor, eventType, keyCode);

		unicodeKey = jniEnv->CallIntMethod(eventObj, method_get_unicode_char, metaState);
	}

	javaVM->DetachCurrentThread();

	//LOGI("Unicode key is: %d", unicodeKey);
	return unicodeKey;
}

bool AppIsRunning=true;
#if INTRA_DISABLED
void AppProcessMessages()
{
	// Read all pending events.
	int ident;
	int events;
	struct android_poll_source* source;

	// If not animating, we will block forever waiting for events.
	// If animating, we loop until all events are read, then continue
	// to draw the next frame of animation.
	while((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events, (void**)&source)) >= 0)
	{
		// Process this event.
		if(source != null)
		{
			source->process(g_GlobalAndroidApp, source);
		}

		// If a sensor has data, process it now.
		if(ident == LOOPER_ID_USER)
		{
			if(engine.accelerometerSensor != NULL)
			{
				ASensorEvent event;
				while(ASensorEventQueue_getEvents(engine.sensorEventQueue, &event, 1) > 0)
				{
					LOGI("accelerometer: x=%f y=%f z=%f",
						event.acceleration.x, event.acceleration.y,
						event.acceleration.z);
				}
			}
		}

		// Check if we are exiting.
		if(g_GlobalAndroidApp->destroyRequested != 0)
		{
			engine_term_display(&engine);
			return 0;
		}
	}

	if(engine.animating)
	{
		// Done with events; draw next animation frame.
		Cube_update();
		//if (engine.state.angle > 1) {
		//	engine.state.angle = 0;
		//}

		// Drawing is throttled to the screen update rate, so there
		// is no need to do timing here.
		engine_draw_frame(&engine);
	}
}

#endif

}

#if INTRA_DISABLED
//Оконная процедура для всех окон
static size_t WINAPI StaticWndProc(HWND hWnd, uint uMsg, size_t wParam, intptr lParam)
{
	if(uMsg==WM_NCCREATE)
	{
		WindowImpl* wndImpl = (WindowImpl*)((CREATESTRUCT*)lParam)->lpCreateParams;
		INTRA_ASSERT(wndImpl!=null);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (intptr)wndImpl);
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	WindowImpl* wndImpl=(WindowImpl*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	if(wndImpl==null) return DefWindowProcW(hWnd, uMsg, wParam, lParam);

	switch(uMsg)
	{
		case WM_KEYUP:
			ws_on_key_release(wndImpl->wndObj, (Key)wParam);
			return 0;

		case WM_KEYDOWN:
			if((lParam&(1<<30))==0) ws_on_key_press(wndImpl->wndObj, (Key)wParam); //Не обрабатываем повторные сообщения после удержания клавиши
			return 0;

		case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
			ws_on_key_press(wndImpl->wndObj, uMsg==WM_LBUTTONDOWN? Key::MouseLButton: uMsg==WM_MBUTTONDOWN? Key::MouseMButton: Key::MouseRButton);
			return 0;

		case WM_CLOSE:
			if(ws_on_close(wndImpl->wndObj)) break; //Если закрытие подтверждено, действуем по умолчанию (закрываем окно)
			return 0;

		case WM_DESTROY:
			ws_on_destroy(wndImpl->wndObj);
			return 0;

		case WM_SIZE:
		{
			ws_on_resize(wndImpl->wndObj, USVec2((ushort)(rect.right-rect.left), (ushort)(rect.bottom-rect.top) ));
		}
		return 0;
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
#endif

#endif

