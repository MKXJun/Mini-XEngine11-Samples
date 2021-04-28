#include <Hierarchy/GameObject.h>

void* Component::operator new(size_t size, Scene* pScene, std::string_view name)
{
	if (pScene)
		return pScene->NotifyComponentCreated(name, size);
	else
		return ::operator new(size);
}

void* Component::operator new(size_t size, void* pObject)
{
	return pObject;
}

void Component::operator delete(void* pObject, Scene* pScene, std::string_view name)
{
	if (pScene)
		return pScene->NotifyComponentDestroyed(name, static_cast<Component*>(pObject));
	else
		return ::operator delete(pObject);
}

void Component::operator delete(void* pObject, void*)
{
	// 不进行释放
	return;
}

Component::Component(GameObject* pGameObject)
	: m_pGameObject(pGameObject), m_IsEnabled(true)
{
}

Component::~Component()
{
}

void Component::Destroy()
{
	Scene* pScene = m_pGameObject->GetScene();
	std::string name = GetName();
	this->~Component();
	operator delete(this, pScene, name);
}
