#include <Graphics/Renderer.h>
#include <Utils/GameInput.h>

class MyRenderer : public Renderer
{
public:
	MyRenderer();
	virtual ~MyRenderer() override;

	virtual bool Initialize(GraphicsCore*) override;
	virtual void Cleanup(GraphicsCore*) override;
	virtual void OnResize(GraphicsCore*) override;
	virtual void Update(float deltaTime) override;
	virtual void BeginNewFrame(GraphicsCore*) override;
	virtual void DrawScene(GraphicsCore*) override;
	virtual void EndFrame(GraphicsCore*) override;

protected:
	GameObject* m_pLight = nullptr;
};

MyRenderer::MyRenderer()
	: Renderer()
{
}

MyRenderer::~MyRenderer()
{
}

bool MyRenderer::Initialize(GraphicsCore* pCore)
{
	m_pEffectManager->CreateEffectsFromJson("effects.json", "record.json");
	m_MainScene.AddModel("mary", "../../Assets/mary/Marry.obj");
	m_MainScene.FindGameObject("DirectionalLight")->Destroy();
	
	m_pLight = m_MainScene.AddCube("PointLight");
	m_pLight->GetTransform()->SetScale(XMath::Vector3::Ones() * 0.025f);
	auto pCubeLight = m_pLight->AddComponent<Light>();
	pCubeLight->SetLightType(Light::PointLight);

	return true;
}

void MyRenderer::Cleanup(GraphicsCore* pCore)
{
}

void MyRenderer::OnResize(GraphicsCore* pCore)
{
}

void MyRenderer::Update(float deltaTime)
{
	

	static float phi = 0.0f, theta = 0.0f, dist = 5.0f, timer = 0.0f;
	static int lastX = GameInput::Mouse::GetX(), lastY = GameInput::Mouse::GetY();
	auto pCamTransform = m_MainScene.FindGameObject("MainCamera")->GetTransform();
	
	if (GameInput::Mouse::IsPressed(GameInput::Mouse::RIGHT_BUTTON))
	{
		phi += (GameInput::Mouse::GetY() - lastY);
		theta += (GameInput::Mouse::GetX() - lastX);
	}
	dist = XMath::Scalar::Clamp(dist + GameInput::Mouse::GetScrollWheel(), 2.0f, 20.0f);
	lastX = GameInput::Mouse::GetX(), lastY = GameInput::Mouse::GetY();
	timer += deltaTime;

	pCamTransform->SetRotation(phi, theta, 0.0f);
	pCamTransform->SetPosition(XMath::Vector3::Zero());
	pCamTransform->Translate(pCamTransform->GetForwardAxis(), -dist);
	pCamTransform->Translate(Transform::UpAxis(), 2.0f);
	
	m_pLight->GetTransform()->SetPosition(XMath::Vector3(
		sinf(timer * 1.5f),
		2.0f + 2.0f * cosf(timer),
		cosf(timer * 0.5f)));
}

void MyRenderer::BeginNewFrame(GraphicsCore* pCore)
{
}

void MyRenderer::DrawScene(GraphicsCore* pCore)
{
	Renderer::DrawScene(pCore);
}

void MyRenderer::EndFrame(GraphicsCore* pCore)
{
}

CREATE_CUSTOM_RENDERER(MyRenderer)
USE_WINMAIN



