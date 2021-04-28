#include <Component/Light.h>
#include <Hierarchy/GameObject.h>

#pragma warning(disable: 26812)

static const std::string s_Name = "Light";

const std::string& Light::GetName() const
{
	return s_Name;
}

const std::string& Light::GetType()
{
	return s_Name;
}

Light::Light(GameObject* pObject)
	: Component(pObject)
{
}

Light::~Light()
{
}

Component* Light::Instantiate(GameObject* pObject)
{
	if (pObject->FindComponent<Light>())
		return nullptr;
	Light* pLight = pObject->AddComponent<Light>();
	pLight->m_Color = m_Color;
	pLight->m_Direction = m_Direction;
	pLight->m_Intensity = m_Intensity;
	pLight->m_IsEnabled = m_IsEnabled;
	pLight->m_LightType = m_LightType;
	pLight->m_Range = m_Range;
	pLight->m_SpotPower = m_SpotPower;
	return pLight;
}

void Light::SetLightType(LightType type)
{
	m_LightType = type;
}

void Light::SetIntensity(float i)
{
	m_Intensity = i;
}

void Light::SetColor(const XMath::Vector3& color)
{
	m_Color = color;
}

void Light::SetRange(float range)
{
	m_Range = range;
}

void Light::SetDirection(const XMath::Vector3& dir)
{
	m_Direction = dir;
}

void Light::SetSpotPower(float spotPower)
{
	m_SpotPower = spotPower;
}

Light::LightType Light::GetLightType() const
{
	return m_LightType;
}

float Light::GetIntensity() const
{
	return m_Intensity;
}

const XMath::Vector3& Light::GetColor() const
{
	return m_Color;
}

float Light::GetRange() const
{
	return m_Range;
}

const XMath::Vector3& Light::GetDirection() const
{
	return m_Direction;
}

float Light::GetSpotPower() const
{
	return m_SpotPower;
}
