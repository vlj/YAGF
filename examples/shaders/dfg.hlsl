cbuffer _Size : register(b0, space0)
{
	float size;
};
Buffer<float2> samplesBuffer : register(t0, space1);
RWTexture2D<float4> output: register(u0, space2);

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

float2 getSpecularDFG(float roughness, float NdotV)
{
	// We assume an implicit referential where N points in Oz
	float3 V = float3(sqrt(1.f - NdotV * NdotV), 0.f, NdotV);

	float DFG1 = 0., DFG2 = 0.;
	for (int i = 0; i < 1024; i++)
	{
		float2 ThetaPhi = ImportanceSamplingGGX(samplesBuffer[i].xy, roughness);
		float Theta = ThetaPhi.x;
		float Phi = ThetaPhi.y;
		float3 H = float3(sin(Theta) * cos(Phi), sin(Theta) * sin(Phi), cos(Theta));
		float3 L = 2 * dot(H, V) * H - V;

		float NdotL = clamp(L.z, 0.f, 1.f);
		float NdotH = clamp(H.z, 0.f, 1.f);
		float VdotH = clamp(dot(V, H), 0.f, 1.f);

		if (NdotL > 0.)
		{
			float Fc = pow(1.f - VdotH, 5.f);
			float G = G_Smith(NdotV, NdotL, roughness) * VdotH / (NdotH * NdotV);
			DFG1 += (1.f - Fc) * G;
			DFG2 += Fc * G;
		}
	}
	return float2(DFG1 / 1024., DFG2 / 1024.);
}

// Given a Seed from a uniform distribution, returns a vector direction
// as a theta, phi pair weighted against cos (n dot V) distribution.
float2 ImportanceSamplingCos(float2 Seeds)
{
	return float2(acos(Seeds.x), 2.f * 3.14f * Seeds.y);
}

float getDiffuseDFG(float roughness, float NdotV)
{
	// We assume an implicit referential where N points in Oz
	float3 V = float3(sqrt(1.f - NdotV * NdotV), 0.f, NdotV);
	float DFG = 0.f;
	for (uint i = 0; i < 1024; i++)
	{
		float2 ThetaPhi = ImportanceSamplingCos(samplesBuffer[i].xy);
		float Theta = ThetaPhi.x;
		float Phi = ThetaPhi.y;
		float3 L = float3(sin(Theta) * cos(Phi), sin(Theta) * sin(Phi), cos(Theta));
		float NdotL = L.z;
		if (NdotL > 0.f)
		{
			float3 H = normalize(L + V);
			float LdotH = dot(L, H);
			float f90 = .5f + 2.f * LdotH * LdotH * roughness * roughness;
			DFG += (1.f + (f90 - 1.f) * (1.f - pow(NdotL, 5.f))) * (1.f + (f90 - 1.f) * (1.f - pow(NdotV, 5.f)));
		}
	}
	return DFG / 1024.f;
}

[numthreads(8, 8, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float roughness = .1f + .9f * float(DTid.x) / float(size - 1);
	float NdotV = float(1 + DTid.y) / float(size);

	float2 specular_dfg = getSpecularDFG(roughness, NdotV);
	float diffuse_dfg = getDiffuseDFG(roughness, NdotV);
	output[DTid.xy] = float4(specular_dfg, diffuse_dfg, 1.);
}