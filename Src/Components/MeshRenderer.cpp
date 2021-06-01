#include <Component/MeshRenderer.h>
#include <Graphics/Shader.h>
#include <Hierarchy/GameObject.h>


#pragma warning(disable: 26812)

//
// MeshRenderer
//
static const std::string s_Name = "MeshRenderer";

const std::string& MeshRenderer::GetName() const
{
	return s_Name;
}

const std::string& MeshRenderer::GetType()
{
	return s_Name;
}

MeshRenderer::MeshRenderer(GameObject* pObject)
	: Component(pObject)
{
}

MeshRenderer::~MeshRenderer()
{
}

Component* MeshRenderer::Instantiate(GameObject* pObject)
{
	if (pObject->FindComponent<MeshRenderer>())
		return nullptr;
	MeshRenderer* pMeshRenderer = pObject->AddComponent<MeshRenderer>();
	if (!m_pMaterials.empty())
	{
		for (auto& ptr : m_pMaterials)
			pMeshRenderer->m_pSharedMaterials.push_back(ptr.get());
	}
	else if (!m_pSharedMaterials.empty())
	{
		pMeshRenderer->m_pSharedMaterials = m_pSharedMaterials;
	}

	pMeshRenderer->m_CastShadows = m_CastShadows;
	pMeshRenderer->m_ReceivedShadows = m_ReceivedShadows;
	return pMeshRenderer;
}

void MeshRenderer::SetNumMaterials(size_t count)
{
	m_pSharedMaterials.resize(count);
}

void MeshRenderer::SetMaterial(Material* pMaterial)
{
	if (m_pSharedMaterials.empty())
		m_pSharedMaterials.push_back(pMaterial);
	else
		m_pSharedMaterials[0] = pMaterial;
}

void MeshRenderer::SetMaterial(size_t pos, Material* pMaterial)
{
	if (pos >= m_pSharedMaterials.size())
		m_pSharedMaterials.resize(pos + 1);
	m_pSharedMaterials[pos] = pMaterial;
}

void MeshRenderer::SetPropertyBlock(MaterialPropertyBlock* pPropertyBlock)
{
	if (m_pMaterialPropertyBlocks.empty())
		m_pMaterialPropertyBlocks.push_back(pPropertyBlock);
	else
		m_pMaterialPropertyBlocks[0] = pPropertyBlock;
}

void MeshRenderer::SetPropertyBlock(size_t pos, MaterialPropertyBlock* pPropertyBlock)
{
	if (pos >= m_pMaterialPropertyBlocks.size())
		m_pMaterialPropertyBlocks.resize(pos + 1);
	m_pMaterialPropertyBlocks[pos] = pPropertyBlock;
}

Material* MeshRenderer::GetMaterial(size_t pos)
{
	if (!m_pSharedMaterials.empty())
		return m_pSharedMaterials[pos];
	else
		return m_pMaterials[pos].get();
}

void MeshRenderer::SetCastShadows(bool enabled)
{
	m_CastShadows = enabled;
}

bool MeshRenderer::GetCastShadows() const
{
	return m_CastShadows;
}

//
// Material
//

Material::Material()
{
}

Material::Material(Shader* pShader)
	: m_pShader(pShader)
{
}

void Material::SetInt(std::string_view name, int value)
{
	m_PropertyBlock.SetInt(name, value);
}

void Material::SetInt(size_t propertyId, int value)
{
	m_PropertyBlock.SetInt(propertyId, value);
}

void Material::SetFloat(std::string_view name, float value)
{
	m_PropertyBlock.SetFloat(name, value);
}

void Material::SetFloat(size_t propertyId, float value)
{
	m_PropertyBlock.SetFloat(propertyId, value);
}

void Material::SetVector(std::string_view name, const XMath::Vector4& value)
{
	m_PropertyBlock.SetVector(name, value);
}

void Material::SetVector(size_t propertyId, const XMath::Vector4& value)
{
	m_PropertyBlock.SetVector(propertyId, value);
}

void Material::SetColor(std::string_view name, const Color& value)
{
	m_PropertyBlock.SetColor(name, value);
}

