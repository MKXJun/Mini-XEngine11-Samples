
#include <map>
#include <cassert>
#include <exception>
#include <wrl/client.h>
// RawInput
#include "Mouse.h"
#include "Keyboard.h"
// DirectInput & XInput
#include "GameInput.h"

#include <Utils/Input.h>


#pragma warning(disable:26812)



namespace
{
	HWND s_hWnd;

	POINT s_MousePos;

	Input::Source s_Source;
	Input::Mouse::Mode s_MouseMode = Input::Mouse::Relative;

	std::unique_ptr<DirectX::Mouse> s_pMouse;
	std::unique_ptr<DirectX::Keyboard> s_pKeyboard;

	DirectX::Mouse::State s_MouseState;
	DirectX::Mouse::State s_LastMouseState;
	DirectX::Mouse::ButtonStateTracker s_MouseTracker;
	DirectX::Keyboard::State s_KeyboardState;
	DirectX::Keyboard::KeyboardStateTracker s_KeyboardTracker;
	bool s_IsMouseModeChanged = true;
	float s_MouseButtonHoldTimes[5] = {};
	float s_KeyboardButtonHoldTimes[256] = {};

	std::unique_ptr<int, void(*)(int*)> s_pGameInputDestroyer{ (int*)(-1), [](int* ptr) {
		GameInput::Shutdown();
	} };


