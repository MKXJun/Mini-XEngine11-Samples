#include <Component/Camera.h>
#include <Hierarchy/GameObject.h>

static const std::string s_Name = "Camera";

const std::string& Camera::GetName() const
{
	return s_Name;
}

const std::string& Camera::GetType()
{
	return s_Name;
}

Camera::Camera(GameObject* pObject)
	: Component(pObject)
{
}

Component* Camera::Instantiate(GameObject* pObject)
{
	if (pObject->FindComponent<Camera>())
		return nullptr;
	Camera* pCamera = pObject->AddComponent<Camera>();

	pCamera->m_AspectRatio = m_AspectRatio;
	pCamera->m_ClearColor = m_ClearColor;
	pCamera->m_ClearDepth = m_ClearDepth;
	pCamera->m_ClearFlag = m_ClearFlag;
	pCamera->m_FarPlane = m_FarPlane;
	pCamera->m_FieldOfViewY = m_FieldOfViewY;
	pCamera->m_IsMainCamera = false;
	pCamera->m_NearPlane = m_NearPlane;
	pCamera->m_ProjectionType = m_ProjectionType;
	pCamera->m_RenderTextureName = m_RenderTextureName;
	pCamera->m_Size = m_Size;
	pCamera->m_ViewPortRect = m_ViewPortRect;

	return pCamera;
}

Camera::~Camera()
{
}

void Camera::SetMainCamera(bool enabled)
{
	Camera* pPrevious = m_pGameObject->GetScene()->GetMainCamera();
	if (pPrevious)
		pPrevious->m_IsMainCamera = false;
	m_IsMainCamera = enabled;
	m_pGameObject->GetScene()->SetMainCamera(this);
}

bool Camera::IsMainCamera() const
{
	return m_IsMainCamera;
}

void Camera::SetClearFlag(ClearFlag flag)
{
	m_ClearFlag = flag;
}

Camera::ClearFlag Camera::GetClearFlag() const
{
	return m_ClearFlag;
}

void Camera::SetRenderTextureName(const std::string& name)
{
	m_RenderTextureName = name;
}

std::string Camera::GetRenderTextureName() const
{
	return m_RenderTextureName;
}

void Camera::SetViewPortRect(const XMath::Vector4& rect)
{
	m_ViewPortRect = rect;
}

void Camera::SetViewPortRect(float x, float y, float w, float h)
{
	m_ViewPortRect = { x, y, w, h };
}

XMath::Vector4 Camera::GetViewPortRect() const
{
	return m_ViewPortRect;
}

void Camera::SetProjectionType(ProjectionType type)
{
	m_ProjectionType = type;
}

Camera::ProjectionType Camera::GetProjectionType() const
{
	return m_ProjectionType;
}

void Camera::SetFieldOfViewY(float fovY)
{
	m_FieldOfViewY = fovY;
	if (m_FieldOfViewY <= 0.0f)
		m_FieldOfViewY = 0.1f;
	else if (m_FieldOfViewY >= 180.0f)
		m_FieldOfViewY = 179.9f;
}

float Camera::GetFieldOfViewY() const
{
	return m_FieldOfViewY;
}

void Camera::SetNearPlane(float nZ)
{
	m_NearPlane = nZ;
	if (m_NearPlane >= m_FarPlane || m_NearPlane <= 0.0f)
		m_NearPlane = 0.1f;
}

void Camera::SetFarPlane(float fZ)
{
	m_FarPlane = fZ;
	if (m_NearPlane >= m_FarPlane || m_FarPlane <= 0.0f)
		m_FarPlane = m_NearPlane + 0.1f;
}

float Camera::GetNearPlane() const
{
	return m_NearPlane;
}

float Camera::GetFarPlane() const
{
	return m_FarPlane;
}

void Camera::SetSize(float size)
{
	m_Size = size;
}

float Camera::GetSize() const
{
	return m_Size;
}

void Camera::SetAspectRatio(float ratio)
{
	m_AspectRatio = ratio;
}

float Camera::GetAspectRatio() const
{
	return m_AspectRatio;
}

void Camera::SetClearDepth(float depth)
{
	if (depth > 1.0f)
		m_ClearDepth = 1.0f;
	else if (depth < 0.0f)
		m_ClearDepth = 0.0f;
	else
		m_ClearDepth = depth;
}

float Camera::GetClearDepth() const
{
	return m_ClearDepth;
}

void Camera::SetClearColor(const XMath::Vector4& color)
{
	m_ClearColor = color;
}

const XMath::Vector4& Camera::GetClearColor() const
{
	return m_ClearColor;
}

XMath::Matrix4x4 Camera::GetProjMatrix() const
{
	if (m_ProjectionType == ProjectionType::Perspective)
	{
		return XMath::Matrix::PerspectiveFovLH(m_FieldOfViewY, m_AspectRatio, m_NearPlane, m_FarPlane);
	}
	/*else
	{
		XMStoreFloat4x4(&proj, XMMatrixOrthographicLH(
			m_Size, m_Size / m_AspectRatio, m_NearPlane, m_FarPlane));
	}*/
	return {};
}
