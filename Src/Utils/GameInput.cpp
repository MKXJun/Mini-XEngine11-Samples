
#include <memory>

#include <cassert>
#include <exception>
#include <wrl/client.h>
#include <windows.h>
#include <Utils/GameInput.h>

#pragma warning(disable:26812)

using Microsoft::WRL::ComPtr;

struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

typedef std::unique_ptr<void, handle_closer> ScopedHandle;

#pragma region WinMouse

class WinMouse
{
public:
	WinMouse() noexcept(false);
	WinMouse(WinMouse&& moveFrom) noexcept;
	WinMouse& operator= (WinMouse&& moveFrom) noexcept;

	WinMouse(WinMouse const&) = delete;
	WinMouse& operator=(WinMouse const&) = delete;

	virtual ~WinMouse();

	using Mode = GameInput::Mouse::Mode;

	struct State
	{
		bool    leftButton;
		bool    middleButton;
		bool    rightButton;
		bool    xButton1;
		bool    xButton2;
		int     x;
		int     y;
		int     scrollWheelValue;
		Mode    positionMode;
	};

	class ButtonStateTracker
	{
	public:
		enum ButtonState
		{
			UP = 0,         // Button is up
			HELD = 1,       // Button is held down
			RELEASED = 2,   // Button was just released
			PRESSED = 3,    // Buton was just pressed
		};

		ButtonState leftButton;
		ButtonState middleButton;
		ButtonState rightButton;
		ButtonState xButton1;
		ButtonState xButton2;

#pragma prefast(suppress: 26495, "Reset() performs the initialization")
		ButtonStateTracker() noexcept { Reset(); }

		void __cdecl Update(const State& state);

		void __cdecl Reset() noexcept;

		State __cdecl GetLastState() const { return lastState; }

	private:
		State lastState;
	};

	// Retrieve the current state of the mouse
	State __cdecl GetState() const;

	// Resets the accumulated scroll wheel value
	void __cdecl ResetScrollWheelValue();

	// Sets mouse mode (defaults to absolute)
	void __cdecl SetMode(Mode mode);

	// Feature detection
	bool __cdecl IsConnected() const;

	// Cursor visibility
	bool __cdecl IsVisible() const;
	void __cdecl SetVisible(bool visible);

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) && defined(WM_USER)
	void __cdecl SetWindow(HWND window);
	static void __cdecl ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);
#endif
	// Singleton
	static WinMouse* __cdecl Get();

private:
	// Private implementation.
	class Impl;

	std::unique_ptr<Impl> pImpl;
};





class WinMouse::Impl
{
public:
	using Mode = GameInput::Mouse::Mode;

	Impl(WinMouse* owner) :
		mState{},
		mOwner(owner),
		mWindow(nullptr),
		mMode(Mode::MODE_ABSOLUTE),
		mLastX(0),
		mLastY(0),
		mRelativeX(INT32_MAX),
		mRelativeY(INT32_MAX),
		mInFocus(true)
	{
		if (s_mouse)
		{
			throw std::exception("Mouse is a singleton");
		}

		s_mouse = this;

		mScrollWheelValue.reset(CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE));
		mRelativeRead.reset(CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE));
		mAbsoluteMode.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
		mRelativeMode.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
		if (!mScrollWheelValue
			|| !mRelativeRead
			|| !mAbsoluteMode
			|| !mRelativeMode)
		{
			throw std::exception("CreateEventEx");
		}
	}

	~Impl()
	{
		s_mouse = nullptr;
	}

	void GetState(State& state) const
	{
		memcpy(&state, &mState, sizeof(State));
		state.positionMode = mMode;

		DWORD result = WaitForSingleObjectEx(mScrollWheelValue.get(), 0, FALSE);
		if (result == WAIT_FAILED)
			throw std::exception("WaitForSingleObjectEx");

		if (result == WAIT_OBJECT_0)
		{
			state.scrollWheelValue = 0;
		}

		if (state.positionMode == Mode::MODE_RELATIVE)
		{
			result = WaitForSingleObjectEx(mRelativeRead.get(), 0, FALSE);

			if (result == WAIT_FAILED)
				throw std::exception("WaitForSingleObjectEx");

			if (result == WAIT_OBJECT_0)
			{
				state.x = 0;
				state.y = 0;
			}
			else
			{
				SetEvent(mRelativeRead.get());
			}
		}
	}

	void ResetScrollWheelValue()
	{
		SetEvent(mScrollWheelValue.get());
	}

	void SetMode(Mode mode)
	{
		if (mMode == mode)
			return;

		SetEvent((mode == Mode::MODE_ABSOLUTE) ? mAbsoluteMode.get() : mRelativeMode.get());

		assert(mWindow != nullptr);

		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_HOVER;
		tme.hwndTrack = mWindow;
		tme.dwHoverTime = 1;
		if (!TrackMouseEvent(&tme))
		{
			throw std::exception("TrackMouseEvent");
		}
	}

	bool IsConnected() const
	{
		return GetSystemMetrics(SM_MOUSEPRESENT) != 0;
	}

	bool IsVisible() const
	{
		if (mMode == Mode::MODE_RELATIVE)
			return false;

		CURSORINFO info = { sizeof(CURSORINFO), 0, nullptr, {} };
		if (!GetCursorInfo(&info))
		{
			throw std::exception("GetCursorInfo");
		}

		return (info.flags & CURSOR_SHOWING) != 0;
	}

	void SetVisible(bool visible)
	{
		if (mMode == Mode::MODE_RELATIVE)
			return;

		CURSORINFO info = { sizeof(CURSORINFO), 0, nullptr, {} };
		if (!GetCursorInfo(&info))
		{
			throw std::exception("GetCursorInfo");
		}

		bool isvisible = (info.flags & CURSOR_SHOWING) != 0;
		if (isvisible != visible)
		{
			ShowCursor(visible);
		}
	}

	void SetWindow(HWND window)
	{
		if (mWindow == window)
			return;

		assert(window != nullptr);

		RAWINPUTDEVICE Rid;
		Rid.usUsagePage = 0x1 /* HID_USAGE_PAGE_GENERIC */;
		Rid.usUsage = 0x2 /* HID_USAGE_GENERIC_MOUSE */;
		Rid.dwFlags = RIDEV_INPUTSINK;
		Rid.hwndTarget = window;
		if (!RegisterRawInputDevices(&Rid, 1, sizeof(RAWINPUTDEVICE)))
		{
			throw std::exception("RegisterRawInputDevices");
		}

		mWindow = window;
	}

	State           mState;

	WinMouse* mOwner;

	static WinMouse::Impl* s_mouse;

