
#pragma once

#include "Component.h"
#include <memory>
#include <vector>
#include <map>


class Material : public Component
{
public:
	enum TextureType {
		AmbientMap = 0,
		DiffuseMap,
		SpecularMap,
		BumpMap,
		DisplacementMap,
		
		// PBR
		RoughnessMap,
		MetallicMap,
		SheenMap,
		EmissiveMap,
		NormalMap
	};

	const std::string& GetName() const override;
	static const std::string& GetType();

	Material(GameObject* pObject);
	Component* Instantiate(GameObject* pObject) override;

	void SetScalar(std::string_view str, float num);
	void SetScalar(std::string_view str, int32_t num);
	void SetScalar(std::string_view str, uint32_t num);

	void SetVector(std::string_view str, uint32_t elemCount, float nums[4]);
	void SetVector(std::string_view str, uint32_t elemCount, int32_t nums[4]);
	void SetVector(std::string_view str, uint32_t elemCount, uint32_t nums[4]);

	// 假定所有行均为float4，最多允许设置4行
	void SetMatrix(std::string_view str, uint32_t rows, float nums[]);
	void SetMatrix(std::string_view str, uint32_t rows, int32_t nums[]);
	void SetMatrix(std::string_view str, uint32_t rows, uint32_t nums[]);

	bool HasAttribute(std::string_view str) const;

	void SetTexture(TextureType type, std::string_view texName);
	const std::string& GetTexture(TextureType type) const;
	bool TryGetTexture(TextureType type, std::string& texOut) const;

	void SetEffectPass(std::string effectName, std::string passName);
	void SetPassName(std::string passName);

	const std::string& GetEffectName() const;
	const std::string& GetPassName() const;

	std::vector<const std::pair<const std::string, std::vector<uint8_t>>*> GetAttributes() const;

private:
	void SetAttribute(std::string_view str, const void* data, uint32_t byteOffset, uint32_t byteCount);
	~Material() override;

private:
	friend class Renderer;

	std::string m_EffectName;
	std::string m_PassName;
	std::map<TextureType, std::string> m_Textures;
	std::map<std::string, std::vector<uint8_t>> m_Attributes;
};