	std::map<Input::Keyboard::Keys, DirectX::Keyboard::Keys> s_RawKeyMap{
		{ Input::Keyboard::Backspace, DirectX::Keyboard::Back },
		{ Input::Keyboard::Tab, DirectX::Keyboard::Tab },
		{ Input::Keyboard::Enter, DirectX::Keyboard::Enter },
		{ Input::Keyboard::Pause, DirectX::Keyboard::Pause },
		{ Input::Keyboard::CapsLock, DirectX::Keyboard::CapsLock },
		{ Input::Keyboard::Escape, DirectX::Keyboard::Escape },
		{ Input::Keyboard::Space, DirectX::Keyboard::Space },
		{ Input::Keyboard::PageUp, DirectX::Keyboard::PageUp },
		{ Input::Keyboard::PageDown, DirectX::Keyboard::PageDown },
		{ Input::Keyboard::End, DirectX::Keyboard::End },
		{ Input::Keyboard::Home, DirectX::Keyboard::Home },
		{ Input::Keyboard::Left, DirectX::Keyboard::Left },
		{ Input::Keyboard::Up, DirectX::Keyboard::Up },
		{ Input::Keyboard::Right, DirectX::Keyboard::Right },
		{ Input::Keyboard::Down, DirectX::Keyboard::Down },
		{ Input::Keyboard::PrintScreen, DirectX::Keyboard::PrintScreen },
		{ Input::Keyboard::Insert, DirectX::Keyboard::Insert },
		{ Input::Keyboard::Delete, DirectX::Keyboard::Delete },
		{ Input::Keyboard::D0, DirectX::Keyboard::D0 },
		{ Input::Keyboard::D1, DirectX::Keyboard::D1 },
		{ Input::Keyboard::D2, DirectX::Keyboard::D2 },
		{ Input::Keyboard::D3, DirectX::Keyboard::D3 },
		{ Input::Keyboard::D4, DirectX::Keyboard::D4 },
		{ Input::Keyboard::D5, DirectX::Keyboard::D5 },
		{ Input::Keyboard::D6, DirectX::Keyboard::D6 },
		{ Input::Keyboard::D7, DirectX::Keyboard::D7 },
		{ Input::Keyboard::D8, DirectX::Keyboard::D8 },
		{ Input::Keyboard::D9, DirectX::Keyboard::D9 },
		{ Input::Keyboard::A, DirectX::Keyboard::A },
		{ Input::Keyboard::B, DirectX::Keyboard::B },
		{ Input::Keyboard::C, DirectX::Keyboard::C },
		{ Input::Keyboard::D, DirectX::Keyboard::D },
		{ Input::Keyboard::E, DirectX::Keyboard::E },
		{ Input::Keyboard::F, DirectX::Keyboard::F },
		{ Input::Keyboard::G, DirectX::Keyboard::G },
		{ Input::Keyboard::H, DirectX::Keyboard::H },
		{ Input::Keyboard::I, DirectX::Keyboard::I },
		{ Input::Keyboard::J, DirectX::Keyboard::J },
		{ Input::Keyboard::K, DirectX::Keyboard::K },
		{ Input::Keyboard::L, DirectX::Keyboard::L },
		{ Input::Keyboard::M, DirectX::Keyboard::M },
		{ Input::Keyboard::N, DirectX::Keyboard::N },
		{ Input::Keyboard::O, DirectX::Keyboard::O },
		{ Input::Keyboard::P, DirectX::Keyboard::P },
		{ Input::Keyboard::Q, DirectX::Keyboard::Q },
		{ Input::Keyboard::R, DirectX::Keyboard::R },
		{ Input::Keyboard::S, DirectX::Keyboard::S },
		{ Input::Keyboard::T, DirectX::Keyboard::T },
		{ Input::Keyboard::U, DirectX::Keyboard::U },
		{ Input::Keyboard::V, DirectX::Keyboard::V },
		{ Input::Keyboard::W, DirectX::Keyboard::W },
		{ Input::Keyboard::X, DirectX::Keyboard::X },
		{ Input::Keyboard::Y, DirectX::Keyboard::Y },
		{ Input::Keyboard::Z, DirectX::Keyboard::Z },
		{ Input::Keyboard::NumPad0, DirectX::Keyboard::NumPad0 },
		{ Input::Keyboard::NumPad1, DirectX::Keyboard::NumPad1 },
		{ Input::Keyboard::NumPad2, DirectX::Keyboard::NumPad2 },
		{ Input::Keyboard::NumPad3, DirectX::Keyboard::NumPad3 },
		{ Input::Keyboard::NumPad4, DirectX::Keyboard::NumPad4 },
		{ Input::Keyboard::NumPad5, DirectX::Keyboard::NumPad5 },
		{ Input::Keyboard::NumPad6, DirectX::Keyboard::NumPad6 },
		{ Input::Keyboard::NumPad7, DirectX::Keyboard::NumPad7 },
		{ Input::Keyboard::NumPad8, DirectX::Keyboard::NumPad8 },
		{ Input::Keyboard::NumPad9, DirectX::Keyboard::NumPad9 },
		{ Input::Keyboard::F1, DirectX::Keyboard::F1 },
		{ Input::Keyboard::F2, DirectX::Keyboard::F2 },
		{ Input::Keyboard::F3, DirectX::Keyboard::F3 },
		{ Input::Keyboard::F4, DirectX::Keyboard::F4 },
		{ Input::Keyboard::F5, DirectX::Keyboard::F5 },
		{ Input::Keyboard::F6, DirectX::Keyboard::F6 },
		{ Input::Keyboard::F7, DirectX::Keyboard::F7 },
		{ Input::Keyboard::F8, DirectX::Keyboard::F8 },
		{ Input::Keyboard::F9, DirectX::Keyboard::F9 },
		{ Input::Keyboard::F10, DirectX::Keyboard::F10 },
		{ Input::Keyboard::F11, DirectX::Keyboard::F11 },
		{ Input::Keyboard::F12, DirectX::Keyboard::F12 },
		{ Input::Keyboard::Multiply, DirectX::Keyboard::Multiply },
		{ Input::Keyboard::Add, DirectX::Keyboard::Add },
		{ Input::Keyboard::Subtract, DirectX::Keyboard::Subtract },
		{ Input::Keyboard::Decimal, DirectX::Keyboard::Decimal },
		{ Input::Keyboard::Divide, DirectX::Keyboard::Divide },
		{ Input::Keyboard::NumLock, DirectX::Keyboard::NumLock },
		{ Input::Keyboard::Scroll, DirectX::Keyboard::Scroll },
		{ Input::Keyboard::LeftShift, DirectX::Keyboard::LeftShift },
		{ Input::Keyboard::RightShift, DirectX::Keyboard::RightShift },
		{ Input::Keyboard::LeftControl, DirectX::Keyboard::LeftControl },
		{ Input::Keyboard::RightControl, DirectX::Keyboard::RightControl },
		{ Input::Keyboard::LeftAlt, DirectX::Keyboard::LeftAlt },
		{ Input::Keyboard::RightAlt, DirectX::Keyboard::RightAlt },
		{ Input::Keyboard::Semicolon, DirectX::Keyboard::OemSemicolon },
		{ Input::Keyboard::Equal, DirectX::Keyboard::OemPlus },
		{ Input::Keyboard::Comma, DirectX::Keyboard::OemComma },
		{ Input::Keyboard::Minus, DirectX::Keyboard::OemMinus },
		{ Input::Keyboard::Period, DirectX::Keyboard::OemPeriod },
		{ Input::Keyboard::Slash, DirectX::Keyboard::OemQuestion },
		{ Input::Keyboard::Grave, DirectX::Keyboard::OemTilde },
		{ Input::Keyboard::LeftBracket, DirectX::Keyboard::OemOpenBrackets },
		{ Input::Keyboard::RightBracket, DirectX::Keyboard::OemCloseBrackets },
		{ Input::Keyboard::Backslash, DirectX::Keyboard::OemBackslash },
	};

