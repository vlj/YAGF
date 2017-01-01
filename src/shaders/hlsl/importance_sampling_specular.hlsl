TextureCube tex : register(t0, space0);
cbuffer Matrix : register(b0, space1)
{
	float4x4 PermutationMatrix;
};
Buffer<float2> samplesBuffer : register(t0, space2);
RWTexture2D<float4> output: register(u0, space3);
sampler AnisotropicSampler : register(s0, space4);
cbuffer _Size : register(b0, space5)
{
	float size;
	float alpha;
};

// See Real Shading in Unreal Engine 4 (Brian Karis) for formula

float G1_Schlick(float XdotN, float k)
{
	return XdotN / (XdotN * (1.f - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
	float k = roughness * roughness / 2.f;
	return G1_Schlick(NdotV, k) * G1_Schlick(NdotL, k);
}

// Given a Seed from a uniform distribution, returns a half vector direction
// as a theta, phi pair weighted against GGX distribution.
float2 ImportanceSamplingGGX(float2 Seeds, float roughness)
{
	float a = roughness * roughness;
	float CosTheta = sqrt((1.f - Seeds.y) / (1.f + (a * a - 1.f) * Seeds.y));
	return float2(acos(CosTheta), 2.f * 3.14f * Seeds.x);
}

[numthreads(8, 8, 1)]
void main(uint3 DispatchId : SV_DispatchThreadID)
{
	float2 uv = DispatchId.xy / size;
	float3 RayDir = 2. * float3(uv, 1.) - 1.;
	RayDir = normalize(mul(PermutationMatrix, float4(RayDir, 0.)).xyz);

	float4 FinalColor = float4(0., 0., 0., 0.);
	float3 up = (RayDir.y < .99) ? float3(0., 1., 0.) : float3(0., 0., 1.);
	float3 Tangent = normalize(cross(up, RayDir));
	float3 Bitangent = cross(RayDir, Tangent);
	float weight = 0.;

	for (int i = 0; i < 1024; i++)
	{
		float2 ThetaPhi = ImportanceSamplingGGX(samplesBuffer[i].xy, alpha);
		float Theta = ThetaPhi.x;
		float Phi = ThetaPhi.y;

		float3 H = cos(Theta) * RayDir + sin(Theta) * cos(Phi) * Tangent + sin(Theta) * sin(Phi) * Bitangent;
		float3 L = 2 * dot(RayDir, H) * H - RayDir;

		float NdotL = clamp(dot(RayDir, L), 0., 1.);
		FinalColor += tex.SampleLevel(AnisotropicSampler, L, 0) * NdotL;
		weight += NdotL;
	}
	output[DispatchId.xy] = FinalColor / weight;
}