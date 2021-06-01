#include <Graphics/RenderTexture.h>
#include "d3dUtil.h"

bool RenderTexture::DepthStencilFormats(uint32_t depthBits, DXGI_FORMAT& depthBufferFormat, DXGI_FORMAT& depthSRVFormat, DXGI_FORMAT& depthDSVFormat)
{
    switch (depthBits)
    {
    case 0:
        return true;
    case 16:
        depthBufferFormat = DXGI_FORMAT_R16_TYPELESS;
        depthSRVFormat = DXGI_FORMAT_R16_UNORM;
        depthDSVFormat = DXGI_FORMAT_D16_UNORM;
        break;
    case 24:
        depthBufferFormat = DXGI_FORMAT_R24G8_TYPELESS;
        depthSRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        depthDSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        break;
    case 32:
        depthBufferFormat = DXGI_FORMAT_R32_TYPELESS;
        depthSRVFormat = DXGI_FORMAT_R32_FLOAT;
        depthDSVFormat = DXGI_FORMAT_D32_FLOAT;
        break;
    default:
        return false;
    }

    return true;
}

RenderTexture::RenderTexture(uint32_t width, uint32_t height, uint32_t depthBits, DXGI_FORMAT colorFormat, bool enableRW)
    : m_Width(width), m_Height(height), m_DepthBits(depthBits), m_ColorFormat(colorFormat), m_UseRandomWrite(enableRW)
{
}

void RenderTexture::SetTextureSize(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
}

void RenderTexture::SetAntiAliasingLevel(uint32_t level)
{
    m_MsaaLevel = level;
}

void RenderTexture::SetMipmapsEnabled(bool enable)
{
    m_UseMipmaps = enable;
}

void RenderTexture::SetRandomWriteEnabled(bool enable)
{
    m_UseRandomWrite = enable;
}

void RenderTexture::SetBindMultiSampleTextureEnabled(bool enable)
{
    m_BindTextureMS = enable;
}

void RenderTexture::SetColorBufferFormat(DXGI_FORMAT format)
{
    m_ColorFormat = format;
}

void RenderTexture::SetDepthBits(uint32_t depthBits)
{
    m_DepthBits = depthBits;
}

void RenderTexture::SetShadowSamplingMode(ShadowSamplingMode mode)
{
    m_ShadowSamplingMode = mode;
}

