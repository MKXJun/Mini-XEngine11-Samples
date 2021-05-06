
#pragma once

#include <string_view>
#include <wrl/client.h>
#include <d3d11_1.h>
#include <map>

#include <Hierarchy/GameObject.h>
// Add all components here
#include <Component/Camera.h>
#include <Component/Light.h>
#include <Component/MeshFilter.h>
#include <Component/Material.h>

#include <Graphics/Effect.h>

struct aiMesh;
struct aiScene;

struct MeshGraphicsResource
{
	~MeshGraphicsResource()
	{
		for (auto ptr : vertexBuffers)
			if (ptr) ptr->Release();
		if (indexBuffer) indexBuffer->Release();
	}

	std::vector<ID3D11Buffer*> vertexBuffers;
	std::vector<uint32_t> strides;
	std::vector<uint32_t> offsets;
	ID3D11Buffer* indexBuffer = nullptr;
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayouts;
	

};

class ResourceManager
{
public:
	ResourceManager(ID3D11Device* device);
	~ResourceManager();

	static ResourceManager& Get();

	GameObject* CreateModel(std::string_view path);
	GameObject* FindModel(std::string_view path);
	GameObject* InstantiateModel(Scene* pScene, std::string_view path);

	ID3D11ShaderResourceView* CreateTextureSRV(std::string_view path);
	ID3D11ShaderResourceView* FindTextureSRV(std::string_view path);

	MeshGraphicsResource* CreateMeshGraphicsResources(MeshData* ptr);
	MeshGraphicsResource* FindMeshGraphicsResources(MeshData* ptr);
	void DestroyMeshGraphicsResources(MeshData* ptr);




private:

	void _LoadSubModel(std::string_view path, GameObject* pModel, const aiScene* pAssimpScene, const aiMesh* pAssimpMesh);

	Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;

	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_pTextureSRVs;

	std::map<std::string, GameObject*> m_pModels;

	std::map<MeshData*, MeshGraphicsResource> m_MeshGraphicsResources;
};
