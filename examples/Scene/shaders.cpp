// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Shaders.h>

#ifdef DXBUILD
#include <D3DAPI/RootSignature.h>
#include <D3DAPI/PSO.h>
#include <D3DAPI/D3DRTTSet.h>

typedef RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
  DescriptorTable<ConstantsBufferResource<0>, ConstantsBufferResource<1>, ShaderResource<0>, ShaderResource<1>, ShaderResource<2>>,
  DescriptorTable<SamplerResource<0>> > RS;

class Sunlight : public PipelineStateObject<Sunlight, VertexLayout<ScreenQuadVertex>>
{
public:
  Sunlight() : PipelineStateObject<Sunlight, VertexLayout<ScreenQuadVertex>>(L"Debug\\screenquad.cso", L"Debug\\sunlight.cso")
  {}

  static void SetRasterizerAndBlendStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psodesc)
  {
    psodesc.pRootSignature = (*RS::getInstance())();
    psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
    psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psodesc.NumRenderTargets = 1;
    psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psodesc.DepthStencilState.DepthEnable = false;
    psodesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
  }
};

union WrapperPipelineState *createSunlightShader()
{
  WrapperPipelineState *result = (WrapperPipelineState*)malloc(sizeof(WrapperPipelineState));
  result->D3DValue.pipelineStateObject = Sunlight::getInstance()->pso.Get();
  result->D3DValue.rootSignature = (*RS::getInstance())();
  return result;
}
#endif

#ifdef GLBUILD
#include <GL/glew.h>
#include <GLAPI/Shaders.h>
#include <GLAPI/Misc.h>
#include <fstream>
#include <GfxApi.h>

class Sunlight : public ShaderHelperSingleton<Sunlight>, TextureRead<UniformBufferResource<0>, UniformBufferResource<1>, TextureResource<GL_TEXTURE_2D, 0>, TextureResource<GL_TEXTURE_2D, 1>, TextureResource<GL_TEXTURE_2D, 2>>
{
public:
  Sunlight()
  {
    std::ifstream vsin("../examples/shaders/screenquad.vs", std::ios::in);

    const std::string &vs = std::string((std::istreambuf_iterator<char>(vsin)), std::istreambuf_iterator<char>());

    std::ifstream fsin("../examples/shaders/sunlight.fs", std::ios::in);

    const std::string &fs = std::string((std::istreambuf_iterator<char>(fsin)), std::istreambuf_iterator<char>());
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vs.c_str(),
      GL_FRAGMENT_SHADER, fs.c_str());

    AssignSamplerNames(Program, "VIEWDATA", "LIGHTDATA", "ntex", "ctex", "dtex");
  }
};

static void sunlightStateSetter()
{
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
}

union WrapperPipelineState *createSunlightShader()
{
  WrapperPipelineState *result = (WrapperPipelineState*)malloc(sizeof(WrapperPipelineState));
  result->GLValue.Program = Sunlight::getInstance()->Program;
  result->GLValue.StateSetter = sunlightStateSetter;
  return result;
}
#endif