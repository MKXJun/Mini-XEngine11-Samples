#include <Utils/ObjectPool.h>

void* ObjectPool::Allocate()
{
	if (!m_FreeIndices.empty())
	{
		size_t top = m_FreeIndices.top();
		m_FreeIndices.pop();
		++m_ElemCount;
		return reinterpret_cast<char*>(m_Pool) + top * m_ObjectSize;
	}
	else if (m_ElemCount < m_Capacity)
	{
		size_t pos = m_ElemCount++;
		return reinterpret_cast<char*>(m_Pool) + pos * m_ObjectSize;
	}
	else
	{
		return nullptr;
	}
}

bool ObjectPool::Free(void* ptr)
{
	size_t diff = (reinterpret_cast<char*>(ptr) - reinterpret_cast<char*>(m_Pool)) / m_ObjectSize;
	if (diff < 0 || static_cast<size_t>(diff) >= m_Capacity)
		return false;
	m_FreeIndices.push(diff);
	m_ElemCount--;
	return true;
}

size_t ObjectPool::GetCapacity() const
{
	return m_Capacity;
}

size_t ObjectPool::GetElemCount() const
{
	return m_ElemCount;
}

ObjectPool::ObjectPool(size_t maxObjects, size_t objectSize)
{
	try {
		m_Pool = ::operator new(maxObjects * objectSize);
		m_Capacity = maxObjects;
		m_ElemCount = 0;
		m_ObjectSize = objectSize;
	}
	catch (std::bad_alloc&)
	{
		m_Capacity = 0;
		m_ElemCount = 0;
		m_ObjectSize = 0;
		m_Pool = nullptr;
	}
}

ObjectPool::ObjectPool(ObjectPool&& moveFrom) noexcept
{
	std::swap(m_Pool, moveFrom.m_Pool);
	m_FreeIndices.swap(moveFrom.m_FreeIndices);
	m_Capacity = moveFrom.m_Capacity;
	m_ElemCount = moveFrom.m_ElemCount;
	m_ObjectSize = moveFrom.m_ObjectSize;
}

ObjectPool::~ObjectPool()
{
	::operator delete(m_Pool);
}

