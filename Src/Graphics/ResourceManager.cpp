#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Graphics/ResourceManager.h>
#include <Graphics/RenderStates.h>
#include <Graphics/Shader.h>

#include <fstream>
#include <vector>
#include <wrl/client.h>

#include "d3dUtil.h"
#include "DXTrace.h"

#include "DDSTextureLoader11.h"
#include "WICTextureLoader11.h"
#include "ScreenGrab11.h"


#include <json.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

using namespace Microsoft::WRL;
using namespace XMath;

namespace
{
	ResourceManager* s_pSingleton = nullptr;
}

ResourceManager::ResourceManager(ID3D11Device* device)
	: m_pDevice(device)
{
	if (s_pSingleton)
		throw std::exception("ResourceManager is a singleton!");
	s_pSingleton = this;
}

ResourceManager::~ResourceManager()
{
	for (auto& p : m_pModels)
	{
		p.second->Destroy();
	}
}

ResourceManager& ResourceManager::Get()
{
	return *s_pSingleton;
}

GameObject* ResourceManager::CreateModel(std::string_view path)
{
	bool emplaced;
	std::map<std::string, GameObject*>::iterator iter;
	std::tie(iter, emplaced) = m_pModels.try_emplace(path.data());
	
	if (!emplaced)
		return nullptr;

	Assimp::Importer importer;

	auto pAssimpScene = importer.ReadFile(path.data(), aiProcess_ConvertToLeftHanded |
		aiProcess_FixInfacingNormals | aiProcess_GenBoundingBoxes | aiProcess_Triangulate | aiProcess_ImproveCacheLocality);

	if (pAssimpScene && !(pAssimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && pAssimpScene->HasMeshes())
	{
		auto pModel = iter->second = GameObject::Create(nullptr, path.data());
		for (uint32_t i = 0; i < pAssimpScene->mNumMeshes; ++i)
		{
			auto pSubModel = GameObject::Create(nullptr, pAssimpScene->mMeshes[i]->mName.C_Str(), pModel);
			_LoadSubModel(path, pSubModel, pAssimpScene, pAssimpScene->mMeshes[i]);
		}

		return pModel;
	}

	return nullptr;
}

GameObject* ResourceManager::FindModel(std::string_view path)
{
	auto it = m_pModels.find(path.data());
	if (it != m_pModels.end())
		return it->second;
	return nullptr;
}

GameObject* ResourceManager::InstantiateModel(Scene* pScene, std::string_view path)
{
	GameObject* pModel = FindModel(path);
	if (!pModel)
		return nullptr;

	GameObject* pObject = pScene->AddGameObject();
	std::function<void(GameObject*, GameObject*, Scene*)> Instantiate = [&Instantiate](GameObject* pDst, GameObject* pSrc, Scene* pScene) {
		auto components = pSrc->GetComponents();
		for (auto& component : components)
			component->Instantiate(pDst);
		GameObject* pChild = nullptr;
		for (int i = 0; pChild = pSrc->GetChild(i); ++i)
		{
			Instantiate(GameObject::Create(pScene, pChild->GetName(), pDst), pChild, pScene);
		}
	};
	Instantiate(pObject, pModel, pScene);
	return pObject;
}

ID3D11ShaderResourceView* ResourceManager::CreateTextureSRV(std::string_view path)
{
	bool emplaced;
	std::map<std::string, ComPtr<ID3D11ShaderResourceView>>::iterator iter;

	std::tie(iter, emplaced) = m_pTextureSRVs.try_emplace(path.data());
	if (!emplaced)
		return nullptr;
	auto wFileName = UTF8ToUCS2(path);
	if (SUCCEEDED(DirectX::CreateDDSTextureFromFile(m_pDevice.Get(),
		wFileName.c_str(), nullptr, iter->second.GetAddressOf())) ||
		SUCCEEDED(DirectX::CreateWICTextureFromFile(m_pDevice.Get(),
			wFileName.c_str(), nullptr, iter->second.GetAddressOf())))
		return iter->second.Get();
	return nullptr;
}

ID3D11ShaderResourceView* ResourceManager::FindTextureSRV(std::string_view path)
{
	auto it = m_pTextureSRVs.find(path.data());
	if (it != m_pTextureSRVs.end())
		return it->second.Get();
	return nullptr;
}

MeshGraphicsResource* ResourceManager::CreateMeshGraphicsResources(MeshData* ptr)
{
	bool emplaced;
	std::map<MeshData*, MeshGraphicsResource>::iterator iter;

	std::tie(iter, emplaced) = m_MeshGraphicsResources.try_emplace(ptr);
	if (!emplaced)
		return nullptr;
	return &iter->second;
}

MeshGraphicsResource* ResourceManager::FindMeshGraphicsResources(MeshData* ptr)
{
	auto it = m_MeshGraphicsResources.find(ptr);
	if (it != m_MeshGraphicsResources.end())
		return &it->second;
	return nullptr;
}

void ResourceManager::DestroyMeshGraphicsResources(MeshData* ptr)
{
	auto it = m_MeshGraphicsResources.find(ptr);
	if (it != m_MeshGraphicsResources.end())
		m_MeshGraphicsResources.erase(it);
}

Material* ResourceManager::CreateMaterial(std::string_view path)
{
	bool emplaced;
	std::map<std::string, Material>::iterator iter;

	std::tie(iter, emplaced) = m_pMaterials.try_emplace(path.data());
	if (!emplaced)
		return nullptr;
	return &iter->second;
}

Material* ResourceManager::FindMaterial(std::string_view path)
{
	auto it = m_pMaterials.find(path.data());
	if (it != m_pMaterials.end())
		return &it->second;
	return nullptr;
}

void ResourceManager::_LoadSubModel(std::string_view path, GameObject* pModel, const aiScene* pAssimpScene, const aiMesh* pAssimpMesh)
{
	uint32_t numVertices = pAssimpMesh->mNumVertices;
	auto pMeshFilter = pModel->AddComponent<MeshFilter>();
	auto pMeshRenderer = pModel->AddComponent<MeshRenderer>();
	auto pMeshData = (pMeshFilter->m_pMesh = std::make_unique<MeshData>()).get();
	auto pMaterial = (pMeshRenderer->m_pMaterials.emplace_back(std::make_unique<Material>())).get();
	// 顶点位置
	if (numVertices > 0)
	{
		pMeshData->vertices.resize(numVertices);
		memcpy_s(pMeshData->vertices.data(), sizeof(Vector3) * numVertices,
			pAssimpMesh->mVertices, sizeof(Vector3) * numVertices);
		pMeshData->vMin = { FLT_MAX, FLT_MAX, FLT_MAX };
		pMeshData->vMax = { FLT_MIN, FLT_MIN, FLT_MIN };
		for (size_t i = 0; i < numVertices; ++i)
		{
			pMeshData->vMin = pMeshData->vMin.cwiseMin(pMeshData->vertices[i]);
			pMeshData->vMax = pMeshData->vMax.cwiseMax(pMeshData->vertices[i]);
		}
	}

	// 法向量
	if (pAssimpMesh->HasNormals())
	{
		pMeshData->normals.resize(numVertices);
		memcpy_s(pMeshData->normals.data(), sizeof(Vector3) * numVertices,
			pAssimpMesh->mNormals, sizeof(Vector3) * numVertices);
	}


	// 切线向量
	if (pAssimpMesh->HasTangentsAndBitangents())
	{
		pMeshData->tangents = std::vector<Vector4>(numVertices, Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		for (uint32_t i = 0; i < numVertices; ++i)
		{
			memcpy_s(pMeshData->tangents.data() + i, sizeof(Vector3),
				pAssimpMesh->mTangents + i, sizeof(Vector3));
		}
	}

	// 纹理坐标
	if (pAssimpMesh->HasTextureCoords(0))
	{
		pMeshData->texcoords.resize(numVertices);
		for (uint32_t i = 0; i < numVertices; ++i)
		{
			memcpy_s(pMeshData->texcoords.data() + i, sizeof(Vector2),
				pAssimpMesh->mTextureCoords[0] + i, sizeof(Vector2));
		}
	}

	// 索引
	uint32_t numFaces = pAssimpMesh->mNumFaces;
	uint32_t numIndices = numFaces * 3;
	if (numIndices > 0)
	{
		pMeshData->indices.resize(numIndices * sizeof(uint32_t));
		pMeshData->indexSize = sizeof(uint32_t);
		uint32_t* pIndices = reinterpret_cast<uint32_t*>(pMeshData->indices.data());
		uint32_t offset;
		for (uint32_t i = 0; i < numFaces; ++i)
		{
			offset = i * 3;
			memcpy_s(pIndices + offset, sizeof(uint32_t) * 3,
				pAssimpMesh->mFaces[i].mIndices, sizeof(uint32_t) * 3);
		}
	}

	

	// 读取材质
	if (pAssimpMesh->mMaterialIndex > 0)
	{
		const aiMaterial* pAssimpMaterial = pAssimpScene->mMaterials[pAssimpMesh->mMaterialIndex];
		Vector4 ambient{}, diffuse{}, specular{};
		ambient.w() = diffuse.w() = specular.w() = 1.0f;
		Vector3 reflective{};
		float shininess{};
		uint32_t num = 3;
		uint32_t success_bit = 0;
		/*if (aiReturn_SUCCESS == pAssimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, (float*)&ambient, &num))
		{
			pMaterial->SetVector(Shader::StringToID(X_CB_MAT_AMBIENT), ambient);
			success_bit |= 0b1u;
		}*/
		if (aiReturn_SUCCESS == pAssimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, (float*)&diffuse, &num))
		{
			pMaterial->SetVector(Shader::StringToID("x_BaseColor"), diffuse);
			success_bit |= 0b10u;
		}
		/*if (aiReturn_SUCCESS == pAssimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, (float*)&specular, &num))
		{
			pMaterial->SetVector(Shader::StringToID(X_CB_MAT_SPECULAR), specular);
			success_bit |= 0b100u;
		}*/

		/*if ((success_bit & 0b111u) == 0b111u)
		{
			pMaterial->SetShaderName("HLSL/Phong");
		}*/

		// 纹理
		if (pAssimpMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			aiString aiPath;
			pAssimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiPath);
			std::string finalPath = path.data();
			size_t pos = finalPath.find_last_of('/');
			if (pos != std::string::npos)
			{
				finalPath.erase(pos + 1);
				finalPath += aiPath.C_Str();
			}
			else
			{
				finalPath = aiPath.C_Str();
			}

			CreateTextureSRV(finalPath);
			pMaterial->SetTexture(Shader::StringToID("x_MainTex"), finalPath);
		}
		//else
		//{
		//	pMaterial->SetShaderName("HLSL/ColorLit");
		//}
	}
}

