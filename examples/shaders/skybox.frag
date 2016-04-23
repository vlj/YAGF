#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0, std140) uniform Matrixes
{
  mat4 ViewMatrix;
  mat4 InvView;
  mat4 InvProj;
};

layout(set = 0, binding = 1) uniform textureCube skytexture;
layout(set = 1, binding = 0) uniform sampler s;

layout(location = 0)in vec2 uv;
layout(location = 0) out vec4 FragColor;

void main(void)
{
    vec3 eyedir = vec3(uv, 1.);
    eyedir = 2.0 * eyedir - 1.0;
    vec4 tmp = (InvProj * vec4(eyedir, 1.));
    tmp /= tmp.w;
    eyedir = (InvView * vec4(tmp.xyz, 0.)).xyz;
    vec4 color = texture(samplerCube(skytexture, s), eyedir);
    FragColor = vec4(color.xyz, 1.);
}

