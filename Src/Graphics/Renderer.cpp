#include <Graphics/Renderer.h>
#include <Graphics/RenderStates.h>
#include <Utils/GameInput.h>
#include <Utils/GameTimer.h>
#include <HLSL/XDefines.h>
#include "DXTrace.h"
#include "d3dUtil.h"

using namespace Microsoft::WRL;

float Renderer::RenderSetting::ambientIntensity = 0.2f;
XMath::Vector3 Renderer::RenderSetting::ambientLightColor = XMath::Vector3(1.0f, 1.0f, 1.0f);

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}


bool Renderer::Initialize(GraphicsCore* pCore)
{
	RendererBase::Initialize(pCore);
	RenderStates::InitAll(pCore->device.Get());
	GameInput::Initialize();
	m_ResourceManager.m_pDevice = pCore->device;
	m_ResourceManager.CreateEffectsFromJson("effects.json");
	return true;
}

void Renderer::Cleanup(GraphicsCore* pCore)
{
}

void Renderer::OnResize(GraphicsCore* pCore)
{
	assert(pCore->device);
	assert(pCore->deviceContext);
	assert(pCore->swapChain);

	m_pBackBuffer[0].Reset();
	m_pDepthStencilBuffer.Reset();

	// 重设交换链并且重新创建渲染目标视图
	ComPtr<ID3D11Texture2D> backBuffer;
	ThrowIfFailed(pCore->swapChain->ResizeBuffers(1, pCore->clientWidth, pCore->clientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ThrowIfFailed(pCore->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf())));
	ThrowIfFailed(pCore->device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_pBackBuffer[0].GetAddressOf()));

	// 设置调试对象名
	D3D11SetDebugObjectName(backBuffer.Get(), "BackBuffer[0]");

	backBuffer.Reset();

	uint32_t msaaQuality;
	pCore->device->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaaQuality);

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = pCore->clientWidth;
	depthStencilDesc.Height = pCore->clientHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 4;
	depthStencilDesc.SampleDesc.Quality = msaaQuality - 1;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	// 创建深度缓冲区以及深度模板视图
	ComPtr<ID3D11Texture2D> depthStencilBuffer;
	ThrowIfFailed(pCore->device->CreateTexture2D(&depthStencilDesc, nullptr, depthStencilBuffer.GetAddressOf()));
	ThrowIfFailed(pCore->device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, m_pDepthStencilBuffer.GetAddressOf()));

	// 将渲染目标视图和深度/模板缓冲区结合到管线
	pCore->deviceContext->OMSetRenderTargets(1, m_pBackBuffer[0].GetAddressOf(), m_pDepthStencilBuffer.Get());

	// 更新主摄像机
	Camera* pCamera = m_MainScene.GetMainCamera();
	pCamera->SetAspectRatio((float)pCore->clientWidth / pCore->clientHeight);

	// 设置调试对象名
	D3D11SetDebugObjectName(m_pDepthStencilBuffer.Get(), "DepthStencilView");
	D3D11SetDebugObjectName(m_pBackBuffer[0].Get(), "BackBufferRTV[0]");
}

