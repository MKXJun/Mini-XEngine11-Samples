#pragma once

// unused

#include <XCore.h>
#include <memory>

class Material;
class MaterialPropertyBlock;
class MeshData;
class GameObject;

class CommandBuffer
{
public:
    CommandBuffer();
    ~CommandBuffer();

    void Clear();
    void ClearRenderTarget(bool clearDepth, bool clearColor, Color backgroundColor, float depth);
    void DrawMesh(MeshData* pMeshData, const XMath::Matrix4x4& matrix, Material* pMaterial, MaterialPropertyBlock* pPropertyBlock = nullptr);
    void SetViewport(const Rect& rect);
    void SetViewMatrix(const XMath::Matrix4x4& matrix);
    void SetProjMatrix(const XMath::Matrix4x4& matrix);
    // need to be implemented after renderTexture implements.
    void SetRenderTarget();


private:
    friend class RenderContext;
    friend class Graphics;
    class Impl;
    std::unique_ptr<Impl> pImpl;
};