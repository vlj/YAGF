cbuffer VIEWDATA : register(b0)
{
  float4x4 InverseViewMatrix;
  float4x4 InverseProjectionMatrix;
}

Texture2D NormalTex : register(t0);
Texture2D ColorTex : register(t1);
Texture2D DepthTex : register(t2);
TextureCube Probe : register(t3);
Texture2D DFGTex : register(t4);

sampler Nearest : register(s0);
sampler Anisotropic : register(s1);
sampler Bilinear : register(s2);


float4 getPosFromUVDepth(float3 uvDepth, float4x4 InverseProjectionMatrix)
{
  float4 pos = float4(2. * uvDepth.xy - 1., uvDepth.z, 1.0);
  pos.xy *= float2(InverseProjectionMatrix[0][0], InverseProjectionMatrix[1][1]);
  pos.zw = float2(pos.z * InverseProjectionMatrix[2][2] + pos.w, pos.z * InverseProjectionMatrix[3][2] + pos.w);
  pos /= pos.w;
  return pos;
}

float3 DecodeNormal(float2 n)
{
  float z = dot(n, n) * 2. - 1.;
  float2 xy = normalize(n) * sqrt(1. - z * z);
  return float3(xy, z);
}

// From "An Efficient Representation for Irradiance Environment Maps" article
// See http://graphics.stanford.edu/papers/envmap/
// Coefficients are calculated in IBL.cpp

/*float4x4 getMatrix(float L00, float L1m1, float L10, float L11, float L2m2, float L2m1, float L20, float L21, float L22)
{
  float c1 = 0.429043, c2 = 0.511664, c3 = 0.743125, c4 = 0.886227, c5 = 0.247708;

  return float4x4(
    c1 * L22, c1 * L2m2, c1 * L21, c2 * L11,
    c1 * L2m2, -c1 * L22, c1 * L2m1, c2 * L1m1,
    c1 * L21, c1 * L2m1, c3 * L20, c2 * L10,
    c2 * L11, c2 * L1m1, c2 * L10, c4 * L00 - c5 * L20
    );
}

float3 DiffuseIBL(float3 normal, float3 V, float roughness, float3 color)
{
  // Convert normal in wobLd space (where SH coordinates were computed)
  float4 extendednormal = transpose(ViewMatrix) * float4(normal, 0.);
  extendednormal.w = 1.;

  mat4 rmat = getMatrix(rL00, rL1m1, rL10, rL11, rL2m2, rL2m1, rL20, rL21, rL22);
  mat4 gmat = getMatrix(gL00, gL1m1, gL10, gL11, gL2m2, gL2m1, gL20, gL21, gL22);
  mat4 bmat = getMatrix(bL00, bL1m1, bL10, bL11, bL2m2, bL2m1, bL20, bL21, bL22);

  float r = dot(extendednormal, rmat * extendednormal);
  float g = dot(extendednormal, gmat * extendednormal);
  float b = dot(extendednormal, bmat * extendednormal);

  float NdotV = clamp(dot(V, normal), 0., 1.);

  return max(float3(r, g, b), float3(0.)) * texture(dfg, float2(NdotV, roughness)).b * color / 3.14;
}*/



float3 SpecularIBL(float3 normal, float3 V, float roughness, float3 F0)
{
  float3 sampleDirection = reflect(-V, normal);
  sampleDirection = mul(InverseViewMatrix, float4(sampleDirection, 0.)).xyz;
  // Assume 8 level of lod (ie 256x256 texture)

  float lodval = 7. * roughness;
  float3 LD = max(Probe.SampleLevel(Anisotropic, sampleDirection, lodval).rgb, float3(0., 0., 0.));

  float NdotV = clamp(dot(V, normal), 0., 1.);
  float2 DFG = DFGTex.Sample(Bilinear, float2(NdotV, roughness)).rg;

  return LD * (F0 * DFG.x + DFG.y);
}

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT In) : SV_TARGET
{
  float3 normal = normalize(DecodeNormal(2. * NormalTex.Sample(Nearest, In.uv).xy - 1.));
  //  float3 color = ColorTex.Sample(Nearest, In.uv).rgb;

  float z = DepthTex.Sample(Nearest, In.uv).x;
  float4 xpos = getPosFromUVDepth(float3(In.uv, z), InverseProjectionMatrix);
  float3 eyedir = -normalize(xpos.xyz);
  float specval = NormalTex.Sample(Nearest, In.uv).z;

  /*  float3 Dielectric = DiffuseIBL(normal, eyedir, specval, color) + SpecularIBL(normal, eyedir, specval, float3(.04, .04, .04));
  float3 Metal = SpecularIBL(normal, eyedir, specval, color);
  float Metalness = NormalTex.Sample(Nearest, In.uv).a;

  return float4(.2 * mix(Dielectric, Metal, Metalness) + emitcolor, ColorTex.Sample(Nearest, In.uv).a);*/
  return float4(SpecularIBL(normal, eyedir, specval, float3(.04, .04, .04)), 1.);
}