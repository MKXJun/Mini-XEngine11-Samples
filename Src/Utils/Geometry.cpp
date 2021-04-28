#include <Utils/Geometry.h>

namespace Geometry
{

	//
	// 几何体方法的实现
	//

	void CreateSphere(MeshData* pMeshData, float radius, uint32_t levels, uint32_t slices)
	{
		using namespace XMath;

		if (!pMeshData)
			return;
		
		uint32_t vertexCount = 2 + (levels - 1) * (slices + 1);
		uint32_t indexCount = 6 * (levels - 1) * slices;
		pMeshData->vertices.resize(vertexCount);
		pMeshData->normals.resize(vertexCount);
		pMeshData->texcoords.resize(vertexCount);
		pMeshData->tangents.resize(vertexCount);
		pMeshData->indices.resize(indexCount * sizeof(uint32_t));
		pMeshData->indexSize = sizeof(uint32_t);
		uint32_t* indexData = reinterpret_cast<uint32_t*>(pMeshData->indices.data());

		uint32_t vIndex = 0, iIndex = 0;

		float phi = 0.0f, theta = 0.0f;
		float per_phi = PI / levels;
		float per_theta = 2 * PI / slices;
		float x, y, z;

		// 放入顶端点
		pMeshData->vertices[vIndex] = Vector3(0.0f, radius, 0.0f);
		pMeshData->normals[vIndex] = Vector3(0.0f, 1.0f, 0.0f);
		pMeshData->tangents[vIndex] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
		pMeshData->texcoords[vIndex++] = Vector2(0.0f, 0.0f);


		for (uint32_t i = 1; i < levels; ++i)
		{
			phi = per_phi * i;
			// 需要slices + 1个顶点是因为 起点和终点需为同一点，但纹理坐标值不一致
			for (uint32_t j = 0; j <= slices; ++j)
			{
				theta = per_theta * j;
				x = radius * sinf(phi) * cosf(theta);
				y = radius * cosf(phi);
				z = radius * sinf(phi) * sinf(theta);
				// 计算出局部坐标、法向量、Tangent向量和纹理坐标
				Vector3 pos = Vector3(x, y, z), normal;

				pMeshData->vertices[vIndex] = pos;
				pMeshData->normals[vIndex] = pos.normalized();
				pMeshData->tangents[vIndex] = Vector4(-sinf(theta), 0.0f, cosf(theta), 1.0f);
				pMeshData->texcoords[vIndex++] = Vector2(theta / 2 / PI, phi / PI);
			}
		}

		// 放入底端点
		pMeshData->vertices[vIndex] = Vector3(0.0f, -radius, 0.0f);
		pMeshData->normals[vIndex] = Vector3(0.0f, -1.0f, 0.0f);
		pMeshData->tangents[vIndex] = Vector4(-1.0f, 0.0f, 0.0f, 1.0f);
		pMeshData->texcoords[vIndex++] = Vector2(0.0f, 1.0f);


		// 放入索引
		if (levels > 1)
		{
			for (uint32_t j = 1; j <= slices; ++j)
			{
				indexData[iIndex++] = 0;
				indexData[iIndex++] = j % (slices + 1) + 1;
				indexData[iIndex++] = j;
			}
		}


		for (uint32_t i = 1; i < levels - 1; ++i)
		{
			for (uint32_t j = 1; j <= slices; ++j)
			{
				indexData[iIndex++] = (i - 1) * (slices + 1) + j;
				indexData[iIndex++] = (i - 1) * (slices + 1) + j % (slices + 1) + 1;
				indexData[iIndex++] = i * (slices + 1) + j % (slices + 1) + 1;

				indexData[iIndex++] = i * (slices + 1) + j % (slices + 1) + 1;
				indexData[iIndex++] = i * (slices + 1) + j;
				indexData[iIndex++] = (i - 1) * (slices + 1) + j;
			}
		}

		// 逐渐放入索引
		if (levels > 1)
		{
			for (uint32_t j = 1; j <= slices; ++j)
			{
				indexData[iIndex++] = (levels - 2) * (slices + 1) + j;
				indexData[iIndex++] = (levels - 2) * (slices + 1) + j % (slices + 1) + 1;
				indexData[iIndex++] = (levels - 1) * (slices + 1) + 1;
			}
		}

		pMeshData->indexSize = sizeof(uint32_t);
		pMeshData->UpdateBoundingData();
		pMeshData->UploadMeshData();
	}

