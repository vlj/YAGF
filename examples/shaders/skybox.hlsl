
TextureCube skytexture : register(t0, space9);
sampler AnisotropicSampler : register(s0, space3);

cbuffer VIEWDATA : register(b0, space7)
{
	float4x4 ViewMatrix;
	float4x4 InverseViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InverseProjectionMatrix;
}

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT In) : SV_TARGET
{
  float3 eyedir = float3(In.uv, 1.);
  eyedir = 2.0 * eyedir - 1.0;
  float4 tmp = mul(InverseProjectionMatrix, float4(eyedir, 1.));
  tmp /= tmp.w;
  eyedir = mul(transpose(ViewMatrix), float4(tmp.xyz, 0.)).xyz;
  float4 color = skytexture.Sample(AnisotropicSampler, eyedir);
  return float4(color.xyz, 1.);
  return float4(color.xyz, 1.);
}

