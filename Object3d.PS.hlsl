#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    int enableLighting;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct DirectionalLight
{
    float4 color; //!< ライトの色
    float3 direction; //!< ライトの向き（単位ベクトル）
    float intensity; //!< ライトの強さ
    
};

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    output.color = gMaterial.color * textureColor;
    
    if (gMaterial.enableLighting != 0)
    { // ライティング有効時
        float cos = saturate(dot(normalize(input.normal), gDirectionalLight.direction));
        output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;
    }
    else
    { // ライティング無効時
        output.color = gMaterial.color * textureColor;
    }
    
    return output;
}