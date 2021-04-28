#include <Hierarchy/Scene.h>
#include <Hierarchy/GameObject.h>
#include <Utils/Geometry.h>
#include <HLSL/XDefines.h>
#include <Graphics/ResourceManager.h>

using namespace XMath;

Scene::Scene()
{
	// 预先分配对象池
	m_GameObjectPools.emplace_back(ObjectPool(500, sizeof(GameObject)));
	m_pCurrGameObjectPool = m_GameObjectPools.begin();

	// 添加默认对象
	auto pMainCamera = AddGameObject("MainCamera");
	m_pMainCamera = pMainCamera->AddComponent<Camera>();
	m_pMainCamera->SetRenderTextureName("@BackBuffer");

	auto pDirectionalLight = AddGameObject("DirectionalLight");
	auto pLight = pDirectionalLight->AddComponent<Light>();
}

Scene::~Scene()
{
	// 摧毁GameObject
	auto ptrs = GetRootGameObjects();
	for (auto ptr : ptrs)
	{
		ptr->Destroy();
	}
}

GameObject* Scene::AddGameObject()
{
	return GameObject::Create(this);
}

GameObject* Scene::AddGameObject(std::string_view name)
{
	return GameObject::Create(this, name);
}

GameObject* Scene::AddCube(std::string_view name)
{
	GameObject* pObj = GameObject::Create(this, name);
	auto& pMeshData = pObj->AddComponent<MeshFilter>()->m_pMesh;
	pMeshData = std::make_unique<MeshData>();
	Geometry::CreateBox(pMeshData.get());
	pObj->AddComponent<Material>();
	return pObj;
}

GameObject* Scene::AddSphere(std::string_view name)
{
	GameObject* pObj = GameObject::Create(this, name);
	pObj->AddComponent<MeshFilter>();
	pObj->AddComponent<Material>();
	return pObj;
}

GameObject* Scene::AddCylinder(std::string_view name)
{
	GameObject* pObj = GameObject::Create(this, name);
	pObj->AddComponent<MeshFilter>();
	pObj->AddComponent<Material>();
	return pObj;
}

GameObject* Scene::AddModel(std::string_view name, std::string_view filename)
{
	auto pModel = ResourceManager::Get().FindModel(filename);
	if (!pModel)
	{
		pModel = ResourceManager::Get().CreateModel(filename);
	}
	if (pModel)
	{
		GameObject* pObject = ResourceManager::Get().InstantiateModel(this, filename);
		pObject->SetName(name);
		return pObject;
	}
	return nullptr;
}

GameObject* Scene::FindGameObject(std::string_view name)
{
	auto it = m_GameObjects.find(name.data());
	if (it != m_GameObjects.end())
		return it->second;
	return nullptr;
}

std::vector<GameObject*> Scene::GetAvailableGameObjects()
{
	std::vector<GameObject*> res(m_GameObjects.size());
	size_t i = 0;
	for (auto& p : m_GameObjects)
	{
		if (p.second->IsEnabled())
			res[i++] = p.second;
	}
	return res;
}

std::vector<GameObject*> Scene::GetGameObjects()
{
	std::vector<GameObject*> res(m_GameObjects.size());
	size_t i = 0;
	for (auto& p : m_GameObjects)
	{
		res[i++] = p.second;
	}
	return res;
}

std::vector<GameObject*> Scene::GetRootGameObjects()
{
	std::vector<GameObject*> res(m_RootGameObjects.size());
	size_t i = 0;
	for (auto& p : m_RootGameObjects)
	{
		res[i++] = p.second;
	}
	return res;
}

std::vector<Component*> Scene::GetComponents(std::string_view componentName)
{
	auto& componentSet = m_Components[componentName.data()];
	std::vector<Component*> components;
	for (auto ptr : componentSet)
	{
		components.push_back(ptr);
	}
	return components;
}

Camera* Scene::GetMainCamera()
{
	return m_pMainCamera;
}

void Scene::SetMainCamera(Camera* pCamera)
{
	if (!pCamera)
		return;
	GameObject* pObject = pCamera->GetGameObject();
	if (pObject && pObject->GetScene() == this)
	{
		m_pMainCamera = pCamera;
	}
}

