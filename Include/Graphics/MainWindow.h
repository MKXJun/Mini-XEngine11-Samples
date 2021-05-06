#pragma once

#include <minwindef.h>

#include <sstream>
#include <string>
#include <memory>
#include <Utils/GameTimer.h>

class Renderer;
class GraphicsCore;

class MainWindow
{
public:
	MainWindow(HINSTANCE hInstance, std::wstring winName, int startWidth, int startHeight);
	~MainWindow();

	bool Initialize(Renderer*);
	int Run();

	LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	bool InitMainWindow();

	void OnResize();

	void CalculateFrameStats();

	GameTimer m_Timer;
	Renderer* m_pRenderer = nullptr;
	GraphicsCore* m_pGraphicsCore = nullptr;

	HINSTANCE m_hInstance;
	std::wstring m_WinName;
	int m_ClientWidth;
	int m_ClientHeight;
};