private:
	HWND            mWindow;
	Mode            mMode;

	ScopedHandle    mScrollWheelValue;
	ScopedHandle    mRelativeRead;
	ScopedHandle    mAbsoluteMode;
	ScopedHandle    mRelativeMode;

	int             mLastX;
	int             mLastY;
	int             mRelativeX;
	int             mRelativeY;

	bool            mInFocus;

	friend void WinMouse::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);

	void ClipToWindow()
	{
		assert(mWindow != nullptr);

		RECT rect;
		GetClientRect(mWindow, &rect);

		POINT ul;
		ul.x = rect.left;
		ul.y = rect.top;

		POINT lr;
		lr.x = rect.right;
		lr.y = rect.bottom;

		MapWindowPoints(mWindow, nullptr, &ul, 1);
		MapWindowPoints(mWindow, nullptr, &lr, 1);

		rect.left = ul.x;
		rect.top = ul.y;

		rect.right = lr.x;
		rect.bottom = lr.y;

		ClipCursor(&rect);
	}
};


WinMouse::Impl* WinMouse::Impl::s_mouse = nullptr;


void WinMouse::SetWindow(HWND window)
{
	pImpl->SetWindow(window);
}


void WinMouse::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	auto pImpl = Impl::s_mouse;

	if (!pImpl)
		return;

	HANDLE evts[3];
	evts[0] = pImpl->mScrollWheelValue.get();
	evts[1] = pImpl->mAbsoluteMode.get();
	evts[2] = pImpl->mRelativeMode.get();
	switch (WaitForMultipleObjectsEx(_countof(evts), evts, FALSE, 0, FALSE))
	{
	case WAIT_OBJECT_0:
		pImpl->mState.scrollWheelValue = 0;
		ResetEvent(evts[0]);
		break;

	case (WAIT_OBJECT_0 + 1):
	{
		pImpl->mMode = Mode::MODE_ABSOLUTE;
		ClipCursor(nullptr);

		POINT point;
		point.x = pImpl->mLastX;
		point.y = pImpl->mLastY;

		// We show the cursor before moving it to support Remote Desktop
		ShowCursor(TRUE);

		if (MapWindowPoints(pImpl->mWindow, nullptr, &point, 1))
		{
			SetCursorPos(point.x, point.y);
		}
		pImpl->mState.x = pImpl->mLastX;
		pImpl->mState.y = pImpl->mLastY;
	}
	break;

	case (WAIT_OBJECT_0 + 2):
	{
		ResetEvent(pImpl->mRelativeRead.get());

		pImpl->mMode = Mode::MODE_RELATIVE;
		pImpl->mState.x = pImpl->mState.y = 0;
		pImpl->mRelativeX = INT32_MAX;
		pImpl->mRelativeY = INT32_MAX;

		ShowCursor(FALSE);

		pImpl->ClipToWindow();
	}
	break;

	case WAIT_FAILED:
		throw std::exception("WaitForMultipleObjectsEx");
	}

	switch (message)
	{
	case WM_ACTIVATEAPP:
		if (wParam)
		{
			pImpl->mInFocus = true;

			if (pImpl->mMode == Mode::MODE_RELATIVE)
			{
				pImpl->mState.x = pImpl->mState.y = 0;

				ShowCursor(FALSE);

				pImpl->ClipToWindow();
			}
		}
		else
		{
			int scrollWheel = pImpl->mState.scrollWheelValue;
			memset(&pImpl->mState, 0, sizeof(State));
			pImpl->mState.scrollWheelValue = scrollWheel;

			pImpl->mInFocus = false;
		}
		return;

	case WM_INPUT:
		if (pImpl->mInFocus && pImpl->mMode == Mode::MODE_RELATIVE)
		{
			RAWINPUT raw;
			UINT rawSize = sizeof(raw);

			UINT resultData = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &rawSize, sizeof(RAWINPUTHEADER));
			if (resultData == UINT(-1))
			{
				throw std::exception("GetRawInputData");
			}

			if (raw.header.dwType == RIM_TYPEMOUSE)
			{
				if (!(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
				{
					pImpl->mState.x = raw.data.mouse.lLastX;
					pImpl->mState.y = raw.data.mouse.lLastY;

					ResetEvent(pImpl->mRelativeRead.get());
				}
				else if (raw.data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP)
				{
					// This is used to make Remote Desktop sessons work
					const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
					const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

					int x = static_cast<int>((float(raw.data.mouse.lLastX) / 65535.0f) * width);
					int y = static_cast<int>((float(raw.data.mouse.lLastY) / 65535.0f) * height);

					if (pImpl->mRelativeX == INT32_MAX)
					{
						pImpl->mState.x = pImpl->mState.y = 0;
					}
					else
					{
						pImpl->mState.x = x - pImpl->mRelativeX;
						pImpl->mState.y = y - pImpl->mRelativeY;
					}

					pImpl->mRelativeX = x;
					pImpl->mRelativeY = y;

					ResetEvent(pImpl->mRelativeRead.get());
				}
			}
		}
		return;

	case WM_MOUSEMOVE:
		break;

	case WM_LBUTTONDOWN:
		pImpl->mState.leftButton = true;
		break;

	case WM_LBUTTONUP:
		pImpl->mState.leftButton = false;
		break;

	case WM_RBUTTONDOWN:
		pImpl->mState.rightButton = true;
		break;

	case WM_RBUTTONUP:
		pImpl->mState.rightButton = false;
		break;

	case WM_MBUTTONDOWN:
		pImpl->mState.middleButton = true;
		break;

	case WM_MBUTTONUP:
		pImpl->mState.middleButton = false;
		break;

	case WM_MOUSEWHEEL:
		pImpl->mState.scrollWheelValue += GET_WHEEL_DELTA_WPARAM(wParam);
		return;

	case WM_XBUTTONDOWN:
		switch (GET_XBUTTON_WPARAM(wParam))
		{
		case XBUTTON1:
			pImpl->mState.xButton1 = true;
			break;

		case XBUTTON2:
			pImpl->mState.xButton2 = true;
			break;
		}
		break;

	case WM_XBUTTONUP:
		switch (GET_XBUTTON_WPARAM(wParam))
		{
		case XBUTTON1:
			pImpl->mState.xButton1 = false;
			break;

		case XBUTTON2:
			pImpl->mState.xButton2 = false;
			break;
		}
		break;

	case WM_MOUSEHOVER:
		break;

	default:
		// Not a mouse message, so exit
		return;
	}

	if (pImpl->mMode == Mode::MODE_ABSOLUTE)
	{
		// All mouse messages provide a new pointer position
		int xPos = static_cast<short>(LOWORD(lParam)); // GET_X_LPARAM(lParam);
		int yPos = static_cast<short>(HIWORD(lParam)); // GET_Y_LPARAM(lParam);

		pImpl->mState.x = pImpl->mLastX = xPos;
		pImpl->mState.y = pImpl->mLastY = yPos;
	}
}