	void CreateBox(MeshData* pMeshData, float width, float height, float depth)
	{
		using namespace XMath;

		if (!pMeshData)
			return;

		pMeshData->vertices.resize(24);
		pMeshData->normals.resize(24);
		pMeshData->tangents.resize(24);
		pMeshData->texcoords.resize(24);

		float w2 = width / 2, h2 = height / 2, d2 = depth / 2;

		// 右面(+X面)
		pMeshData->vertices[0] = Vector3(w2, -h2, -d2);
		pMeshData->vertices[1] = Vector3(w2, h2, -d2);
		pMeshData->vertices[2] = Vector3(w2, h2, d2);
		pMeshData->vertices[3] = Vector3(w2, -h2, d2);
		// 左面(-X面)
		pMeshData->vertices[4] = Vector3(-w2, -h2, d2);
		pMeshData->vertices[5] = Vector3(-w2, h2, d2);
		pMeshData->vertices[6] = Vector3(-w2, h2, -d2);
		pMeshData->vertices[7] = Vector3(-w2, -h2, -d2);
		// 顶面(+Y面)
		pMeshData->vertices[8] = Vector3(-w2, h2, -d2);
		pMeshData->vertices[9] = Vector3(-w2, h2, d2);
		pMeshData->vertices[10] = Vector3(w2, h2, d2);
		pMeshData->vertices[11] = Vector3(w2, h2, -d2);
		// 底面(-Y面)
		pMeshData->vertices[12] = Vector3(w2, -h2, -d2);
		pMeshData->vertices[13] = Vector3(w2, -h2, d2);
		pMeshData->vertices[14] = Vector3(-w2, -h2, d2);
		pMeshData->vertices[15] = Vector3(-w2, -h2, -d2);
		// 背面(+Z面)
		pMeshData->vertices[16] = Vector3(w2, -h2, d2);
		pMeshData->vertices[17] = Vector3(w2, h2, d2);
		pMeshData->vertices[18] = Vector3(-w2, h2, d2);
		pMeshData->vertices[19] = Vector3(-w2, -h2, d2);
		// 正面(-Z面)
		pMeshData->vertices[20] = Vector3(-w2, -h2, -d2);
		pMeshData->vertices[21] = Vector3(-w2, h2, -d2);
		pMeshData->vertices[22] = Vector3(w2, h2, -d2);
		pMeshData->vertices[23] = Vector3(w2, -h2, -d2);

		for (uint32_t i = 0; i < 4; ++i)
		{
			// 右面(+X面)
			pMeshData->normals[i] = Vector3(1.0f, 0.0f, 0.0f);
			pMeshData->tangents[i] = Vector4(0.0f, 0.0f, 1.0f, 1.0f);
			// 左面(-X面)
			pMeshData->normals[i + 4] = Vector3(-1.0f, 0.0f, 0.0f);
			pMeshData->tangents[i + 4] = Vector4(0.0f, 0.0f, -1.0f, 1.0f);
			// 顶面(+Y面)
			pMeshData->normals[i + 8] = Vector3(0.0f, 1.0f, 0.0f);
			pMeshData->tangents[i + 8] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
			// 底面(-Y面)
			pMeshData->normals[i + 12] = Vector3(0.0f, -1.0f, 0.0f);
			pMeshData->tangents[i + 12] = Vector4(-1.0f, 0.0f, 0.0f, 1.0f);
			// 背面(+Z面)
			pMeshData->normals[i + 16] = Vector3(0.0f, 0.0f, 1.0f);
			pMeshData->tangents[i + 16] = Vector4(-1.0f, 0.0f, 0.0f, 1.0f);
			// 正面(-Z面)
			pMeshData->normals[i + 20] = Vector3(0.0f, 0.0f, -1.0f);
			pMeshData->tangents[i + 20] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
		}

		for (uint32_t i = 0; i < 6; ++i)
		{
			pMeshData->texcoords[i * 4] = Vector2(0.0f, 1.0f);
			pMeshData->texcoords[i * 4 + 1] = Vector2(0.0f, 0.0f);
			pMeshData->texcoords[i * 4 + 2] = Vector2(1.0f, 0.0f);
			pMeshData->texcoords[i * 4 + 3] = Vector2(1.0f, 1.0f);
		}

		pMeshData->indexSize = sizeof(uint32_t);
		pMeshData->indices.resize(36 * sizeof(uint32_t));
		uint32_t* indexData = reinterpret_cast<uint32_t*>(pMeshData->indices.data());
		uint32_t indices[] = {
			0, 1, 2, 2, 3, 0,		// 右面(+X面)
			4, 5, 6, 6, 7, 4,		// 左面(-X面)
			8, 9, 10, 10, 11, 8,	// 顶面(+Y面)
			12, 13, 14, 14, 15, 12,	// 底面(-Y面)
			16, 17, 18, 18, 19, 16, // 背面(+Z面)
			20, 21, 22, 22, 23, 20	// 正面(-Z面)
		};
		memcpy_s(indexData, sizeof indices, indices, sizeof indices);

		pMeshData->UpdateBoundingData();
		pMeshData->UploadMeshData();
		
	}

