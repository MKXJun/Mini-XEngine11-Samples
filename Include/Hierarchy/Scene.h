
#pragma once

#include <string_view>
#include <map>
#include <set>
#include <vector>
#include <list>
#include <Utils/ObjectPool.h>

class GameObject;
class Component;
class Camera;

class Scene
{
public:
	Scene();
	~Scene();

	static Scene* GetMainScene();
	void SetAsMainScene();

	GameObject* AddGameObject();
	GameObject* AddGameObject(std::string_view name);
	GameObject* AddCube(std::string_view name);
	GameObject* AddSphere(std::string_view name);
	GameObject* AddCylinder(std::string_view name);
	GameObject* AddPlane(std::string_view name);
	GameObject* AddModel(std::string_view name, std::string_view filename);

	GameObject* FindGameObject(std::string_view name);

	std::vector<GameObject*> GetAvailableGameObjects();
	std::vector<GameObject*> GetGameObjects();
	std::vector<GameObject*> GetRootGameObjects();
	std::vector<Component*> GetComponents(std::string_view componentName);

	Camera* GetMainCamera();
	void SetMainCamera(Camera* pCamera);

	

private:
	friend class GameObject;
	friend class Component;

	void* NotifyGameObjectCreated(std::string_view name);
	void NotifyGameObjectDestroyed(std::string_view name, GameObject* pObject);
	bool NotifyGameObjectRenamed(std::string_view oldName, std::string_view newName);
	size_t GetDefaultGameObjectCounter();

	void* NotifyComponentCreated(std::string_view componentName, size_t size);
	void NotifyComponentDestroyed(std::string_view componentName, Component* pComponent);


private:

	size_t m_DefaultGameObjectCounter = 0;

	std::map<std::string, GameObject*> m_GameObjects;
	std::map<std::string, GameObject*> m_RootGameObjects;
	
	std::map<std::string, std::set<Component*>> m_Components;

	// 对象/组件内存池
	std::list<ObjectPool> m_GameObjectPools;
	std::list<ObjectPool>::iterator m_pCurrGameObjectPool;
	std::map<std::string, std::list<ObjectPool>> m_ComponentPools;
	std::map<std::string, std::list<ObjectPool>::iterator> m_pCurrComponentPools;

	Camera* m_pMainCamera = nullptr;

};
