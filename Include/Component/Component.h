
#pragma once

#include <string_view>

class GameObject;
class Scene;

class Component
{
public:
	virtual const std::string& GetName() const = 0;
	bool IsEnabled() const { return m_IsEnabled; }
	void SetEnable(bool enabled) { m_IsEnabled = enabled; }
	GameObject* GetGameObject() const { return m_pGameObject; }
	
	Component(GameObject* pGameObject);
	virtual Component* Instantiate(GameObject* pObject) = 0;
	void Destroy();

	
protected:
	friend class GameObject;

	static void* operator new(size_t size, Scene* pScene, std::string_view name);		// operator new
	static void* operator new(size_t size, void* pObject);								// placement new
	static void operator delete(void* pObject, Scene* pScene, std::string_view name);	// operator delete
	static void operator delete(void* pObject, void*);									// placement delete

	virtual ~Component() = 0;

	GameObject* m_pGameObject;
	bool m_IsEnabled;
};