	void CreateCylinder(MeshData* pMeshData, float radius, float height, uint32_t slices, uint32_t stacks, float texU, float texV)
	{
		using namespace XMath;

		if (!pMeshData)
			return;

		CreateCylinderNoCap(pMeshData, radius, height, slices, stacks, texU, texV);
		uint32_t vertexCount = (slices + 1) * (stacks + 3) + 2;
		uint32_t indexCount = 6 * slices * (stacks + 1);

		pMeshData->vertices.resize(vertexCount);
		pMeshData->normals.resize(vertexCount);
		pMeshData->tangents.resize(vertexCount);
		pMeshData->texcoords.resize(vertexCount);

		pMeshData->indices.resize(indexCount * sizeof(uint32_t));
		pMeshData->indexSize = sizeof(uint32_t);
		uint32_t* indexData = reinterpret_cast<uint32_t*>(pMeshData->indices.data());

		float h2 = height / 2;
		float theta = 0.0f;
		float per_theta = 2 * PI / slices;

		uint32_t vIndex = (slices + 1) * (stacks + 1), iIndex = 6 * slices * stacks;
		uint32_t offset = vIndex;

		// 放入顶端圆心
		pMeshData->vertices[vIndex] = Vector3(0.0f, h2, 0.0f);
		pMeshData->normals[vIndex] = Vector3(0.0f, 1.0f, 0.0f);
		pMeshData->tangents[vIndex] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
		pMeshData->texcoords[vIndex++] = Vector2(0.5f, 0.5f);

		// 放入顶端圆上各点
		for (uint32_t i = 0; i <= slices; ++i)
		{
			theta = i * per_theta;
			float u = cosf(theta) * radius / height + 0.5f;
			float v = sinf(theta) * radius / height + 0.5f;
			pMeshData->vertices[vIndex] = Vector3(radius * cosf(theta), h2, radius * sinf(theta));
			pMeshData->normals[vIndex] = Vector3(0.0f, 1.0f, 0.0f);
			pMeshData->tangents[vIndex] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
			pMeshData->texcoords[vIndex++] = Vector2(u, v);
		}

		// 放入底端圆心
		pMeshData->vertices[vIndex] = Vector3(0.0f, -h2, 0.0f);
		pMeshData->normals[vIndex] = Vector3(0.0f, -1.0f, 0.0f);
		pMeshData->tangents[vIndex] = Vector4(-1.0f, 0.0f, 0.0f, 1.0f);
		pMeshData->texcoords[vIndex++] = Vector2(0.5f, 0.5f);

		// 放入底部圆上各点
		for (uint32_t i = 0; i <= slices; ++i)
		{
			theta = i * per_theta;
			float u = cosf(theta) * radius / height + 0.5f;
			float v = sinf(theta) * radius / height + 0.5f;
			pMeshData->vertices[vIndex] = Vector3(radius * cosf(theta), -h2, radius * sinf(theta));
			pMeshData->normals[vIndex] = Vector3(0.0f, -1.0f, 0.0f);
			pMeshData->tangents[vIndex] = Vector4(-1.0f, 0.0f, 0.0f, 1.0f);
			pMeshData->texcoords[vIndex++] = Vector2(u, v);
		}


		// 放入顶部三角形索引
		for (uint32_t i = 1; i <= slices; ++i)
		{
			indexData[iIndex++] = offset;
			indexData[iIndex++] = offset + i % (slices + 1) + 1;
			indexData[iIndex++] = offset + i;
		}

		// 放入底部三角形索引
		offset += slices + 2;
		for (uint32_t i = 1; i <= slices; ++i)
		{
			indexData[iIndex++] = offset;
			indexData[iIndex++] = offset + i;
			indexData[iIndex++] = offset + i % (slices + 1) + 1;
		}

		pMeshData->UpdateBoundingData();
		pMeshData->UploadMeshData();
	}

