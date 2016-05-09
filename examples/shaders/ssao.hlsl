
// From paper http://graphics.cs.williams.edu/papers/AlchemyHPG11/
// and improvements here http://graphics.cs.williams.edu/papers/SAOHPG12/

cbuffer ssao_param : register(b0, space0)
{
	float ProjectionMatrix00;
	float ProjectionMatrix11;
	float radius;
	float k;
	float sigma;
	float tau;
	float beta;
	float epsilon;
};

texture2D<float> linear_depth_texture : register(t0, space2);
sampler s : register(s0, space3);

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float3 getXcYcZc(float x, float y, float zC)
{
	// We use perspective symetric projection matrix hence P(0,2) = P(1, 2) = 0
	float xC = (2 * x - 1.) * zC / ProjectionMatrix00;
	float yC = (2 * y - 1.) * zC / ProjectionMatrix11;
	return float3(xC, yC, zC);
}

float4 main(PS_INPUT In) : SV_TARGET
{
#define SAMPLES 16
	const float invSamples = 1. / SAMPLES;
	const float2 screen = float2(1024, 1024);

	float2 uv = In.uv;
	float lineardepth = linear_depth_texture.SampleLevel(s, uv, 0.).x;
	float3 FragPos = getXcYcZc(uv.x, uv.y, lineardepth);

	// get the normal of current fragment
	float3 dx = ddx(FragPos);
	float3 dy = ddy(FragPos);
	float3 norm = normalize(cross(dy, dx));

	int x = int(uv.x * 1024.);
	int y = int(uv.y * 1024.);

	float r = radius / FragPos.z;
	float phi = 3. * (x ^ y) + x * y;
	float m = 0.;//log2(r) + 6 + log2(invSamples);
	float theta = 2. * 3.14 * tau * .5 * invSamples + phi;
	float2 rotations = float2(cos(theta), sin(theta)) * screen;
	float2 offset = float2(cos(invSamples), sin(invSamples));

	float bl = 0.0;
	for (int i = 0; i < SAMPLES; ++i) {
		float alpha = (i + .5) * invSamples;
		rotations = float2(rotations.x * offset.x - rotations.y * offset.y, rotations.x * offset.y + rotations.y * offset.x);
		float h = r * alpha;
		float2 localoffset = h * rotations;
		//m = m + .5;
		float2 occluder_uv = uv + localoffset / 1024.;
		if (occluder_uv.x < 0 || occluder_uv.x > 1. || occluder_uv.y < 0 || occluder_uv.y > 1.) continue;
		float LinearoccluderFragmentDepth = linear_depth_texture.SampleLevel(s, occluder_uv, max(m, 0.)).x;
		float3 OccluderPos = getXcYcZc(occluder_uv.x, occluder_uv.y, LinearoccluderFragmentDepth);
		float3 vi = OccluderPos - FragPos;
		bl += max(0.f, dot(vi, norm) - FragPos.z * beta) / (dot(vi, vi) + epsilon);
	}
	return max(pow(1.f - min(2. * sigma * bl * invSamples, 0.99), k), 0.);
}