	std::map<Input::Keyboard::Keys, GameInput::DigitalInput> s_DInputKeyMap{
		{ Input::Keyboard::Backspace, GameInput::kKey_back },
		{ Input::Keyboard::Tab, GameInput::kKey_tab },
		{ Input::Keyboard::Enter, GameInput::kKey_return },
		{ Input::Keyboard::Pause, GameInput::kKey_pause },
		{ Input::Keyboard::CapsLock, GameInput::kKey_capital },
		{ Input::Keyboard::Escape, GameInput::kKey_escape },
		{ Input::Keyboard::Space, GameInput::kKey_space },
		{ Input::Keyboard::PageUp, GameInput::kKey_pgup },
		{ Input::Keyboard::PageDown, GameInput::kKey_pgdn },
		{ Input::Keyboard::End, GameInput::kKey_end },
		{ Input::Keyboard::Home, GameInput::kKey_home },
		{ Input::Keyboard::Left, GameInput::kKey_left },
		{ Input::Keyboard::Up, GameInput::kKey_up },
		{ Input::Keyboard::Right, GameInput::kKey_right },
		{ Input::Keyboard::Down, GameInput::kKey_down },
		{ Input::Keyboard::Insert, GameInput::kKey_insert },
		{ Input::Keyboard::Delete, GameInput::kKey_delete },
		{ Input::Keyboard::D0, GameInput::kKey_0 },
		{ Input::Keyboard::D1, GameInput::kKey_1 },
		{ Input::Keyboard::D2, GameInput::kKey_2 },
		{ Input::Keyboard::D3, GameInput::kKey_3 },
		{ Input::Keyboard::D4, GameInput::kKey_4 },
		{ Input::Keyboard::D5, GameInput::kKey_5 },
		{ Input::Keyboard::D6, GameInput::kKey_6 },
		{ Input::Keyboard::D7, GameInput::kKey_7 },
		{ Input::Keyboard::D8, GameInput::kKey_8 },
		{ Input::Keyboard::D9, GameInput::kKey_9 },
		{ Input::Keyboard::A, GameInput::kKey_a },
		{ Input::Keyboard::B, GameInput::kKey_b },
		{ Input::Keyboard::C, GameInput::kKey_c },
		{ Input::Keyboard::D, GameInput::kKey_d },
		{ Input::Keyboard::E, GameInput::kKey_e },
		{ Input::Keyboard::F, GameInput::kKey_f },
		{ Input::Keyboard::G, GameInput::kKey_g },
		{ Input::Keyboard::H, GameInput::kKey_h },
		{ Input::Keyboard::I, GameInput::kKey_i },
		{ Input::Keyboard::J, GameInput::kKey_j },
		{ Input::Keyboard::K, GameInput::kKey_k },
		{ Input::Keyboard::L, GameInput::kKey_l },
		{ Input::Keyboard::M, GameInput::kKey_m },
		{ Input::Keyboard::N, GameInput::kKey_n },
		{ Input::Keyboard::O, GameInput::kKey_o },
		{ Input::Keyboard::P, GameInput::kKey_p },
		{ Input::Keyboard::Q, GameInput::kKey_q },
		{ Input::Keyboard::R, GameInput::kKey_r },
		{ Input::Keyboard::S, GameInput::kKey_s },
		{ Input::Keyboard::T, GameInput::kKey_t },
		{ Input::Keyboard::U, GameInput::kKey_u },
		{ Input::Keyboard::V, GameInput::kKey_v },
		{ Input::Keyboard::W, GameInput::kKey_w },
		{ Input::Keyboard::X, GameInput::kKey_x },
		{ Input::Keyboard::Y, GameInput::kKey_y },
		{ Input::Keyboard::Z, GameInput::kKey_z },
		{ Input::Keyboard::NumPad0, GameInput::kKey_numpad0 },
		{ Input::Keyboard::NumPad1, GameInput::kKey_numpad1 },
		{ Input::Keyboard::NumPad2, GameInput::kKey_numpad2 },
		{ Input::Keyboard::NumPad3, GameInput::kKey_numpad3 },
		{ Input::Keyboard::NumPad4, GameInput::kKey_numpad4 },
		{ Input::Keyboard::NumPad5, GameInput::kKey_numpad5 },
		{ Input::Keyboard::NumPad6, GameInput::kKey_numpad6 },
		{ Input::Keyboard::NumPad7, GameInput::kKey_numpad7 },
		{ Input::Keyboard::NumPad8, GameInput::kKey_numpad8 },
		{ Input::Keyboard::NumPad9, GameInput::kKey_numpad9 },
		{ Input::Keyboard::F1, GameInput::kKey_f1 },
		{ Input::Keyboard::F2, GameInput::kKey_f2 },
		{ Input::Keyboard::F3, GameInput::kKey_f3 },
		{ Input::Keyboard::F4, GameInput::kKey_f4 },
		{ Input::Keyboard::F5, GameInput::kKey_f5 },
		{ Input::Keyboard::F6, GameInput::kKey_f6 },
		{ Input::Keyboard::F7, GameInput::kKey_f7 },
		{ Input::Keyboard::F8, GameInput::kKey_f8 },
		{ Input::Keyboard::F9, GameInput::kKey_f9 },
		{ Input::Keyboard::F10, GameInput::kKey_f10 },
		{ Input::Keyboard::F11, GameInput::kKey_f11 },
		{ Input::Keyboard::F12, GameInput::kKey_f12 },
		{ Input::Keyboard::Multiply, GameInput::kKey_multiply },
		{ Input::Keyboard::Add, GameInput::kKey_add },
		{ Input::Keyboard::Subtract, GameInput::kKey_subtract },
		{ Input::Keyboard::Decimal, GameInput::kKey_decimal },
		{ Input::Keyboard::Divide, GameInput::kKey_divide },
		{ Input::Keyboard::NumLock, GameInput::kKey_numlock },
		{ Input::Keyboard::Scroll, GameInput::kKey_scroll },
		{ Input::Keyboard::LeftShift, GameInput::kKey_lshift },
		{ Input::Keyboard::RightShift, GameInput::kKey_rshift },
		{ Input::Keyboard::LeftControl, GameInput::kKey_lcontrol },
		{ Input::Keyboard::RightControl, GameInput::kKey_rcontrol },
		{ Input::Keyboard::LeftAlt, GameInput::kKey_lalt },
		{ Input::Keyboard::RightAlt, GameInput::kKey_ralt },
		{ Input::Keyboard::Semicolon, GameInput::kKey_semicolon },
		{ Input::Keyboard::Equal, GameInput::kKey_equals },
		{ Input::Keyboard::Comma, GameInput::kKey_comma },
		{ Input::Keyboard::Minus, GameInput::kKey_minus },
		{ Input::Keyboard::Period, GameInput::kKey_period },
		{ Input::Keyboard::Slash, GameInput::kKey_slash },
		{ Input::Keyboard::Grave, GameInput::kKey_grave },
		{ Input::Keyboard::LeftBracket, GameInput::kKey_lbracket },
		{ Input::Keyboard::Slash, GameInput::kKey_slash },
		{ Input::Keyboard::RightBracket, GameInput::kKey_rbracket },
		{ Input::Keyboard::Backslash, GameInput::kKey_backslash }
	};
}


