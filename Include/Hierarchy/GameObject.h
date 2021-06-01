
#pragma once

#include <Hierarchy/Scene.h>
#include <Component/Transform.h>

class GameObject
{
public:

	static GameObject* Create(Scene* pScene, std::string_view name = "", GameObject* pParent = nullptr);

	static GameObject* Instantiate(GameObject* pObject);
	static GameObject* Instantiate(GameObject* pObjcet, const XMath::Vector3& position);
	static GameObject* Instantiate(GameObject* pObjcet, const XMath::Vector3& position, const XMath::Quaternion& rotation);

	void Destroy();

	const std::string& GetName();
	bool SetName(std::string_view name);

	bool IsEnabled() const;
	void SetEnabled(bool enabled);

	GameObject* GetParent();
	GameObject* GetChild(size_t index);

	Scene* GetScene();

	//
	// 组件
	//

	Transform* GetTransform();
	const Transform* GetTransform() const;

	// 添加组件，若已经有组件则创建失败并返回nullptr
	template<class ComponentType>
	ComponentType* AddComponent();

	// 获取所有组件
	std::vector<Component*> GetComponents();

	// 寻找组件，若没有组件则返回nullptr
	template<class ComponentType>
	ComponentType* FindComponent();

	// 寻找组件，若没有组件则返回nullptr
	template<class ComponentType>
	const ComponentType* FindComponent() const;

	// 移除组件
	// 不能移除Transform
	template<class ComponentType>
	bool RemoveComponent();

	// 移除组件，仅物体本身组件有效
	// 不能移除Transform
	bool RemoveComponent(Component* pComponent);

private:
	static void* operator new(size_t size, Scene* pScene, std::string_view name);		// operator new
	static void* operator new(size_t size, void* pObject);								// placement new
	static void operator delete(void* pObject, Scene* pScene, std::string_view name);	// operator delete
	static void operator delete(void* pObject, void*);									// placement delete

	GameObject(Scene* pScene, std::string_view name, GameObject* pParent = nullptr);
	~GameObject();

private:
	friend class Scene;
	friend class RenderContext;
	friend class ResourceManager;
	
	std::string m_Name;

	Scene* m_pScene = nullptr;
	
	bool m_IsEnabled = true;

	GameObject* m_pParent = nullptr;
	std::list<GameObject*> m_pChildrens;

	std::map<std::string, Component*> m_Components;
	Transform* m_pTransform;
	
	uint32_t m_ObjCounter = 0;

};


template<class ComponentType>
inline ComponentType* GameObject::AddComponent()
{
	std::map<std::string, Component*>::iterator iter;
	bool emplaced = false;
	std::tie(iter, emplaced) = m_Components.try_emplace(ComponentType::GetType(), nullptr);
	if (emplaced)
	{
		iter->second = reinterpret_cast<Component*>(Component::operator new(sizeof(ComponentType), m_pScene, ComponentType::GetType()));
		new (iter->second) ComponentType(this);
		return static_cast<ComponentType*>(iter->second);
	}
	return nullptr;
}

template<class ComponentType>
inline ComponentType* GameObject::FindComponent()
{
	auto it = m_Components.find(ComponentType::GetType());
	if (it != m_Components.end())
		return static_cast<ComponentType*>(it->second);
	return nullptr;
}

template<class ComponentType>
inline const ComponentType* GameObject::FindComponent() const
{
	auto it = m_Components.find(ComponentType::GetType());
	if (it != m_Components.end())
		return static_cast<const ComponentType*>(it->second);
	return nullptr;
}

template<class ComponentType>
inline bool GameObject::RemoveComponent()
{
	auto it = m_Components.find(ComponentType::GetType());
	if (it != m_Components.end())
	{
		static_cast<ComponentType*>(it->second)->~ComponentType();
		Component::operator delete(it->second, m_pScene, ComponentType::GetType());
		m_Components.erase(it);
		return true;
	}
	return false;
}

template<>
inline bool GameObject::RemoveComponent<Transform>()
{
	return false;
}