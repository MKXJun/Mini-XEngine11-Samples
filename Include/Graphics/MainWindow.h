#pragma once

#include <minwindef.h>

#include <sstream>
#include <string>
#include <memory>
#include <Utils/Timer.h>
#include <Graphics/RenderPipeline.h>
class MainWindow
{
public:
	MainWindow(HINSTANCE hInstance, std::wstring winName, int startWidth, int startHeight);
	~MainWindow();

	bool Initialize();
	void RegisterRenderPipeline(RenderPipeline* rp);
	int Run();


	LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	bool InitMainWindow();

	void CalculateFrameStats();

	HWND m_hWindow;
	HINSTANCE m_hInstance;
	std::wstring m_WinName;

	int m_ClientWidth;
	int m_ClientHeight;

	bool m_IsAppPaused = false;
	bool m_IsMinimized = false;
	bool m_IsMaximized = false;
	bool m_IsResizing = false;
};
