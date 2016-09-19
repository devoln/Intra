#pragma once

#include "GUI/WindowSystemApiDeclarations.h"
#include "Utils/Event.h"
#include "Math/Vector.h"

namespace Intra {

namespace WindowAPI {
	void AppProcessMessages();
	extern bool AppIsRunning;

	void OnMove(void* wnd, Math::SVec2 newPos);
	void OnResize(void* wnd, Math::USVec2 newSize);
}

class AGraphics;

//Класс графического окна. В нём инициализируется графический API
class GraphicsWindow
{
	friend class AGraphics;
	friend class GraphicsGL;

	friend void WindowAPI::OnMove(void* wnd, Math::SVec2 newPos);
	friend void WindowAPI::OnResize(void* wnd, Math::USVec2 newSize);
public:
	GraphicsWindow(StringView caption, WindowType type=0, Math::SVec2 pos={100, 50},
		Math::USVec2 size={800, 600}, WindowState state=WindowState::Normal);
	virtual ~GraphicsWindow();

	void SetCaption(StringView caption);
	void SetState(WindowState state);
	WindowState GetState() const;
	bool IsActive() const;
	void SetCursorVisibility(bool visible);

	void Close();
	bool IsClosed() const {return hndl==null;}

	WindowAPI::WindowHandle GetHandle() const {return hndl;}
	AnyPtr GetNativeHandle() const;

	//Функции событий:
	virtual bool OnClose() {return true;}

	void impl_on_destroy() {hndl=null; OnDestroy();}

	void impl_on_key_release(Key key)
	{
		pressed_keys[byte(key)/32] &= ~( 1u << (byte(key)%32u) );
		was_released_keys[byte(key)/32] |= 1u << (byte(key)%32u);
		OnKeyRelease(key);
	}

	virtual void OnKeyRelease(Key) {}

	void impl_on_key_press(Key key)
	{
		pressed_keys[byte(key)/32] |= 1u << (byte(key)%32u);
		was_pressed_keys[byte(key)/32] |= 1u << (byte(key)%32u);
		OnKeyPress(key);
	}

	virtual void OnKeyPress(Key) {}
	virtual void OnMouseMove(Math::SVec2) {}

	void SetPosition(Math::SVec2 newpos);

	void SetSize(Math::USVec2 newsize);

	void SetRect(Math::SVec2 newpos, Math::USVec2 newsize)
	{
		SetPosition(newpos);
		SetSize(newsize);
	}

	Math::SVec2 Position() const {return position;}
	Math::USVec2 Size() const {return size;}
	
	bool IsKeyPressed(Key key) {return (pressed_keys[byte(key)/32] & (1u << (byte(key) % 32u))) != 0;}
	bool KeyWasPressed(Key key)
	{
		const bool result = (was_pressed_keys[byte(key)/32] & (1u << (byte(key) % 32u)))!=0 && IsKeyPressed(key);
		was_pressed_keys[byte(key)/32u] &= ~( 1u << (byte(key) % 32u) );
		return result;
	}

	bool KeyWasReleased(Key key)
	{
		const bool result = (was_released_keys[byte(key)/32u] & ( 1u << (byte(key) % 32u)))!=0 && IsKeyPressed(key);
		was_released_keys[byte(key)/32u] &= ~(1u << (byte(key) % 32u));
		return result;
	}

	//Позиция мыши относительно окна
	Math::SVec2 GetCursorPos();
	void SetCursorPos(Math::SVec2 pos);

	AGraphics* Graphics() const {return my_graphics;}
	bool IsCursorVisible() const {return is_cursor_visible;}

	Utils::Event OnDestroy, OnResize, OnMove;

private:
	WindowAPI::WindowHandle hndl;
	AGraphics* my_graphics;
	bool is_cursor_visible;
	Math::SVec2 position;
	Math::USVec2 size;

	flag32 was_pressed_keys[8], was_released_keys[8], pressed_keys[8];
	
	void swap_buffers();
	void make_current();

	//Запрещаем присваивание и копирование окон
	GraphicsWindow& operator=(const GraphicsWindow&) = delete;
	GraphicsWindow(const GraphicsWindow&) = delete;
};

Math::USVec2 GetScreenResolution();

}