#pragma warning( disable : 4355 )

// Public constructor.
WinMouse::WinMouse() noexcept(false)
	: pImpl(std::make_unique<Impl>(this))
{
}


// Move constructor.
WinMouse::WinMouse(WinMouse&& moveFrom) noexcept
	: pImpl(std::move(moveFrom.pImpl))
{
	pImpl->mOwner = this;
}


// Move assignment.
WinMouse& WinMouse::operator= (WinMouse&& moveFrom) noexcept
{
	pImpl = std::move(moveFrom.pImpl);
	pImpl->mOwner = this;
	return *this;
}


// Public destructor.
WinMouse::~WinMouse()
{
}


WinMouse::State WinMouse::GetState() const
{
	State state;
	pImpl->GetState(state);
	return state;
}


void WinMouse::ResetScrollWheelValue()
{
	pImpl->ResetScrollWheelValue();
}


void WinMouse::SetMode(Mode mode)
{
	pImpl->SetMode(mode);
}


bool WinMouse::IsConnected() const
{
	return pImpl->IsConnected();
}

bool WinMouse::IsVisible() const
{
	return pImpl->IsVisible();
}

void WinMouse::SetVisible(bool visible)
{
	pImpl->SetVisible(visible);
}

WinMouse* WinMouse::Get()
{
	if (!Impl::s_mouse || !Impl::s_mouse->mOwner)
		return nullptr;

	return Impl::s_mouse->mOwner;
}



//======================================================================================
// ButtonStateTracker
//======================================================================================

#define UPDATE_BUTTON_STATE(field) field = static_cast<WinMouse::ButtonStateTracker::ButtonState>( ( !!state.field ) | ( ( !!state.field ^ !!lastState.field ) << 1 ) );

