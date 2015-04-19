

Texture2D NormalTex : register(t0);
Texture2D ColorTex : register(t1);
//Texture2D DepthTex : register(t2);
sampler NearestSampler : register(s0);

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};


float4 main(PS_INPUT In) : SV_TARGET
{
  return ColorTex.Sample(NearestSampler, In.uv);
}