namespace Input
{
	namespace Mouse
	{
		bool IsPressed(Button button)
		{
			if (s_Source == Input::RawInput)
			{
				switch (button)
				{
				case Input::Mouse::LeftButton: return s_MouseState.leftButton;
				case Input::Mouse::RightButton: return s_MouseState.rightButton;
				case Input::Mouse::MiddleButton: return s_MouseState.middleButton;
				case Input::Mouse::XButton1: return s_MouseState.xButton1;
				case Input::Mouse::XButton2: return s_MouseState.xButton2;
				default: return false;
				}
			}
			else
			{
				switch (button)
				{
				case Input::Mouse::LeftButton: return GameInput::IsPressed(GameInput::kMouse0);
				case Input::Mouse::RightButton: return GameInput::IsPressed(GameInput::kMouse1);
				case Input::Mouse::MiddleButton: return GameInput::IsPressed(GameInput::kMouse2);
				case Input::Mouse::XButton1: return GameInput::IsPressed(GameInput::kMouse3);
				case Input::Mouse::XButton2: return GameInput::IsPressed(GameInput::kMouse4);
				default: return false;
				}
			}
		}

		bool IsFirstPressed(Button button)
		{
			if (s_Source == Input::RawInput)
			{
				switch (button)
				{
				case Input::Mouse::LeftButton: return s_MouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::PRESSED;
				case Input::Mouse::RightButton: return s_MouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::PRESSED;
				case Input::Mouse::MiddleButton: return s_MouseTracker.middleButton == DirectX::Mouse::ButtonStateTracker::PRESSED;
				case Input::Mouse::XButton1: return s_MouseTracker.xButton1 == DirectX::Mouse::ButtonStateTracker::PRESSED;
				case Input::Mouse::XButton2: return s_MouseTracker.xButton2 == DirectX::Mouse::ButtonStateTracker::PRESSED;
				default: return false;
				}
			}
			else
			{
				switch (button)
				{
				case Input::Mouse::LeftButton: return GameInput::IsFirstPressed(GameInput::kMouse0);
				case Input::Mouse::RightButton: return GameInput::IsFirstPressed(GameInput::kMouse1);
				case Input::Mouse::MiddleButton: return GameInput::IsFirstPressed(GameInput::kMouse2);
				case Input::Mouse::XButton1: return GameInput::IsFirstPressed(GameInput::kMouse3);
				case Input::Mouse::XButton2: return GameInput::IsFirstPressed(GameInput::kMouse4);
				default: return false;
				}
			}
		}