void Renderer::Update(float deltaTime)
{
	//
	// 更新Effect帧相关常量缓冲区
	//
	std::vector<Component*> lights = m_MainScene.GetComponents(Light::GetType());
	std::vector<LightData> lightData;
	for (Component* ptr : lights)
	{
		if (!ptr->IsEnabled())
			continue;
		Light* pLight = static_cast<Light*>(ptr);
		Transform* pTransform = pLight->GetGameObject()->GetTransform();
		lightData.push_back(LightData{ pLight->GetColor() * pLight->GetIntensity(), static_cast<int>(pLight->GetLightType()),
			pTransform->GetPosition(), pLight->GetRange(), pLight->GetDirection(), pLight->GetSpotPower() });
	}


	Camera* pCamera = m_MainScene.GetMainCamera();
	Transform* pCameraTransform = pCamera->GetGameObject()->GetTransform();
	auto camPos = pCameraTransform->GetPosition();

	// Eigen是列矩阵，按列存储，需要转置
	XMath::Vector3 ambientColor = RenderSetting::ambientIntensity * RenderSetting::ambientLightColor;
	XMath::Matrix4x4 V = pCameraTransform->GetWorldToLocalMatrix();
	XMath::Matrix4x4 P = XMath::Matrix::PerspectiveFovLH(XMath::Scalar::ConvertToRadians(pCamera->GetFieldOfViewY()), pCamera->GetAspectRatio(), 
		pCamera->GetNearPlane(), pCamera->GetFarPlane());
	XMath::Matrix4x4 VP = P * V;

	V.transposeInPlace();
	P.transposeInPlace();
	VP.transposeInPlace();

	for (auto& p : m_ResourceManager.m_pEffects)
	{
		if (!p.second->GetFrameUpdateFunc())
		{
			auto cbLightAmbientColor = p.second->GetConstantBufferVariable(X_CB_LIGHT_AMBIENT_COLOR);
			auto cbLight = p.second->GetConstantBufferVariable(X_CB_LIGHT);
			auto cbEyePosW = p.second->GetConstantBufferVariable(X_CB_EYEPOS_W);
			auto cbLightCount = p.second->GetConstantBufferVariable(X_CB_LIGHT_COUNT);
			auto cbView = p.second->GetConstantBufferVariable(X_CB_VIEW);
			auto cbProj = p.second->GetConstantBufferVariable(X_CB_PROJ);
			auto cbViewProj = p.second->GetConstantBufferVariable(X_CB_VIEW_PROJ);
			if (cbLightAmbientColor) cbLightAmbientColor->SetFloatVector(3, ambientColor.data());
			if (cbLight) cbLight->SetRaw(lightData.data(), 0, (size_t)sizeof(LightData) * (uint32_t)lightData.size());
			if (cbEyePosW) cbEyePosW->SetFloatVector(3, camPos.data());
			if (cbLightCount) cbLightCount->SetSInt((std::min)((int)lightData.size(), X_LIGHT_MAX_COUNT));
			if (cbView) cbView->SetFloatMatrix(4, 4, V.transpose().data());
			if (cbProj) cbProj->SetFloatMatrix(4, 4, P.transpose().data());
			if (cbViewProj) cbViewProj->SetFloatMatrix(4, 4, VP.transpose().data());
		}
	}

}

void Renderer::BeginNewFrame(GraphicsCore* pCore)
{
	Camera* pCamera = m_MainScene.GetMainCamera();
	pCore->deviceContext->ClearRenderTargetView(m_pBackBuffer[0].Get(), pCamera->GetClearColor().data());
	switch (pCamera->GetClearFlag())
	{
	case Camera::ClearFlag::SolidColor:
		pCore->deviceContext->ClearDepthStencilView(m_pDepthStencilBuffer.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, pCamera->GetClearDepth(), 0);
		break;
	case Camera::ClearFlag::DepthOnly:
		pCore->deviceContext->ClearDepthStencilView(m_pDepthStencilBuffer.Get(), D3D11_CLEAR_DEPTH, pCamera->GetClearDepth(), 0);
		break;
	case Camera::ClearFlag::None:
		break;
	}
	auto viewPortRect = pCamera->GetViewPortRect();
	D3D11_VIEWPORT vp{ 
		viewPortRect.x() * pCore->clientWidth, 
		viewPortRect.y() * pCore->clientHeight,
		viewPortRect.z() * pCore->clientWidth,
		viewPortRect.w() * pCore->clientHeight,
		0.0f, 1.0f
	};
	pCore->deviceContext->RSSetViewports(1, &vp);
}

void Renderer::DrawScene(GraphicsCore* pCore)
{
	auto gameObjects = m_MainScene.GetRootGameObjects();
	for (auto pObject : gameObjects)
	{
		DrawGameObject(pCore, pObject);
	}
}

