cbuffer ObjectData : register(b0)
{
  float4x4 ModelMatrix;
  float4x4 InverseModelMatrix;
}

cbuffer ViewMatrix : register(b1)
{
  float4x4 ViewProjectionMatrix;
}

struct VS_INPUT
{
  float3 pos : POSITION;
  float2 texc : TEXCOORD0;
  float3 normal : NORMAL;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float3 normal : TEXCOORD1;
};

PS_INPUT main(VS_INPUT In)
{
  PS_INPUT result;

  float4 position = mul(ModelMatrix, float4(In.pos.x, In.pos.y, In.pos.z, 1.));
  result.pos = mul(ViewProjectionMatrix, position);
  result.uv = In.texc;
  result.normal = mul(transpose(InverseModelMatrix), float4(In.normal.x, In.normal.y, In.normal.z, 0.)).xyz;
  return result;
}