		bool IsReleased(Button button)
		{
			return !IsPressed(button);
		}

		bool IsFirstReleased(Button button)
		{
			if (s_Source == Input::RawInput)
			{
				switch (button)
				{
				case Input::Mouse::LeftButton: return s_MouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::RELEASED;
				case Input::Mouse::RightButton: return s_MouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::RELEASED;
				case Input::Mouse::MiddleButton: return s_MouseTracker.middleButton == DirectX::Mouse::ButtonStateTracker::RELEASED;
				case Input::Mouse::XButton1: return s_MouseTracker.xButton1 == DirectX::Mouse::ButtonStateTracker::RELEASED;
				case Input::Mouse::XButton2: return s_MouseTracker.xButton2 == DirectX::Mouse::ButtonStateTracker::RELEASED;
				default: return false;
				}
			}
			else
			{
				switch (button)
				{
				case Input::Mouse::LeftButton: return GameInput::IsFirstReleased(GameInput::kMouse0);
				case Input::Mouse::RightButton: return GameInput::IsFirstReleased(GameInput::kMouse1);
				case Input::Mouse::MiddleButton: return GameInput::IsFirstReleased(GameInput::kMouse2);
				case Input::Mouse::XButton1: return GameInput::IsFirstReleased(GameInput::kMouse3);
				case Input::Mouse::XButton2: return GameInput::IsFirstReleased(GameInput::kMouse4);
				default: return false;
				}
			}
		}