void WinMouse::ButtonStateTracker::Update(const WinMouse::State& state)
{
	UPDATE_BUTTON_STATE(leftButton);

	assert((!state.leftButton && !lastState.leftButton) == (leftButton == UP));
	assert((state.leftButton && lastState.leftButton) == (leftButton == HELD));
	assert((!state.leftButton && lastState.leftButton) == (leftButton == RELEASED));
	assert((state.leftButton && !lastState.leftButton) == (leftButton == PRESSED));

	UPDATE_BUTTON_STATE(middleButton);
	UPDATE_BUTTON_STATE(rightButton);
	UPDATE_BUTTON_STATE(xButton1);
	UPDATE_BUTTON_STATE(xButton2);

	lastState = state;
}

#undef UPDATE_BUTTON_STATE


void WinMouse::ButtonStateTracker::Reset() noexcept
{
	memset(this, 0, sizeof(ButtonStateTracker));
}

#pragma endregion WinMouse

#pragma region WinKeyboard

class WinKeyboard
{
public:
	WinKeyboard() noexcept(false);
	WinKeyboard(WinKeyboard&& moveFrom) noexcept;
	WinKeyboard& operator= (WinKeyboard&& moveFrom) noexcept;

	WinKeyboard(WinKeyboard const&) = delete;
	WinKeyboard& operator=(WinKeyboard const&) = delete;

	virtual ~WinKeyboard();

	using Keys = GameInput::Keyboard::Keys;

