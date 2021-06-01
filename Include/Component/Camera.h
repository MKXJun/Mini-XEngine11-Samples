

#pragma once

#include "Component.h"
#include <XCore.h>

class Camera : public Component
{
public:
	enum class ClearFlag
	{
		// Skybox,
		SolidColor,
		DepthOnly,
		None
	};

	enum class ProjectionType
	{
		Perspective,
		Orthographic
	};

	const std::string& GetName() const override;
	static const std::string& GetType();

	Camera(GameObject* pObject);
	Component* Instantiate(GameObject* pObject) override;

	

	// 设置为主摄像机
	void SetMainCamera(bool enabled);
	// 是否为主摄像机
	bool IsMainCamera() const;

	// 设置清除标签
	void SetClearFlag(ClearFlag flag);
	// 获取清除标签
	ClearFlag GetClearFlag() const;

	// 设置渲染到纹理的名称
	void SetRenderTextureName(const std::string& name);
	// 获取渲染到纹理的名称
	std::string GetRenderTextureName() const;

	// 设置视口区域
	void SetViewPortRect(const Rect& rect);
	// 设置视口区域
	void SetViewPortRect(float x = 0.0f, float y = 0.0f, float w = 1.0f, float h = 1.0f);
	// 获取视口区域
	Rect GetViewPortRect() const;

	// 设置投影类型
	void SetProjectionType(ProjectionType type);
	// 获取投影类型
	ProjectionType GetProjectionType() const;

	// 设置垂直视场角(角度制)
	void SetFieldOfViewY(float fovY);
	// 获取垂直视场角(角度制)
	float GetFieldOfViewY() const;

	// 设置近平面
	void SetNearPlane(float nZ);
	// 设置远平面
	void SetFarPlane(float fZ);

	// 获取近平面
	float GetNearPlane() const;
	// 获取远平面
	float GetFarPlane() const;

	// 设置投影大小(水平)
	void SetSize(float size);
	// 获取投影大小(水平）
	float GetSize() const;

	// 设置屏幕宽高比
	void SetAspectRatio(float ratio);
	// 获取屏幕宽高比
	float GetAspectRatio() const;

	// 设置清除深度
	void SetClearDepth(float depth);
	// 获取清除深度
	float GetClearDepth() const;

	// 设置清除颜色
	void SetClearColor(const XMath::Vector4& color);
	// 获取清除颜色
	const XMath::Vector4& GetClearColor() const;

	// 获取投影矩阵
	XMath::Matrix4x4 GetProjMatrix() const;



private:
	~Camera() override;

	// 渲染到纹理的名称
	std::string m_RenderTextureName;

	// 是否为主摄像机
	bool m_IsMainCamera = false;

	// 清除方式
	ClearFlag m_ClearFlag = ClearFlag::SolidColor;

	// 视口区域
	Rect m_ViewPortRect = { 0.0f, 0.0f, 1.0f, 1.0f };

	// 投影
	ProjectionType m_ProjectionType = ProjectionType::Perspective;
	float m_FieldOfViewY = 60.0f;
	float m_NearPlane = 0.3f;
	float m_FarPlane = 1000.0f;
	float m_Size = 5.0f;
	float m_AspectRatio = 1.0f;

	// 清除
	float m_ClearDepth = 1.0f;
	XMath::Vector4 m_ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

};

