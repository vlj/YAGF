#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// From paper http://graphics.cs.williams.edu/papers/AlchemyHPG11/
// and improvements here http://graphics.cs.williams.edu/papers/SAOHPG12/

layout(set = 0, binding = 0, std140) uniform SSAOBuffer
{
	float zn;
	float zf;
};
layout(set = 0, binding = 1) uniform texture2D tex;
layout(set = 0, binding = 2, r32f) writeonly uniform image2D Depth;
layout(set = 1, binding = 4) uniform sampler nearest;

layout(location = 0) out vec4 FragColor;

void main()
{
	float d = texelFetch(sampler2D(tex, nearest), ivec2(gl_FragCoord.xy), 0).x;
	float c0 = zn * zf;
	float c1 = zn - zf;
	float c2 = zf;
	FragColor = vec4(c0 / (d * c1 + c2));
}
