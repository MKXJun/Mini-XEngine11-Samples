#include <Component/MeshFilter.h>
#include <Hierarchy/GameObject.h>

#pragma warning(disable: 26812)

using namespace XMath;

void MeshData::UpdateBoundingData()
{
	uint32_t vCount = (uint32_t)vertices.size();
	if (vCount == 0)
		vMax = vMin = Vector3::Zero();
	else
	{
		vMax = vMin = vertices[0];
		for (uint32_t i = 1; i < vCount; ++i)
		{
			vMax = vMax.cwiseMax(vertices[i]);
			vMin = vMin.cwiseMin(vertices[i]);
		}
	}
}

void MeshData::MarkVertexDynamic(bool isDynamic)
{
	m_IsDynamicVertex = isDynamic;
}

void MeshData::UploadMeshData()
{
	m_IsUploading = true;

	size_t vertCount = vertices.size();
	size_t normalCount = normals.size();
	size_t tangentCount = tangents.size();
	size_t texcoordCount = texcoords.size();
	size_t colorCount = colors.size();
	if (vertCount)
	{
		if (normalCount && vertCount != normalCount ||
			tangentCount && vertCount != tangentCount ||
			texcoordCount && vertCount != texcoordCount ||
			colorCount && vertCount != colorCount)
			throw std::exception("Error: Other vertex types(like normal) should"
				"have identity number of vertices or zero!");

	}
}

static const std::string s_Name = "MeshFilter";

const std::string& MeshFilter::GetName() const
{
	return s_Name;
}

const std::string& MeshFilter::GetType()
{
	return s_Name;
}

MeshFilter::MeshFilter(GameObject* pObject)
	: Component(pObject)
{
}

MeshFilter::~MeshFilter()
{
}

Component* MeshFilter::Instantiate(GameObject* pObject)
{
	if (pObject->FindComponent<MeshFilter>())
		return nullptr;
	MeshFilter* pMeshFilter = pObject->AddComponent<MeshFilter>();
	if (m_pSharedMesh)
		pMeshFilter->m_pSharedMesh = m_pSharedMesh;
	else
		pMeshFilter->m_pSharedMesh = m_pMesh.get();
	pMeshFilter->m_IsEnabled = m_IsEnabled;
	return pMeshFilter;
}

MeshData* MeshFilter::GetSharedMesh() 
{ 
	return m_pSharedMesh;
}

MeshData* MeshFilter::GetMesh() 
{ 
	return m_pMesh.get(); 
}

MeshData* MeshFilter::CreateMesh() 
{ 
	if (m_pMesh)
		return nullptr;
	m_pMesh = std::unique_ptr<MeshData>(); 
	m_pSharedMesh = nullptr;
	return m_pMesh.get();
}