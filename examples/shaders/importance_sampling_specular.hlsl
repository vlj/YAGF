TextureCube tex : register(t0, space0);
cbuffer Matrix : register(b0, space1)
{
	float4x4 PermutationMatrix;
};
tbuffer samplesBuffer : register(t0, space2)
{
  float2 samples[1024];
};
RWTexture2D<float4> output: register(u0, space3);
sampler AnisotropicSampler : register(s0, space4);


[numthreads(1, 1, 1)]
void main(uint3 DispatchId : SV_DispatchThreadID)
{
	float2 uv = DispatchId.xy;
  float3 RayDir = 2. * float3(uv, 1.) - 1.;
  RayDir = normalize(mul(PermutationMatrix, float4(RayDir, 0.)).xyz);

  float4 FinalColor = float4(0., 0., 0., 0.);
  float3 up = (RayDir.y < .99) ? float3(0., 1., 0.) : float3(0., 0., 1.);
  float3 Tangent = normalize(cross(up, RayDir));
  float3 Bitangent = cross(RayDir, Tangent);
  float weight = 0.;

  for (int i = 0; i < 1024; i++)
  {
    float Theta = samples[i].x;
    float Phi = samples[i].y;

    float3 H = cos(Theta) * RayDir + sin(Theta) * cos(Phi) * Tangent + sin(Theta) * sin(Phi) * Bitangent;
    float3 L = 2 * dot(RayDir, H) * H - RayDir;

    float NdotL = clamp(dot(RayDir, L), 0., 1.);
    FinalColor += tex.SampleLevel(AnisotropicSampler, L, 0) * NdotL;
    weight += NdotL;
  }
  output[uv] = FinalColor / weight;
}