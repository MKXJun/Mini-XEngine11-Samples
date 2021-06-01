
#pragma warning(disable: 26812)

#include <windows.h>
#include <Utils/CameraController.h>

//
// CameraController
//
CameraController::CameraController(Transform* pCamTransform)
	: m_pCamTransform(pCamTransform)
{
}

void CameraController::ApplyMomentum(float& oldValue, float& newValue, float deltaTime)
{
	float blendedValue;
	if (fabs(newValue) > fabs(oldValue))
		blendedValue = XMath::Scalar::Lerp(newValue, oldValue, powf(0.6f, deltaTime * 60.0f));
	else
		blendedValue = XMath::Scalar::Lerp(newValue, oldValue, powf(0.8f, deltaTime * 60.0f));
	oldValue = blendedValue;
	newValue = blendedValue;
}

//
// FlyingFPSCamera
//
FlyingFPSCamera::FlyingFPSCamera(Transform* pCamTransform)
	: CameraController(pCamTransform)
{
	if (!pCamTransform)
		throw std::exception("Error: pTransform is null!");
	auto rot = pCamTransform->GetRotation();
	m_CurrentPitch = rot.x();
	m_CurrentYaw = rot.y();

	m_CurrentPitch = XMath::Scalar::Clamp(m_CurrentPitch, -80, 80);
	m_CurrentYaw = XMath::Scalar::ModAngles(m_CurrentYaw);

}

void FlyingFPSCamera::Update(float deltaTime)
{
	float yaw = 0.0f, pitch = 0.0f, forward = 0.0f, strafe = 0.0f;
	if (Input::Mouse::GetMode() == Input::Mouse::Absolute && Input::Mouse::IsPressed(Input::Mouse::RightButton) ||
		Input::Mouse::GetMode() == Input::Mouse::Relative)
	{
		yaw += Input::Mouse::GetDeltaX() * m_MouseSensitivityX;
		pitch += Input::Mouse::GetDeltaY() * m_MouseSensitivityY;
		forward = m_MoveSpeed * (
			(Input::Keyboard::IsPressed(Input::Keyboard::W) ? deltaTime : 0.0f) +
			(Input::Keyboard::IsPressed(Input::Keyboard::S) ? -deltaTime : 0.0f)
			);
		strafe = m_StrafeSpeed * (
			(Input::Keyboard::IsPressed(Input::Keyboard::A) ? -deltaTime : 0.0f) +
			(Input::Keyboard::IsPressed(Input::Keyboard::D) ? deltaTime : 0.0f)
			);
	}

	if (m_Momentum)
	{
		ApplyMomentum(m_LastForward, forward, deltaTime);
		ApplyMomentum(m_LastStrafe, strafe, deltaTime);
	}

	m_CurrentYaw += yaw;
	m_CurrentPitch += pitch;
	m_CurrentPitch = XMath::Scalar::Clamp(m_CurrentPitch, -XMath::PI / 2, XMath::PI / 2);

	if (m_CurrentPitch > XMath::PI)
		m_CurrentPitch -= XMath::PI * 2;
	else if (m_CurrentPitch <= -XMath::PI)
		m_CurrentPitch += XMath::PI * 2;

	m_pCamTransform->SetRotation({ XMath::Scalar::ConvertToDegrees(m_CurrentPitch),
		XMath::Scalar::ConvertToDegrees(m_CurrentYaw), 
		0.0f });
	m_pCamTransform->Translate(m_pCamTransform->GetForwardAxis(), forward);
	m_pCamTransform->Translate(m_pCamTransform->GetRightAxis(), strafe);
}

void FlyingFPSCamera::SlowMovement(bool enable)
{
	m_FineMovement = enable;
}

void FlyingFPSCamera::SlowRotation(bool enable)
{
	m_FineRotation = enable;
}

void FlyingFPSCamera::SetMouseSensitivity(float x, float y)
{
	m_MouseSensitivityX = x;
	m_MouseSensitivityY = y;
}

void FlyingFPSCamera::EnableMomentum(bool enable)
{
	m_Momentum = true;
}

Transform* FlyingFPSCamera::GetTransform()
{
	return m_pCamTransform;
}

OrbitCamera::OrbitCamera(Transform* pTransform)
	: CameraController(pTransform)
{
}

void OrbitCamera::Update(float deltaTime)
{
	assert(m_pTarget);
	float yaw = 0.0f;
	float pitch = 0.0f;
	float closeness = 0.0f;

	if (Input::Mouse::GetMode() == Input::Mouse::Absolute && Input::Mouse::IsPressed(Input::Mouse::RightButton) ||
		Input::Mouse::GetMode() == Input::Mouse::Relative)
	{
		yaw += Input::Mouse::GetDeltaX() * m_MouseSensitivityX;
		pitch += Input::Mouse::GetDeltaY() * m_MouseSensitivityY;
	}
	closeness += Input::Mouse::GetScrollWheel() * m_ApproachDist;


	m_CurrentPitch += pitch;
	m_CurrentPitch = XMath::Scalar::Clamp(m_CurrentPitch + pitch, -XMath::PI / 2, XMath::PI / 2);

	m_CurrentYaw = XMath::Scalar::ModAngles(m_CurrentYaw + yaw);

	m_CurrentDist += closeness;

	m_pCamTransform->SetRotation(
		XMath::Scalar::ConvertToDegrees(m_CurrentPitch), 
		XMath::Scalar::ConvertToDegrees(m_CurrentYaw),
		0.0f);
	m_pCamTransform->SetPosition(m_pTarget->GetPosition() + m_Offset);
	m_pCamTransform->Translate(m_pCamTransform->GetForwardAxis(), -m_CurrentDist);
}

void OrbitCamera::EnableMomentum(bool enable)
{
	m_Momentum = enable;
}

void OrbitCamera::SetTarget(Transform* pTargetTransform, XMath::Vector3 offset)
{
	m_pTarget = pTargetTransform;
	m_Offset = offset;
}

void OrbitCamera::SetDistances(float minDist, float maxDist, float approachDist, float startDist)
{
	assert(minDist > 0.0f && maxDist > 0.0f && approachDist > 0.0f && startDist > 0.0f);
	assert(minDist <= maxDist && approachDist <= maxDist - minDist);
	assert(startDist >= minDist && startDist <= maxDist);
	m_MinDist = minDist;
	m_MaxDist = maxDist;
	m_ApproachDist = approachDist;
	m_CurrentDist = startDist;

}

void OrbitCamera::SetMouseSensitivity(float x, float y)
{
	m_MouseSensitivityX = x;
	m_MouseSensitivityY = y;
}