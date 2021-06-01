#include "RenderContextImpl.h"
#include "CommandBufferImpl.h"
#include <Hierarchy/GameObject.h>
#include "GraphicsImpl.h"
#include <Component/Camera.h>


using namespace Microsoft::WRL;

//
// RenderContext
//

RenderContext::RenderContext()
	: pImpl(std::make_unique<RenderContext::Impl>())
{
}

RenderContext::~RenderContext()
{
}

void RenderContext::DrawSkybox(Camera& pCamera)
{
}

void RenderContext::SetupCameraProperties(Camera& camera)
{
	pImpl->m_CommandBuffer.SetViewMatrix(camera.GetGameObject()->GetTransform()->GetWorldToLocalMatrix());
	pImpl->m_CommandBuffer.SetProjMatrix(camera.GetProjMatrix());
	pImpl->m_CommandBuffer.SetViewport(camera.GetViewPortRect());
	pImpl->m_CommandBuffer.SetRenderTarget();
}

void RenderContext::ExecuteCommandBuffer(CommandBuffer& commandBuffer)
{
    commandBuffer.pImpl->m_pCommandLists.push_back(nullptr);
    commandBuffer.pImpl->m_pDeferredContext->FinishCommandList(true, commandBuffer.pImpl->m_pCommandLists.back().GetAddressOf());
    for (auto& pCmdList : commandBuffer.pImpl->m_pCommandLists)
        pImpl->m_CommandBuffer.pImpl->m_pDeferredContext->ExecuteCommandList(pCmdList.Get(), true);
}

void RenderContext::DrawGameObject(GameObject* pObject)
{
	if (!pObject)
		return;

	for (auto pChild : pObject->m_pChildrens)
	{
		DrawGameObject(pChild);
	}

	MeshRenderer* pMeshRenderer = pObject->FindComponent<MeshRenderer>();
	if (!pMeshRenderer)
		return;

	Material* pMat = pMeshRenderer->GetMaterial();
	if (!pMat)
		return;

	Transform* pTransform = pObject->GetTransform();

	MeshFilter* pMeshFilter = pObject->FindComponent<MeshFilter>();

	if (pMat && pMeshFilter)
	{
		MeshData* pMeshData = pMeshFilter->GetMesh();
		if (pMeshData == nullptr)
			pMeshData = pMeshFilter->GetSharedMesh();
		if (pMeshData)
		{
			pImpl->m_CommandBuffer.DrawMesh(pMeshData, pTransform->GetLocalToWorldMatrix(), pMat);
		}
	}

	
}

void RenderContext::Submit()
{
    Graphics::Impl::SubmitRenderContext();
}

