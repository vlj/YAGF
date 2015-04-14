Texture2D<float4> Tex : register(t0);
sampler TexSampler : register(s0);

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float3 normal : TEXCOORD1;
};

struct PS_OUTPUT
{
  float4 normal_and_depth : SV_TARGET0;
  float4 base_color : SV_TARGET1;
};


float2 EncodeNormal(float3 n)
{
  return normalize(n.xy) * sqrt(n.z * 0.5 + 0.5);
}

PS_OUTPUT main(PS_INPUT In)
{
  PS_OUTPUT result;
  result.base_color = Tex.Sample(TexSampler, In.uv);
  result.normal_and_depth.xy = 0.5 * EncodeNormal(normalize(In.normal)) + 0.5;
  result.normal_and_depth.z = 1.;
  result.normal_and_depth.w = 0.;
  return result;
}