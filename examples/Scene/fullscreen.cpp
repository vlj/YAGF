// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <FullscreenPass.h>
#include <Maths/matrix4.h>

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

static union WrapperPipelineState *createSunlightShader()
{
  WrapperPipelineState *result = (WrapperPipelineState*)malloc(sizeof(WrapperPipelineState));
  result->D3DValue.pipelineStateObject = Sunlight::getInstance()->pso.Get();
  result->D3DValue.rootSignature = (*RS::getInstance())();
  return result;
}
#endif

Microsoft::WRL::ComPtr<ID3D12Resource> ScreenQuad;
D3D12_VERTEX_BUFFER_VIEW ScreenQuadView;
D3DRTTSet *fbo[2];
Microsoft::WRL::ComPtr<ID3D12Resource> DepthTextureCopyDest;
WrapperResource *depthtexturecopy;

struct ViewData
{
  float InverseViewMatrix[16];
  float InverseProjectionMatrix[16];
};

struct LightData
{
  float sun_direction[3];
  float sun_angle;
  float sun_col[3];
};

FullscreenPassManager::FullscreenPassManager(RenderTargets &rtts) : RTT(rtts)
{
  SunlightPSO = createSunlightShader();
  CommandList = GlobalGFXAPI->createCommandList();

  // Create Screenquad
    Microsoft::WRL::ComPtr<ID3D12Resource> ScreenQuadCPU;
    Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(3 * 4 * sizeof(float)),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&ScreenQuadCPU)
      );

    const float tri_vertex[] = {
      -1., -1., 0., 2.,
      -1., 3., 0., 0.,
      3., -1., 2., 2.
    };
    void *tmp;
    ScreenQuadCPU->Map(0, nullptr, &tmp);
    memcpy(tmp, tri_vertex, 3 * 4 * sizeof(float));
    ScreenQuadCPU->Unmap(0, nullptr);

    Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(3 * 4 * sizeof(float)),
      D3D12_RESOURCE_USAGE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&ScreenQuad)
      );

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdalloc;
    HRESULT hr = Context::getInstance()->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdlist;
    hr = Context::getInstance()->dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), Sunlight::getInstance()->pso.Get(), IID_PPV_ARGS(&cmdlist));

    cmdlist->CopyBufferRegion(ScreenQuad.Get(), 0, ScreenQuadCPU.Get(), 0, 3 * 4 * sizeof(float), D3D12_COPY_NONE);
    cmdlist->Close();
    Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());

    ScreenQuadView.BufferLocation = ScreenQuad->GetGPUVirtualAddress();
    ScreenQuadView.StrideInBytes = 4 * sizeof(float);
    ScreenQuadView.SizeInBytes = 3 * 4 * sizeof(float);

    // temp depth texture
    Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, 1024, 1024),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&DepthTextureCopyDest)
      );

    depthtexturecopy = (WrapperResource*)malloc(sizeof(WrapperResource));
    depthtexturecopy->D3DValue.resource = DepthTextureCopyDest.Get();
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_view = {};
    srv_view.Format = DXGI_FORMAT_R32_FLOAT;
    srv_view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_view.Texture2D.MipLevels = 1;
    depthtexturecopy->D3DValue.description.SRV = srv_view;

    viewdata = GlobalGFXAPI->createConstantsBuffer(sizeof(ViewData));
    lightdata = GlobalGFXAPI->createConstantsBuffer(sizeof(LightData));

    SunlightInputs = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap(
    {
      std::make_tuple(viewdata, RESOURCE_VIEW::CONSTANTS_BUFFER, 0),
      std::make_tuple(lightdata, RESOURCE_VIEW::CONSTANTS_BUFFER, 1),
      std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_NORMAL_AND_DEPTH), RESOURCE_VIEW::SHADER_RESOURCE, 0),
      std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_BASE_COLOR), RESOURCE_VIEW::SHADER_RESOURCE, 1),
      std::make_tuple(depthtexturecopy, RESOURCE_VIEW::SHADER_RESOURCE, 2),
    });
    Samplers = GlobalGFXAPI->createSamplerHeap({ 0 });

    // FBO
    fbo[0] = new D3DRTTSet({ Context::getInstance()->getBackBuffer(0) }, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, 1024, 1024, nullptr, nullptr);
    fbo[1] = new D3DRTTSet({ Context::getInstance()->getBackBuffer(1) }, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, 1024, 1024, nullptr, nullptr);
}

