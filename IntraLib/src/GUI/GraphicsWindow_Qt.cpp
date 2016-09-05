#include "Core/Core.h"

#if(INTRA_LIBRARY_WINDOW_SYSTEM==INTRA_LIBRARY_WINDOW_SYSTEM_Qt)

#include "GUI/WindowSystemApi.h"

#include <QtGui/QApplication>
#include <QtCore/QTextCodec>
#include <QtOpenGL/QGLWidget>
#include <QtGui/QDesktopWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QMessageBox>


namespace Intra { namespace WindowAPI {

using namespace Math;

struct Window: public QGLWidget
{
public:
	Window(Qt::WindowFlags flags, void* wndObj):
		QGLWidget(null, null, flags), myWndObj(wndObj) {core::memset(pressed_keys, 0, sizeof(pressed_keys));}

	void keyPressEvent(QKeyEvent* kev)
	{
		const Key key=convert_Qt_key(kev);
		if(key==Key(0)) return;
		pressed_keys[(byte)key]=true;
		pressed_keys[(byte)Key::Enter]=(pressed_keys[(byte)Key::MainEnter] || pressed_keys[(byte)Key::NumpadEnter]);
		if(myWndObj!=null) ws_on_key_press(myWndObj, key);
	}

	void keyReleaseEvent(QKeyEvent* kev)
	{
		const Key key=convert_Qt_key(kev);
		if(key==Key(0)) return;
		pressed_keys[(byte)key]=false;
		pressed_keys[(byte)Key::Enter]=(pressed_keys[(byte)Key::MainEnter] || pressed_keys[(byte)Key::NumpadEnter]);
		if(myWndObj!=null) ws_on_key_release(myWndObj, key);
	}

	void resizeEvent(QResizeEvent* rev)
	{if(myWndObj!=null) ws_on_resize(myWndObj, ussize2(rev->size().width(), rev->size().height()));}

	void moveEvent(QMoveEvent* mev)
	{if(myWndObj!=null) ws_on_move(myWndObj, spoint2(mev->pos().x(), mev->pos().y()));}

	void closeEvent(QCloseEvent* cev)
	{if(myWndObj==null || ws_on_close(myWndObj)) cev->accept(); else cev->ignore();}

