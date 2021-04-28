#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Graphics/ResourceManager.h>
#include <Graphics/RenderStates.h>

#include <fstream>
#include <vector>
#include <wrl/client.h>

#include <HLSL/XDefines.h>

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

ResourceManager::ResourceManager()
{
	if (s_pSingleton)
		throw std::exception("ResourceManager is a singleton!");
	s_pSingleton = this;
}

ResourceManager::~ResourceManager()
{
	for (auto ptr : m_pModels)
		ptr.second->Destroy();
}

ResourceManager& ResourceManager::Get()
{
	return *s_pSingleton;
}

void ResourceManager::CreateEffectsFromJson(std::string_view jsonPath)

{
	std::ifstream fin(jsonPath.data());
	nlohmann::json json;
	if (!fin.is_open())
		throw std::exception("Error: effect json file is not found!");
	fin >> json;
	if (!json.is_array())
		throw std::exception("Error: effect json file must start with array!");
	size_t effectCount = json.size();
	for (size_t i = 0; i < effectCount; ++i)
	{
		auto& effectObject = json[i];
		if (!effectObject.is_object())
			throw std::exception("Error: effect object must be json object!");

		auto effectName = effectObject["effectName"].get<std::string>();
		auto effectPath = effectObject["path"].get<std::string>();
		auto& passArray = effectObject["passes"];
		auto passCount = passArray.size();

		auto pEffect = std::make_unique<Effect>();

		std::map<std::string, std::map<std::string, ComPtr<ID3DBlob>>> shaderBlobs;
		std::vector<std::string> shaderTypes = {
			"vs", "ds", "hs", "gs", "ps", "cs"
		};

		for (size_t j = 0; j < passCount; ++j)
		{
			shaderBlobs.try_emplace(shaderTypes[j]);
			auto& passObject = passArray[j];
			auto passName = passObject["passName"].get<std::string>();
			EffectPassDesc passDesc;
			size_t shaderTypeCount = shaderTypes.size();
			std::vector<std::string> shaderNames(shaderTypeCount);
			for (size_t k = 0; k < shaderTypeCount; ++k)
			{
				auto it = passObject.find(shaderTypes[k]);
				if (it != passObject.end() && !it->is_null())
				{
					shaderNames[k] = it->get<std::string>();
					LPCSTR* strs = reinterpret_cast<LPCSTR*>(&passDesc);
					strs[k] = shaderNames[k].c_str();

					auto it = shaderBlobs[shaderTypes[k]].find(strs[k]);
					if (it == shaderBlobs[shaderTypes[k]].end())
					{
						it = shaderBlobs[shaderTypes[k]].emplace(strs[k], nullptr).first;
						std::wstring hlslPath = UTF8ToUCS2(effectPath);
						if (hlslPath.back() != L'/' && hlslPath.back() != L'\\')
							hlslPath.push_back(L'/');
						hlslPath += UTF8ToUCS2(effectName);
						std::wstring csoPath = hlslPath + L'_';
						csoPath += UTF8ToUCS2(shaderNames[k]);
						std::string sm = shaderTypes[k] + "_5_0";
						hlslPath += L".hlsl";
						csoPath += L".cso";
						ThrowIfFailed(CreateShaderFromFile(csoPath.c_str(), hlslPath.c_str(), strs[k], sm.c_str(), it->second.GetAddressOf()));
						ThrowIfFailed(pEffect->AddShader(strs[k], m_pDevice.Get(), it->second.Get()));
					}
				}
			}

			ThrowIfFailed(pEffect->AddEffectPass(passName.c_str(), m_pDevice.Get(), &passDesc));
		}

		int slot = pEffect->GetSamplerStateByName(X_SAMPLER_LINEAR_WARP);
		if (slot != -1) pEffect->SetSamplerStateBySlot(slot, RenderStates::SSLinearWrap.Get());
		slot = pEffect->GetSamplerStateByName(X_SAMPLER_ANISTROPIC_WRAP);
		if (slot != -1) pEffect->SetSamplerStateBySlot(slot, RenderStates::SSAnistropicWrap.Get());
		slot = pEffect->GetSamplerStateByName(X_SAMPLER_POINT_CLAMP);
		if (slot != -1) pEffect->SetSamplerStateBySlot(slot, RenderStates::SSPointClamp.Get());
		slot = pEffect->GetSamplerStateByName(X_SAMPLER_SHADOW);
		if (slot != -1) pEffect->SetSamplerStateBySlot(slot, RenderStates::SSShadow.Get());

		m_pEffects[effectName].swap(pEffect);

	}
}

bool ResourceManager::RegisterEffect(std::string_view effectName, std::unique_ptr<Effect>&& pEffect)
{
	return m_pEffects.try_emplace(effectName.data(), std::move(pEffect)).second;
}

Effect* ResourceManager::FindEffect(std::string_view effectName)
{
	auto it = m_pEffects.find(effectName.data());
	if (it != m_pEffects.end())
		return it->second.get();
	return nullptr;
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

void ResourceManager::_LoadSubModel(std::string_view path, GameObject* pModel, const aiScene* pAssimpScene, const aiMesh* pAssimpMesh)
{
	uint32_t numVertices = pAssimpMesh->mNumVertices;
	auto pMeshFilter = pModel->AddComponent<MeshFilter>();
	auto pMaterial = pModel->AddComponent<Material>();
	auto pMeshData = (pMeshFilter->m_pMesh = std::make_unique<MeshData>()).get();

	// 顶点位置
	if (numVertices > 0)
	{
		pMeshData->vertices.resize(numVertices);
		memcpy_s(pMeshData->vertices.data(), sizeof(Vector3) * numVertices,
			pAssimpMesh->mVertices, sizeof(Vector3) * numVertices);
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
		if (aiReturn_SUCCESS == pAssimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, (float*)&ambient, &num))
		{
			pMaterial->SetVector(X_CB_MAT_AMBIENT, 4, (float*)&ambient);
			success_bit |= 0b1u;
		}
		if (aiReturn_SUCCESS == pAssimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, (float*)&diffuse, &num))
		{
			pMaterial->SetVector(X_CB_MAT_DIFFUSE, 4, (float*)&diffuse);
			success_bit |= 0b10u;
		}
		if (aiReturn_SUCCESS == pAssimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, (float*)&specular, &num))
		{
			pMaterial->SetVector(X_CB_MAT_SPECULAR, 4, (float*)&specular);
			success_bit |= 0b100u;
		}
		if (aiReturn_SUCCESS == pAssimpMaterial->Get(AI_MATKEY_COLOR_REFLECTIVE, (float*)&reflective, &num))
		{
			pMaterial->SetVector(X_CB_MAT_REFLECTIVE, 3, (float*)&reflective);
			success_bit |= 0b1000u;
		}
		if (aiReturn_SUCCESS == pAssimpMaterial->Get(AI_MATKEY_SHININESS, (float*)&shininess, &num))
		{
			pMaterial->SetVector(X_CB_MAT_SHININESS, 1, (float*)&shininess);
			success_bit |= 0b10000u;
		}

		if ((success_bit & 0b111u) == 0b111u)
		{
			pMaterial->SetEffectPass("Phong", "Basic");
		}
		else if (success_bit & 0b11010u)
		{
			pMaterial->SetEffectPass("BlinnPhong", "Basic");
		}

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
			pMaterial->SetTexture(Material::DiffuseMap, finalPath);
		}
		else
		{
			pMaterial->SetPassName("Color");
		}
	}
}

