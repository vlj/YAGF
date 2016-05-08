cbuffer VIEWDATA : register(b0, space7)
{
	float4x4 ViewMatrix;
	float4x4 InverseViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InverseProjectionMatrix;
}


Texture2D ColorTex : register(t0, space4);
Texture2D NormalTex : register(t0, space5);
Texture2D Roughness_Metalness : register(t0, space6);
Texture2D DepthTex : register(t0, space14);

TextureCube Probe : register(t0, space11);
Texture2D DFGTex : register(t0, space12);

sampler Anisotropic : register(s0, space3);
sampler Bilinear : register(s0, space13);

cbuffer SHCoeff : register(b0, space10)
{
	float bL00;
	float bL1m1;
	float bL10;
	float bL11;
	float bL2m2;
	float bL2m1;
	float bL20;
	float bL21;
	float bL22;

	float gL00;
	float gL1m1;
	float gL10;
	float gL11;
	float gL2m2;
	float gL2m1;
	float gL20;
	float gL21;
	float gL22;

	float rL00;
	float rL1m1;
	float rL10;
	float rL11;
	float rL2m2;
	float rL2m1;
	float rL20;
	float rL21;
	float rL22;
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

// From "An Efficient Representation for Irradiance Environment Maps" article
// See http://graphics.stanford.edu/papers/envmap/
// Coefficients are calculated in IBL.cpp

float4x4 getMatrix(float L00, float L1m1, float L10, float L11, float L2m2, float L2m1, float L20, float L21, float L22)
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
	float4 extendednormal = mul(transpose(ViewMatrix), float4(normal, 0.));
	extendednormal.w = 1.;

	float4x4 rmat = getMatrix(rL00, rL1m1, rL10, rL11, rL2m2, rL2m1, rL20, rL21, rL22);
	float4x4 gmat = getMatrix(gL00, gL1m1, gL10, gL11, gL2m2, gL2m1, gL20, gL21, gL22);
	float4x4 bmat = getMatrix(bL00, bL1m1, bL10, bL11, bL2m2, bL2m1, bL20, bL21, bL22);

	float r = dot(extendednormal, mul(rmat, extendednormal));
	float g = dot(extendednormal, mul(gmat, extendednormal));
	float b = dot(extendednormal, mul(bmat, extendednormal));

	float NdotV = clamp(dot(V, normal), 0., 1.);

	return max(float3(r, g, b), float3(0., 0., 0.)) * DFGTex.Sample(Bilinear, float2(roughness, NdotV)).b * color / 3.14;
}


float3 SpecularIBL(float3 normal, float3 V, float roughness, float3 F0)
{
	float3 sampleDirection = reflect(-V, normal);
	sampleDirection = mul(InverseViewMatrix, float4(sampleDirection, 0.)).xyz;
	// Assume 8 level of lod (ie 256x256 texture)

	float lodval = 7. * roughness;
	float3 LD = max(Probe.SampleLevel(Anisotropic, sampleDirection, lodval).rgb, float3(0., 0., 0.));

	float NdotV = clamp(dot(V, normal), .01, 1.);
	float2 DFG = DFGTex.Sample(Bilinear, float2(roughness, NdotV)).rg;

	return LD *(F0 * DFG.x + DFG.y);
}

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT In) : SV_TARGET
{
	int3 uv;
	uv.xy = In.uv * 1024;
	uv.z = 0;
	float z = DepthTex.Load(uv).x;
	float3 projectedPos = float3(In.uv.x, 1. - In.uv.y, z);
	float4 xpos = getPosFromUVDepth(projectedPos, InverseProjectionMatrix);

	float3 normal = normalize(DecodeNormal(2. * NormalTex.Load(uv).xy - 1.));
	float3 color = ColorTex.Load(uv).xyz;

	float3 eyedir = -normalize(xpos.xyz);
	float roughness = Roughness_Metalness.Load(uv).x;

	float3 Dielectric = DiffuseIBL(normal, eyedir, roughness, color) + SpecularIBL(normal, eyedir, roughness, float3(.04, .04, .04));
	float3 Metal = SpecularIBL(normal, eyedir, roughness, color);
	float Metalness = Roughness_Metalness.Load(uv).y;

	return float4(lerp(Dielectric, Metal, Metalness), ColorTex.Load(uv).a);
}