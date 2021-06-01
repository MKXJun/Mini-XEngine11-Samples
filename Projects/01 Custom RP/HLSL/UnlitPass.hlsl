#ifndef CUSTOM_UNLIT_PASS_INCLUDED
#define CUSTOM_UNLIT_PASS_INCLUDED

cbuffer PerDraw
{
    matrix x_Matrix_LocalToWorld;
    matrix x_Matrix_WorldToLocal;
    float4 x_BaseColor;
};

cbuffer PerFrame
{
    matrix x_Matrix_ViewProj;
    matrix x_Matrix_View;
};

#include "../ShaderLibrary/Common.hlsl"

struct Attributes
{
    float3 posL : POSITION;
};

struct Varyings
{
    float4 posH : SV_POSITION;
};

Varyings UnlitPassVertex(Attributes input)
{
    Varyings output;
    float3 posW = TransformObjectToWorld(input.posL);
    output.posH = TransformWorldToHClip(posW);
    
    return output;
}


float4 UnlitPassPixel(Varyings input) : SV_Target
{
    return x_BaseColor;
}

#endif
