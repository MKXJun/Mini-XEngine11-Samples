#include <Component/Material.h>
#include <Hierarchy/GameObject.h>

#pragma warning(disable: 26812)

static const std::string s_Name = "Material";

const std::string& Material::GetName() const
{
	return s_Name;
}

const std::string& Material::GetType()
{
	return s_Name;
}

Material::Material(GameObject* pObject)
	: Component(pObject)
{
}

Material::~Material()
{
}

Component* Material::Instantiate(GameObject* pObject)
{
	if (pObject->FindComponent<Material>())
		return nullptr;
	Material* pMaterial = pObject->AddComponent<Material>();
	pMaterial->m_Attributes = m_Attributes;
	pMaterial->m_EffectName = m_EffectName;
	pMaterial->m_IsEnabled = m_IsEnabled;
	pMaterial->m_PassName = m_PassName;
	pMaterial->m_Textures = m_Textures;
	return pMaterial;
}

void Material::SetScalar(std::string_view str, float num)
{
	SetAttribute(str, &num, 0, sizeof(uint32_t));
}

void Material::SetScalar(std::string_view str, int32_t num)
{
	SetAttribute(str, &num, 0, sizeof(uint32_t));
}

void Material::SetScalar(std::string_view str, uint32_t num)
{
	SetAttribute(str, &num, 0, sizeof(uint32_t));
}

void Material::SetVector(std::string_view str, uint32_t elemCount, float nums[4])
{
	elemCount = elemCount < 4 ? elemCount : 4;
	elemCount = elemCount > 0 ? elemCount : 0;
	SetAttribute(str, nums, 0, sizeof(uint32_t) * elemCount);
}

void Material::SetVector(std::string_view str, uint32_t elemCount, int32_t nums[4])
{
	elemCount = elemCount < 4 ? elemCount : 4;
	elemCount = elemCount > 0 ? elemCount : 0;
	SetAttribute(str, nums, 0, sizeof(uint32_t) * elemCount);
}

void Material::SetVector(std::string_view str, uint32_t elemCount, uint32_t nums[4])
{
	elemCount = elemCount < 4 ? elemCount : 4;
	elemCount = elemCount > 0 ? elemCount : 0;
	SetAttribute(str, nums, 0, sizeof(uint32_t) * elemCount);
}

void Material::SetMatrix(std::string_view str, uint32_t rows, float nums[])
{
	rows = rows < 4 ? rows : 4;
	rows = rows > 0 ? rows : 0;
	SetAttribute(str, nums, 0, sizeof(uint32_t) * rows * 4);
}

void Material::SetMatrix(std::string_view str, uint32_t rows, int32_t nums[])
{
	rows = rows < 4 ? rows : 4;
	rows = rows > 0 ? rows : 0;
	SetAttribute(str, nums, 0, sizeof(uint32_t) * rows * 4);
}

void Material::SetMatrix(std::string_view str, uint32_t rows, uint32_t nums[])
{
	rows = rows < 4 ? rows : 4;
	rows = rows > 0 ? rows : 0;
	SetAttribute(str, nums, 0, sizeof(uint32_t) * rows * 4);
}

bool Material::HasAttribute(std::string_view str) const
{
	return m_Attributes.find(str.data()) != m_Attributes.end();
}

void Material::SetTexture(TextureType type, std::string_view texName)
{
	m_Textures[type] = texName;
}

const std::string& Material::GetTexture(TextureType type) const
{
	auto it = m_Textures.find(type);
	if (it == m_Textures.end())
		throw std::exception("Texture not found!");
	return it->second;
}

void Material::SetEffectPass(std::string effectName, std::string passName)
{
	m_EffectName = std::move(effectName);
	m_PassName = std::move(passName);
}

void Material::SetPassName(std::string passName)
{
	m_PassName = std::move(passName);
}

bool Material::TryGetTexture(TextureType type, std::string& texOut) const
{
	auto it = m_Textures.find(type);
	if (it == m_Textures.end())
		return false;
	texOut = it->second;
	return true;
}

void Material::SetAttribute(std::string_view str, const void* data, uint32_t byteOffset, uint32_t byteCount)
{
	if (byteCount == 0)
		return;

	if (m_Attributes.find(str.data()) == m_Attributes.end())
	{
		if (byteOffset != 0)
			throw std::bad_alloc();

		auto& attri = m_Attributes.try_emplace(str.data(), std::vector<uint8_t>(byteCount)).first->second;
		memcpy_s(attri.data(), byteCount, data, byteCount);
	}
	else
	{
		auto& attri = m_Attributes[str.data()];
		if (byteOffset + byteCount > (uint32_t)attri.size())
			throw std::exception("Error: Attribute Access out-of-range.");
		memcpy_s(attri.data() + byteOffset, byteCount, data, byteCount);
	}
}

const std::string& Material::GetPassName() const
{
	return m_PassName;
}

std::vector<const std::pair<const std::string, std::vector<uint8_t>>*> Material::GetAttributes() const
{
	std::vector<const std::pair<const std::string, std::vector<uint8_t>>*> attributes;
	for (auto& p : m_Attributes)
	{
		attributes.push_back(&p);
	}
	return attributes;

}