#include "Basic.hlsli"

// 顶点着色器
VertexOutBasic VS(VertexPosNormalTex vIn)
{
    VertexOutBasic vOut;
    
    vector posW = mul(float4(vIn.PosL, 1.0f), g_World);
    
    vOut.PosW = posW.xyz;
    vOut.PosH = mul(float4(vIn.PosL, 1.0f), g_WorldViewProj);
    vOut.NormalW = mul(vIn.NormalL, (float3x3) g_WorldInvTranspose);
    vOut.Tex = vIn.Tex;
    vOut.ShadowPosH = mul(posW, g_ShadowTransform);
    return vOut;
}
