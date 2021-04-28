#pragma once

#include <wrl/client.h>
#include <Graphics/ResourceManager.h>
#include <Graphics/RendererBase.h>

// TODO LIST:
// [ ] Mouse mode switching problem.
// [ ] ImGui support.
// [ ] Effects.json need to support RenderStates.
// [ ] Template Project.
// [ ] GAMES202 HW1.

class Renderer : public RendererBase
{
public:
	Renderer();
	virtual ~Renderer() override;

	virtual bool Initialize(GraphicsCore*) override;
	virtual void Cleanup(GraphicsCore*) override;
	virtual void OnResize(GraphicsCore*) override;
	virtual void Update(float deltaTime) override;
	virtual void BeginNewFrame(GraphicsCore*) override;
	virtual void DrawScene(GraphicsCore*) override;
	virtual void EndFrame(GraphicsCore*) override;

	class RenderSetting {
	public:
		static float ambientIntensity;
		static XMath::Vector3 ambientLightColor;
	};

protected:
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pBackBuffer[2];
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_pDepthStencilBuffer;
	Scene m_MainScene{};
	ResourceManager m_ResourceManager{};

private:
	void UpdateMeshResource(GraphicsCore* pCore, MeshData* pMeshData, MeshGraphicsResource* pMeshResource, const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputLayout);
	void DrawGameObject(GraphicsCore* pCore, GameObject* pObject);
};

#define CREATE_CUSTOM_RENDERER(ClassName) \
RendererBase* GetRenderer() \
{\
	static std::unique_ptr<ClassName> s_pRenderer;\
	if (!s_pRenderer)\
		s_pRenderer = std::make_unique<ClassName>();\
	return s_pRenderer.get();\
}

#define USE_WINMAIN \
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd);

#define USE_MAIN \
int main(int argc, char* argv[]);