#ifndef X_COMMON_INCLUDED
#define X_COMMON_INCLUDED


// [Warning]
// Global uniforms unsupport! Need to use cbuffer to wrap up
// 
// [Matrix Definition used in Mini XEngine]
// x_Matrix_LocalToWorld
// x_Matrix_WorldToLocal
// x_Matrix_View
// x_Matrix_Proj
// x_Matrix_ViewProj
//
// [Vector Definition used in Mini XEngine]
// x_BaseColor
//
// [Texture Definition used in Mini XEngine]
// x_MainTex
//

//
// Transformations
//
float3 TransformObjectToWorld(float3 posL)
{
    return mul(X_MATRIX_M, float4(posL, 1.0f)).xyz;
}

float3 TransformWorldToObject(float3 posL)
{
    return mul(X_MATRIX_INV_M, float4(posL, 1.0f)).xyz;
}

float4 TransformWorldToHClip(float3 posW)
{
    return mul(X_MATRIX_VP, float4(posW, 1.0f));
}



#endif
