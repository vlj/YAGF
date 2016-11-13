

Texture2D tex : register(t0);
sampler NearestSampler : register(s0);

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT In) : SV_TARGET
{
  float4 col = tex.Sample(NearestSampler, In.uv);

//  float3 Yxy = getCIEYxy(col.rgb);
  col.rgb *= 1.5;//getRGBFromCIEXxy(vec3(3.14 * Yxy.x, Yxy.y, Yxy.z));

  float4 perChannel = (col * (6.9 * col + .6)) / (col * (5.2 * col + 2.5) + 0.06);
  return pow(perChannel, 2.2);
}