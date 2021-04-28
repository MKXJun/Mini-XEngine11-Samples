
#pragma once

#include <vector>
#include <memory>
#include <Component/Component.h>
#include <Math/XMath.h>

class MeshData;

class MeshFilter : public Component
{
public:
	const std::string& GetName() const override;
	static const std::string& GetType();

	MeshFilter(GameObject* pObject);
	Component* Instantiate(GameObject* pObject) override;

	MeshData* GetSharedMesh();
	MeshData* GetMesh();
	MeshData* CreateMesh();

private:
	friend class ResourceManager;
	friend class Scene;

	~MeshFilter() override;

	MeshData* m_pSharedMesh = nullptr;
	std::unique_ptr<MeshData> m_pMesh = nullptr;
};

class MeshData
{
public:

	std::vector<XMath::Vector3> vertices;
	std::vector<XMath::Vector3> normals;
	std::vector<XMath::Vector2> texcoords;
	std::vector<XMath::Vector4> tangents;
	std::vector<XMath::Vector4> colors;

	std::vector<uint8_t> indices;
	uint32_t indexSize = 0;
	uint32_t vertexMask = 0;

	XMath::Vector3 vMin = {};
	XMath::Vector3 vMax = {};

	void UpdateBoundingData();

	void MarkVertexDynamic(bool isDynamic = true);

	void UploadMeshData();
private:
	friend class Renderer;

	uint32_t m_VertexCount = 0;				// 已创建顶点缓冲区的顶点数目
	uint32_t m_VertexCapacity = 0;			// 已创建顶点缓冲区的最大容量
	uint32_t m_VertexMask = 0;				//  4|3| 2|1|0 
											//  c|t|uv|n|v|
	uint32_t m_IndexCount = 0;				// 已创建索引缓冲区的索引数目
	uint32_t m_IndexSize = 0;				// 已创建索引缓冲区的索引大小
	uint32_t m_IndexCapacity = 0;			// 已创建顶点缓冲区的最大字节数

	bool m_IsUploading = true;				// 默认缓冲区需要重建Buffer
											// 动态顶点缓冲区仅大小增加时重建Buffer
	bool m_IsDynamicVertex = false;			// 使用动态顶点缓冲区标记为true
};