	void CreateCylinderNoCap(MeshData* pMeshData, float radius, float height, uint32_t slices, uint32_t stacks, float texU, float texV)
	{
		using namespace XMath;

		if (!pMeshData)
			return;

		uint32_t vertexCount = (slices + 1) * (stacks + 1);
		uint32_t indexCount = 6 * slices * stacks;

		
		pMeshData->vertices.resize(vertexCount);
		pMeshData->normals.resize(vertexCount);
		pMeshData->texcoords.resize(vertexCount);
		pMeshData->tangents.resize(vertexCount);

		pMeshData->indices.resize(indexCount * sizeof(uint32_t));
		pMeshData->indexSize = sizeof(uint32_t);
		uint32_t* indexData = reinterpret_cast<uint32_t*>(pMeshData->indices.data());

		float h2 = height / 2;
		float theta = 0.0f;
		float per_theta = 2 * PI / slices;
		float stackHeight = height / stacks;

		// 自底向上铺设侧面端点
		uint32_t vIndex = 0;
		for (uint32_t i = 0; i < stacks + 1; ++i)
		{
			float y = -h2 + i * stackHeight;
			// 当前层顶点
			for (uint32_t j = 0; j <= slices; ++j)
			{
				theta = j * per_theta;
				float u = theta / 2 / PI;
				float v = 1.0f - (float)i / stacks;

				pMeshData->vertices[vIndex] = Vector3(radius * cosf(theta), y, radius * sinf(theta)), Vector3(cosf(theta), 0.0f, sinf(theta));
				pMeshData->normals[vIndex] = Vector3(cosf(theta), 0.0f, sinf(theta));
				pMeshData->tangents[i] = Vector4(-sinf(theta), 0.0f, cosf(theta), 1.0f);
				pMeshData->texcoords[vIndex++] = Vector2(u * texU, v * texV);
			}
		}

		// 放入索引
		uint32_t iIndex = 0;
		for (uint32_t i = 0; i < stacks; ++i)
		{
			for (uint32_t j = 0; j < slices; ++j)
			{
				indexData[iIndex++] = i * (slices + 1) + j;
				indexData[iIndex++] = (i + 1) * (slices + 1) + j;
				indexData[iIndex++] = (i + 1) * (slices + 1) + j + 1;

				indexData[iIndex++] = i * (slices + 1) + j;
				indexData[iIndex++] = (i + 1) * (slices + 1) + j + 1;
				indexData[iIndex++] = i * (slices + 1) + j + 1;
			}
		}
	}

	void CreateCone(MeshData* pMeshData, float radius, float height, uint32_t slices)
	{
		using namespace XMath;
		
		if (!pMeshData)
			return;
		
		CreateConeNoCap(pMeshData, radius, height, slices);

		uint32_t vertexCount = 3 * slices + 1;
		uint32_t indexCount = 6 * slices;
		pMeshData->vertices.resize(vertexCount);
		pMeshData->normals.resize(vertexCount);
		pMeshData->tangents.resize(vertexCount);
		pMeshData->texcoords.resize(vertexCount);

		pMeshData->indices.resize(indexCount * sizeof(uint32_t));
		pMeshData->indexSize = sizeof(uint32_t);
		uint32_t* indexData = reinterpret_cast<uint32_t*>(pMeshData->indices.data());

		float h2 = height / 2;
		float theta = 0.0f;
		float per_theta = 2 * PI / slices;
		uint32_t iIndex = 3 * slices;
		uint32_t vIndex = 2 * slices;

		// 放入圆锥底面顶点
		for (uint32_t i = 0; i < slices; ++i)
		{
			theta = i * per_theta;

			pMeshData->vertices[vIndex] = Vector3(radius * cosf(theta), -h2, radius * sinf(theta)),
			pMeshData->normals[vIndex] = Vector3(0.0f, -1.0f, 0.0f);
			pMeshData->tangents[vIndex] = Vector4(-1.0f, 0.0f, 0.0f, 1.0f);
			pMeshData->texcoords[vIndex++] = Vector2(cosf(theta) / 2 + 0.5f, sinf(theta) / 2 + 0.5f);
		}
		// 放入圆锥底面圆心
		pMeshData->vertices[vIndex] = Vector3(0.0f, -h2, 0.0f),
			pMeshData->normals[vIndex] = Vector3(0.0f, -1.0f, 0.0f);
		pMeshData->texcoords[vIndex++] = Vector2(0.5f, 0.5f);

		// 放入索引
		uint32_t offset = 2 * slices;
		for (uint32_t i = 0; i < slices; ++i)
		{
			indexData[iIndex++] = offset + slices;
			indexData[iIndex++] = offset + i % slices;
			indexData[iIndex++] = offset + (i + 1) % slices;
		}

		pMeshData->UpdateBoundingData();
		pMeshData->UploadMeshData();
	}

