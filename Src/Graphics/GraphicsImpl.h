#include <Graphics/Graphics.h>
#include <Graphics/RenderPipeline.h>

class Graphics::Impl
{
public:
    static bool Initialize(HWND hWnd, int startWidth, int startHeight);

    static bool IsInit();

    static void OnResize(int newWidth, int newHeight);

    static int GetClientWidth();

    static int GetClientHeight();

    static void RegisterRenderPipeline(RenderPipeline* rp);

    static void RunRenderPipeline();

    static void SubmitRenderContext();

    static ID3D11RenderTargetView* GetColorBuffer();

    static ID3D11DepthStencilView* GetDepthBuffer();

    static ID3D11Device* GetDevice();
};