	struct State
	{
		bool Reserved0 : 8;
		bool Back : 1;              // VK_BACK, 0x8
		bool Tab : 1;               // VK_TAB, 0x9
		bool Reserved1 : 3;
		bool Enter : 1;             // VK_RETURN, 0xD
		bool Reserved2 : 2;
		bool Reserved3 : 3;
		bool Pause : 1;             // VK_PAUSE, 0x13
		bool CapsLock : 1;          // VK_CAPITAL, 0x14
		bool Kana : 1;              // VK_KANA, 0x15
		bool Reserved4 : 2;
		bool Reserved5 : 1;
		bool Kanji : 1;             // VK_KANJI, 0x19
		bool Reserved6 : 1;
		bool Escape : 1;            // VK_ESCAPE, 0x1B
		bool ImeConvert : 1;        // VK_CONVERT, 0x1C
		bool ImeNoConvert : 1;      // VK_NONCONVERT, 0x1D
		bool Reserved7 : 2;
		bool Space : 1;             // VK_SPACE, 0x20
		bool PageUp : 1;            // VK_PRIOR, 0x21
		bool PageDown : 1;          // VK_NEXT, 0x22
		bool End : 1;               // VK_END, 0x23
		bool Home : 1;              // VK_HOME, 0x24
		bool Left : 1;              // VK_LEFT, 0x25
		bool Up : 1;                // VK_UP, 0x26
		bool Right : 1;             // VK_RIGHT, 0x27
		bool Down : 1;              // VK_DOWN, 0x28
		bool Select : 1;            // VK_SELECT, 0x29
		bool Print : 1;             // VK_PRINT, 0x2A
		bool Execute : 1;           // VK_EXECUTE, 0x2B
		bool PrintScreen : 1;       // VK_SNAPSHOT, 0x2C
		bool Insert : 1;            // VK_INSERT, 0x2D
		bool Delete : 1;            // VK_DELETE, 0x2E
		bool Help : 1;              // VK_HELP, 0x2F
		bool D0 : 1;                // 0x30
		bool D1 : 1;                // 0x31
		bool D2 : 1;                // 0x32
		bool D3 : 1;                // 0x33
		bool D4 : 1;                // 0x34
		bool D5 : 1;                // 0x35
		bool D6 : 1;                // 0x36
		bool D7 : 1;                // 0x37
		bool D8 : 1;                // 0x38
		bool D9 : 1;                // 0x39
		bool Reserved8 : 6;
		bool Reserved9 : 1;
		bool A : 1;                 // 0x41
		bool B : 1;                 // 0x42
		bool C : 1;                 // 0x43
		bool D : 1;                 // 0x44
		bool E : 1;                 // 0x45
		bool F : 1;                 // 0x46
		bool G : 1;                 // 0x47
		bool H : 1;                 // 0x48
		bool I : 1;                 // 0x49
		bool J : 1;                 // 0x4A
		bool K : 1;                 // 0x4B
		bool L : 1;                 // 0x4C
		bool M : 1;                 // 0x4D
		bool N : 1;                 // 0x4E
		bool O : 1;                 // 0x4F
		bool P : 1;                 // 0x50
		bool Q : 1;                 // 0x51
		bool R : 1;                 // 0x52
		bool S : 1;                 // 0x53
		bool T : 1;                 // 0x54
		bool U : 1;                 // 0x55
		bool V : 1;                 // 0x56
		bool W : 1;                 // 0x57
		bool X : 1;                 // 0x58
		bool Y : 1;                 // 0x59
		bool Z : 1;                 // 0x5A
		bool LeftWindows : 1;       // VK_LWIN, 0x5B
		bool RightWindows : 1;      // VK_RWIN, 0x5C
		bool Apps : 1;              // VK_APPS, 0x5D
		bool Reserved10 : 1;
		bool Sleep : 1;             // VK_SLEEP, 0x5F
		bool NumPad0 : 1;           // VK_NUMPAD0, 0x60
		bool NumPad1 : 1;           // VK_NUMPAD1, 0x61
		bool NumPad2 : 1;           // VK_NUMPAD2, 0x62
		bool NumPad3 : 1;           // VK_NUMPAD3, 0x63
		bool NumPad4 : 1;           // VK_NUMPAD4, 0x64
		bool NumPad5 : 1;           // VK_NUMPAD5, 0x65
		bool NumPad6 : 1;           // VK_NUMPAD6, 0x66
		bool NumPad7 : 1;           // VK_NUMPAD7, 0x67
		bool NumPad8 : 1;           // VK_NUMPAD8, 0x68
		bool NumPad9 : 1;           // VK_NUMPAD9, 0x69
		bool Multiply : 1;          // VK_MULTIPLY, 0x6A
		bool Add : 1;               // VK_ADD, 0x6B
		bool Separator : 1;         // VK_SEPARATOR, 0x6C
		bool Subtract : 1;          // VK_SUBTRACT, 0x6D
		bool Decimal : 1;           // VK_DECIMANL, 0x6E
		bool Divide : 1;            // VK_DIVIDE, 0x6F
		bool F1 : 1;                // VK_F1, 0x70
		bool F2 : 1;                // VK_F2, 0x71
		bool F3 : 1;                // VK_F3, 0x72
		bool F4 : 1;                // VK_F4, 0x73
		bool F5 : 1;                // VK_F5, 0x74
		bool F6 : 1;                // VK_F6, 0x75
		bool F7 : 1;                // VK_F7, 0x76
		bool F8 : 1;                // VK_F8, 0x77
		bool F9 : 1;                // VK_F9, 0x78
		bool F10 : 1;               // VK_F10, 0x79
		bool F11 : 1;               // VK_F11, 0x7A
		bool F12 : 1;               // VK_F12, 0x7B
		bool F13 : 1;               // VK_F13, 0x7C
		bool F14 : 1;               // VK_F14, 0x7D
		bool F15 : 1;               // VK_F15, 0x7E
		bool F16 : 1;               // VK_F16, 0x7F
		bool F17 : 1;               // VK_F17, 0x80
		bool F18 : 1;               // VK_F18, 0x81
		bool F19 : 1;               // VK_F19, 0x82
		bool F20 : 1;               // VK_F20, 0x83
		bool F21 : 1;               // VK_F21, 0x84
		bool F22 : 1;               // VK_F22, 0x85
		bool F23 : 1;               // VK_F23, 0x86
		bool F24 : 1;               // VK_F24, 0x87
		bool Reserved11 : 8;
		bool NumLock : 1;           // VK_NUMLOCK, 0x90
		bool Scroll : 1;            // VK_SCROLL, 0x91
		bool Reserved12 : 6;
		bool Reserved13 : 8;
		bool LeftShift : 1;         // VK_LSHIFT, 0xA0
		bool RightShift : 1;        // VK_RSHIFT, 0xA1
		bool LeftControl : 1;       // VK_LCONTROL, 0xA2
		bool RightControl : 1;      // VK_RCONTROL, 0xA3
		bool LeftAlt : 1;           // VK_LMENU, 0xA4
		bool RightAlt : 1;          // VK_RMENU, 0xA5
		bool BrowserBack : 1;       // VK_BROWSER_BACK, 0xA6
		bool BrowserForward : 1;    // VK_BROWSER_FORWARD, 0xA7
		bool BrowserRefresh : 1;    // VK_BROWSER_REFRESH, 0xA8
		bool BrowserStop : 1;       // VK_BROWSER_STOP, 0xA9
		bool BrowserSearch : 1;     // VK_BROWSER_SEARCH, 0xAA
		bool BrowserFavorites : 1;  // VK_BROWSER_FAVORITES, 0xAB
		bool BrowserHome : 1;       // VK_BROWSER_HOME, 0xAC
		bool VolumeMute : 1;        // VK_VOLUME_MUTE, 0xAD
		bool VolumeDown : 1;        // VK_VOLUME_DOWN, 0xAE
		bool VolumeUp : 1;          // VK_VOLUME_UP, 0xAF
		bool MediaNextTrack : 1;    // VK_MEDIA_NEXT_TRACK, 0xB0
		bool MediaPreviousTrack : 1;// VK_MEDIA_PREV_TRACK, 0xB1
		bool MediaStop : 1;         // VK_MEDIA_STOP, 0xB2
		bool MediaPlayPause : 1;    // VK_MEDIA_PLAY_PAUSE, 0xB3
		bool LaunchMail : 1;        // VK_LAUNCH_MAIL, 0xB4
		bool SelectMedia : 1;       // VK_LAUNCH_MEDIA_SELECT, 0xB5
		bool LaunchApplication1 : 1;// VK_LAUNCH_APP1, 0xB6
		bool LaunchApplication2 : 1;// VK_LAUNCH_APP2, 0xB7
		bool Reserved14 : 2;
		bool OemSemicolon : 1;      // VK_OEM_1, 0xBA
		bool OemPlus : 1;           // VK_OEM_PLUS, 0xBB
		bool OemComma : 1;          // VK_OEM_COMMA, 0xBC
		bool OemMinus : 1;          // VK_OEM_MINUS, 0xBD
		bool OemPeriod : 1;         // VK_OEM_PERIOD, 0xBE
		bool OemQuestion : 1;       // VK_OEM_2, 0xBF
		bool OemTilde : 1;          // VK_OEM_3, 0xC0
		bool Reserved15 : 7;
		bool Reserved16 : 8;
		bool Reserved17 : 8;
		bool Reserved18 : 3;
		bool OemOpenBrackets : 1;   // VK_OEM_4, 0xDB
		bool OemPipe : 1;           // VK_OEM_5, 0xDC
		bool OemCloseBrackets : 1;  // VK_OEM_6, 0xDD
		bool OemQuotes : 1;         // VK_OEM_7, 0xDE
		bool Oem8 : 1;              // VK_OEM_8, 0xDF
		bool Reserved19 : 2;
		bool OemBackslash : 1;      // VK_OEM_102, 0xE2
		bool Reserved20 : 2;
		bool ProcessKey : 1;        // VK_PROCESSKEY, 0xE5
		bool Reserved21 : 2;
		bool Reserved22 : 8;
		bool Reserved23 : 2;
		bool OemCopy : 1;           // 0XF2
		bool OemAuto : 1;           // 0xF3
		bool OemEnlW : 1;           // 0xF4
		bool Reserved24 : 1;
		bool Attn : 1;              // VK_ATTN, 0xF6
		bool Crsel : 1;             // VK_CRSEL, 0xF7
		bool Exsel : 1;             // VK_EXSEL, 0xF8
		bool EraseEof : 1;          // VK_EREOF, 0xF9
		bool Play : 1;              // VK_PLAY, 0xFA
		bool Zoom : 1;              // VK_ZOOM, 0xFB
		bool Reserved25 : 1;
		bool Pa1 : 1;               // VK_PA1, 0xFD
		bool OemClear : 1;          // VK_OEM_CLEAR, 0xFE
		bool Reserved26 : 1;

