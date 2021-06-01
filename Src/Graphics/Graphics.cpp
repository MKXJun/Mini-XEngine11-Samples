#include <Graphics/ResourceManager.h>
#include <Graphics/RenderStates.h>
#include "RenderContextImpl.h"
#include "CommandBufferImpl.h"
#include "GraphicsImpl.h"
#include "ShaderImpl.h"
#include "DXTrace.h"
#include "d3dUtil.h"

using namespace Microsoft::WRL;

//
// static
//
namespace
{
	ComPtr<ID3D11Device> s_pDevice;
	ComPtr<ID3D11DeviceContext> s_pImmediateContext;
	ComPtr<IDXGISwapChain> s_pSwapChain;
	ComPtr<IDXGIAdapter> s_pAdapter;

	ComPtr<ID3D11RenderTargetView> s_pBackBuffer;
	ComPtr<ID3D11DepthStencilView> s_pDepthStencilBuffer;

	std::unique_ptr<ResourceManager> s_pResourceManager;

	std::unique_ptr<RenderContext> s_pRenderContext;
	RenderPipeline* s_pRenderPipeline;

	std::wstring s_WinName;
	int s_ClientWidth;
	int s_ClientHeight;
}

//
// Graphics::Impl
//
bool Graphics::Impl::Initialize(HWND hWnd, int startWidth, int startHeight)
{
	if (IsInit())
		return true;

	if (!hWnd || !startWidth || !startHeight)
		return false;

	s_ClientWidth = startWidth;
	s_ClientHeight = startHeight;

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
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	D3D_FEATURE_LEVEL featureLevel;
	D3D_DRIVER_TYPE d3dDriverType;
	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		d3dDriverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, d3dDriverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, s_pDevice.GetAddressOf(), &featureLevel, s_pImmediateContext.GetAddressOf());

		if (hr == E_INVALIDARG)
		{
			// Direct3D 11.0 的API不承认D3D_FEATURE_LEVEL_11_1，所以我们需要尝试特性等级11.0以及以下的版本
			hr = D3D11CreateDevice(nullptr, d3dDriverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, s_pDevice.GetAddressOf(), &featureLevel, s_pImmediateContext.GetAddressOf());
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
	s_pDevice->CheckMultisampleQualityLevels(
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
	ThrowIfFailed(s_pDevice.As(&dxgiDevice));
	ThrowIfFailed(dxgiDevice->GetAdapter(s_pAdapter.GetAddressOf()));
	ThrowIfFailed(s_pAdapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgiFactory1.GetAddressOf())));

	// 查看该对象是否包含IDXGIFactory2接口
	hr = dxgiFactory1.As(&dxgiFactory2);
	// 如果包含，则说明支持D3D11.1
	if (dxgiFactory2 != nullptr)
	{
		// 填充各种结构体用以描述交换链
		DXGI_SWAP_CHAIN_DESC1 sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.Width = s_ClientWidth;
		sd.Height = s_ClientHeight;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
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
		ThrowIfFailed(dxgiFactory2->CreateSwapChainForHwnd(s_pDevice.Get(), hWnd, &sd, &fd, nullptr, swapChain1.GetAddressOf()));
		ThrowIfFailed(swapChain1.As(&s_pSwapChain));
	}
	else
	{
		// 填充DXGI_SWAP_CHAIN_DESC用以描述交换链
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferDesc.Width = s_ClientWidth;
		sd.BufferDesc.Height = s_ClientHeight;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;
		sd.OutputWindow = hWnd;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;
		ThrowIfFailed(dxgiFactory1->CreateSwapChain(s_pDevice.Get(), &sd, s_pSwapChain.GetAddressOf()));
	}

	// 可以禁止alt+enter全屏
	dxgiFactory1->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);

	// 设置调试对象名
	D3D11SetDebugObjectName(s_pImmediateContext.Get(), "ImmediateContext");
	DXGISetDebugObjectName(s_pSwapChain.Get(), "SwapChain");

	// 全局初始化
	s_pRenderContext = std::unique_ptr<RenderContext>(new RenderContext);
	s_pResourceManager = std::make_unique<ResourceManager>(s_pDevice.Get());
	RenderStates::InitAll(s_pDevice.Get());
	Shader::Impl::InitAll(s_pDevice.Get());
	/*auto pMat = ResourceManager::Get().CreateMaterial("@DefaultColorLit");
	pMat->SetColor(Shader::StringToID(X_CB_MAT_AMBIENT), Color(0.2f, 0.2f, 0.2f, 1.0f));
	pMat->SetColor(Shader::StringToID(X_CB_MAT_DIFFUSE), Color::White());
	pMat->SetColor(Shader::StringToID(X_CB_MAT_SPECULAR), Color::Black());*/

	return true;
}

