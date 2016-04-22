cbuffer VIEWDATA : register(b0, space3)
{
  float4x4 ViewMatrix;
  float4x4 InverseViewMatrix;
  float4x4 InverseProjectionMatrix;
}

cbuffer LIGHTDATA : register(b0, space4)
{
  float3 sun_direction;
  float sun_angle;
  float3 sun_col;
}

Texture2D ColorTex : register(t0, space0);
Texture2D NormalTex : register(t0, space1);
Texture2D DepthTex : register(t0, space2);

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

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

// Sun Most Representative Point (used for MRP area lighting method)
// From "Frostbite going PBR" paper
float3 SunMRP(float3 normal, float3 eyedir)
{
  float4 local_sundir_notnorm = mul(transpose(InverseViewMatrix), float4(sun_direction, 0.));
  float3 local_sundir = normalize(local_sundir_notnorm.xyz);
  float3 R = reflect(-eyedir, normal);
  float angularRadius = 3.14 * sun_angle / 180.;
  float3 D = local_sundir;
  float d = cos(angularRadius);
  float r = sin(angularRadius);
  float DdotR = dot(D, R);
  float3 S = R - DdotR * D;
  return (DdotR < d) ? normalize(d * D + normalize(S) * r) : R;
}

// Burley model from Frostbite going pbr paper
float3 DiffuseBRDF(float3 normal, float3 eyedir, float3 lightdir, float3 color, float roughness)
{
  float biaised_roughness = 0.05 + 0.95 * roughness;
  // Half Light View direction
  float3 H = normalize(eyedir + lightdir);
  float LdotH = clamp(dot(lightdir, H), 0., 1.);
  float NdotL = clamp(dot(lightdir, normal), 0., 1.);
  float NdotV = clamp(dot(lightdir, eyedir), 0., 1.);

  float Fd90 = 0.5 + 2 * LdotH * LdotH * biaised_roughness * biaised_roughness;
  float SchlickFresnelL = (1. + (Fd90 - 1.) * (1. - pow(NdotL, 5.)));
  float SchlickFresnelV = (1. + (Fd90 - 1.) * (1. - pow(NdotV, 5.)));
  return color * SchlickFresnelL * SchlickFresnelV / 3.14;
}

// Fresnel Schlick approximation
float3 Fresnel(float3 viewdir, float3 halfdir, float3 basecolor)
{
  return clamp(basecolor + (1. - basecolor) * pow(1. - clamp(dot(viewdir, halfdir), 0., 1.), 5.), float3(0., 0., 0.), float3(1., 1., 1.));
}

// Schlick geometry term
float G1(float3 V, float3 normal, float k)
{
  float NdotV = clamp(dot(V, normal), 0., 1.);
  return 1. / (NdotV * (1. - k) + k);
}

// Smith model
// We factor the (n.v) (n.l) factor in the denominator
float ReducedGeometric(float3 lightdir, float3 viewdir, float3 normal, float roughness)
{
  float k = (roughness + 1.) * (roughness + 1.) / 8.;
  return G1(lightdir, normal, k) * G1(viewdir, normal, k);
}

// GGX
float Distribution(float roughness, float3 normal, float3 halfdir)
{
  float alpha = roughness * roughness * roughness * roughness;
  float NdotH = clamp(dot(normal, halfdir), 0., 1.);
  float normalisationFactor = 1. / 3.14;
  float denominator = NdotH * NdotH * (alpha - 1.) + 1.;
  return normalisationFactor * alpha / (denominator * denominator);
}

float3 SpecularBRDF(float3 normal, float3 eyedir, float3 lightdir, float3 color, float roughness)
{
  // Half Light View direction
  float3 H = normalize(eyedir + lightdir);
  float biaised_roughness = 0.05 + 0.95 * roughness;

  // Microfacet model
  return Fresnel(eyedir, H, color) * ReducedGeometric(lightdir, eyedir, normal, biaised_roughness) * Distribution(biaised_roughness, normal, H) / 4.;
}

float4 main(PS_INPUT In) : SV_TARGET
{
	int3 uv;
	uv.xy = In.uv * 1024;
	uv.z = 0;
  float z = DepthTex.Load(uv).x;
  float3 projectedPos= float3(In.uv.x, 1. - In.uv.y, z);
  float4 xpos = getPosFromUVDepth(projectedPos, InverseProjectionMatrix);

  float3 norm = normalize(DecodeNormal(2. * NormalTex.Load(uv).xy - 1.));
  float3 color = ColorTex.Load(uv).xyz;

  float roughness = .1;//NormalTex.Load(uv).z;
  float3 eyedir = -normalize(xpos.xyz);

  float3 Lightdir = SunMRP(norm, eyedir);
  float NdotL = clamp(dot(norm, Lightdir), 0., 1.);

  float metalness = 0.;//NormalTex.Load(uv).a;

  float3 Dielectric = DiffuseBRDF(norm, eyedir, Lightdir, color, roughness) + SpecularBRDF(norm, eyedir, Lightdir, float3(.04, .04, .04), roughness);
  float3 Metal = SpecularBRDF(norm, eyedir, Lightdir, color, roughness);
  return float4(NdotL * sun_col * lerp(Dielectric, Metal, metalness), 1.);
}