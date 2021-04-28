#pragma once

#include <queue>

class ObjectPool
{
public:
	void* Allocate();
	bool Free(void* ptr);

	size_t GetCapacity() const;
	size_t GetElemCount() const;

	ObjectPool(size_t maxObjects, size_t objectSize);
	ObjectPool(ObjectPool&& moveFrom) noexcept;
	~ObjectPool();
private:
	void* m_Pool = nullptr;		// 内存池
	std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> m_FreeIndices;	// 已释放索引
	size_t m_Capacity = 0;		// 对象最大数目
	size_t m_ElemCount = 0;		// 对象数目
	size_t m_ObjectSize = 0;	// 对象所占内存字节数
};

template<size_t alignment>
inline constexpr size_t AlignedSize(const size_t size)
{
	static_assert(!(alignment & alignment - 1), "alignment must be 2^n!");
	return (size + (alignment - 1)) & ~(alignment - 1);
}
