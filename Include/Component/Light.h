
#pragma once

#include "Component.h"
#include <Math/XMath.h>

class Light : public Component
{
public:

	enum LightType {
		DirectionalLight = 0,
		PointLight,
		SpotLight
	};

	const std::string& GetName() const override;
	static const std::string& GetType();

	Light(GameObject* pObject);
	Component* Instantiate(GameObject* pObject) override;

	void SetLightType(LightType type);
	
	void SetIntensity(float i);
	void SetColor(const XMath::Vector3& color);
	void SetRange(float range);
	void SetDirection(const XMath::Vector3& dir);
	void SetSpotPower(float spotPower);

	LightType GetLightType() const;
	float GetIntensity() const;
	const XMath::Vector3& GetColor() const;
	float GetRange() const;
	const XMath::Vector3& GetDirection() const;
	float GetSpotPower() const;

private:
	~Light() override;

	LightType m_LightType = LightType::DirectionalLight;
	float m_Intensity = 1.0f;
	float m_Range = 0.0f;
	float m_SpotPower = 10.0f;
	XMath::Vector3 m_Color = XMath::Vector3::Ones();
	XMath::Vector3 m_Direction = XMath::Vector3(0.0f, -sqrtf(2.0f) / 2.0f, sqrtf(2.0f) / 2.0f);
};
