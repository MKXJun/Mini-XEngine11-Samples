
#pragma once

#include "Component.h"
#include <string>
#include <Math/XMath.h>


class Transform : public Component
{
public:
	const std::string& GetName() const override;
	static const std::string& GetType();

	Transform(GameObject* pObject);

	Component* Instantiate(GameObject* pObject) override;

	static XMath::Vector3 RightAxis();
	static XMath::Vector3 UpAxis();
	static XMath::Vector3 ForwardAxis();

	// 获取对象缩放比例
	XMath::Vector3 GetScale() const;

	// 获取对象欧拉角(弧度制)
	// 对象以Z-X-Y轴顺序旋转
	XMath::Vector3 GetRotation() const;

	// 获取对象旋转四元数
	XMath::Quaternion GetRotationQuat() const;

	// 获取对象位置
	XMath::Vector3 GetPosition() const;

	// 获取右方向轴
	XMath::Vector3 GetRightAxis() const;

	// 获取上方向轴
	XMath::Vector3 GetUpAxis() const;

	// 获取前方向轴
	XMath::Vector3 GetForwardAxis() const;

	// 获取世界变换矩阵
	XMath::Matrix4x4 GetLocalToWorldMatrix() const;

	// 获取逆世界变换矩阵
	XMath::Matrix4x4 GetWorldToLocalMatrix() const;

	// 设置对象缩放比例
	void SetScale(const XMath::Vector3& scale);
	// 设置对象缩放比例
	void SetScale(float x, float y, float z);

	// 设置对象欧拉角(角度制)
	// 对象将以Z-X-Y轴顺序旋转
	void SetRotation(const XMath::Vector3& eulerAnglesInDegree);
	// 设置对象欧拉角(角度制)
	// 对象将以Z-X-Y轴顺序旋转
	void SetRotation(float x, float y, float z);
	// 设置对象旋转四元数
	void SetRotation(const XMath::Quaternion& quat);

	// 设置对象位置
	void SetPosition(const XMath::Vector3& position);
	// 设置对象位置
	void SetPosition(float x, float y, float z);
	
	// 指定欧拉角旋转对象
	void Rotate(const XMath::Vector3& eulerAnglesInDegree);
	// 指定以原点为中心绕轴旋转
	void RotateAxis(const XMath::Vector3& axis, float degrees);
	// 指定以point为旋转中心绕轴旋转
	void RotateAround(const XMath::Vector3& point, const XMath::Vector3& axis, float degrees);

	// 沿着某一方向平移
	void Translate(const XMath::Vector3& direction, float magnitude);

	// 观察某一点
	void LookAt(const XMath::Vector3& target, const XMath::Vector3& up = UpAxis());
	// 沿着某一方向观察
	void LookTo(const XMath::Vector3& direction, const XMath::Vector3& up = UpAxis());

private:
	~Transform() override;

private:

	XMath::Vector3 m_Scale = { 1.0f, 1.0f, 1.0f };					// 缩放
	XMath::Quaternion m_Rotation = { 1.0f, 0.0f, 0.0f, 0.0f };		// 旋转四元数
	XMath::Vector3 m_Position = { 0.0f, 0.0f, 0.0f };				// 位置
};