void* Scene::NotifyGameObjectCreated(std::string_view name)
{
	void* pRes = m_pCurrGameObjectPool->Allocate();
	if (pRes == nullptr)
	{
		// 遍历一圈，寻找空位
		auto pCurrPool = m_pCurrGameObjectPool;
		++pCurrPool;
		if (pCurrPool == m_GameObjectPools.end())
			pCurrPool = m_GameObjectPools.begin();
		while (pCurrPool != m_pCurrGameObjectPool)
		{
			pRes = pCurrPool->Allocate();
			if (pRes)
				break;
			++pCurrPool;
			if (pCurrPool == m_GameObjectPools.end())
				pCurrPool = m_GameObjectPools.begin();
		}
	}
	// 如果仍没有，则需要新建对象池
	if (pRes == nullptr)
	{
		size_t newSize = static_cast<size_t>(m_GameObjectPools.back().GetCapacity() * 1.25f) + 1;
		m_GameObjectPools.emplace_back(ObjectPool(newSize, sizeof(GameObject)));
		pRes = m_GameObjectPools.back().Allocate();
	}

	if (pRes)
	{
		m_GameObjects[std::string(name.data())] = static_cast<GameObject*>(pRes);
	}

	return pRes;
}

void Scene::NotifyGameObjectDestroyed(std::string_view name, GameObject* pObject)
{
	// 键值不匹配
	auto it = m_GameObjects.find(name.data());
	if (it == m_GameObjects.end() || it->second != pObject)
		return;

	m_GameObjects.erase(name.data());
	if (m_pCurrGameObjectPool->Free(pObject))
		return;

	// 循环遍历寻找
	auto pCurrPool = m_pCurrGameObjectPool;
	++pCurrPool;
	while (pCurrPool != m_pCurrGameObjectPool)
	{
		if (pCurrPool == m_GameObjectPools.end())
			pCurrPool = m_GameObjectPools.begin();

		if (pCurrPool->Free(pObject))
			return;
		++pCurrPool;
	}
}

bool Scene::NotifyGameObjectRenamed(std::string_view oldName, std::string_view newName)
{
	// 先找有没有重复的新名称
	auto it = m_GameObjects.find(newName.data());
	if (it != m_GameObjects.end())
		return false;
	// 再找有没有存在的旧名称
	it = m_GameObjects.find(oldName.data());
	if (it == m_GameObjects.end())
		return false;
	// 开始换名
	GameObject* pObject = it->second;
	m_GameObjects.erase(it->first);
	m_GameObjects[std::string(newName.data())] = pObject;
	return true;
}

size_t Scene::GetDefaultGameObjectCounter()
{
	return m_DefaultGameObjectCounter++;
}

void* Scene::NotifyComponentCreated(std::string_view componentName, size_t size)
{
	// 若组件池为空，则新建一个
	std::string name = componentName.data();
	auto& componentPoolList = m_ComponentPools[name];
	auto& pCurrComponentPool = m_pCurrComponentPools[name];
	if (componentPoolList.empty())
	{
		componentPoolList.emplace_back(ObjectPool(500, size));
		pCurrComponentPool = componentPoolList.begin();
	}

	void* pRes = pCurrComponentPool->Allocate();
	if (pRes == nullptr)
	{
		// 遍历一圈，寻找空位
		auto pCurrPool = pCurrComponentPool;
		++pCurrPool;
		if (pCurrPool == componentPoolList.end())
			pCurrPool = componentPoolList.begin();
		while (pCurrPool != pCurrComponentPool)
		{
			pRes = pCurrPool->Allocate();
			if (pRes)
				break;
			++pCurrPool;
			if (pCurrPool == componentPoolList.end())
				pCurrPool = componentPoolList.begin();
		}
	}
	// 如果仍没有，则需要新建对象池
	if (pRes == nullptr)
	{
		size_t newSize = static_cast<size_t>(componentPoolList.back().GetCapacity() * 1.25f) + 1;
		componentPoolList.emplace_back(ObjectPool(newSize, size));
		pRes = componentPoolList.back().Allocate();
	}

	if (pRes)
	{
		m_Components[name].insert(reinterpret_cast<Component*>(pRes));
	}

	return pRes;
}

void Scene::NotifyComponentDestroyed(std::string_view componentName, Component* pComponent)
{
	std::string name = componentName.data();
	// 没有这样的组件类型
	auto it = m_Components.find(name);
	if (it == m_Components.end())
		return;

	// 没有这样的组件
	if (it->second.find(pComponent) == it->second.end())
		return;

	it->second.erase(pComponent);

	auto& pCurrComponentPool = m_pCurrComponentPools[name];
	if (pCurrComponentPool->Free(pComponent))
		return;

	// 循环遍历寻找
	auto& components = m_ComponentPools[name];
	auto pCurrPool = pCurrComponentPool;
	++pCurrPool;
	while (pCurrPool != pCurrComponentPool)
	{
		if (pCurrPool == components.end())
			pCurrPool = components.begin();

		if (pCurrPool->Free(pComponent))
			return;
		++pCurrPool;
	}
}
