#include "d3dUtil.h"
#include <Graphics/MainWindow.h>
#include <Graphics/Renderer.h>
#include <Utils/GameInput.h>
#include "DXTrace.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	try {
#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

		Renderer* pRenderer = GetRenderer();
		MainWindow win(hInstance, L"Mini XEngine11", 1280, 720);
		if (!win.Initialize(pRenderer))
			return EXIT_FAILURE;
		return win.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
	}
	catch (std::exception& e)
	{
		std::wstring wstr = UTF8ToUCS2(e.what());
		MessageBox(nullptr, wstr.c_str(), L"Exception", MB_OK);
	}
	return EXIT_FAILURE;
}

int main(int argc, char* argv[])
{
	try {
#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

		Renderer* pRenderer = GetRenderer();
		MainWindow win(GetModuleHandle(nullptr), L"Mini XEngine11", 1280, 720);
		if (!win.Initialize(pRenderer))
			return EXIT_FAILURE;
		return win.Run();
	}
	catch (DxException& e)
	{
		wprintf(L"HR Failed\n%ls", e.ToString().c_str());
	}
	catch (std::exception& e)
	{
		std::wstring wstr = UTF8ToUCS2(e.what());
		wprintf(L"Exception\n%ls", wstr.c_str());
	}
	return EXIT_FAILURE;
}

namespace
{
	MainWindow* s_pWin = nullptr;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return s_pWin->MainWndProc(hwnd, msg, wParam, lParam);
}

MainWindow::MainWindow(HINSTANCE hInstance, std::wstring winName, int startWidth, int startHeight)
	: m_hInstance(hInstance), m_WinName(winName), m_ClientWidth(startWidth), m_ClientHeight(startHeight)
{
	if (s_pWin)
		throw std::exception("MainWindow is a singleton!");
	s_pWin = this;
}

MainWindow::~MainWindow()
{
	if (m_pGraphicsCore)
	{
		delete m_pGraphicsCore;
		m_pGraphicsCore = nullptr;
	}
}

bool MainWindow::Initialize(Renderer* pRenderer)
{
	m_pRenderer = pRenderer;
	if (m_pGraphicsCore)
		delete m_pGraphicsCore;
	m_pGraphicsCore = new GraphicsCore;
	m_pGraphicsCore->hInstance = m_hInstance;
	m_pGraphicsCore->winName = m_WinName;
	InitMainWindow();
	GameInput::Initialize();
	if (!m_pRenderer->_Initialize(m_pGraphicsCore))	// internal def
		return false;
	if (!m_pRenderer->Initialize(m_pGraphicsCore))	// user def
		return false;
	OnResize();
	return true;
}

int MainWindow::Run()
{
	MSG msg = { 0 };

	m_Timer.Reset();
	while (msg.message != WM_QUIT)
	{

		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else
		{
			m_Timer.Tick();
			if (!m_pGraphicsCore->isAppPaused)
			{
				CalculateFrameStats();
				float deltaTime = m_Timer.DeltaTime();
				GameInput::Update(deltaTime);
				m_pRenderer->Update(deltaTime);		// user def
				m_pRenderer->_Update(deltaTime);	// internal def

				m_pRenderer->_BeginNewFrame(m_pGraphicsCore);	// internal def
				m_pRenderer->BeginNewFrame(m_pGraphicsCore);	// user def

				m_pRenderer->DrawScene(m_pGraphicsCore);

				m_pRenderer->EndFrame(m_pGraphicsCore);		// user def
				m_pRenderer->_EndFrame(m_pGraphicsCore);	// internal def
			}
			else
			{
				Sleep(100);
			}
		}
	}

	m_pRenderer->Cleanup(m_pGraphicsCore);

	return (int)msg.wParam;
}


LRESULT MainWindow::MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			if (m_pGraphicsCore)
				m_pGraphicsCore->isAppPaused = true;
			m_Timer.Stop();
		}
		else
		{
			if (m_pGraphicsCore)
				m_pGraphicsCore->isAppPaused = false;
			m_Timer.Start();
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		m_pGraphicsCore->clientWidth = LOWORD(lParam);
		m_pGraphicsCore->clientHeight = HIWORD(lParam);
		if (m_pGraphicsCore->device)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				m_pGraphicsCore->isAppPaused = true;
				m_pGraphicsCore->isMinimized = true;
				m_pGraphicsCore->isMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				m_pGraphicsCore->isAppPaused = false;
				m_pGraphicsCore->isMinimized = false;
				m_pGraphicsCore->isMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (m_pGraphicsCore->isMinimized)
				{
					m_pGraphicsCore->isAppPaused = false;
					m_pGraphicsCore->isMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (m_pGraphicsCore->isMaximized)
				{
					m_pGraphicsCore->isAppPaused = false;
					m_pGraphicsCore->isMaximized = false;
					OnResize();
				}
				else if (m_pGraphicsCore->isResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or m_pSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		m_pGraphicsCore->isAppPaused = true;
		m_pGraphicsCore->isResizing = true;
		m_Timer.Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		m_pGraphicsCore->isAppPaused = false;
		m_pGraphicsCore->isResizing = false;
		m_Timer.Start();
		OnResize();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

		// 监测这些键盘/鼠标事件
	case WM_INPUT:

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_XBUTTONDOWN:

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:

	case WM_MOUSEWHEEL:
	case WM_MOUSEHOVER:
	case WM_MOUSEMOVE:
		GameInput::Internal::Mouse::ProcessMessage(msg, wParam, lParam);
		return 0;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		GameInput::Internal::Keyboard::ProcessMessage(msg, wParam, lParam);
		return 0;

	case WM_ACTIVATEAPP:
		GameInput::Internal::Mouse::ProcessMessage(msg, wParam, lParam);
		GameInput::Internal::Keyboard::ProcessMessage(msg, wParam, lParam);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool MainWindow::InitMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = ::MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInstance;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"D3DWndClassName";

	if (!RegisterClass(&wc))
	{
		throw std::exception("RegisterClass Failed.");
	}

	// 将窗口调整到中心
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, m_ClientWidth, m_ClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	m_pGraphicsCore->hWindow = CreateWindow(L"D3DWndClassName", m_WinName.c_str(),
		WS_OVERLAPPEDWINDOW, (screenWidth - width) / 2, (screenHeight - height) / 2, width, height, 0, 0, m_hInstance, 0);

	if (!m_pGraphicsCore->hWindow)
	{
		throw std::exception("CreateWindow Failed.");
	}

	ShowWindow(m_pGraphicsCore->hWindow, SW_SHOW);
	UpdateWindow(m_pGraphicsCore->hWindow);

	return true;
}

void MainWindow::OnResize()
{
	m_pRenderer->_OnResize(m_pGraphicsCore);
	m_pRenderer->OnResize(m_pGraphicsCore);
}

void MainWindow::CalculateFrameStats()
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	if ((m_Timer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wostringstream outs;
		outs.precision(6);
		outs << m_pGraphicsCore->winName << L"    "
			<< L"FPS: " << fps << L"    "
			<< L"Frame Time: " << mspf << L" (ms)";
		SetWindowText(m_pGraphicsCore->hWindow, outs.str().c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}