		void SetMode(Mode mode)
		{
			s_IsMouseModeChanged = (s_MouseMode != mode);
			s_MouseMode = mode;
			if (s_Source == Source::RawInput)
			{
				s_pMouse->SetMode(mode == Mode::Absolute ? DirectX::Mouse::MODE_ABSOLUTE : DirectX::Mouse::MODE_RELATIVE);
				s_pMouse->SetVisible(mode == Mode::Absolute);
			}
			else
			{
				GameInput::Shutdown();
				GameInput::Initialize(s_hWnd, s_MouseMode == Mouse::Relative);
			}
		}

		Mode GetMode()
		{
			return s_MouseMode;
		}

		int GetX()
		{
			if (s_MouseMode == Mouse::Absolute)
				return s_MouseState.x;
			else
				return s_MousePos.x;
		}

		int GetY()
		{
			if (s_MouseState.positionMode == DirectX::Mouse::MODE_ABSOLUTE)
				return s_MouseState.y;
			else
				return s_MousePos.y;
		}

		float GetDeltaX()
		{
			if (s_Source == Source::DirectInput)
				return GameInput::GetAnalogInput(GameInput::kAnalogMouseX);
			else if (s_MouseMode == Mode::Absolute)
				return s_MouseState.x - s_LastMouseState.x;
			else
				return s_MouseState.x;
		}

		float GetDeltaY()
		{
			if (s_Source == Source::DirectInput)
				return GameInput::GetAnalogInput(GameInput::kAnalogMouseY);
			else if (s_MouseMode == Mode::Absolute)
				return (float)(s_MouseState.y - s_LastMouseState.y);
			else
				return (float)s_MouseState.y;
		}

		int GetScrollWheel()
		{
			if (s_MouseState.positionMode == Mode::Absolute)
			{
				return -(s_MouseState.scrollWheelValue - s_LastMouseState.scrollWheelValue) / 120;
			}
			else if (s_Source == Source::RawInput)
			{
				return -s_MouseState.scrollWheelValue / 120;
			}
			else
			{
				return (int)GameInput::GetTimeCorrectedAnalogInput(GameInput::kAnalogMouseScroll);
			}
		}

		float GetDurationPressed(Button button)
		{
			if (s_Source == Source::RawInput)
				return s_MouseButtonHoldTimes[button];
			else
			{
				switch (button)
				{
				case Input::Mouse::LeftButton: return GameInput::GetDurationPressed(GameInput::kMouse0);
				case Input::Mouse::RightButton: return GameInput::GetDurationPressed(GameInput::kMouse1);
				case Input::Mouse::MiddleButton: return GameInput::GetDurationPressed(GameInput::kMouse2);
				case Input::Mouse::XButton1: return GameInput::GetDurationPressed(GameInput::kMouse3);
				case Input::Mouse::XButton2: return GameInput::GetDurationPressed(GameInput::kMouse4);
				default: return false;
				}
			}
		}
	}

	namespace Keyboard
	{
		bool IsPressed(Keys key)
		{
			if (s_Source == Source::RawInput)
				return s_KeyboardState.IsKeyDown(s_RawKeyMap[key]);
			else
				return GameInput::IsPressed(s_DInputKeyMap[key]);
		}

		bool IsFirstPressed(Keys key)
		{
			if (s_Source == Source::RawInput)
				return s_KeyboardTracker.IsKeyPressed(s_RawKeyMap[key]);
			else
				return GameInput::IsFirstPressed(s_DInputKeyMap[key]);
		}

		bool IsReleased(Keys key)
		{
			return !IsPressed(key);
		}

		bool IsFirstReleased(Keys key)
		{
			if (s_Source == Source::RawInput)
				return s_KeyboardTracker.IsKeyReleased(s_RawKeyMap[key]);
			else
				return GameInput::IsFirstReleased(s_DInputKeyMap[key]);
		}

		float GetDurationPressed(Keys key)
		{
			if (s_Source == Source::RawInput)
				return s_KeyboardButtonHoldTimes[s_RawKeyMap[key]];
			else
				return GameInput::GetDurationPressed(s_DInputKeyMap[key]);
		}
	}

	namespace Internal
	{
		namespace Mouse
		{
			void __cdecl ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
			{
				if (s_pMouse)
					s_pMouse->ProcessMessage(message, wParam, lParam);
			}
		}