		bool __cdecl IsKeyDown(Keys key) const
		{
			if (key >= 0 && key <= 0xfe)
			{
				auto ptr = reinterpret_cast<const uint32_t*>(this);
				unsigned int bf = 1u << (key & 0x1f);
				return (ptr[(key >> 5)] & bf) != 0;
			}
			return false;
		}

		bool __cdecl IsKeyUp(Keys key) const
		{
			if (key >= 0 && key <= 0xfe)
			{
				auto ptr = reinterpret_cast<const uint32_t*>(this);
				unsigned int bf = 1u << (key & 0x1f);
				return (ptr[(key >> 5)] & bf) == 0;
			}
			return false;
		}
	};

	class KeyboardStateTracker
	{
	public:
		State released;
		State pressed;

#pragma prefast(suppress: 26495, "Reset() performs the initialization")
		KeyboardStateTracker() noexcept { Reset(); }

		void __cdecl Update(const State& state);

		void __cdecl Reset() noexcept;

		bool __cdecl IsKeyPressed(Keys key) const { return pressed.IsKeyDown(key); }
		bool __cdecl IsKeyReleased(Keys key) const { return released.IsKeyDown(key); }

		State __cdecl GetLastState() const { return lastState; }

	public:
		State lastState;
	};

	// Retrieve the current state of the keyboard
	State __cdecl GetState() const;

	// Reset the keyboard state
	void __cdecl Reset();

	// Feature detection
	bool __cdecl IsConnected() const;

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) && defined(WM_USER)
	static void __cdecl ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);
#endif

	// Singleton
	static WinKeyboard* __cdecl Get();

private:
	// Private implementation.
	class Impl;

	std::unique_ptr<Impl> pImpl;
};

static_assert(sizeof(WinKeyboard::State) == (256 / 8), "Size mismatch for State");

namespace
{
	void KeyDown(int key, WinKeyboard::State& state)
	{
		if (key < 0 || key > 0xfe)
			return;

		auto ptr = reinterpret_cast<uint32_t*>(&state);

		unsigned int bf = 1u << (key & 0x1f);
		ptr[(key >> 5)] |= bf;
	}

	void KeyUp(int key, WinKeyboard::State& state)
	{
		if (key < 0 || key > 0xfe)
			return;

		auto ptr = reinterpret_cast<uint32_t*>(&state);

		unsigned int bf = 1u << (key & 0x1f);
		ptr[(key >> 5)] &= ~bf;
	}
}

class WinKeyboard::Impl
{
public:
	Impl(WinKeyboard* owner) :
		mState{},
		mOwner(owner)
	{
		if (s_WinKeyboard)
		{
			throw std::exception("WinKeyboard is a singleton");
		}

		s_WinKeyboard = this;
	}

	~Impl()
	{
		s_WinKeyboard = nullptr;
	}

	void GetState(State& state) const
	{
		memcpy(&state, &mState, sizeof(State));
	}

	void Reset()
	{
		memset(&mState, 0, sizeof(State));
	}

	bool IsConnected() const
	{
		return true;
	}

	State           mState;
	WinKeyboard* mOwner;

	static WinKeyboard::Impl* s_WinKeyboard;
};

WinKeyboard::Impl* WinKeyboard::Impl::s_WinKeyboard = nullptr;

