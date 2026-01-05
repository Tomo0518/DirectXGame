#include "Object3d.hlsli"

struct TransfomationMartrix
{
    float4x4 WVP;
    float4x4 World;
};

struct DirectionalLight
{
    float4 color; //!< ライトの色
    float3 direction; //!< ライトの向き（単位ベクトル）
    float intensity; //!< ライトの強さ
    
};

ConstantBuffer<TransfomationMartrix> gTransformationMatrix : register(b0);


struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};


VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));
    return output;
}