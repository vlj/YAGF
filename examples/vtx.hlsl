cbuffer Matrixes : register(b0)
{
  float4x4 ModelMatrix;
  float4x4 ViewProjectionMatrix;
}

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

PS_INPUT main(float4 pos : POSITION, float2 texc : TEXCOORD0)
{
  PS_INPUT result;
  float4 position = mul(ModelMatrix, float4(pos.x, pos.y, pos.z, 1.));
  result.pos = float4(position.x, position.y, .5 + position.z * .1, 1.);
  result.uv = texc;
  return result;
}