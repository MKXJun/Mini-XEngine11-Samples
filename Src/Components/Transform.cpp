#include <Hierarchy/GameObject.h>

using namespace XMath;

static const std::string s_Name = "Transform";

const std::string& Transform::GetName() const
{
	return s_Name;
}

const std::string& Transform::GetType()
{
	return s_Name;
}

Transform::Transform(GameObject* pObject) 
	: Component(pObject) 
{
}

Transform::~Transform()
{
}

Component* Transform::Instantiate(GameObject* pObject)
{
	Transform* pTransform = pObject->GetTransform();
	pTransform->m_Scale = m_Scale;
	pTransform->m_Rotation = m_Rotation;
	pTransform->m_Position = m_Position;
	return pTransform;
}

Vector3 Transform::RightAxis()
{
	static const Vector3 right(1.0f, 0.0f, 0.0f);
	return right;
}

Vector3 Transform::UpAxis()
{
	static const Vector3 up(0.0f, 1.0f, 0.0f);
	return up;
}

Vector3 Transform::ForwardAxis()
{
	static const Vector3 forward(0.0f, 0.0f, 1.0f);
	return forward;
}

Vector3 Transform::GetScale() const
{
	return m_Scale;
}

Vector3 Transform::GetRotation() const
{
	float sinX = 2 * (m_Rotation.w() * m_Rotation.x() - m_Rotation.y() * m_Rotation.z());
	float sinY_cosX = 2 * (m_Rotation.w() * m_Rotation.y() + m_Rotation.x() * m_Rotation.z());
	float cosY_cosX = 1 - 2 * (m_Rotation.x() * m_Rotation.x() + m_Rotation.y() * m_Rotation.y());
	float sinZ_cosX = 2 * (m_Rotation.w() * m_Rotation.z() + m_Rotation.x() * m_Rotation.y());
	float cosZ_cosX = 1 - 2 * (m_Rotation.x() * m_Rotation.x() + m_Rotation.z() * m_Rotation.z());
	
	Vector3 rotation;
	if (fabs(sinX) >= 1.0f)
		rotation.x() = Scalar::ConvertToDegrees(copysignf(PI / 2, sinX));
	else
		rotation.x() = Scalar::ConvertToDegrees(asinf(sinX));
	rotation.y() = Scalar::ConvertToDegrees(atan2f(sinY_cosX, cosY_cosX));
	rotation.z() = Scalar::ConvertToDegrees(atan2f(sinZ_cosX, cosZ_cosX));

	return rotation;
}

Quaternion Transform::GetRotationQuat() const
{
	return m_Rotation;
}

Vector3 Transform::GetPosition() const
{
	return m_Position;
}

Vector3 Transform::GetRightAxis() const
{
	return m_Rotation.toRotationMatrix().col(0);
}

Vector3 Transform::GetUpAxis() const
{
	return m_Rotation.toRotationMatrix().col(1);
}

Vector3 Transform::GetForwardAxis() const
{
	return m_Rotation.toRotationMatrix().col(2);
}

Matrix4x4 Transform::GetLocalToWorldMatrix() const
{
	// T = T1 * R1 * ... * TN * RN * SN * ... * S2 * S1
	Vector3 S = m_Scale;
	Matrix4x4A RT = Matrix4x4A::Identity();
	RT.topLeftCorner(3, 3) = m_Rotation.toRotationMatrix();
	RT.topRightCorner(3, 1) = m_Position;
	GameObject* pParent = m_pGameObject->GetParent();
	while (pParent)
	{
		Transform* pParentTransform = pParent->GetTransform();
		S = S.cwiseProduct(pParentTransform->GetScale());
		Matrix4x4A parentRT = Matrix4x4A::Identity();
		RT.topLeftCorner(3, 3) = pParentTransform->GetRotationQuat().toRotationMatrix();
		RT.topRightCorner(3, 1) = pParentTransform->GetPosition();
		RT = parentRT * RT;
		pParent = pParent->GetParent();
	}
	RT.topLeftCorner<3, 3>() *= S.asDiagonal();
	return RT;
}

Matrix4x4 Transform::GetWorldToLocalMatrix() const
{
	return GetLocalToWorldMatrix().inverse();
}

void Transform::SetScale(const Vector3& scale)
{
	m_Scale = scale;
}

void Transform::SetScale(float x, float y, float z)
{
	m_Scale = Vector3(x, y, z);
}

void Transform::SetRotation(const Vector3& eulerAnglesInDegree)
{
	m_Rotation = Quat::RotationRollPitchYaw(Matrix::ConvertToRadians(eulerAnglesInDegree));
}

void Transform::SetRotation(float x, float y, float z)
{
	SetRotation(Vector3(x, y, z));
}

void Transform::SetRotation(const XMath::Quaternion& quat)
{
	m_Rotation = quat.normalized();
}

void Transform::SetPosition(const Vector3& position)
{
	m_Position = position;
}

void Transform::SetPosition(float x, float y, float z)
{
	m_Position = Vector3(x, y, z);
}

void Transform::Rotate(const Vector3& eulerAnglesInDegree)
{
	QuaternionA rotQuat = m_Rotation;
	auto newQuat = Quat::RotationRollPitchYaw(Matrix::ConvertToRadians(eulerAnglesInDegree));
	m_Rotation = rotQuat * newQuat;
}

void Transform::RotateAxis(const Vector3& axis, float degrees)
{
	QuaternionA rotQuat = m_Rotation;
	QuaternionA newQuat = QuaternionA(Eigen::AngleAxisf(Scalar::ConvertToRadians(degrees), axis.normalized()));
	m_Rotation = rotQuat * newQuat;
}

void Transform::RotateAround(const Vector3& point, const Vector3& axis, float degrees)
{
	Matrix4x4A Mat = Matrix4x4A::Identity();
	Mat.topLeftCorner(3, 3) = m_Rotation.toRotationMatrix();
	Mat.topRightCorner(3, 1) = m_Position - point;
	Mat = Eigen::Translation3f(point) * Eigen::AngleAxisf(Scalar::ConvertToRadians(degrees), axis) * Mat;
	m_Rotation = Mat.block<3, 3>(0, 0);
	m_Position = Mat.topRightCorner(3, 1);
}

void Transform::Translate(const Vector3& direction, float magnitude)
{
	m_Position += direction * magnitude;
}

void Transform::LookAt(const Vector3& target, const Vector3& up)
{
	LookTo(target - m_Position, up);
}

void Transform::LookTo(const Vector3& direction, const Vector3& up)
{
	eigen_assert(direction.norm() != 0.0f);
	eigen_assert(up.norm() != 0.0f);

	Matrix3x3 R;
	R.col(2) = direction.normalized();
	Vector3 R1 = up.normalized();
	R.col(0) = R1.cross(R.col(2));

	eigen_assert(R.col(0).norm() != 0.0f);

	R.col(1) = R.col(2).cross(R.col(0));
	m_Rotation = R;
}
