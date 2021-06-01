
#pragma once

#include <minwindef.h>

namespace Input
{
	enum Source
	{
		RawInput = 0,
		DirectInput
	};

	namespace Mouse
	{
		enum Mode
		{
			Absolute = 0,
			Relative,
		};

		enum Button
		{
			LeftButton = 0,
			RightButton,
			MiddleButton,
			XButton1,
			XButton2
		};
		
		bool IsPressed(Button button);
		bool IsFirstPressed(Button button);
		bool IsReleased(Button button);
		bool IsFirstReleased(Button button);

		void SetMode(Mode mode);
		Mode GetMode();

		// The actual position X
		int GetX();
		// The actual position Y
		int GetY();

		// The relative movement X
		float GetDeltaX();
		// The relative movement Y
		float GetDeltaY();

		int GetScrollWheel();

		float GetDurationPressed(Button button);
	}

	namespace Keyboard
	{
		enum Keys
		{
			Backspace,
			Tab,
			Enter,
			Pause,
			CapsLock,
			Escape,
			Space,
			PageUp,
			PageDown,
			End,
			Home,
			Left,
			Up,
			Right,
			Down,
			PrintScreen,
			Insert,
			Delete,
			D0,
			D1,
			D2,
			D3,
			D4,
			D5,
			D6,
			D7,
			D8,
			D9,
			A,
			B,
			C,
			D,
			E,
			F,
			G,
			H,
			I,
			J,
			K,
			L,
			M,
			N,
			O,
			P,
			Q,
			R,
			S,
			T,
			U,
			V,
			W,
			X,
			Y,
			Z,
			LeftWindows,
			RightWindows,

			NumPad0,
			NumPad1,
			NumPad2,
			NumPad3,
			NumPad4,
			NumPad5,
			NumPad6,
			NumPad7,
			NumPad8,
			NumPad9,
			Multiply,
			Add,
			Subtract,
			Decimal,
			Divide,

			F1,
			F2,
			F3,
			F4,
			F5,
			F6,
			F7,
			F8,
			F9,
			F10,
			F11,
			F12,

			NumLock,
			Scroll,

			LeftShift,
			RightShift,
			LeftControl,
			RightControl,
			LeftAlt,
			RightAlt,

			Semicolon,		// ;
			Equal,			// =
			Comma,			// ,
			Minus,			// -
			Period,			// . 
			Slash,			// /
			Grave,			// `

			LeftBracket,	// [
			RightBracket,	// ]
			Backslash		// \ 
		};

		bool IsPressed(Keys key);
		bool IsFirstPressed(Keys key);
		bool IsReleased(Keys key);
		bool IsFirstReleased(Keys key);

		float GetDurationPressed(Keys key);
	}

	namespace GamePad
	{

	}

	void Initialize(HWND hwnd, Source defaultSrc = RawInput, Mouse::Mode mode = Mouse::Absolute);
	// hwnd need to be init at first!
	void Initialize(Source defaultSrc = RawInput, Mouse::Mode mode = Mouse::Absolute);
	// For Mouse/Keyboard
	void SetSource(Source src);
	void Update(float dt);

	namespace Internal
	{
		namespace Mouse
		{
			void __cdecl ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);
		}
		
		namespace Keyboard
		{
			void __cdecl ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);
		}
	}
}