void Renderer::EndFrame(GraphicsCore* pCore)
{
	pCore->swapChain->Present(1, 0);
}

void Renderer::UpdateMeshResource(GraphicsCore* pCore, MeshData* pMeshData, MeshGraphicsResource* pMeshResource, const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputLayout)
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
		throw std::exception("You should provide vertex datas which inputLayout requires.");

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
			CreateVertexBuffer(pCore->device.Get(), datas[idx].first, (uint32_t)datas[idx].second,
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
			pCore->deviceContext->Map(pMeshResource->vertexBuffers[currPos], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
			memcpy_s(mappedData.pData, datas[idx].second, datas[idx].first, datas[idx].second);
			pCore->deviceContext->Unmap(pMeshResource->vertexBuffers[currPos++], 0);
			pMeshData->m_VertexCount = (uint32_t)pMeshData->vertices.size();
		}
	}

	if (pMeshData->m_IndexCapacity < pMeshData->indices.size())
	{
		CreateIndexBuffer(pCore->device.Get(), pMeshData->indices.data(), (uint32_t)pMeshData->indices.size(), &pMeshResource->indexBuffer);
		pMeshData->m_IndexCapacity = (uint32_t)pMeshData->indices.size();
		pMeshData->m_IndexSize = pMeshData->indexSize;
		pMeshData->m_IndexCount = pMeshData->m_IndexCapacity / pMeshData->m_IndexSize;
	}
	// TODO: 动态索引缓冲区

}

void Renderer::DrawGameObject(GraphicsCore* pCore, GameObject* pObject)
{
	Transform* pTransform = pObject->GetTransform();
	Material* pMat = pObject->FindComponent<Material>();
	MeshFilter* pMeshFilter = pObject->FindComponent<MeshFilter>();

	if (pMat && pMeshFilter)
	{
		MeshData* pMeshData = pMeshFilter->GetMesh();
		if (pMeshData == nullptr)
			pMeshData = pMeshFilter->GetSharedMesh();
		if (pMeshData)
		{
			auto& matAttributes = pMat->m_Attributes;

			// 按对象更新Effect数据
			auto pEffect = m_ResourceManager.FindEffect(pMat->m_EffectName);
			if (!pEffect)
			{
				std::string errMsg = "Error: Effect " + pMat->m_EffectName + " is not found!";
				throw std::exception(errMsg.c_str());
			}

			const auto& passName = pMat->GetPassName();
			auto pass = pEffect->GetEffectPass(passName.c_str());
			if (!pass)
				return;
			pEffect->GetGameObjectUpdateFunc()(pEffect, pObject);
			pass->Apply(pCore->deviceContext.Get());

			// 更新网格图形资源
			auto pMeshResource = m_ResourceManager.FindMeshGraphicsResources(pMeshData);
			if (!pMeshResource)
			{
				pMeshResource = m_ResourceManager.CreateMeshGraphicsResources(pMeshData);
			}
			if (pMeshData->m_IsUploading)
			{
				UpdateMeshResource(pCore, pMeshData, pMeshResource, pass->GetInputSignatures());
			}
			// TODO: 动态的顶点、索引

			pCore->deviceContext->IASetVertexBuffers(0, (uint32_t)pMeshResource->vertexBuffers.size(), pMeshResource->vertexBuffers.data(),
				pMeshResource->strides.data(), pMeshResource->offsets.data());
			pCore->deviceContext->IASetIndexBuffer(pMeshResource->indexBuffer, (pMeshData->m_IndexSize == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT), 0);
			pCore->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			pCore->deviceContext->IASetInputLayout(pass->GetInputLayout());
			pCore->deviceContext->DrawIndexed(pMeshData->m_IndexCount, 0, 0);
		}
	}

	for (auto pChild : pObject->m_pChildrens)
	{
		DrawGameObject(pCore, pChild);
	}
}