HRESULT RenderTexture::Create(ID3D11Device* pDevice)
{
    if (!pDevice)
        return E_INVALIDARG;

    if (m_DepthBits != 0 && m_DepthBits != 16 && m_DepthBits != 24 && m_DepthBits != 32)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    uint32_t maxQuality = 0;
    pDevice->CheckMultisampleQualityLevels(m_ColorFormat, m_MsaaLevel, &maxQuality);
    if (m_MsaaLevel > 1 && maxQuality == 0)
        return E_INVALIDARG;

    if (m_BindTextureMS && m_MsaaLevel == 1)
        return E_INVALIDARG;

    Clear();

    DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT depthSRVFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT depthDSVFormat = DXGI_FORMAT_UNKNOWN;
    DepthStencilFormats(m_DepthBits, depthFormat, depthSRVFormat, depthDSVFormat);

    //
    // Color Buffer
    //
    if (!m_BindTextureMS && m_ShadowSamplingMode == ShadowSamplingMode::None)
    {
        CD3D11_TEXTURE2D_DESC texDesc(m_ColorFormat, m_Width, m_Height, 1, (m_UseMipmaps ? 0 : 1),
            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | (m_UseRandomWrite ? D3D11_BIND_UNORDERED_ACCESS : 0),
            D3D11_USAGE_DEFAULT, 0, 1, 0, (m_UseMipmaps ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0));
        hr = pDevice->CreateTexture2D(&texDesc, nullptr, m_pColorBuffer.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }

    //
    // MSAA Color Buffer
    //
    if (m_MsaaLevel > 1 && m_ShadowSamplingMode == ShadowSamplingMode::None)
    {
        CD3D11_TEXTURE2D_DESC texDesc(m_ColorFormat, m_Width, m_Height, 1, (m_UseMipmaps ? 0 : 1),
            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | (m_UseRandomWrite ? D3D11_BIND_UNORDERED_ACCESS : 0),
            D3D11_USAGE_DEFAULT, 0, m_MsaaLevel, maxQuality - 1, (m_UseMipmaps && m_BindTextureMS ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0));
        hr = pDevice->CreateTexture2D(&texDesc, nullptr, m_pColorBufferMS.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }

    //
    // Depth Stencil Buffer
    //
    if (m_DepthBits != 0)
    {
        CD3D11_TEXTURE2D_DESC texDesc(depthFormat, m_Width, m_Height, 1, 1,
            D3D11_BIND_DEPTH_STENCIL, D3D11_USAGE_DEFAULT, 0, m_MsaaLevel, maxQuality - 1);
        hr = pDevice->CreateTexture2D(&texDesc, nullptr, m_pDepthBuffer.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }
    

    //
    // RTVs & SRVs
    //
    if (m_pColorBufferMS && m_BindTextureMS)
    {
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(m_pColorBufferMS.Get(), D3D11_RTV_DIMENSION_TEXTURE2DMS);
        hr = pDevice->CreateRenderTargetView(m_pColorBufferMS.Get(), &rtvDesc, m_pColorBufferMSRTV.GetAddressOf());
        if (FAILED(hr))
            return hr;

        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(m_pColorBufferMS.Get(), D3D11_SRV_DIMENSION_TEXTURE2DMS);
        hr = pDevice->CreateShaderResourceView(m_pColorBufferMS.Get(), &srvDesc, m_pColorBufferMSSRV.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }
    else if (m_pColorBuffer)
    {
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(m_pColorBuffer.Get(), D3D11_RTV_DIMENSION_TEXTURE2D);
        hr = pDevice->CreateRenderTargetView(m_pColorBuffer.Get(), &rtvDesc, m_pColorBufferRTV.GetAddressOf());
        if (FAILED(hr))
            return hr;

        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(m_pColorBuffer.Get(), D3D11_SRV_DIMENSION_TEXTURE2D);
        hr = pDevice->CreateShaderResourceView(m_pColorBuffer.Get(), &srvDesc, m_pColorBufferSRV.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }

    if (m_DepthBits != 0)
    {
        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(m_pDepthBuffer.Get(), (m_MsaaLevel > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D), 
            depthSRVFormat);
        hr = pDevice->CreateShaderResourceView(m_pDepthBuffer.Get(), &srvDesc, m_pDepthBufferSRV.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }
    
    //
    // UAVs
    //
    if (m_pColorBuffer && m_UseRandomWrite)
    {
        CD3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc(m_pColorBuffer.Get(), D3D11_UAV_DIMENSION_TEXTURE2D);
        hr = pDevice->CreateUnorderedAccessView(m_pColorBuffer.Get(), &uavDesc, m_pColorBufferUAV.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }

    //
    // DSV
    //
    if (m_DepthBits != 0)
    {
        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(m_pDepthBuffer.Get(), m_MsaaLevel > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D);
        hr = pDevice->CreateDepthStencilView(m_pDepthBuffer.Get(), &dsvDesc, m_pDepthBufferDSV.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }

    return hr;
}

void RenderTexture::Clear()
{
    m_pColorBufferMSSRV.Reset();
    m_pColorBufferMSRTV.Reset();

    m_pColorBufferSRV.Reset();
    m_pColorBufferRTV.Reset();
    m_pColorBufferUAV.Reset();

    m_pDepthBufferSRV.Reset();
    m_pDepthBufferDSV.Reset();

    m_pColorBuffer.Reset();
    m_pColorBufferMS.Reset();
    m_pDepthBuffer.Reset();
}

HRESULT RenderTexture::CreateFromSwapChain(ID3D11Device* pDevice, ID3D11Texture2D* pBackBuffer)
{
    if (!pDevice)
        return E_INVALIDARG;

    if (m_DepthBits != 0 && m_DepthBits != 16 && m_DepthBits != 24 && m_DepthBits != 32)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    Clear();

    D3D11_TEXTURE2D_DESC texDesc;
    pBackBuffer->GetDesc(&texDesc);
    m_Width = texDesc.Width;
    m_Height = texDesc.Height;
    m_MsaaLevel = texDesc.SampleDesc.Count;
    uint32_t maxQuality = texDesc.SampleDesc.Quality;
    m_ColorFormat = texDesc.Format;
    if (m_MsaaLevel > 1)
    {
        m_pColorBufferMS = pBackBuffer;
        m_BindTextureMS = true;
    }
    else
    {
        m_pColorBuffer = pBackBuffer;
        m_BindTextureMS = false;
    }
        
    DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT depthSRVFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT depthDSVFormat = DXGI_FORMAT_UNKNOWN;
    DepthStencilFormats(m_DepthBits, depthFormat, depthSRVFormat, depthDSVFormat);

    //
    // Depth Stencil Buffer
    //
    if (m_DepthBits != 0)
    {
        CD3D11_TEXTURE2D_DESC texDesc(depthFormat, m_Width, m_Height, 1, 1,
            D3D11_BIND_DEPTH_STENCIL, D3D11_USAGE_DEFAULT, 0, m_MsaaLevel, maxQuality - 1);
        hr = pDevice->CreateTexture2D(&texDesc, nullptr, m_pDepthBuffer.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }

    //
    // RTVs
    //
    if (m_pColorBufferMS && m_BindTextureMS)
    {
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(m_pColorBufferMS.Get(), D3D11_RTV_DIMENSION_TEXTURE2DMS);
        hr = pDevice->CreateRenderTargetView(m_pColorBufferMS.Get(), &rtvDesc, m_pColorBufferMSRTV.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }
    else if (m_pColorBuffer)
    {
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(m_pColorBuffer.Get(), D3D11_RTV_DIMENSION_TEXTURE2D);
        hr = pDevice->CreateRenderTargetView(m_pColorBuffer.Get(), &rtvDesc, m_pColorBufferRTV.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }

    //
    // SRVs
    //
    if (m_DepthBits != 0)
    {
        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(m_pDepthBuffer.Get(), (m_MsaaLevel > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D),
            depthSRVFormat);
        hr = pDevice->CreateShaderResourceView(m_pDepthBuffer.Get(), &srvDesc, m_pDepthBufferSRV.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }

    //
    // DSV
    //
    if (m_DepthBits != 0)
    {
        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(m_pDepthBuffer.Get(), m_MsaaLevel > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D);
        hr = pDevice->CreateDepthStencilView(m_pDepthBuffer.Get(), &dsvDesc, m_pDepthBufferDSV.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }

    return hr;

}

void RenderTexture::GenerateMips(ID3D11DeviceContext* deviceContext)
{
    if (m_pColorBufferMS && m_BindTextureMS)
        deviceContext->GenerateMips(m_pColorBufferMSSRV.Get());
    else
        deviceContext->GenerateMips(m_pColorBufferSRV.Get());
}

void RenderTexture::ResolveAntiAliasedSurface(ID3D11DeviceContext* deviceContext)
{
    if (m_pColorBuffer && m_pColorBufferMS)
        deviceContext->ResolveSubresource(m_pColorBuffer.Get(), 0, m_pColorBufferMS.Get(), 0, m_ColorFormat);
}

ID3D11ShaderResourceView* RenderTexture::GetColorBufferSRV()
{
    if (m_pColorBufferMS && m_BindTextureMS)
        return m_pColorBufferMSSRV.Get();
    else
        return m_pColorBufferSRV.Get();
}

ID3D11RenderTargetView* RenderTexture::GetColorBufferRTV()
{
    if (m_pColorBufferMS && m_BindTextureMS)
        return m_pColorBufferMSRTV.Get();
    else
        return m_pColorBufferRTV.Get();
}

ID3D11UnorderedAccessView* RenderTexture::GetColorBufferUAV()
{
    return m_pColorBufferUAV.Get();
}

ID3D11ShaderResourceView* RenderTexture::GetDepthBufferSRV()
{
    return m_pDepthBufferSRV.Get();
}

ID3D11DepthStencilView* RenderTexture::GetDepthBufferDSV()
{
    return m_pDepthBufferDSV.Get();
}

void RenderTexture::SetDebugObjectName(const std::string& name)
{
    //
}