	void CreateConeNoCap(MeshData* pMeshData, float radius, float height, uint32_t slices)
	{
		using namespace XMath;

		if (!pMeshData)
			return;

		
		uint32_t vertexCount = 2 * slices;
		uint32_t indexCount = 3 * slices;
		pMeshData->vertices.resize(vertexCount);
		pMeshData->normals.resize(vertexCount);
		pMeshData->tangents.resize(vertexCount);
		pMeshData->texcoords.resize(vertexCount);

		pMeshData->indices.resize(indexCount * sizeof(uint32_t));
		pMeshData->indexSize = sizeof(uint32_t);
		uint32_t* indexData = reinterpret_cast<uint32_t*>(pMeshData->indices.data());

		float h2 = height / 2;
		float theta = 0.0f;
		float per_theta = 2 * PI / slices;
		float len = sqrtf(height * height + radius * radius);
		uint32_t iIndex = 0;
		uint32_t vIndex = 0;

		// 放入圆锥尖端顶点(每个顶点位置相同，但包含不同的法向量和切线向量)
		for (uint32_t i = 0; i < slices; ++i)
		{
			theta = i * per_theta + per_theta / 2;
			pMeshData->vertices[vIndex] = Vector3(0.0f, h2, 0.0f);
			pMeshData->normals[vIndex] = Vector3(radius * cosf(theta) / len, height / len, radius * sinf(theta) / len);
			pMeshData->tangents[vIndex] = Vector4(-sinf(theta), 0.0f, cosf(theta), 1.0f);
			pMeshData->texcoords[vIndex++] = Vector2(0.5f, 0.5f);
		}

		// 放入圆锥侧面底部顶点
		for (uint32_t i = 0; i < slices; ++i)
		{
			theta = i * per_theta;
			pMeshData->vertices[vIndex] = Vector3(radius * cosf(theta), -h2, radius * sinf(theta));
			pMeshData->normals[vIndex] = Vector3(radius * cosf(theta) / len, height / len, radius * sinf(theta) / len);
			pMeshData->tangents[vIndex] = Vector4(-sinf(theta), 0.0f, cosf(theta), 1.0f);
			pMeshData->texcoords[vIndex++] = Vector2(cosf(theta) / 2 + 0.5f, sinf(theta) / 2 + 0.5f);
		}

		// 放入索引
		for (uint32_t i = 0; i < slices; ++i)
		{
			indexData[iIndex++] = i;
			indexData[iIndex++] = slices + (i + 1) % slices;
			indexData[iIndex++] = slices + i % slices;
		}

		
	}

	void CreatePlane(MeshData* pMeshData, const XMath::Vector2& planeSize, const XMath::Vector2& maxTexCoord)
	{
		CreatePlane(pMeshData, planeSize.x(), planeSize.y(), maxTexCoord.x(), maxTexCoord.y());
	}

	void CreatePlane(MeshData* pMeshData, float width, float depth, float texU, float texV)
	{
		using namespace XMath;

		if (!pMeshData)
			return;

		
		pMeshData->vertices.resize(4);
		pMeshData->normals.resize(4);
		pMeshData->tangents.resize(4);
		pMeshData->texcoords.resize(4);


		uint32_t vIndex = 0;
		pMeshData->vertices[vIndex] = Vector3(-width / 2, 0.0f, -depth / 2);
		pMeshData->normals[vIndex] = Vector3(0.0f, 1.0f, 0.0f);
		pMeshData->tangents[vIndex] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
		pMeshData->texcoords[vIndex++] = Vector2(0.0f, texV);

		pMeshData->vertices[vIndex] = Vector3(-width / 2, 0.0f, depth / 2);
		pMeshData->normals[vIndex] = Vector3(0.0f, 1.0f, 0.0f);
		pMeshData->tangents[vIndex] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
		pMeshData->texcoords[vIndex++] = Vector2(0.0f, 0.0f);

		pMeshData->vertices[vIndex] = Vector3(width / 2, 0.0f, depth / 2);
		pMeshData->normals[vIndex] = Vector3(0.0f, 1.0f, 0.0f);
		pMeshData->tangents[vIndex] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
		pMeshData->texcoords[vIndex++] = Vector2(texU, 0.0f);

		pMeshData->vertices[vIndex] = Vector3(width / 2, 0.0f, -depth / 2);
		pMeshData->normals[vIndex] = Vector3(0.0f, 1.0f, 0.0f);
		pMeshData->tangents[vIndex] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
		pMeshData->texcoords[vIndex++] = Vector2(texU, texV);

		pMeshData->indices.resize(6 * sizeof(uint32_t));
		pMeshData->indexSize = sizeof(uint32_t);
		uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
		memcpy_s(pMeshData->indices.data(), sizeof indices, indices, sizeof indices);

		pMeshData->UpdateBoundingData();
		pMeshData->UploadMeshData();
		
	}
}