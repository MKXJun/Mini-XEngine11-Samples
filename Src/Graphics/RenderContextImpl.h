#include <Graphics/RenderContext.h>

#include <Math/XMath.h>


class RenderContext::Impl
{
public:
    Impl() 
    {
    }
    ~Impl() = default;


    void DrawSkybox(Camera& pCamera);

    void SetupCameraProperties(Camera& pCamera);
    void ExecuteCommandBuffer(CommandBuffer& pCommandBuffer);

    void Submit();

    CommandBuffer m_CommandBuffer;
};