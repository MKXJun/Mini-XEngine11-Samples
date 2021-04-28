#include <Graphics/ResourceManager.h>
#include <string>

GameObject* GameObject::Create(Scene* pScene, std::string_view name, GameObject* pParent)
{
	// 防止重名
	if (pScene && pScene->FindGameObject(name))
		return nullptr;
	else if (!pScene && ResourceManager::Get().FindModel(name))
		return nullptr;

	if (name == "")
	{
		std::string base_name = "GameObject";
		std::string name;
		size_t counter;
		do {
			name = base_name;
			counter = pScene->GetDefaultGameObjectCounter();
			if (counter)
			{
				name += std::to_string(counter);
			}
		} while (pScene->FindGameObject(name));

		void* pObject = operator new(sizeof(GameObject), pScene, name);
		return new (pObject) GameObject(pScene, name, pParent);
	}
	else
	{
		void* pObject = operator new(sizeof(GameObject), pScene, name);
		return new (pObject) GameObject(pScene, name, pParent);
	}
}

GameObject* GameObject::Instantiate(GameObject* pObject)
{
	GameObject* pNewObject = nullptr;
	pNewObject = Create(pObject->m_pScene, pObject->m_Name);
	if (!pNewObject)
	{
		while ((pNewObject = Create(pObject->m_pScene, pObject->m_Name + std::to_string(pObject->m_ObjCounter++))) == nullptr)
			continue;
	}
	
	auto pComponents = pNewObject->GetComponents();
	for (auto pComponent : pComponents)
	{
		pComponent->Instantiate(pNewObject);
	}
	for (auto pChild : pObject->m_pChildrens)
	{
		GameObject* pNewChild = Instantiate(pChild);
		pNewChild->m_pParent = pNewObject;
		pNewObject->m_pChildrens.push_back(pNewChild);
	}
	return pNewObject;
}

GameObject* GameObject::Instantiate(GameObject* pObjcet, const XMath::Vector3& position)
{
	GameObject* pNewObject = Instantiate(pObjcet);
	pNewObject->GetTransform()->SetPosition(position);
	return pNewObject;
}

GameObject* GameObject::Instantiate(GameObject* pObjcet, const XMath::Vector3& position, const XMath::Quaternion& rotation)
{
	GameObject* pNewObject = Instantiate(pObjcet);
	pNewObject->GetTransform()->SetPosition(position);
	pNewObject->GetTransform()->SetRotation(rotation);
	return pNewObject;
}

void GameObject::Destroy()
{
	while (!m_pChildrens.empty())
		m_pChildrens.front()->Destroy();
	std::string name = m_Name;
	Scene* pScene = m_pScene;
	this->~GameObject();
	operator delete(this, m_pScene, name);
}

const std::string& GameObject::GetName()
{
	return m_Name;
}

bool GameObject::SetName(std::string_view name)
{
	if (m_pScene->NotifyGameObjectRenamed(m_Name, name))
	{
		m_Name = name;
		return true;
	}
	else
	{
		return false;
	}
}

bool GameObject::IsEnabled() const
{
	return m_IsEnabled;
}

void GameObject::SetEnabled(bool enabled)
{
	m_IsEnabled = enabled;
}

GameObject* GameObject::GetParent()
{
	return m_pParent;
}

GameObject* GameObject::GetChild(size_t index)
{
	auto it = m_pChildrens.begin();
	while (index-- && it != m_pChildrens.end())
	{
		++it;
	}
	if (it != m_pChildrens.end())
		return *it;
	return nullptr;
}

Scene* GameObject::GetScene()
{
	return m_pScene;
}

Transform* GameObject::GetTransform()
{
	return m_pTransform;
}

const Transform* GameObject::GetTransform() const
{
	return m_pTransform;
}

std::vector<Component*> GameObject::GetComponents()
{
	std::vector<Component*> res(m_Components.size());
	res[0] = m_pTransform;
	auto ed = m_Components.end();
	size_t i = 1;
	for (auto it = m_Components.begin(); it != ed; ++it)
	{
		if (it->second != m_pTransform)
		{
			res[i++] = it->second;
		}
	}
	return res;
}

bool GameObject::RemoveComponent(Component* pComponent)
{
	if (!pComponent)
		return true;
	auto it = m_Components.begin();
	auto ed = m_Components.end();
	while (it != ed)
	{
		if (it->second == pComponent && pComponent->GetName() != "Transform")
		{
			it->second->~Component();
			Component::operator delete(it->second, m_pScene, it->first);
			m_Components.erase(it);
			return true;
		}
		++it;
	}
	return false;
}

void* GameObject::operator new(size_t size, Scene* pScene, std::string_view name)
{
	if (pScene)
		return pScene->NotifyGameObjectCreated(name);
	else
		return ::operator new(sizeof(GameObject));
}

void* GameObject::operator new(size_t size, void* pObject)
{
	return pObject;
}

void GameObject::operator delete(void* pObject, Scene* pScene, std::string_view name)
{
	if (pScene)
		pScene->NotifyGameObjectDestroyed(name, static_cast<GameObject*>(pObject));
	else
		::operator delete(pObject);
}

void GameObject::operator delete(void* pObject, void*)
{
	return;
}

GameObject::GameObject(Scene* pScene, std::string_view name, GameObject* pParent)
	: m_Name(name), m_pScene(pScene)
{
	if (pParent)
	{
		m_pParent = pParent;
		m_pParent->m_pChildrens.push_back(this);
	}
	else if (pScene)
	{
		m_pScene->m_RootGameObjects[name.data()] = this;
	}
	m_pTransform = AddComponent<Transform>();
}

GameObject::~GameObject()
{
	if (m_pParent)
	{
		m_pParent->m_pChildrens.remove(this);
	}
	else if (m_pScene)
	{
		m_pScene->m_RootGameObjects.erase(m_Name);
	}
	for (auto& p : m_Components)
	{
		p.second->~Component();
		Component::operator delete(p.second, m_pScene, p.first);
	}
}