bool Graphics::Impl::IsInit()
{
	return s_pDevice != nullptr;
}

void Graphics::Impl::OnResize(int newWidth, int newHeight)
{
	assert(s_pDevice);
	assert(s_pImmediateContext);
	assert(s_pSwapChain);

	s_pBackBuffer.Reset();
	s_pDepthStencilBuffer.Reset();

	s_ClientWidth = newWidth;
	s_ClientHeight = newHeight;

	// 重设交换链并且重新创建渲染目标视图
	ComPtr<ID3D11Texture2D> backBuffer;
	ThrowIfFailed(s_pSwapChain->ResizeBuffers(1, s_ClientWidth, s_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ThrowIfFailed(s_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf())));
	ThrowIfFailed(s_pDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, s_pBackBuffer.GetAddressOf()));

	// 设置调试对象名
	D3D11SetDebugObjectName(backBuffer.Get(), "BackBuffer[0]");

	backBuffer.Reset();

	uint32_t msaaQuality;
	s_pDevice->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaaQuality);

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = s_ClientWidth;
	depthStencilDesc.Height = s_ClientHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	// 创建深度缓冲区以及深度模板视图
	ComPtr<ID3D11Texture2D> depthStencilBuffer;
	ThrowIfFailed(s_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencilBuffer.GetAddressOf()));
	ThrowIfFailed(s_pDevice->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, s_pDepthStencilBuffer.GetAddressOf()));

	// 将渲染目标视图和深度/模板缓冲区结合到管线
	s_pImmediateContext->OMSetRenderTargets(1, s_pBackBuffer.GetAddressOf(), s_pDepthStencilBuffer.Get());

	// 设置调试对象名
	D3D11SetDebugObjectName(s_pDepthStencilBuffer.Get(), "DepthStencilView");
	D3D11SetDebugObjectName(s_pBackBuffer.Get(), "BackBufferRTV[0]");
}

int Graphics::Impl::GetClientWidth()
{
	return s_ClientWidth;
}

int Graphics::Impl::GetClientHeight()
{
	return s_ClientHeight;
}

void Graphics::Impl::RegisterRenderPipeline(RenderPipeline* rp)
{
	s_pRenderPipeline = rp;
}

void Graphics::Impl::RunRenderPipeline()
{
	auto cameras = Scene::GetMainScene()->GetComponents(Camera::GetType());
	
	auto it = std::remove_if(cameras.begin(), cameras.end(), [](Component* pCamera) {
		return !pCamera->IsEnabled();
		});
	size_t remains = (std::max)(it - cameras.begin(), 0i64);
	std::vector<Camera*> _cameras(remains);
	memcpy_s(_cameras.data(), remains * sizeof(Camera*), cameras.data(), remains * sizeof(Camera*));
	std::for_each(_cameras.begin(), _cameras.end(), [](Camera* pCamera) {
		pCamera->SetAspectRatio((float)s_ClientWidth / s_ClientHeight);
		});
	s_pRenderPipeline->Render(s_pRenderContext.get(), _cameras);
	s_pSwapChain->Present(1, 0);
}

void Graphics::Impl::SubmitRenderContext()
{
	ComPtr<ID3D11CommandList> pCmdList;
	s_pRenderContext->pImpl->m_CommandBuffer.pImpl->m_pDeferredContext->FinishCommandList(false, pCmdList.GetAddressOf());
	s_pImmediateContext->ExecuteCommandList(pCmdList.Get(), false);
}

ID3D11RenderTargetView* Graphics::Impl::GetColorBuffer()
{
	return s_pBackBuffer.Get();
}

ID3D11DepthStencilView* Graphics::Impl::GetDepthBuffer()
{
	return s_pDepthStencilBuffer.Get();
}

ID3D11Device* Graphics::Impl::GetDevice()
{
	return s_pDevice.Get();
}