	void* myWndObj;
	bool pressed_keys[256];

private:
	Key convert_Qt_key(QKeyEvent* qtkey)
	{
		const int k=qtkey->key();
		//const int nk=qtkey->nativeVirtualKey();
		if(k>='A' && k<='Z') return Key(k);
		//if(nk>='a' && nk<='z') return Key(nk-'a'+'A');
		if(k>='0' && k<='9')
			if((qtkey->modifiers() & Qt::KeypadModifier)==0) return Key(k);
			else return Key((byte)Key::Numpad0+(k-'0'));
		if(k>=Qt::Key_Left && k<=Qt::Key_Down) return Key((byte)Key::Left+(k-Qt::Key_Left));
		if(k>=Qt::Key_F1 && k<=Qt::Key_F12) return Key((byte)Key::F1+(k-Qt::Key_F1));
	#define CONV(kc) case Qt::Key_ ## kc: return Key::kc
	#define CONV2(qkc, kc) case Qt::Key_ ## qkc: return Key::kc
		switch(k)
		{
			CONV(Tab); CONV(Escape); CONV(Space);
			CONV(Shift); CONV(Control); CONV(End); CONV(Home);
			CONV(Insert); CONV(Delete); CONV(Pause); CONV2(Print, PrintScreen);
			CONV2(Enter, NumpadEnter); CONV2(Return, MainEnter);
			CONV2(multiply, Star); CONV(Plus); CONV(Minus);
			CONV2(Period, Point); CONV2(division, Divide);

			CONV(Comma); CONV(Semicolon); CONV(Equal); CONV(Backspace);
			CONV(PageUp); CONV(PageDown); CONV(Alt); CONV(Menu); CONV(Slash);
			CONV2(BracketLeft, QuadBracketLeft); CONV2(BracketRight, QuadBracketRight);
			CONV2(QuoteLeft, Quote);
		}
	#undef CONV
	#undef CONV2
		return Key(0); //Ошибка, неизвестная клавиша
	}
};

//Создание окна и инициализация в нём OpenGL
WindowHandle WindowCreate(StringView caption, WindowType type, SVec2 pos, USVec2 size, void* wndObj)
{
	Qt::WindowFlags flags=0;
	if(type & WindowType_NoCaption) flags |= Qt::FramelessWindowHint;

	WindowHandle wnd = new Window(flags, wndObj);
	WindowSetPos(wnd, pos);
	WindowetSize(wnd, size);
	WindowSetCaption(wnd, caption);
	wnd->setFormat(QGLFormat(QGL::DoubleBuffer|QGL::DepthBuffer));
	WindowGLMakeCurrent(wnd);
	return wnd;
}

void WindowDelete(WindowHandle wnd) {delete wnd;}
void WindowSetCaption(WindowHandle wnd, StringView caption) {wnd->setWindowTitle(caption);}
bool WindowIsActive(WindowHandle wnd) {return wnd->isActiveWindow();}

void WindowSetState(WindowHandle wnd, WindowState state)
{
	Qt::WindowStates stateTable[] = {
		Qt::WindowMaximized, Qt::WindowMinimized,
		Qt::WindowFullScreen, Qt::WindowNoState, Qt::WindowNoState
	};
	wnd->setWindowState(stateTable[(ushort)state]|Qt::WindowActive);
	wnd->setHidden(state==WindowState::Hidden);
}

WindowState WindowGetState(WindowHandle wnd)
{
	if(wnd->isHidden()) return WindowState::Hidden;
	Qt::WindowStates state = wnd->windowState();
	if(state & Qt::WindowFullScreen) return WindowState::FullScreen;
	if(state & Qt::WindowMinimized) return WindowState::Minimized;
	if(state & Qt::WindowMaximized) return WindowState::Maximized;
	return WindowState::Normal;
}


void WindowShowCursor(WindowHandle wnd, bool visible)
{
	static QCursor emptyCursor(Qt::BlankCursor);
	if(!visible) wnd->setCursor(emptyCursor);
	else wnd->unsetCursor();
}

void WindowSetPos(WindowHandle wnd, SVec2 newpos) {wnd->move(newpos.x, newpos.y);}
void WindowSetSize(WindowHandle wnd, USVec2 newsize) {wnd->resize(newsize.x, newsize.y);}
bool WindowIsKeyPressed(WindowHandle wnd, Key key) {return wnd->pressed_keys[(byte)key];}

SVec2 WindowGetCursorPos(WindowHandle wnd)
{
	const QPoint pos = wnd->mapFromGlobal(QCursor::pos());
	return SVec2(pos.x(), pos.y());
}

void WindowSetCursorPos(WindowHandle wnd, SVec2 pos) {QCursor::setPos(wnd->mapToGlobal(QPoint(pos.x, pos.y)));}
void WindowGLSwapBuffers(WindowHandle wnd) {wnd->swapBuffers();}
void WindowGLMakeCurrent(WindowHandle wnd) {wnd->makeCurrent();}

void WindowSetSizeLimits(WindowHandle wnd, USVec2 minsize, USVec2 maxsize)
{
	wnd->setMaximumSize(maxsize.x, maxsize.y);
	wnd->setMinimumSize(minsize.x, minsize.y);
}

USVec2 GetScreenResolution()
{
	QSize sz = QApplication::desktop()->size();
	return USVec2(ushort(sz.width()), ushort(sz.height()));
}

static int __argc=1;
static char* __argv[]={(char*)"application", null};

class App: public QApplication
{
public:
	App(): QApplication(__argc, __argv) {QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());}
};
static App* const app=new App;

bool AppIsRunning=true;

void AppProcessMessages() {QApplication::processEvents();}

}}

#endif
