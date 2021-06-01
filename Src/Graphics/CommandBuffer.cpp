#include "GraphicsImpl.h"
#include "CommandBufferImpl.h"
#include "ShaderImpl.h"
#include "DXTrace.h"
#include "d3dUtil.h"


bool CommandBuffer::Impl::UpdateMeshResource(MeshData* pMeshData, MeshGraphicsResource* pMeshResource, const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputLayout)
{
	// 顶点缓冲的更新

	// 默认缓冲区需要重建Buffer
	// 动态顶点缓冲区仅大小增加时重建Buffer
	static std::map<std::string_view, int> semanticMap = {
		{ "POSITION", 0 },
		{ "NORMAL", 1 },
		{ "TANGENT", 2 },
		{ "COLOR", 3 },
		{ "TEXCOORD", 4 }
	};
	std::vector<std::pair<void*, size_t>> datas = {
		{ pMeshData->vertices.data(), pMeshData->vertices.size() * sizeof(XMath::Vector3) },
		{ pMeshData->normals.data(), pMeshData->normals.size() * sizeof(XMath::Vector3) },
		{ pMeshData->tangents.data(), pMeshData->tangents.size() * sizeof(XMath::Vector4) },
		{ pMeshData->colors.data(), pMeshData->colors.size() * sizeof(XMath::Vector4) },
		{ pMeshData->texcoords.data(), pMeshData->texcoords.size() * sizeof(XMath::Vector2) }
	};

	uint32_t vertexMask = 0;
	size_t elemTypes = datas.size();
	for (size_t i = 0; i < elemTypes; ++i)
	{
		if (datas[i].second > 0)
			vertexMask |= (1 << i);
	}

	uint32_t layoutMask = 0;
	for (const auto& inputElem : inputLayout)
	{
		layoutMask |= (1 << semanticMap[inputElem.SemanticName]);
	}

	if ((layoutMask & vertexMask) != layoutMask)
		return false;

	if (layoutMask != pMeshData->m_VertexMask || pMeshData->m_VertexCapacity < pMeshData->vertices.size())
	{
		pMeshResource->vertexBuffers.clear();
		pMeshResource->strides.clear();
		pMeshResource->vertexBuffers.resize(inputLayout.size());
		pMeshResource->inputLayouts = inputLayout;
		pMeshResource->offsets.assign(inputLayout.size(), 0);

		pMeshData->m_VertexMask = layoutMask;
		pMeshData->m_VertexCapacity = pMeshData->m_VertexCount = (uint32_t)pMeshData->vertices.size();

		size_t currPos = 0;
		for (const auto& inputElem : inputLayout)
		{
			int idx = semanticMap[inputElem.SemanticName];
			CreateVertexBuffer(Graphics::Impl::GetDevice(), datas[idx].first, (uint32_t)datas[idx].second,
				&pMeshResource->vertexBuffers[currPos++], pMeshData->m_IsDynamicVertex);
			pMeshResource->strides.push_back((uint32_t)datas[idx].second / pMeshData->m_VertexCount);
		}
	}
	else if (pMeshData->m_IsDynamicVertex)
	{
		D3D11_MAPPED_SUBRESOURCE mappedData;
		size_t currPos = 0;
		for (const auto& inputElem : inputLayout)
		{
			int idx = semanticMap[inputElem.SemanticName];
			m_pDeferredContext->Map(pMeshResource->vertexBuffers[currPos], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
			memcpy_s(mappedData.pData, datas[idx].second, datas[idx].first, datas[idx].second);
			m_pDeferredContext->Unmap(pMeshResource->vertexBuffers[currPos++], 0);
			pMeshData->m_VertexCount = (uint32_t)pMeshData->vertices.size();
		}
	}

	if (pMeshData->m_IndexCapacity < pMeshData->indices.size())
	{
		CreateIndexBuffer(Graphics::Impl::GetDevice(), pMeshData->indices.data(), (uint32_t)pMeshData->indices.size(), &pMeshResource->indexBuffer);
		pMeshData->m_IndexCapacity = (uint32_t)pMeshData->indices.size();
		pMeshData->m_IndexSize = pMeshData->indexSize;
		pMeshData->m_IndexCount = pMeshData->m_IndexCapacity / pMeshData->m_IndexSize;
	}
	// TODO: 动态索引缓冲区

	return true;
}

CommandBuffer::CommandBuffer()
    : pImpl(std::make_unique<CommandBuffer::Impl>())
{
    ThrowIfFailed(Graphics::Impl::GetDevice()->CreateDeferredContext(0, pImpl->m_pDeferredContext.GetAddressOf()));
}

CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::Clear()
{
    pImpl->m_pCommandLists.clear();
}

void CommandBuffer::ClearRenderTarget(bool clearDepth, bool clearColor, Color backgroundColor, float depth)
{
    if (clearColor)
    {
        pImpl->m_pDeferredContext->ClearRenderTargetView(Graphics::Impl::GetColorBuffer(), backgroundColor);
    }
    if (clearDepth)
    {
        pImpl->m_pDeferredContext->ClearDepthStencilView(Graphics::Impl::GetDepthBuffer(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, 0);
    }
}

void CommandBuffer::DrawMesh(MeshData* pMeshData, const XMath::Matrix4x4& matrix, Material* pMaterial, MaterialPropertyBlock* pPropertyBlock)
{

    Shader* pShader = pMaterial->GetShader();
	if (!pShader)
		return;

    auto pMeshResource = ResourceManager::Get().FindMeshGraphicsResources(pMeshData);
    if (!pMeshResource)
    {
        pMeshResource = ResourceManager::Get().CreateMeshGraphicsResources(pMeshData);
    }
    if (pMeshData->m_IsUploading)
    {
		// TODO: 多Pass
        if (!pImpl->UpdateMeshResource(pMeshData, pMeshResource, pShader->pImpl->m_Passes[0].GetInputSignatures()))
			return;
    }

	//
	// 常量缓冲区更新
	//
	pShader->SetGlobalMatrix(Shader::StringToID("x_Matrix_LocalToWorld"), matrix);
	pShader->SetGlobalMatrix(Shader::StringToID("x_Matrix_WorldToLocal"), matrix.inverse());
	pShader->SetGlobalMatrix(Shader::StringToID("x_Matrix_View"), pImpl->m_View);
	pShader->SetGlobalMatrix(Shader::StringToID("x_Matrix_Proj"), pImpl->m_Proj);
	pShader->SetGlobalMatrix(Shader::StringToID("x_Matrix_ViewProj"), pImpl->m_Proj * pImpl->m_View);

	for (auto& attri : pMaterial->m_PropertyBlock.m_Properties)
	{
		switch (attri.second.index())
		{
		case 0: pShader->SetGlobalInt(attri.first, std::get<0>(attri.second)); break;
		case 1: pShader->SetGlobalFloat(attri.first, std::get<1>(attri.second)); break;
		case 2: pShader->SetGlobalVector(attri.first, std::get<2>(attri.second)); break;
		case 3: pShader->SetGlobalColor(attri.first, std::get<3>(attri.second)); break;
		case 4: pShader->SetGlobalMatrix(attri.first, std::get<4>(attri.second)); break;
		case 5: pShader->SetGlobalVectorArray(attri.first, std::get<5>(attri.second)); break;
		case 6: pShader->SetGlobalMatrixArray(attri.first, std::get<6>(attri.second)); break;
		}
	}

	if (pPropertyBlock)
	{
		for (auto& attri : pPropertyBlock->m_Properties)
		{
			switch (attri.second.index())
			{
			case 0: pShader->SetGlobalInt(attri.first, std::get<0>(attri.second)); break;
			case 1: pShader->SetGlobalFloat(attri.first, std::get<1>(attri.second)); break;
			case 2: pShader->SetGlobalVector(attri.first, std::get<2>(attri.second)); break;
			case 3: pShader->SetGlobalColor(attri.first, std::get<3>(attri.second)); break;
			case 4: pShader->SetGlobalMatrix(attri.first, std::get<4>(attri.second)); break;
			case 5: pShader->SetGlobalVectorArray(attri.first, std::get<5>(attri.second)); break;
			case 6: pShader->SetGlobalMatrixArray(attri.first, std::get<6>(attri.second)); break;
			}
		}
	}
	//
	// TODO: 纹理更新
	//
	/*std::string_view texPath = pMaterial->GetTexture(Shader::StringToID(X_TEX_DIFFUSE));
	if (!texPath.empty())
	{
		pShader->pImpl->m_Passes[0].SetShaderResource(X_TEX_DIFFUSE, ResourceManager::Get().FindTextureSRV(texPath));
	}*/

	// TODO: 多Pass
	pShader->pImpl->m_Passes[0].Apply(pImpl->m_pDeferredContext.Get());

	pImpl->m_pDeferredContext->IASetVertexBuffers(0, (uint32_t)pMeshResource->vertexBuffers.size(), pMeshResource->vertexBuffers.data(),
		pMeshResource->strides.data(), pMeshResource->offsets.data());
	pImpl->m_pDeferredContext->IASetIndexBuffer(pMeshResource->indexBuffer, (pMeshData->m_IndexSize == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT), 0);
	pImpl->m_pDeferredContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pImpl->m_pDeferredContext->IASetInputLayout(pShader->pImpl->m_Passes[0].pVSInfo->pDefaultInputLayout.Get());
	pImpl->m_pDeferredContext->DrawIndexed(pMeshData->m_IndexCount, 0, 0);
}

void CommandBuffer::SetViewport(const Rect& rect)
{
    D3D11_VIEWPORT vp{ rect.x(), rect.y(), rect.width() * Graphics::Impl::GetClientWidth(), rect.height() * Graphics::Impl::GetClientHeight(), 0.0f, 1.0f };
    pImpl->m_pDeferredContext->RSSetViewports(1, &vp);
}

void CommandBuffer::SetViewMatrix(const XMath::Matrix4x4& matrix)
{
    pImpl->m_View = matrix;
}

void CommandBuffer::SetProjMatrix(const XMath::Matrix4x4& matrix)
{
    pImpl->m_Proj = matrix;
}

void CommandBuffer::SetRenderTarget()
{
	ID3D11RenderTargetView* pRTVs[1] = { Graphics::Impl::GetColorBuffer() };
	pImpl->m_pDeferredContext->OMSetRenderTargets(1, pRTVs, Graphics::Impl::GetDepthBuffer());
}
