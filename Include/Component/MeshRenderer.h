
#pragma once

#include "Component.h"
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <variant>
#include <XCore.h>

class Shader;

class MaterialPropertyBlock
{
public:
	using Property = std::variant<
		int, float, XMath::Vector4, Color, XMath::Matrix4x4,
		std::vector<XMath::Vector4>, std::vector<XMath::Matrix4x4>>;

	void SetInt(std::string_view name, int value);
	void SetInt(size_t propertyId, int value);

	void SetFloat(std::string_view name, float value);
	void SetFloat(size_t propertyId, float value);

	void SetVector(std::string_view name, const XMath::Vector4& value);
	void SetVector(size_t propertyId, const XMath::Vector4& value);

	void SetColor(std::string_view name, const Color& value);
	void SetColor(size_t propertyId, const Color& value);

	void SetMatrix(std::string_view name, const XMath::Matrix4x4& value);
	void SetMatrix(size_t propertyId, const XMath::Matrix4x4& value);

	void SetVectorArray(std::string_view name, const std::vector<XMath::Vector4>& value);
	void SetVectorArray(size_t propertyId, const std::vector<XMath::Vector4>& value);

	void SetMatrixArray(std::string_view name, const std::vector<XMath::Matrix4x4>& value);
	void SetMatrixArray(size_t propertyId, const std::vector<XMath::Matrix4x4>& value);

	bool HasProperty(std::string_view name) const;

private:
	friend class CommandBuffer;

	std::unordered_map<size_t, Property> m_Properties;
};

class Material
{
public:

	Material();
	Material(Shader* pShader);

	void SetInt(std::string_view name, int value);
	void SetInt(size_t propertyId, int value);

	void SetFloat(std::string_view name, float value);
	void SetFloat(size_t propertyId, float value);

	void SetVector(std::string_view name, const XMath::Vector4& value);
	void SetVector(size_t propertyId, const XMath::Vector4& value);

	void SetColor(std::string_view name, const Color& value);
	void SetColor(size_t propertyId, const Color& value);

	void SetMatrix(std::string_view name, const XMath::Matrix4x4& value);
	void SetMatrix(size_t propertyId, const XMath::Matrix4x4& value);

	void SetVectorArray(std::string_view name, const std::vector<XMath::Vector4>& value);
	void SetVectorArray(size_t propertyId, const std::vector<XMath::Vector4>& value);

	void SetMatrixArray(std::string_view name, const std::vector<XMath::Matrix4x4>& value);
	void SetMatrixArray(size_t propertyId, const std::vector<XMath::Matrix4x4>& value);

	bool HasProperty(std::string_view name) const;

	void SetTexture(size_t propertyId, std::string_view texName);
	std::string_view GetTexture(size_t propertyId) const;

	void SetShader(Shader* pShader);
	Shader* GetShader();

private:
	friend class CommandBuffer;
	
	Shader* m_pShader = nullptr;
	std::map<size_t, std::string> m_Textures;
	MaterialPropertyBlock m_PropertyBlock;
};

class MeshRenderer : public Component
{
public:
	const std::string& GetName() const override;
	static const std::string& GetType();

	MeshRenderer(GameObject* pObject);
	Component* Instantiate(GameObject* pObject) override;

	void SetNumMaterials(size_t count);
	void SetMaterial(Material* pMaterial);
	void SetMaterial(size_t pos, Material* pMaterial);
	void SetPropertyBlock(MaterialPropertyBlock* pPropertyBlock);
	void SetPropertyBlock(size_t pos, MaterialPropertyBlock* pPropertyBlock);

	Material* GetMaterial(size_t pos = 0);

	void SetCastShadows(bool enabled);
	bool GetCastShadows() const;

private:
	~MeshRenderer() override;

private:
	friend class ResourceManager;
	friend class Scene;


	std::vector<std::unique_ptr<Material>> m_pMaterials;
	std::vector<Material*> m_pSharedMaterials;
	std::vector<MaterialPropertyBlock*> m_pMaterialPropertyBlocks;
	bool m_CastShadows = true;
	bool m_ReceivedShadows = true;
};
