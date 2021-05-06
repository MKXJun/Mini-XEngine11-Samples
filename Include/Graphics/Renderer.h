#pragma once

#include <wrl/client.h>
#include <Graphics/ResourceManager.h>
#include <Graphics/EffectManager.h>

// TODO LIST:
// [ ] Mouse mode switching problem.
// [ ] ImGui support.
// [ ] Template Project.
// [ ] GAMES202 HW0.
// [*] Effects.json need to support RenderStates.

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

class Renderer
{
public:
	Renderer();
	virtual ~Renderer() = 0;

	virtual bool Initialize(GraphicsCore*) = 0;
	virtual void Cleanup(GraphicsCore*) = 0;
	virtual void OnResize(GraphicsCore*) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void BeginNewFrame(GraphicsCore*) = 0;
	virtual void DrawScene(GraphicsCore*);
	virtual void EndFrame(GraphicsCore*) = 0;

	class RenderSetting {
	public:
		static float ambientIntensity;
		static XMath::Vector3 ambientLightColor;
	};

protected:
	friend class MainWindow;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pBackBuffer[2];
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_pDepthStencilBuffer;
	std::unique_ptr<ResourceManager> m_pResourceManager;
	std::unique_ptr<EffectManager> m_pEffectManager;
	Scene m_MainScene;

private:
	void _UpdateMeshResource(GraphicsCore* pCore, MeshData* pMeshData, MeshGraphicsResource* pMeshResource, const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputLayout);
	void _DrawGameObject(GraphicsCore* pCore, GameObject* pObject);

	bool _Initialize(GraphicsCore*);
	void _OnResize(GraphicsCore*);
	void _Update(float deltaTime);
	void _BeginNewFrame(GraphicsCore*);
	void _EndFrame(GraphicsCore*);


private:
	std::set<std::string> m_UsedEffects;
};

Renderer* GetRenderer();

#define CREATE_CUSTOM_RENDERER(ClassName) \
Renderer* GetRenderer() \
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