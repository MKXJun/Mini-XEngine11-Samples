#ifndef X_UTILS
#define X_UTILS

#include "XDefines.h"


void ComputePhongDirectionalLight(Light light, float3 ambientColor, float4 matAmbient, float4 matDiffuse, float4 matSpecular,
    float3 normal, float3 toEye,
    out float4 outAmbient, out float4 outDiffuse, out float4 outSpecular)
{
    outAmbient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    outDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    outSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// 光向量与照射方向相反
    float3 lightVec = -light.Direction;

	// 添加环境光
    outAmbient = matAmbient * float4(ambientColor, 1.0f);

	// 添加漫反射光和镜面光
    float diffuseFactor = dot(lightVec, normal);

    if (diffuseFactor > 0.0f)
    {
        float3 v = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(v, toEye), 0.0f), matSpecular.w);

        outDiffuse = diffuseFactor * matDiffuse * float4(light.Strength, 1.0f);
        outSpecular = specFactor * matSpecular * float4(light.Strength, 1.0f);
    }
}

void ComputePhongPointLight(Light light, float3 ambientColor, float4 matAmbient, float4 matDiffuse, float4 matSpecular,
    float3 pos, float3 normal, float3 toEye,
	out float4 outAmbient, out float4 outDiffuse, out float4 outSpecular)
{
	// 初始化输出
    outAmbient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    outDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    outSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);

    
    float3 lightVec = light.Position - pos;
    float att = 1.0f / length(lightVec);
    lightVec = normalize(lightVec);

	// 环境光计算
    outAmbient = matAmbient * float4(ambientColor, 1.0f);

	// 漫反射和镜面计算
    float diffuseFactor = dot(lightVec, normal);

	// 展开以避免动态分支
	[flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 v = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(v, toEye), 0.0f), matSpecular.w);

        outDiffuse = diffuseFactor * matDiffuse * float4(light.Strength, 1.0f);
        outSpecular = specFactor * matSpecular * float4(light.Strength, 1.0f);
    }

    outDiffuse *= att;
    outSpecular *= att;
}

void ComputePhongSpotLight(Light light, float3 ambientColor, float4 matAmbient, float4 matDiffuse, float4 matSpecular,
    float3 pos, float3 normal, float3 toEye,
	out float4 outAmbient, out float4 outDiffuse, out float4 outSpecular)
{
	// 初始化输出
    outAmbient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    outDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    outSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 lightVec = light.Position - pos;
    float att = 1.0f / length(lightVec);
    lightVec = normalize(lightVec);

	// 计算环境光部分
    outAmbient = matAmbient * float4(ambientColor, 1.0f);


    // 计算漫反射光和镜面反射光部分
    float diffuseFactor = dot(lightVec, normal);

	// 展开以避免动态分支
	[flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 v = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(v, toEye), 0.0f), matSpecular.w);

        outDiffuse = diffuseFactor * matDiffuse * float4(light.Strength, 1.0f);
        outSpecular = specFactor * matSpecular * float4(light.Strength, 1.0f);
    }

	// 计算汇聚因子和衰弱系数
    float spot = pow(max(dot(-lightVec, light.Direction), 0.0f), light.SpotPower);

    outAmbient *= spot;
    outDiffuse *= spot * att;
    outSpecular *= spot * att;
}


#endif
