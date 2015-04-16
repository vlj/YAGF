

#if DXBUILD
#include <D3DAPI/PSO.h>

class SunLight : public PipelineStateObject<SunLight, VertexLayout<ScreenQuadVertex>>
{
public:
  SunLight() : PipelineStateObject<SunLight, VertexLayout<ScreenQuadVertex>>(L"", L"")
  { }

  static void SetRasterizerAndBlendStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psodesc)
  {
  }
};

#endif

#ifdef GLBUILD

#endif

#ifdef DXBUILD
class FullScreen
{
public:
  static void applySunLightPass(ID3D12GraphicsCommandList *cmdlist)
  {

  }
};
#endif