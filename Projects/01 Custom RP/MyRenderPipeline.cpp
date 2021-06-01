#include <Graphics/Rendering.h>
#include <Utils/Input.h>
#include <Utils/Timer.h>
#include <Utils/CameraController.h>

class CameraRenderer
{
public:
    CameraRenderer()
    {
        m_MainScene.SetAsMainScene();
        
        Shader::InitFromJson("shaders.json", "record.json");

        m_Material.SetShader(Shader::Find("HLSL/Unlit"));
        m_Material.SetColor(Shader::StringToID("x_BaseColor"), Color(0.5f, 0.5f, 0.5f, 1.0f));
        auto pCube = m_MainScene.AddCube("Cube");
        pCube->AddComponent<MeshRenderer>()->SetMaterial(&m_Material);
        pCube->GetTransform()->SetRotation(30.0f, 30.0f, 0.0f);
        m_MainScene.FindGameObject("MainCamera")->GetTransform()->SetPosition(XMath::Vector3(0.0f, 0.0f, -20.0f));

        m_pCameraController = std::make_unique<FlyingFPSCamera>(m_MainScene.FindGameObject("MainCamera")->GetTransform());
    }

    void Render(RenderContext* pContext, Camera* pCamera)
    {
        m_pRenderContext = pContext;
        m_pCamera = pCamera;

        Setup();
        DrawGameObjects();
        Submit();
    }

private:

    void Setup()
    {
        m_pCameraController->Update(Time::DeltaTime());
        m_pRenderContext->SetupCameraProperties(*m_pCamera);
        m_CommandBuffer.ClearRenderTarget(true, true, Color::Black(), 1.0f);
        ExecuteBuffer();
    }

    void DrawGameObjects()
    {
        auto gameObjects = m_MainScene.GetRootGameObjects();
        for (auto pObject : gameObjects)
        {
            m_pRenderContext->DrawGameObject(pObject);
        }
    }

    void ExecuteBuffer()
    {
        m_pRenderContext->ExecuteCommandBuffer(m_CommandBuffer);
        m_CommandBuffer.Clear();
    }

    void Submit()
    {
        ExecuteBuffer();
        m_pRenderContext->Submit();
    }

    

private:
    RenderContext* m_pRenderContext = nullptr;
    Camera* m_pCamera = nullptr;
    std::unique_ptr<FlyingFPSCamera> m_pCameraController;
    CommandBuffer m_CommandBuffer;

    Material m_Material;
    
    Scene m_MainScene;

};

class MyRenderPipeline : public RenderPipeline
{
public:
    void Render(RenderContext* pContext, std::vector<Camera*>& pCameras) override
    {
        for (auto& pCamera : pCameras)
            m_CameraRenderer.Render(pContext, pCamera);
    }

private:
    CameraRenderer m_CameraRenderer;
};


CREATE_CUSTOM_RENDERPIPELINE(MyRenderPipeline)
USE_WINMAIN

