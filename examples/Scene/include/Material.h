// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include <GfxApi.h>

#ifdef GLBUILD
#include <GLAPI/Shaders.h>

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

layout(location = 1) out vec4 EncodedNormal_Roughness_Metalness;
layout(location = 0) out vec4 Colors;
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

    AssignSamplerNames(Program, "ObjectData", "ViewMatrices", "tex");
  }
};

static void ObjectStateSetter()
{
  glEnable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
}

inline WrapperPipelineState *createObjectShader()
{
  WrapperPipelineState *result = (WrapperPipelineState*) malloc(sizeof(WrapperPipelineState));
  result->GLValue.Program = ObjectShader::getInstance()->Program;
  result->GLValue.StateSetter = ObjectStateSetter;
  return result;
}

#endif

#ifdef DXBUILD
#include <D3DAPI/RootSignature.h>
#include <D3DAPI/PSO.h>

typedef RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
  DescriptorTable<ConstantsBufferResource<1>>,
  DescriptorTable<ConstantsBufferResource<0>, ShaderResource<0>>,
  DescriptorTable<SamplerResource<0>> > RS;

class Object : public PipelineStateObject<Object, VertexLayout<irr::video::S3DVertex2TCoords>>
{
public:
  Object() : PipelineStateObject<Object, VertexLayout<irr::video::S3DVertex2TCoords>>(L"Debug\\object_pass_vtx.cso", L"Debug\\object_pass_pix.cso")
  {}

  static void SetRasterizerAndBlendStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psodesc)
  {
    psodesc.pRootSignature = (*RS::getInstance())();
    psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
    psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psodesc.NumRenderTargets = 2;
    psodesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psodesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psodesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
  }
};

inline WrapperPipelineState *createObjectShader()
{
  WrapperPipelineState *result = (WrapperPipelineState*)malloc(sizeof(WrapperPipelineState));
  result->D3DValue.pipelineStateObject = Object::getInstance()->pso.Get();
  result->D3DValue.rootSignature = (*RS::getInstance())();
  return result;
}
#endif

#endif