void WinKeyboard::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	auto pImpl = Impl::s_WinKeyboard;

	if (!pImpl)
		return;

	bool down = false;

	switch (message)
	{
	case WM_ACTIVATEAPP:
		pImpl->Reset();
		return;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		down = true;
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		break;

	default:
		return;
	}

	int vk = static_cast<int>(wParam);
	switch (vk)
	{
	case VK_SHIFT:
		vk = MapVirtualKey((lParam & 0x00ff0000) >> 16, MAPVK_VSC_TO_VK_EX);
		if (!down)
		{
			// Workaround to ensure left vs. right shift get cleared when both were pressed at same time
			KeyUp(VK_LSHIFT, pImpl->mState);
			KeyUp(VK_RSHIFT, pImpl->mState);
		}
		break;

	case VK_CONTROL:
		vk = (lParam & 0x01000000) ? VK_RCONTROL : VK_LCONTROL;
		break;

	case VK_MENU:
		vk = (lParam & 0x01000000) ? VK_RMENU : VK_LMENU;
		break;
	}

	if (down)
	{
		KeyDown(vk, pImpl->mState);
	}
	else
	{
		KeyUp(vk, pImpl->mState);
	}
}





#pragma warning( disable : 4355 )

// Public constructor.
WinKeyboard::WinKeyboard() noexcept(false)
	: pImpl(std::make_unique<Impl>(this))
{
}


// Move constructor.
WinKeyboard::WinKeyboard(WinKeyboard&& moveFrom) noexcept
	: pImpl(std::move(moveFrom.pImpl))
{
	pImpl->mOwner = this;
}


// Move assignment.
WinKeyboard& WinKeyboard::operator= (WinKeyboard&& moveFrom) noexcept
{
	pImpl = std::move(moveFrom.pImpl);
	pImpl->mOwner = this;
	return *this;
}


// Public destructor.
WinKeyboard::~WinKeyboard()
{
}


WinKeyboard::State WinKeyboard::GetState() const
{
	State state;
	pImpl->GetState(state);
	return state;
}


void WinKeyboard::Reset()
{
	pImpl->Reset();
}


bool WinKeyboard::IsConnected() const
{
	return pImpl->IsConnected();
}

WinKeyboard* WinKeyboard::Get()
{
	if (!Impl::s_WinKeyboard || !Impl::s_WinKeyboard->mOwner)
		return nullptr;

	return Impl::s_WinKeyboard->mOwner;
}



//======================================================================================
// WinKeyboardStateTracker
//======================================================================================

void WinKeyboard::KeyboardStateTracker::Update(const State& state)
{
	auto currPtr = reinterpret_cast<const uint32_t*>(&state);
	auto prevPtr = reinterpret_cast<const uint32_t*>(&lastState);
	auto releasedPtr = reinterpret_cast<uint32_t*>(&released);
	auto pressedPtr = reinterpret_cast<uint32_t*>(&pressed);
	for (size_t j = 0; j < (256 / 32); ++j)
	{
		*pressedPtr = *currPtr & ~(*prevPtr);
		*releasedPtr = ~(*currPtr) & *prevPtr;

		++currPtr;
		++prevPtr;
		++releasedPtr;
		++pressedPtr;
	}

	lastState = state;
}


void WinKeyboard::KeyboardStateTracker::Reset() noexcept
{
	memset(this, 0, sizeof(KeyboardStateTracker));
}

#pragma endregion WinKeyboard

namespace
{
	WinMouse::State s_MouseState;
	WinMouse::State s_LastMouseState;
    WinMouse::ButtonStateTracker s_MouseTracker;
	WinKeyboard::State s_KeyboardState;
    WinKeyboard::KeyboardStateTracker s_KeyboardTracker;
	bool s_IsMouseModeChanged = true;
	float s_MouseButtonHoldTimes[5] = {};
	float s_KeyboardButtonHoldTimes[256] = {};
}

namespace GameInput
{
	std::unique_ptr<WinMouse> g_pMouse;
	std::unique_ptr<WinKeyboard> g_pKeyboard;

	namespace Mouse
	{
		bool IsPressed(Button button)
		{
			switch (button)
			{
			case GameInput::Mouse::LEFT_BUTTON: return s_MouseState.leftButton;
			case GameInput::Mouse::RIGHT_BUTTON: return s_MouseState.rightButton;
			case GameInput::Mouse::MIDDLE_BUTTON: return s_MouseState.middleButton;
			case GameInput::Mouse::XBUTTON_1: return s_MouseState.xButton1;
			case GameInput::Mouse::XBUTTON_2: return s_MouseState.xButton2;
			default: return false;
			}
		}

		bool IsFirstPressed(Button button)
		{
			switch (button)
			{
			case GameInput::Mouse::LEFT_BUTTON: return s_MouseTracker.leftButton == WinMouse::ButtonStateTracker::PRESSED;
			case GameInput::Mouse::RIGHT_BUTTON: return s_MouseTracker.rightButton == WinMouse::ButtonStateTracker::PRESSED;
			case GameInput::Mouse::MIDDLE_BUTTON: return s_MouseTracker.middleButton == WinMouse::ButtonStateTracker::PRESSED;
			case GameInput::Mouse::XBUTTON_1: return s_MouseTracker.xButton1 == WinMouse::ButtonStateTracker::PRESSED;
			case GameInput::Mouse::XBUTTON_2: return s_MouseTracker.xButton2 == WinMouse::ButtonStateTracker::PRESSED;
			default: return false;
			}
		}

