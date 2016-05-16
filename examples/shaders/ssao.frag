#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// From paper http://graphics.cs.williams.edu/papers/AlchemyHPG11/
// and improvements here http://graphics.cs.williams.edu/papers/SAOHPG12/

layout(set = 0, binding = 0, std140) uniform Matrixes
{
	float ProjectionMatrix00;
	float ProjectionMatrix11;
	float radius;
	float tau;
	float beta;
	float epsilon;
};
layout(set = 0, binding = 2) uniform texture2D dtex;
layout(set = 1, binding = 3) uniform sampler s;

vec3 getXcYcZc(float x, float y, float zC)
{
	// We use perspective symetric projection matrix hence P(0,2) = P(1, 2) = 0
	float xC= (2 * x - 1.) * zC / ProjectionMatrix00;
	float yC= (2 * y - 1.) * zC / ProjectionMatrix11;
	return vec3(xC, yC, zC);
}

layout(location = 0) out vec4 FragColor;

void main(void)
{
#define SAMPLES 16
	float invSamples = 1. / SAMPLES;
	vec2 screen = vec2(1024, 1024);

	vec2 uv = gl_FragCoord.xy / screen;
	float lineardepth = texture(sampler2D(dtex, s), uv).x;
	vec3 FragPos = getXcYcZc(uv.x, uv.y, lineardepth);

	// get the normal of current fragment
	vec3 ddx = dFdx(FragPos);
	vec3 ddy = dFdy(FragPos);
	vec3 norm = normalize(cross(ddy, ddx));

	int x = int(FragPos.x * 1024.), y = int(FragPos.y * 1024.);

	float r = radius / FragPos.z;
	float phi = 30. * (x ^ y) + 10. * x * y;
	float m = 0;//log2(r) + 6 + log2(invSamples);

	float occluded_factor = 0.0;
	float theta = 2. * 3.14 * tau * .5 * invSamples + phi;
	vec2 rotations = vec2(cos(theta), sin(theta)) * screen;
	vec2 offset = vec2(cos(invSamples), sin(invSamples));
	for(int i = 0; i < SAMPLES; ++i) {
		float alpha = (i + .5) * invSamples;
		float theta = 2. * 3.14 * alpha * tau * invSamples + phi;
		vec2 rotations = vec2(cos(theta), sin(theta));
		float h = r * alpha;
		vec2 localoffset = h * rotations;
		//m = m + .5;
		vec2 occluder_uv = uv + localoffset / 1024.;
		if (occluder_uv.x < 0 || occluder_uv.x > 1. || occluder_uv.y < 0 || occluder_uv.y > 1.) continue;
		float LinearoccluderFragmentDepth = texture(sampler2D(dtex, s), occluder_uv).x;//, max(m, 0.)).x;
		vec3 OccluderPos = getXcYcZc(occluder_uv.x, occluder_uv.y, LinearoccluderFragmentDepth);

		vec3 vi = OccluderPos - FragPos;
		float square_r_minus_square_v = max(r * r - dot(vi, vi), 0.f);
		occluded_factor += pow(square_r_minus_square_v, 3) * max(0.f, dot(vi, norm) - beta) / (dot(vi, vi) + epsilon);
	}
	FragColor = vec4(max(1.f - 5 * occluded_factor * invSamples / pow(r, 6), 0.));
}
