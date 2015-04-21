// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Material.h>

#ifdef GLBUILD
#include <GLAPI/Shaders.h>
#include <API/glapi.h>

const char *vtxshader = TO_STRING(
  \#version 330 \n

  layout(std140) uniform ObjectData
{
  mat4 ModelMatrix;
  mat4 InverseModelMatrix;
};

layout(std140) uniform ViewMatrices
{
  mat4 ViewProjectionMatrix;
};

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec2 Texcoord;
layout(location = 4) in vec2 SecondTexcoord;
layout(location = 5) in vec3 Tangent;
layout(location = 6) in vec3 Bitangent;

layout(location = 7) in int index0;
layout(location = 8) in float weight0;
layout(location = 9) in int index1;
layout(location = 10) in float weight1;
layout(location = 11) in int index2;
layout(location = 12) in float weight2;
layout(location = 13) in int index3;
layout(location = 14) in float weight3;

out vec3 nor;
out vec3 tangent;
out vec3 bitangent;
out vec2 uv;
out vec2 uv_bis;
out vec4 color;

void main()
{
  color = Color.zyxw;
  mat4 ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
  mat4 TransposeInverseModelView = transpose(InverseModelMatrix);// *InverseViewMatrix);
  gl_Position = ModelViewProjectionMatrix * vec4(Position.xyz, 1.);
  nor = (TransposeInverseModelView * vec4(Normal, 0.)).xyz;
  //  tangent = (TransposeInverseModelView * vec4(Tangent, 0.)).xyz;
  //  bitangent = (TransposeInverseModelView * vec4(Bitangent, 0.)).xyz;
  uv = vec4(Texcoord, 1., 1.).xy;
  //  uv_bis = SecondTexcoord;
}
);

const char *fragshader = TO_STRING(
  \#version 330 \n

  uniform sampler2D tex;
uniform sampler2D glosstex;

in vec3 nor;
in vec2 uv;
in vec4 color;

layout(location = 0) out vec4 EncodedNormal_Roughness_Metalness;
layout(location = 1) out vec4 Colors;
layout(location = 2) out float EmitMap;

// from Crytek "a bit more deferred CryEngine"
vec2 EncodeNormal(vec3 n)
{
  return normalize(n.xy) * sqrt(n.z * 0.5 + 0.5);
}

void main(void)
{
  Colors = vec4(texture(tex, uv).rgb * pow(color.rgb, vec3(2.2)), 1.);
  float glossmap = texture(glosstex, uv).r;
  float reflectance = texture(glosstex, uv).g;
  EncodedNormal_Roughness_Metalness.xy = 0.5 * EncodeNormal(normalize(nor)) + 0.5;
  EncodedNormal_Roughness_Metalness.z = 1. - glossmap;
  EncodedNormal_Roughness_Metalness.w = reflectance;
  EmitMap = texture(glosstex, uv).b;
}
);

class ObjectShader : public ShaderHelperSingleton<ObjectShader>, public TextureRead<UniformBufferResource<0>, UniformBufferResource<1>, TextureResource<GL_TEXTURE_2D, 0> >
{
public:
  ObjectShader()
  {
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vtxshader,
      GL_FRAGMENT_SHADER, fragshader);

    AssignSamplerNames(Program, "ViewMatrices", "ObjectData", "tex");
  }
};

static void ObjectStateSetter()
{
  glEnable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
}

struct WrapperPipelineState *createObjectShader()
{
  WrapperPipelineState *result = (WrapperPipelineState*)malloc(sizeof(WrapperPipelineState));
  result->GLValue.Program = ObjectShader::getInstance()->Program;
  result->GLValue.StateSetter = ObjectStateSetter;
  return result;
}

#endif