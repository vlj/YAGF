Texture2D<float4> Tex : register(t0);
sampler TexSampler : register(s0);

cbuffer CONSTANT_BUF : register(b0)
{
	float4 color;
}

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT In) : SV_TARGET
{
  return Tex.Sample(TexSampler, In.uv);
}