void FullscreenPassManager::renderSunlight()
{
  irr::core::matrix4 View, Proj, InvView, invProj;
  Proj.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
  Proj.getInverse(invProj);
  ViewData * vdata = (ViewData *)GlobalGFXAPI->mapConstantsBuffer(viewdata);
  memcpy(&vdata->InverseViewMatrix, InvView.pointer(), 16 * sizeof(float));
  memcpy(&vdata->InverseProjectionMatrix, invProj.pointer(), 16 * sizeof(float));

  LightData *data = (LightData*)GlobalGFXAPI->mapConstantsBuffer(lightdata);
  data->sun_col[0] = 1.;
  data->sun_col[1] = 1.;
  data->sun_col[2] = 1.;
  data->sun_direction[1] = 1.;
  GlobalGFXAPI->unmapConstantsBuffers(lightdata);
  GlobalGFXAPI->openCommandList(CommandList);

  // Copy depth (until crash is fixed ?)
  {
    D3D12_RESOURCE_BARRIER_DESC barriers[2] = {
      setResourceTransitionBarrier(RTT.getDepthBuffer()->D3DValue.resource, D3D12_RESOURCE_USAGE_DEPTH, D3D12_RESOURCE_USAGE_COPY_SOURCE),
      setResourceTransitionBarrier(depthtexturecopy->D3DValue.resource, D3D12_RESOURCE_USAGE_GENERIC_READ, D3D12_RESOURCE_USAGE_COPY_DEST)
    };
    CommandList->D3DValue.CommandList->ResourceBarrier(2, barriers);


    D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
    dst.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
    dst.Subresource = 0;
    dst.pResource = depthtexturecopy->D3DValue.resource;
    src.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
    src.Subresource = 0;
    src.pResource = RTT.getDepthBuffer()->D3DValue.resource;
    CommandList->D3DValue.CommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr, D3D12_COPY_NONE);

    D3D12_RESOURCE_BARRIER_DESC barriers_back[2] = {
      setResourceTransitionBarrier(RTT.getDepthBuffer()->D3DValue.resource, D3D12_RESOURCE_USAGE_COPY_SOURCE, D3D12_RESOURCE_USAGE_DEPTH),
      setResourceTransitionBarrier(depthtexturecopy->D3DValue.resource, D3D12_RESOURCE_USAGE_COPY_DEST, D3D12_RESOURCE_USAGE_GENERIC_READ)
    };
    CommandList->D3DValue.CommandList->ResourceBarrier(2, barriers_back);
  }

  GlobalGFXAPI->writeResourcesTransitionBarrier(CommandList,
  {
    std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_NORMAL_AND_DEPTH), RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::READ_GENERIC),
    std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_BASE_COLOR), RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::READ_GENERIC),
  });
  GlobalGFXAPI->setPipelineState(CommandList, SunlightPSO);
  fbo[Context::getInstance()->getCurrentBackBufferIndex()]->Bind(CommandList->D3DValue.CommandList);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 0, SunlightInputs);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 1, Samplers);
//  GlobalGFXAPI->setRTTSet(CommandList, RTT.getRTTSet(RenderTargets::FBO_COLORS));
  CommandList->D3DValue.CommandList->SetVertexBuffers(0, &ScreenQuadView, (UINT)1);
  GlobalGFXAPI->drawInstanced(CommandList, 3, 1, 0, 0);

  GlobalGFXAPI->writeResourcesTransitionBarrier(CommandList,
  {
    std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_NORMAL_AND_DEPTH), RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::RENDER_TARGET),
    std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_BASE_COLOR), RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::RENDER_TARGET),
  });
  GlobalGFXAPI->closeCommandList(CommandList);

  GlobalGFXAPI->submitToQueue(CommandList);
  Context::getInstance()->Swap();
  HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
  WaitForSingleObject(handle, INFINITE);
  CloseHandle(handle);
}
