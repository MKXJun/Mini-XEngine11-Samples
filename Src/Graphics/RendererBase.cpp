#include <d3d11_1.h>
#include <wrl/client.h>
#include <Graphics/RendererBase.h>
#include <Graphics/ResourceManager.h>
#include <Utils/GameInput.h>
#include <Utils/GameTimer.h>
#include <HLSL/XDefines.h>
#include "DXTrace.h"
#include "d3dUtil.h"

using namespace Microsoft::WRL;



bool RendererBase::Initialize(GraphicsCore* pCore)
{
	if (!pCore)
		return false;

	HRESULT hr = S_OK;

	// 创建D3D设备 和 D3D设备上下文
	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	// 驱动类型数组
	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	// 特性等级数组
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	D3D_FEATURE_LEVEL featureLevel;
	D3D_DRIVER_TYPE d3dDriverType;
	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		d3dDriverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, d3dDriverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, pCore->device.GetAddressOf(), &featureLevel, pCore->deviceContext.GetAddressOf());

		if (hr == E_INVALIDARG)
		{
			// Direct3D 11.0 的API不承认D3D_FEATURE_LEVEL_11_1，所以我们需要尝试特性等级11.0以及以下的版本
			hr = D3D11CreateDevice(nullptr, d3dDriverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, pCore->device.GetAddressOf(), &featureLevel, pCore->deviceContext.GetAddressOf());
		}

		if (SUCCEEDED(hr))
			break;
	}

	if (FAILED(hr))
	{
		throw std::exception("Error: D3D11CreateDevice Failed.");
	}

	// 检测是否支持特性等级11.0或11.1
	if (featureLevel != D3D_FEATURE_LEVEL_11_0 && featureLevel != D3D_FEATURE_LEVEL_11_1)
	{
		throw std::exception("Error: Direct3D Feature Level 11 unsupported.");
	}

	// 检测 MSAA支持的质量等级
	uint32_t msaaQuality;
	pCore->device->CheckMultisampleQualityLevels(
		DXGI_FORMAT_B8G8R8A8_UNORM, 4, &msaaQuality);	// 注意此处DXGI_FORMAT_B8G8R8A8_UNORM
	if (msaaQuality == 0)
	{
		throw std::exception("Error: MSAA(4x) unsupported.");
	}


	ComPtr<IDXGIDevice> dxgiDevice = nullptr;
	ComPtr<IDXGIFactory1> dxgiFactory1 = nullptr;	// D3D11.0(包含DXGI1.1)的接口类
	ComPtr<IDXGIFactory2> dxgiFactory2 = nullptr;	// D3D11.1(包含DXGI1.2)特有的接口类

	// 为了正确创建 DXGI交换链，首先我们需要获取创建 D3D设备 的 DXGI工厂，否则会引发报错：
	// "IDXGIFactory::CreateSwapChain: This function is being called with a device from a different IDXGIFactory."
	ThrowIfFailed(pCore->device.As(&dxgiDevice));
	ThrowIfFailed(dxgiDevice->GetAdapter(pCore->adapter.GetAddressOf()));
	ThrowIfFailed(pCore->adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgiFactory1.GetAddressOf())));

	// 查看该对象是否包含IDXGIFactory2接口
	hr = dxgiFactory1.As(&dxgiFactory2);
	// 如果包含，则说明支持D3D11.1
	if (dxgiFactory2 != nullptr)
	{
		// 填充各种结构体用以描述交换链
		DXGI_SWAP_CHAIN_DESC1 sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.Width = pCore->clientWidth;
		sd.Height = pCore->clientHeight;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 4;
		sd.SampleDesc.Quality = msaaQuality - 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fd;
		fd.RefreshRate.Numerator = 60;
		fd.RefreshRate.Denominator = 1;
		fd.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		fd.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		fd.Windowed = TRUE;
		// 为当前窗口创建交换链
		ComPtr<IDXGISwapChain1> swapChain1;
		ThrowIfFailed(dxgiFactory2->CreateSwapChainForHwnd(pCore->device.Get(), pCore->hWindow, &sd, &fd, nullptr, swapChain1.GetAddressOf()));
		ThrowIfFailed(swapChain1.As(&pCore->swapChain));
	}
	else
	{
		// 填充DXGI_SWAP_CHAIN_DESC用以描述交换链
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferDesc.Width = pCore->clientWidth;
		sd.BufferDesc.Height = pCore->clientHeight;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = 4;
		sd.SampleDesc.Quality = msaaQuality - 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;
		sd.OutputWindow = pCore->hWindow;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;
		ThrowIfFailed(dxgiFactory1->CreateSwapChain(pCore->device.Get(), &sd, pCore->swapChain.GetAddressOf()));
	}

	// 可以禁止alt+enter全屏
	dxgiFactory1->MakeWindowAssociation(pCore->hWindow, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);

	// 设置调试对象名
	D3D11SetDebugObjectName(pCore->deviceContext.Get(), "ImmediateContext");
	DXGISetDebugObjectName(pCore->swapChain.Get(), "SwapChain");

	return true;
}
