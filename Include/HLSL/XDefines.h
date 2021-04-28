#ifndef X_DEFINES
#define X_DEFINES


#ifdef __cplusplus

#pragma once

    #include <Math/XMath.h>
    
    #define var_or_text(name) #name

    struct LightData
    {
        XMath::Vector3 strength;
        int type;
        XMath::Vector3 position;
        float range;
        XMath::Vector3 direction;
        float spotPower;
    };

#else

    //
    // Declaration
    //
    #define var_or_text(name) name

    // Light 
    struct Light
    {
        float3 Strength; 
        int Type;

        float3 Position;
        float Range;

        float3 Direction;
        float SpotPower;
    };

    #define X_CB_LIGHT_DECLARE(COUNT) Light g_Light[COUNT];
    #define X_CB_LIGHT_COUNT_DECLARE uint g_LightCount;
    #define X_CB_LIGHT_AMBIENT_COLOR_DECLARE vector g_LightAmbientColor;
    #define X_LIGHT_DIRECTIONAL 0
    #define X_LIGHT_POINT 1
    #define X_LIGHT_SPOT 2

    // Camera
    #define X_CB_EYEPOS_W_DECLARE float3 g_EyePosW;
   
    // Matrices
    #define X_CB_WORLD_DECLARE matrix g_World;
    #define X_CB_VIEW_DECLARE matrix g_View;
    #define X_CB_PROJ_DECLARE matrix g_Proj;
    #define X_CB_VIEW_PROJ_DECLARE matrix g_ViewProj;
    #define X_CB_WORLD_VIEW_PROJ_DECLARE matrix g_WorldViewProj;
    #define X_CB_WORLD_INV_TRANSPOSE_DECLARE matrix g_WorldInvTranspose;
    #define X_CB_SHADOW_TRANSFORM matrix g_ShadowTransform;

    // Material
    #define X_CB_MAT_AMBIENT_DECLARE vector g_MatAmbient;
    #define X_CB_MAT_DIFFUSE_DECLARE vector g_MatDiffuse;
    #define X_CB_MAT_SPECULAR_DECLARE vector g_MatSpecular;
    #define X_CB_MAT_REFLECTIVE_DECLARE vector g_MatReflective;
    #define X_CB_MAT_SHININESS_DECLARE vector g_MatShininess;
    #define X_CB_MAT_PHONG_DECLARE_PACKED X_CB_MAT_AMBIENT_DECLARE X_CB_MAT_DIFFUSE_DECLARE X_CB_MAT_SPECULAR_DECLARE
    #define X_CB_MAT_BLINN_PHONG_DECLARE_PACKED X_CB_MAT_DIFFUSE_DECLARE X_CB_MAT_REFLECTIVE_DECLARE X_CB_MAT_SHININESS_DECLARE

    // Texture
    #define X_TEX_AMBIENT_DECLARE Texture2D g_AmbientMap;
    #define X_TEX_DIFFUSE_DECLARE Texture2D g_DiffuseMap;
    #define X_TEX_SPECULAR_DECLARE Texture2D g_DiffuseMap;
    #define X_TEX_BUMP_DECLARE Texture2D g_BumpMap;
    #define X_TEX_DISPLACEMENT_DECLARE Texture2D g_DisplacementMap;

    // Sampler
    #define X_SAMPLER_POINT_CLAMP_DECLARE SamplerState g_SamPointClamp;
    #define X_SAMPLER_LINEAR_WARP_DECLARE SamplerState g_SamLinearWrap;
    #define X_SAMPLER_ANISTROPIC_WRAP_DECLARE SamplerState g_SamAnistropicWrap;
    #define X_SAMPLER_SHADOW_DECLARE SamplerState g_SamShadow;
    
    

#endif

//
// Usage
//

// light
#define X_CB_LIGHT var_or_text(g_Light)
#define X_CB_LIGHT_COUNT var_or_text(g_LightCount)
#define X_LIGHT_MAX_COUNT 16
#define X_CB_LIGHT_AMBIENT_COLOR var_or_text(g_LightAmbientColor)

// camera
#define X_CB_EYEPOS_W var_or_text(g_EyePosW)

// matrices
#define X_CB_WORLD var_or_text(g_World)
#define X_CB_VIEW var_or_text(g_View)
#define X_CB_PROJ var_or_text(g_Proj)
#define X_CB_VIEW_PROJ var_or_text(g_ViewProj)
#define X_CB_WORLD_VIEW_PROJ var_or_text(g_WorldViewProj)
#define X_CB_WORLD_INV_TRANSPOSE var_or_text(g_WorldInvTranspose)
#define X_CB_SHADOW_TRANSFORM var_or_text(g_ShadowTransform)

// material
#define X_CB_MAT_AMBIENT var_or_text(g_MatAmbient)
#define X_CB_MAT_DIFFUSE var_or_text(g_MatDiffuse)
#define X_CB_MAT_SPECULAR var_or_text(g_MatSpecular)
#define X_CB_MAT_REFLECTIVE var_or_text(g_MatReflective)
#define X_CB_MAT_SHININESS var_or_text(g_MatShininess)

#define X_CB_MAT_PHONG X_CB_MAT_AMBIENT, X_CB_MAT_DIFFUSE, X_CB_MAT_SPECULAR

// texture
#define X_TEX_AMBIENT var_or_text(g_AmbientMap)
#define X_TEX_AMBIENT var_or_text(g_AmbientMap)
#define X_TEX_DIFFUSE var_or_text(g_DiffuseMap)
#define X_TEX_SPECULAR var_or_text(g_DiffuseMap)
#define X_TEX_BUMP var_or_text(g_BumpMap)
#define X_TEX_DISPLACEMENT var_or_text(g_DisplacementMap)

// sampler
#define X_SAMPLER_POINT_CLAMP var_or_text(g_SamPointClamp)
#define X_SAMPLER_LINEAR_WARP var_or_text(g_SamLinearWrap)
#define X_SAMPLER_ANISTROPIC_WRAP var_or_text(g_SamAnistropicWrap)
#define X_SAMPLER_SHADOW var_or_text(g_SamShadow)


#endif
