#include <Graphics/CommandBuffer.h>
#include <Graphics/ResourceManager.h>
#include <wrl/client.h>
#include <d3d11_1.h>


class CommandBuffer::Impl
{
public:
    Impl()
    {
    }
    ~Impl() = default;

    bool UpdateMeshResource(MeshData* pMeshData, MeshGraphicsResource* pMeshResource, const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputLayout);

    // Deferred Context
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pDeferredContext;
    std::vector<Microsoft::WRL::ComPtr<ID3D11CommandList>> m_pCommandLists;

    XMath::Matrix4x4 m_View;
    XMath::Matrix4x4 m_Proj;
};