		namespace Keyboard
		{
			void __cdecl ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
			{
				if (s_pKeyboard)
					s_pKeyboard->ProcessMessage(message, wParam, lParam);
			}
		}
	}
}

void Input::Initialize(HWND hWnd, Source defaultSrc, Mouse::Mode mode)
{
	s_hWnd = hWnd;
	Initialize(defaultSrc, mode);
}

void Input::Initialize(Source defaultSrc, Mouse::Mode mode)
{
	s_Source = defaultSrc;
	if (defaultSrc == Source::DirectInput)
	{
		GameInput::Shutdown();
		GameInput::Initialize(s_hWnd, s_MouseMode == Mouse::Relative);
	}
	else
	{
		s_pMouse = nullptr;
		s_pKeyboard = nullptr;
		s_pMouse = std::make_unique<DirectX::Mouse>();
		s_pKeyboard = std::make_unique<DirectX::Keyboard>();
		s_pMouse->SetWindow(s_hWnd);
	}
	Input::Mouse::SetMode(mode);
}

void Input::SetSource(Source src)
{
	if (src == Source::DirectInput && s_Source == Source::RawInput)
	{
		s_pKeyboard = nullptr;
		s_pMouse = nullptr;
		GameInput::Initialize(s_hWnd, s_MouseMode == Mouse::Relative);
	}
	else if (src == Source::RawInput && s_Source == Source::DirectInput)
	{
		GameInput::Shutdown();
		s_pMouse = std::make_unique<DirectX::Mouse>();
		s_pKeyboard = std::make_unique<DirectX::Keyboard>();
	}
	s_Source = src;
}

void Input::Update(float dt)
{
	GetCursorPos(&s_MousePos);
	ScreenToClient(s_hWnd, &s_MousePos);
	if (s_Source == Source::DirectInput)
	{
		GameInput::Update(dt);
	}
	else
	{
		s_LastMouseState = s_MouseState;

		s_MouseState = s_pMouse->GetState();
		s_KeyboardState = s_pKeyboard->GetState();

		if (s_IsMouseModeChanged)
		{
			s_IsMouseModeChanged = false;
			s_MouseTracker.Update(s_MouseState);
		}

		s_MouseTracker.Update(s_MouseState);
		s_KeyboardTracker.Update(s_KeyboardState);

		if (s_MouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::HELD) s_MouseButtonHoldTimes[0] += dt;
		if (s_MouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::HELD) s_MouseButtonHoldTimes[1] += dt;
		if (s_MouseTracker.middleButton == DirectX::Mouse::ButtonStateTracker::HELD) s_MouseButtonHoldTimes[2] += dt;
		if (s_MouseTracker.xButton1 == DirectX::Mouse::ButtonStateTracker::HELD) s_MouseButtonHoldTimes[3] += dt;
		if (s_MouseTracker.xButton2 == DirectX::Mouse::ButtonStateTracker::HELD) s_MouseButtonHoldTimes[4] += dt;

		if (s_MouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::RELEASED) s_MouseButtonHoldTimes[0] = 0.0f;
		if (s_MouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::RELEASED) s_MouseButtonHoldTimes[1] = 0.0f;
		if (s_MouseTracker.middleButton == DirectX::Mouse::ButtonStateTracker::RELEASED) s_MouseButtonHoldTimes[2] = 0.0f;
		if (s_MouseTracker.xButton1 == DirectX::Mouse::ButtonStateTracker::RELEASED) s_MouseButtonHoldTimes[3] = 0.0f;
		if (s_MouseTracker.xButton2 == DirectX::Mouse::ButtonStateTracker::RELEASED) s_MouseButtonHoldTimes[4] = 0.0f;

		for (int i = 0; i <= 0xff; ++i)
		{
			if (s_KeyboardState.IsKeyDown(s_RawKeyMap[static_cast<Input::Keyboard::Keys>(i)]))
				s_KeyboardButtonHoldTimes[i] += dt;
			else if (s_KeyboardTracker.IsKeyReleased(s_RawKeyMap[static_cast<Input::Keyboard::Keys>(i)]))
				s_KeyboardButtonHoldTimes[i] = 0.0f;
		}
	}
	

	

	
}



