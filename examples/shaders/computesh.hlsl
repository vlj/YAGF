TextureCube probe : register(t0, space1);
sampler Anisotropic : register(s0, space4);

cbuffer DATA : register(b0, space0)
{
	float edge_size;
};

struct SHCoeff
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

RWStructuredBuffer<SHCoeff> SH : register(u0, space2);

float3 getVectorFromCubeAndUV(uint face, float2 uv)
{
	float u = uv.x, v = uv.y;
	if (face == 0)
		return float3(1., -u, -v);
	if (face == 1)
		return float3(-1., -u, v);
	if (face == 2)
		return float3(v, 1., u);
	if (face == 3)
		return float3(v, -1., u);
	if (face == 4)
		return float3(v, -u, 1.);
	if (face == 5)
		return float3(-v, -u, -1.);
	return float3(0., 0., 0.);
}



// Compute solid angle
// From http://www.rorydriscoll.com/2012/01/15/cubemap-texel-solid-angle/
float AreaElement(float x, float y)
{
	return atan2(x * y, sqrt(x * x + y * y + 1));
}

float TexelCoordSolidAngle(float2 uv)
{
	float InvResolution = 1.0f / edge_size;

	// U and V are the -1..1 texture coordinate on the current face.
	// Get projected area for this texel
	float x0 = uv.x - InvResolution;
	float y0 = uv.y - InvResolution;
	float x1 = uv.x + InvResolution;
	float y1 = uv.y + InvResolution;
	float SolidAngle = AreaElement(x0, y0) - AreaElement(x0, y1) - AreaElement(x1, y0) + AreaElement(x1, y1);

	return SolidAngle;
}

[numthreads(1, 1, 1)]
void main()
{
	SH[0].bL00 = 0.;
	SH[0].bL1m1 = 0.;
	SH[0].bL10 = 0.;
	SH[0].bL11 = 0.;
	SH[0].bL2m2 = 0.;
	SH[0].bL2m1 = 0.;
	SH[0].bL20 = 0.;
	SH[0].bL21 = 0.;
	SH[0].bL22 = 0.;

	SH[0].gL00 = 0.;
	SH[0].gL1m1 = 0.;
	SH[0].gL10 = 0.;
	SH[0].gL11 = 0.;
	SH[0].gL2m2 = 0.;
	SH[0].gL2m1 = 0.;
	SH[0].gL20 = 0.;
	SH[0].gL21 = 0.;
	SH[0].gL22 = 0.;

	SH[0].rL00 = 0.;
	SH[0].rL1m1 = 0.;
	SH[0].rL10 = 0.;
	SH[0].rL11 = 0.;
	SH[0].rL2m2 = 0.;
	SH[0].rL2m1 = 0.;
	SH[0].rL20 = 0.;
	SH[0].rL21 = 0.;
	SH[0].rL22 = 0.;

	for (uint face = 0; face < 6; face++)
	{
		// Can be run on several thread with float atomics...
		for (uint i = 0; i < edge_size; i++)
		{
			for (uint j = 0; j < edge_size; j++)
			{
				float2 uv = 2. * float2(i, j) / (edge_size - 1.) - 1.;
				float3 vect = getVectorFromCubeAndUV(face, uv);
				float d = length(vect);
				float solidangle = TexelCoordSolidAngle(uv);
				float x = vect.x, y = vect.y, z = vect.z;

				// constant part of Ylm
				float c00 = 0.282095f;
				float c1minus1 = 0.488603f;
				float c10 = 0.488603f;
				float c11 = 0.488603f;
				float c2minus2 = 1.092548f;
				float c2minus1 = 1.092548f;
				float c21 = 1.092548f;
				float c20 = 0.315392f;
				float c22 = 0.546274f;

				float Y00 = c00 * solidangle;
				float Y1minus1 = c1minus1 * y * solidangle;
				float Y10 = c10 * z * solidangle;
				float Y11 = c11 * x * solidangle;
				float Y2minus2 = c2minus2 * x * y * solidangle;
				float Y2minus1 = c2minus1 * y * z * solidangle;
				float Y21 = c21 * x * z * solidangle;
				float Y20 = c20 * (3 * z * z - 1.) * solidangle;
				float Y22 = c22 * (x * x - y * y) * solidangle;

				float4 color = probe.SampleLevel(Anisotropic, vect, 0);
				float4 SH00 = color * Y00;
				SH[0].bL00 += SH00.b;
				SH[0].rL00 += SH00.r;
				SH[0].gL00 += SH00.g;

				float4 SH1minus1 = color * Y1minus1;
				SH[0].bL1m1 += SH1minus1.b;
				SH[0].rL1m1 += SH1minus1.r;
				SH[0].gL1m1 += SH1minus1.g;

				float4 SH10 = color * Y10;
				SH[0].bL10 += SH10.b;
				SH[0].rL10 += SH10.r;
				SH[0].gL10 += SH10.g;

				float4 SH11 = color * Y11;
				SH[0].bL11 += SH11.b;
				SH[0].rL11 += SH11.r;
				SH[0].gL11 += SH11.g;

				float4 SH2minus2 = color * Y2minus2;
				SH[0].bL2m2 += SH2minus2.b;
				SH[0].rL2m2 += SH2minus2.r;
				SH[0].gL2m2 += SH2minus2.g;

				float4 SH2minus1 = color * Y2minus1;
				SH[0].bL2m1 += SH2minus1.b;
				SH[0].rL2m1 += SH2minus1.r;
				SH[0].gL2m1 += SH2minus1.g;

				float4 SH20 = color * Y20;
				SH[0].bL20 += SH20.b;
				SH[0].rL20 += SH20.r;
				SH[0].gL20 += SH20.g;

				float4 SH21 = color * Y21;
				SH[0].bL21 += SH21.b;
				SH[0].rL21 += SH21.r;
				SH[0].gL21 += SH21.g;

				float4 SH22 = color * Y22;
				SH[0].bL22 += SH22.b;
				SH[0].rL22 += SH22.r;
				SH[0].gL22 += SH22.g;
			}
		}
	}
}
