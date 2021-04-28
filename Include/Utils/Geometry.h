
#pragma once

#include <vector>
#include <string>
#include <map>
#include <functional>
#include <Component/MeshFilter.h>

namespace Geometry
{

	// 创建球体网格数据，levels和slices越大，精度越高。
	void CreateSphere(MeshData* pMeshData, float radius = 1.0f, uint32_t levels = 20, uint32_t slices = 20);

	// 创建立方体网格数据
	void CreateBox(MeshData* pMeshData, float width = 2.0f, float height = 2.0f, float depth = 2.0f);

	// 创建圆柱体网格数据，slices越大，精度越高。
	void CreateCylinder(MeshData* pMeshData, float radius = 1.0f, float height = 2.0f, uint32_t slices = 20, uint32_t stacks = 10, float texU = 1.0f, float texV = 1.0f);

	// 创建只有圆柱体侧面的网格数据，slices越大，精度越高
	void CreateCylinderNoCap(MeshData* pMeshData, float radius = 1.0f, float height = 2.0f, uint32_t slices = 20, uint32_t stacks = 10, float texU = 1.0f, float texV = 1.0f);

	// 创建圆锥体网格数据，slices越大，精度越高。
	void CreateCone(MeshData* pMeshData, float radius = 1.0f, float height = 2.0f, uint32_t slices = 20);

	// 创建只有圆锥体侧面网格数据，slices越大，精度越高。
	void CreateConeNoCap(MeshData* pMeshData, float radius = 1.0f, float height = 2.0f, uint32_t slices = 20);

	
	// 创建一个平面
	void CreatePlane(MeshData* pMeshData, const XMath::Vector2& planeSize, const XMath::Vector2& maxTexCoord = { 1.0f, 1.0f });
	void CreatePlane(MeshData* pMeshData, float width = 10.0f, float depth = 10.0f, float texU = 1.0f, float texV = 1.0f);

}






