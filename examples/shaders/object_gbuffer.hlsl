Texture2D<float4> Tex : register(t0, space2);
sampler TexSampler : register(s0, space3);

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float3 normal : TEXCOORD1;
};

struct PS_OUTPUT
{
  float4 base_color : SV_TARGET0;
  float2 normal : SV_TARGET1;
  float2 roughness_metalness : SV_TARGET2;
};


float2 EncodeNormal(float3 n)
{
  return normalize(n.xy) * sqrt(n.z * 0.5 + 0.5);
}

PS_OUTPUT main(PS_INPUT In)
{
  PS_OUTPUT result;
  result.base_color = Tex.Sample(TexSampler, float2(In.uv.x, 1. - In.uv.y));
  result.normal.xy = 0.5 * EncodeNormal(normalize(In.normal)) + 0.5;
  result.roughness_metalness.x = .3;
  result.roughness_metalness.y = 1.;
  return result;
}