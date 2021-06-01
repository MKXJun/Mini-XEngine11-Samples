#pragma once

// unused

#include <XCore.h>
#include <memory>
#include <Graphics/CommandBuffer.h>

class Camera;

// Get From Camera
class DrawingData
{
public:

private:
    friend class RenderContext;
    class Impl;
    std::unique_ptr<DrawingData::Impl> m_pImpl;
};

class DrawingSettings
{
public:
    size_t shaderPassId;
};


class RenderContext
{
public:
    ~RenderContext();

    void DrawSkybox(Camera& pCamera);
    
    void SetupCameraProperties(Camera& camera);
    void ExecuteCommandBuffer(CommandBuffer& commandBuffer);
    void DrawGameObject(GameObject* pObject);

    void Submit();

private:
    RenderContext();
    

private:
    friend class Graphics;
    class Impl;
    std::unique_ptr<RenderContext::Impl> pImpl;
};

