#pragma once

#include <minwindef.h>
#include <wrl/client.h>
#include <d3d11_1.h>

#include <string_view>



class GraphicsCore
{
public:
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;

	HWND hWindow = nullptr;
	HINSTANCE hInstance = nullptr;
	std::wstring winName;
	int clientWidth = 0;
	int clientHeight = 0;

	bool isAppPaused = false;
	bool isMinimized = false;
	bool isMaximized = false;
	bool isResizing = false;
};

class RendererBase
{
public:
	virtual bool Initialize(GraphicsCore*) = 0;
	virtual void Cleanup(GraphicsCore*) = 0;

	virtual void OnResize(GraphicsCore*) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void BeginNewFrame(GraphicsCore*) = 0;
	virtual void DrawScene(GraphicsCore*) = 0;
	virtual void EndFrame(GraphicsCore*) = 0;
	virtual ~RendererBase() {}
};

RendererBase* GetRenderer();