void Material::SetColor(size_t propertyId, const Color& value)
{
	m_PropertyBlock.SetColor(propertyId, value);
}

void Material::SetMatrix(std::string_view name, const XMath::Matrix4x4& value)
{
	m_PropertyBlock.SetMatrix(name, value);
}

void Material::SetMatrix(size_t propertyId, const XMath::Matrix4x4& value)
{
	m_PropertyBlock.SetMatrix(propertyId, value);
}

void Material::SetVectorArray(std::string_view name, const std::vector<XMath::Vector4>& value)
{
	m_PropertyBlock.SetVectorArray(name, value);
}

void Material::SetVectorArray(size_t propertyId, const std::vector<XMath::Vector4>& value)
{
	m_PropertyBlock.SetVectorArray(propertyId, value);
}

void Material::SetMatrixArray(std::string_view name, const std::vector<XMath::Matrix4x4>& value)
{
	m_PropertyBlock.SetMatrixArray(name, value);
}

void Material::SetMatrixArray(size_t propertyId, const std::vector<XMath::Matrix4x4>& value)
{
	m_PropertyBlock.SetMatrixArray(propertyId, value);
}

bool Material::HasProperty(std::string_view name) const
{
	return m_PropertyBlock.HasProperty(name);
}

void Material::SetTexture(size_t propertyId, std::string_view texName)
{
	m_Textures[propertyId] = texName;
}

std::string_view Material::GetTexture(size_t propertyId) const
{
	if (auto it = m_Textures.find(propertyId); it != m_Textures.end())
		return it->second;
	else
		return std::string_view();
}

void Material::SetShader(Shader* pShader)
{
	m_pShader = pShader;
}

Shader* Material::GetShader()
{
	return m_pShader;
}

//
// MaterialPropertyBlock
//
void MaterialPropertyBlock::SetInt(std::string_view name, int value)
{
	SetInt(Shader::StringToID(name), value);
}

void MaterialPropertyBlock::SetInt(size_t propertyId, int value)
{
	m_Properties[propertyId] = value;
}

void MaterialPropertyBlock::SetFloat(std::string_view name, float value)
{
	SetFloat(Shader::StringToID(name), value);
}

void MaterialPropertyBlock::SetFloat(size_t propertyId, float value)
{
	m_Properties[propertyId] = value;
}

void MaterialPropertyBlock::SetVector(std::string_view name, const XMath::Vector4& value)
{
	SetVector(Shader::StringToID(name), value);
}

void MaterialPropertyBlock::SetVector(size_t propertyId, const XMath::Vector4& value)
{
	m_Properties[propertyId] = value;
}

void MaterialPropertyBlock::SetColor(std::string_view name, const Color& value)
{
	SetColor(Shader::StringToID(name), value);
}

void MaterialPropertyBlock::SetColor(size_t propertyId, const Color& value)
{
	m_Properties[propertyId] = value;
}

void MaterialPropertyBlock::SetMatrix(std::string_view name, const XMath::Matrix4x4& value)
{
	SetMatrix(Shader::StringToID(name), value);
}

void MaterialPropertyBlock::SetMatrix(size_t propertyId, const XMath::Matrix4x4& value)
{
	m_Properties[propertyId] = value;
}

void MaterialPropertyBlock::SetVectorArray(std::string_view name, const std::vector<XMath::Vector4>& value)
{
	SetVectorArray(Shader::StringToID(name), value);
}

void MaterialPropertyBlock::SetVectorArray(size_t propertyId, const std::vector<XMath::Vector4>& value)
{
	m_Properties[propertyId] = value;
}

void MaterialPropertyBlock::SetMatrixArray(std::string_view name, const std::vector<XMath::Matrix4x4>& value)
{
	SetMatrixArray(Shader::StringToID(name), value);
}

void MaterialPropertyBlock::SetMatrixArray(size_t propertyId, const std::vector<XMath::Matrix4x4>& value)
{
	m_Properties[propertyId] = value;
}

bool MaterialPropertyBlock::HasProperty(std::string_view name) const
{
	return m_Properties.find(Shader::StringToID(name)) != m_Properties.end();
}
