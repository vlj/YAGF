
TextureCube skytexture : register(t0);
sampler AnisotropicSampler : register(s0);

cbuffer Matrixes : register(b0)
{
  float4x4 InvView;
  float4x4 InvProj;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT In) : SV_TARGET
{
  In.uv.y = 1. - In.uv.y;
  float3 eyedir = float3(In.uv, 1.);
  eyedir = 2.0 * eyedir - 1.0;
  float4 tmp = mul(InvProj, float4(eyedir, 1.));
  tmp /= tmp.w;
  eyedir = mul(InvView, float4(tmp.xyz, 0.)).xyz;
  float4 color = skytexture.Sample(AnisotropicSampler, eyedir);
  return float4(color.xyz, 1.);
}

