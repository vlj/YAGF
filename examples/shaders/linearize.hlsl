
cbuffer CONSTANT_BUF : register(b0)
{
  float4x4 ModelMatrix;
  float4x4 ViewProjectionMatrix;
  float4x4 ProjectionMatrix;
  float4 color;
  float zn;
  float zf;
};

Texture2D Tex : register(t0);
sampler TexSampler : register(s0);

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};


float4 main(PS_INPUT In) : SV_TARGET
{
  return Tex.Sample(TexSampler, In.uv);
}