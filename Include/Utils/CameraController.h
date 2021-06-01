
#pragma once

#include <Component/Transform.h>
#include <Utils/Input.h>


class CameraController
{
public:
	CameraController(Transform* pCamTransform);
	CameraController& operator=(const CameraController&) = delete;
	virtual ~CameraController() {}
	virtual void Update(float deltaTime) = 0;
	

	// Helper function
	static void ApplyMomentum(float& oldValue, float& newValue, float deltaTime);

protected:
	Transform* m_pCamTransform = nullptr;
};

class FlyingFPSCamera : public CameraController
{
public:

	FlyingFPSCamera(Transform* pCamTransform);

	void Update(float deltaTime) override;

	void SlowMovement(bool enable);
	void SlowRotation(bool enable);

	void SetMouseSensitivity(float x, float y);
	void SetMoveSpeed(float speed);
	void SetStrafeSpeed(float speed);

	void EnableMomentum(bool enable);

	Transform* GetTransform();

private:

	float m_HorizontalLookSensitivity = 2.0f;
	float m_VerticalLookSensitivity = 2.0f;
	float m_MoveSpeed = 5.0f;
	float m_StrafeSpeed = 5.0f;
	float m_MouseSensitivityX = 0.005f;
	float m_MouseSensitivityY = 0.005f;

	float m_CurrentYaw = 0.0f;
	float m_CurrentPitch = 0.0f;

	bool m_FineMovement = false;
	bool m_FineRotation = false;
	bool m_Momentum = true;

	float m_LastYaw = 0.0f;
	float m_LastPitch = 0.0f;
	float m_LastForward = 0.0f;
	float m_LastStrafe = 0.0f;
	float m_LastAscent = 0.0f;
};

class OrbitCamera : public CameraController
{
public:
	OrbitCamera(Transform* pTransform);

	virtual void Update(float deltaTime) override;

	void EnableMomentum(bool enable);
	void SetTarget(Transform* pTargetTransform, XMath::Vector3 offset = XMath::Vector3::Zero());
	void SetDistances(float minDist, float maxDist, float approachDist, float startDist);
	void SetMouseSensitivity(float x, float y);

private:
	Transform* m_pTarget = nullptr;
	XMath::Vector3 m_Offset = XMath::Vector3::Zero();

	float m_MinDist = 1.0f;
	float m_MaxDist = 10.0f;
	float m_ApproachDist = 1.0f;

	float m_MouseSensitivityX = 0.005f;
	float m_MouseSensitivityY = 0.005f;

	bool m_Momentum = true;

	float m_CurrentDist = 5.0f;
	float m_CurrentYaw = 0.0f;
	float m_CurrentPitch = 0.0f;

	float m_LastDist = 5.0f;
	float m_LastYaw = 0.0f;
	float m_LastPitch = 0.0f;
};