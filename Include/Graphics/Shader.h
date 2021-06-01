
#pragma once

//
// 宏相关
//

// 默认开启图形调试器具名化
// 如果不需要该项功能，可通过全局文本替换将其值设置为0
#if (defined(DEBUG) || defined(_DEBUG))
	#ifndef GRAPHICS_DEBUGGER_OBJECT_NAME
	#define GRAPHICS_DEBUGGER_OBJECT_NAME 1
	#endif
#endif


#include <string_view>
#include <XCore.h>
#include <Math/XMath.h>

struct ID3D11ShaderResourceView;

class Shader
{
public:
	using ShaderTagId = size_t;

	static size_t StringToID(std::string_view str)
	{
		static std::hash<std::string_view> hash;
		return hash(str);
	}

	static void InitFromJson(std::string_view jsonFile, std::string_view recordFile = "");
	static Shader* Find(std::string_view name);
	

	void Destroy();

	void SetGlobalInt(std::string_view name, int value);
	void SetGlobalInt(size_t propertyId, int value);

	void SetGlobalFloat(std::string_view name, float value);
	void SetGlobalFloat(size_t propertyId, float value);

	void SetGlobalVector(std::string_view name, const XMath::Vector4& value);
	void SetGlobalVector(size_t propertyId, const XMath::Vector4& value);
	
	void SetGlobalColor(std::string_view name, const Color& value);
	void SetGlobalColor(size_t propertyId, const Color& value);

	void SetGlobalMatrix(std::string_view name, const XMath::Matrix4x4& value);
	void SetGlobalMatrix(size_t propertyId, const XMath::Matrix4x4& value);

	void SetGlobalVectorArray(std::string_view name, const std::vector<XMath::Vector4>& value);
	void SetGlobalVectorArray(size_t propertyId, const std::vector<XMath::Vector4>& value);

	void SetGlobalMatrixArray(std::string_view name, const std::vector<XMath::Matrix4x4>& value);
	void SetGlobalMatrixArray(size_t propertyId, const std::vector<XMath::Matrix4x4>& value);

	size_t GetPassCount() const;

private:
	Shader(std::string_view name);
	~Shader();

private:
	friend class Graphics;
	friend class CommandBuffer;
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

