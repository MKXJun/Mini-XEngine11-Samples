#include "XUtils.hlsli"

X_TEX_DIFFUSE_DECLARE;

X_SAMPLER_LINEAR_WARP_DECLARE;

cbuffer CBObject
{
    X_CB_WORLD_DECLARE
    X_CB_WORLD_VIEW_PROJ_DECLARE
    X_CB_WORLD_INV_TRANSPOSE_DECLARE
    X_CB_MAT_PHONG_DECLARE_PACKED
};

cbuffer CBPass
{
    X_CB_VIEW_DECLARE
    X_CB_PROJ_DECLARE
    X_CB_VIEW_PROJ_DECLARE
    X_CB_LIGHT_DECLARE(X_LIGHT_MAX_COUNT)
    X_CB_LIGHT_AMBIENT_COLOR_DECLARE
    X_CB_EYEPOS_W_DECLARE
    X_CB_LIGHT_COUNT_DECLARE
};

struct VertexPosNormal
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexPosNormalTex
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
};

struct VertexPosHWNormal
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};

struct VertexPosHWNormalTex
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD0;
};


VertexPosHWNormal Color_VS(VertexPosNormal vIn)
{
    VertexPosHWNormal vOut;
    
    vector posW = mul(float4(vIn.PosL, 1.0f), X_CB_WORLD);
    
    vOut.PosW = posW.xyz;
    vOut.PosH = mul(posW, X_CB_VIEW_PROJ);
    vOut.NormalW = mul(vIn.NormalL, (float3x3) X_CB_WORLD_INV_TRANSPOSE);
    
    return vOut;
}

VertexPosHWNormalTex Basic_VS(VertexPosNormalTex vIn)
{
    VertexPosHWNormalTex vOut;
    
    vector posW = mul(float4(vIn.PosL, 1.0f), X_CB_WORLD);
    
    vOut.PosW = posW.xyz;
    vOut.PosH = mul(posW, X_CB_VIEW_PROJ);
    vOut.NormalW = mul(vIn.NormalL, (float3x3) X_CB_WORLD_INV_TRANSPOSE);
    vOut.Tex = vIn.Tex;
    return vOut;
}

float4 Color_PS(VertexPosHWNormal pIn) : SV_Target
{
    // 标准化法向量
    pIn.NormalW = normalize(pIn.NormalW);
    
    // 求出顶点指向眼睛的向量，以及顶点与眼睛的距离
    float3 toEyeW = normalize(X_CB_EYEPOS_W - pIn.PosW);
    float distToEye = distance(X_CB_EYEPOS_W, pIn.PosW);
    
    // 初始化为0 
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 A = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 D = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 S = float4(0.0f, 0.0f, 0.0f, 0.0f);
    uint i = 0;
    for (i = 0; i < X_CB_LIGHT_COUNT; ++i)
    {
        switch (X_CB_LIGHT[i].Type)
        {
            case X_LIGHT_DIRECTIONAL:
                ComputePhongDirectionalLight(X_CB_LIGHT[i], X_CB_LIGHT_AMBIENT_COLOR.rgb, X_CB_MAT_PHONG, pIn.NormalW, toEyeW, A, D, S);
                break;
            case X_LIGHT_POINT:
                ComputePhongPointLight(X_CB_LIGHT[i], X_CB_LIGHT_AMBIENT_COLOR.rgb, X_CB_MAT_PHONG, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
                break;
            case X_LIGHT_SPOT:
                ComputePhongSpotLight(X_CB_LIGHT[i], X_CB_LIGHT_AMBIENT_COLOR.rgb, X_CB_MAT_PHONG, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
                break;
        }
        ambient += A;
        diffuse += D;
        spec += S;
    }
    
    
    float4 litColor = X_CB_MAT_DIFFUSE * (ambient + diffuse) + spec;
    litColor.a = X_CB_MAT_DIFFUSE.a;
    return litColor;
    
}


// 像素着色器(3D)
float4 Basic_PS(VertexPosHWNormalTex pIn) : SV_Target
{
    float4 texColor = X_TEX_DIFFUSE.Sample(X_SAMPLER_LINEAR_WARP, pIn.Tex);

    // 标准化法向量
    pIn.NormalW = normalize(pIn.NormalW);

    // 求出顶点指向眼睛的向量，以及顶点与眼睛的距离
    float3 toEyeW = normalize(X_CB_EYEPOS_W - pIn.PosW);
    float distToEye = distance(X_CB_EYEPOS_W, pIn.PosW);

    // 初始化为0 
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 A = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 D = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 S = float4(0.0f, 0.0f, 0.0f, 0.0f);
    uint i = 0;
    for (i = 0; i < X_CB_LIGHT_COUNT; ++i)
    {
        switch (X_CB_LIGHT[i].Type)
        {
            case X_LIGHT_DIRECTIONAL:
                ComputePhongDirectionalLight(X_CB_LIGHT[i], X_CB_LIGHT_AMBIENT_COLOR.rgb, X_CB_MAT_PHONG, pIn.NormalW, toEyeW, A, D, S);
                break;
            case X_LIGHT_POINT:
                ComputePhongPointLight(X_CB_LIGHT[i], X_CB_LIGHT_AMBIENT_COLOR.rgb, X_CB_MAT_PHONG, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
                break;
            case X_LIGHT_SPOT:
                ComputePhongSpotLight(X_CB_LIGHT[i], X_CB_LIGHT_AMBIENT_COLOR.rgb, X_CB_MAT_PHONG, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
                break;
        }
        ambient += A;
        diffuse += D;
        spec += S;
    }
  
    float4 litColor = texColor * (ambient + diffuse) + spec;
    litColor.a = texColor.a * X_CB_MAT_DIFFUSE.a;
    return litColor;
}
