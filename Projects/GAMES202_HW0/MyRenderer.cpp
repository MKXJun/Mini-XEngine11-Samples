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
	if (!Renderer::Initialize(pCore))
		return false;

	m_MainScene.AddModel("mary", "assets/mary/Marry.obj");

	
	return true;
}

void MyRenderer::Cleanup(GraphicsCore* pCore)
{
	Renderer::Cleanup(pCore);
}

void MyRenderer::OnResize(GraphicsCore* pCore)
{
	Renderer::OnResize(pCore);
}

#include <DirectXMath.h>

void MyRenderer::Update(float deltaTime)
{
	GameInput::Update(deltaTime);

	static float phi = 0.0f, theta = 0.0f, dist = 5.0f;
	static int lastX = GameInput::Mouse::GetX(), lastY = GameInput::Mouse::GetY();
	auto pCamTransform = m_MainScene.FindGameObject("MainCamera")->GetTransform();
	
	if (GameInput::Mouse::IsPressed(GameInput::Mouse::RIGHT_BUTTON))
	{
		phi += (GameInput::Mouse::GetY() - lastY);
		theta += (GameInput::Mouse::GetX() - lastX);
	}
	dist = XMath::Scalar::Clamp(dist + GameInput::Mouse::GetScrollWheel(), 2.0f, 20.0f);
	lastX = GameInput::Mouse::GetX(), lastY = GameInput::Mouse::GetY();

	pCamTransform->SetRotation(phi, theta, 0.0f);
	pCamTransform->SetPosition(XMath::Vector3::Zero());
	pCamTransform->Translate(pCamTransform->GetForwardAxis(), -dist);
	pCamTransform->Translate(Transform::UpAxis(), 2.0f);
	
	
	

	

	Renderer::Update(deltaTime);
}

void MyRenderer::BeginNewFrame(GraphicsCore* pCore)
{
	Renderer::BeginNewFrame(pCore);
}

void MyRenderer::DrawScene(GraphicsCore* pCore)
{
	Renderer::DrawScene(pCore);
}

void MyRenderer::EndFrame(GraphicsCore* pCore)
{
	Renderer::EndFrame(pCore);
}

CREATE_CUSTOM_RENDERER(MyRenderer)
USE_WINMAIN



