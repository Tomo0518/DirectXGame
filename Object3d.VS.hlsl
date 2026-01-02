#include "Object3d.hlsli"

struct TransfomationMartrix
{
    float4x4 WVP;
};
ConstantBuffer<TransfomationMartrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};


VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    return output;
}