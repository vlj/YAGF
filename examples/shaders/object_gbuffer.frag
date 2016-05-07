#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 2) uniform texture2D tex;
layout(set = 3, binding = 3) uniform sampler s;
//uniform sampler2D glosstex;

layout(location = 0) in vec3 nor;
layout(location = 1) in vec2 uv;
in vec4 color;

layout(location = 0) out vec4 Colors;
layout(location = 1) out vec4 EncodedNormal;
layout(location = 2) out vec4 Roughness_Metalness;

// from Crytek "a bit more deferred CryEngine"
vec2 EncodeNormal(vec3 n)
{
  return normalize(n.xy) * sqrt(n.z * 0.5 + 0.5);
}

void main(void)
{
  Colors = texture(sampler2D(tex, s), vec2(uv.x, 1. - uv.y));
  EncodedNormal.xy = 0.5 * EncodeNormal(normalize(nor)) + 0.5;
  Roughness_Metalness.xy = vec2(0.3, 0.);

//  Colors = vec4(texel.rgb * pow(color.rgb, vec3(2.2)), 1.);
/*  float glossmap = texture(glosstex, uv).r;
  float reflectance = texture(glosstex, uv).g;
  EncodedNormal_Roughness_Metalness.xy = 0.5 * EncodeNormal(normalize(nor)) + 0.5;
  EncodedNormal_Roughness_Metalness.z = 1.;
  EncodedNormal_Roughness_Metalness.w = 0.;
  EmitMap = texture(glosstex, uv).b;*/
}