		bool IsReleased(Button button)
		{
			return !IsPressed(button);
		}

		bool IsFirstReleased(Button button)
		{
			switch (button)
			{
			case GameInput::Mouse::LEFT_BUTTON: return s_MouseTracker.leftButton == WinMouse::ButtonStateTracker::RELEASED;
			case GameInput::Mouse::RIGHT_BUTTON: return s_MouseTracker.rightButton == WinMouse::ButtonStateTracker::RELEASED;
			case GameInput::Mouse::MIDDLE_BUTTON: return s_MouseTracker.middleButton == WinMouse::ButtonStateTracker::RELEASED;
			case GameInput::Mouse::XBUTTON_1: return s_MouseTracker.xButton1 == WinMouse::ButtonStateTracker::RELEASED;
			case GameInput::Mouse::XBUTTON_2: return s_MouseTracker.xButton2 == WinMouse::ButtonStateTracker::RELEASED;
			default: return false;
			}
		}

		void SetMode(Mode mode)
		{
			g_pMouse->SetMode(mode);
		}

		int GetX()
		{
			return s_MouseState.x;
		}

		int GetY()
		{
			return s_MouseState.y;
		}

		int GetScrollWheel()
		{
			if (s_MouseState.positionMode == Mode::MODE_ABSOLUTE)
			{
				return -(s_MouseState.scrollWheelValue - s_LastMouseState.scrollWheelValue) / 120;
			}
			else
			{
				return -s_MouseState.scrollWheelValue / 120;
			}
		}

		float GetDurationPressed(Button button)
		{
			return s_MouseButtonHoldTimes[button];
		}
	}

	namespace Keyboard
	{
		bool IsPressed(Keys key)
		{
			return s_KeyboardState.IsKeyDown(key);
		}

		bool IsFirstPressed(Keys key)
		{
			return s_KeyboardTracker.IsKeyPressed(key);
		}

		bool IsReleased(Keys key)
		{
			return s_KeyboardState.IsKeyUp(key);
		}

		bool IsFirstReleased(Keys key)
		{
			return s_KeyboardTracker.IsKeyReleased(key);
		}

		float GetDurationPressed(Keys key)
		{
			return s_KeyboardButtonHoldTimes[key];
		}
	}

	namespace Internal
	{
		namespace Mouse
		{
			void __cdecl ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
			{
				g_pMouse->ProcessMessage(message, wParam, lParam);
			}
		}

		namespace Keyboard
		{
			void __cdecl ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
			{
				g_pKeyboard->ProcessMessage(message, wParam, lParam);
			}
		}
	}
}

void GameInput::Initialize()
{
	if (!g_pMouse)
	{
		g_pMouse = std::make_unique<WinMouse>();
		g_pKeyboard = std::make_unique<WinKeyboard>();
	}
}

void GameInput::Update(float dt)
{
	s_LastMouseState = s_MouseState;

	s_MouseState = g_pMouse->GetState();
	s_KeyboardState = g_pKeyboard->GetState();

	if (s_IsMouseModeChanged)
	{
		s_IsMouseModeChanged = false;
		s_MouseTracker.Update(s_MouseState);
	}

	s_MouseTracker.Update(s_MouseState);
	s_KeyboardTracker.Update(s_KeyboardState);

	if (s_MouseTracker.leftButton == WinMouse::ButtonStateTracker::HELD) s_MouseButtonHoldTimes[0] += dt;
	if (s_MouseTracker.rightButton == WinMouse::ButtonStateTracker::HELD) s_MouseButtonHoldTimes[1] += dt;
	if (s_MouseTracker.middleButton == WinMouse::ButtonStateTracker::HELD) s_MouseButtonHoldTimes[2] += dt;
	if (s_MouseTracker.xButton1 == WinMouse::ButtonStateTracker::HELD) s_MouseButtonHoldTimes[3] += dt;
	if (s_MouseTracker.xButton2 == WinMouse::ButtonStateTracker::HELD) s_MouseButtonHoldTimes[4] += dt;

	if (s_MouseTracker.leftButton == WinMouse::ButtonStateTracker::RELEASED) s_MouseButtonHoldTimes[0] = 0.0f;
	if (s_MouseTracker.rightButton == WinMouse::ButtonStateTracker::RELEASED) s_MouseButtonHoldTimes[1] = 0.0f;
	if (s_MouseTracker.middleButton == WinMouse::ButtonStateTracker::RELEASED) s_MouseButtonHoldTimes[2] = 0.0f;
	if (s_MouseTracker.xButton1 == WinMouse::ButtonStateTracker::RELEASED) s_MouseButtonHoldTimes[3] = 0.0f;
	if (s_MouseTracker.xButton2 == WinMouse::ButtonStateTracker::RELEASED) s_MouseButtonHoldTimes[4] = 0.0f;

	for (int i = 0; i <= 0xff; ++i)
	{
		if (s_KeyboardState.IsKeyDown((GameInput::Keyboard::Keys)i))
			s_KeyboardButtonHoldTimes[i] += dt;
		else if (s_KeyboardTracker.IsKeyReleased((GameInput::Keyboard::Keys)i))
			s_KeyboardButtonHoldTimes[i] = 0.0f;
	}
}



