#include <D3DAPI/Misc.h>
#include <D3DAPI/PSO.h>
#include <Util/GeometryCreator.h>
#include <D3DAPI/VAO.h>
#include <Maths/matrix4.h>
#include <D3DAPI/RootSignature.h>
#include <D3DAPI/Resource.h>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

class Object : public PipelineStateObject<Object, VertexLayout<irr::video::S3DVertex2TCoords>>
{
public:
  Object() : PipelineStateObject<Object, VertexLayout<irr::video::S3DVertex2TCoords> >(L"Debug\\ssao_vtx.cso", L"Debug\\ssao_pix.cso")
  {}

  static void SetRasterizerAndBlendStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psodesc)
  {
    psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
    psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psodesc.NumRenderTargets = 1;
    psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psodesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
  }
};

typedef RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
  DescriptorTable<ConstantsBufferResource<0>>,
  DescriptorTable<ShaderResource<0> >,
  DescriptorTable<SamplerResource<0>> > RS;

class LinearizeDepthShader : public PipelineStateObject<LinearizeDepthShader, VertexLayout<ScreenQuadVertex>>
{
public:
  LinearizeDepthShader() : PipelineStateObject<LinearizeDepthShader, VertexLayout<ScreenQuadVertex> >(L"Debug\\screenquad.cso", L"Debug\\linearize.cso")
  {}

  static void SetRasterizerAndBlendStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psodesc)
  {
    psodesc.pRootSignature = RS::getInstance()->pRootSignature.Get();
    psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
    psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psodesc.NumRenderTargets = 1;
    psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psodesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psodesc.DepthStencilState.DepthEnable = false;
    psodesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
  }
};

struct SSAOBuffer
{
  float ModelMatrix[16];
  float ViewProjectionMatrix[16];
  float ProjectionMatrix[16];
  float color[4];
  float zn;
  float zf;
};

FormattedVertexStorage<irr::video::S3DVertex> *vao;

using namespace Microsoft::WRL;
ComPtr<ID3D12Resource> DepthTexture;
ComPtr<ID3D12Resource> DepthBuffer;
ComPtr<ID3D12Resource> cbufferdata[2];
ComPtr<ID3D12DescriptorHeap> descriptors;
ComPtr<ID3D12DescriptorHeap> depth_tex_descriptors;
ComPtr<ID3D12DescriptorHeap> depth_descriptors;
ComPtr<ID3D12Resource> ScreenQuad;
D3D12_VERTEX_BUFFER_VIEW ScreenQuadView;
ComPtr<ID3D12DescriptorHeap> SamplerHeap;

void Init(HWND hWnd)
{
  Context::getInstance()->InitD3D(hWnd);

  // Create Screenquad
  {
    ComPtr<ID3D12Resource> ScreenQuadCPU;
    Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(3 * 4 * sizeof(float)),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&ScreenQuadCPU)
      );

    const float tri_vertex[] = {
      -1., -1., 0., 0.,
      -1., 3., 0., 2.,
      3., -1., 2., 0.
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

    ComPtr<ID3D12CommandAllocator> cmdalloc;
    HRESULT hr = Context::getInstance()->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));
    ComPtr<ID3D12GraphicsCommandList> cmdlist;
    hr = Context::getInstance()->dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), Object::getInstance()->pso.Get(), IID_PPV_ARGS(&cmdlist));

    cmdlist->CopyBufferRegion(ScreenQuad.Get(), 0, ScreenQuadCPU.Get(), 0, 3 * 4 * sizeof(float), D3D12_COPY_NONE);
    cmdlist->Close();
    Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());

    ScreenQuadView.BufferLocation = ScreenQuad->GetGPUVirtualAddress();
    ScreenQuadView.StrideInBytes = 4 * sizeof(float);
    ScreenQuadView.SizeInBytes = 3 * 4 * sizeof(float);
  }

  irr::scene::SMeshBuffer *buffer = GeometryCreator::createCubeMeshBuffer(
    irr::core::vector3df(1., 1., 1.));
  std::vector<irr::scene::SMeshBuffer> buffers = { *buffer };

  vao = new FormattedVertexStorage<irr::video::S3DVertex>(Context::getInstance()->cmdqueue.Get(), buffers);

  // Create render targets
  Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, 1024, 1024, 1, 0, 1, 0, D3D12_RESOURCE_MISC_ALLOW_DEPTH_STENCIL),
    D3D12_RESOURCE_USAGE_DEPTH,
    &CD3D12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1., 0),
    IID_PPV_ARGS(&DepthBuffer)
    );

  depth_descriptors = createDescriptorHeap(Context::getInstance()->dev.Get(), 1, D3D12_DSV_DESCRIPTOR_HEAP, false);

  D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_desc = {};
  depth_stencil_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  depth_stencil_desc.Format = DXGI_FORMAT_D32_FLOAT;
  depth_stencil_desc.Texture2D.MipSlice = 0;
  Context::getInstance()->dev->CreateDepthStencilView(DepthBuffer.Get(), &depth_stencil_desc, depth_descriptors->GetCPUDescriptorHandleForHeapStart());

  Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Buffer(sizeof(struct SSAOBuffer)),
    D3D12_RESOURCE_USAGE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&cbufferdata[0])
    );

  Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Buffer(sizeof(struct SSAOBuffer)),
    D3D12_RESOURCE_USAGE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&cbufferdata[1])
    );

  descriptors = createDescriptorHeap(Context::getInstance()->dev.Get(), 2, D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP, true);

  D3D12_CONSTANT_BUFFER_VIEW_DESC cbvdesc = {};
  cbvdesc.BufferLocation = cbufferdata[0]->GetGPUVirtualAddress();
  cbvdesc.SizeInBytes = 256;
  Context::getInstance()->dev->CreateConstantBufferView(&cbvdesc, descriptors->GetCPUDescriptorHandleForHeapStart());
  cbvdesc.BufferLocation = cbufferdata[1]->GetGPUVirtualAddress();
  cbvdesc.SizeInBytes = 256;
  Context::getInstance()->dev->CreateConstantBufferView(&cbvdesc, descriptors->GetCPUDescriptorHandleForHeapStart().MakeOffsetted(Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP)));

  Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, 1024, 1024),
    D3D12_RESOURCE_USAGE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&DepthTexture)
    );

  depth_tex_descriptors = createDescriptorHeap(Context::getInstance()->dev.Get(), 1, D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP, true);

  D3D12_SHADER_RESOURCE_VIEW_DESC srv_view = {};
  srv_view.Format = DXGI_FORMAT_R32_FLOAT;
  srv_view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_view.Texture2D.MipLevels = 1;
  Context::getInstance()->dev->CreateShaderResourceView(DepthTexture.Get(), &srv_view, depth_tex_descriptors->GetCPUDescriptorHandleForHeapStart());

  SamplerHeap = createDescriptorHeap(Context::getInstance()->dev.Get(), 1, D3D12_SAMPLER_DESCRIPTOR_HEAP, true);

  D3D12_SAMPLER_DESC samplerdesc = {};
  samplerdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  samplerdesc.AddressU = D3D12_TEXTURE_ADDRESS_WRAP;
  samplerdesc.AddressV = D3D12_TEXTURE_ADDRESS_WRAP;
  samplerdesc.AddressW = D3D12_TEXTURE_ADDRESS_WRAP;
  samplerdesc.MaxAnisotropy = 1;
  samplerdesc.MinLOD = 0;
  samplerdesc.MaxLOD = 0;
  Context::getInstance()->dev->CreateSampler(&samplerdesc, SamplerHeap->GetCPUDescriptorHandleForHeapStart());

  delete buffer;
}

void Draw()
{
  D3D12_RECT rect = {};
  rect.left = 0;
  rect.top = 0;
  rect.bottom = 1024;
  rect.right = 1024;

  D3D12_VIEWPORT view = {};
  view.Height = 1024;
  view.Width = 1024;
  view.TopLeftX = 0;
  view.TopLeftY = 0;
  view.MinDepth = 0;
  view.MaxDepth = 1.;

  ComPtr<ID3D12CommandAllocator> cmdalloc;
  HRESULT hr = Context::getInstance()->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));
  ComPtr<ID3D12GraphicsCommandList> cmdlist;
  hr = Context::getInstance()->dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), Object::getInstance()->pso.Get(), IID_PPV_ARGS(&cmdlist));
  cmdlist->RSSetViewports(1, &view);
  cmdlist->RSSetScissorRects(1, &rect);


  cmdlist->SetDescriptorHeaps(descriptors.GetAddressOf(), 1);
  cmdlist->SetDescriptorHeaps(depth_tex_descriptors.GetAddressOf(), 1);
  cmdlist->SetDescriptorHeaps(SamplerHeap.GetAddressOf(), 1);

  float tmp[] = { 0., 0., 0., 0. };
  cmdlist->ResourceBarrier(1, &setResourceTransitionBarrier(Context::getInstance()->getCurrentBackBuffer(), D3D12_RESOURCE_USAGE_PRESENT, D3D12_RESOURCE_USAGE_RENDER_TARGET));
  cmdlist->ClearDepthStencilView(depth_descriptors->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_DEPTH, 1.f, 0, nullptr, 0);

  cmdlist->SetGraphicsRootSignature(RS::getInstance()->pRootSignature.Get());
  cmdlist->SetRenderTargets(&Context::getInstance()->getCurrentBackBufferDescriptor(), true, 1, &depth_descriptors->GetCPUDescriptorHandleForHeapStart());
  cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmdlist->SetVertexBuffers(0, vao->getVertexBufferView().data(), (UINT)vao->getVertexBufferView().size());
  cmdlist->SetIndexBuffer(&vao->getIndexBufferView());

  irr::core::matrix4 Model, View;
  View.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
  Model.setTranslation(irr::core::vector3df(0., 0., 10.));
  Model.setScale(2.);

  SSAOBuffer *cbufdata;
  cbufferdata[0]->Map(0, nullptr, (void**)&cbufdata);
  memcpy(cbufdata->ViewProjectionMatrix, View.pointer(), 16 * sizeof(float));
  memcpy(cbufdata->ProjectionMatrix, View.pointer(), 16 * sizeof(float));
  memcpy(cbufdata->ModelMatrix, Model.pointer(), 16 * sizeof(float));
  cbufdata->color[0] = 1.;
  cbufdata->color[1] = 0.;
  cbufdata->color[2] = 0.;
  cbufdata->color[3] = 0.;
  cbufdata->zn = 1.f;
  cbufdata->zf = 100.f;
  cbufferdata[0]->Unmap(0, nullptr);

  cmdlist->SetGraphicsRootDescriptorTable(0, descriptors->GetGPUDescriptorHandleForHeapStart().MakeOffsetted(Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP)));
  cmdlist->DrawIndexedInstanced((UINT)std::get<0>(vao->meshOffset[0]), 1, 0, 0, 0);

  Model.setTranslation(irr::core::vector3df(0.f, 0.f, 8.));
  Model.setScale(1.);
  cbufferdata[1]->Map(0, nullptr, (void**)&cbufdata);
  memcpy(cbufdata->ViewProjectionMatrix, View.pointer(), 16 * sizeof(float));
  memcpy(cbufdata->ProjectionMatrix, View.pointer(), 16 * sizeof(float));
  memcpy(cbufdata->ModelMatrix, Model.pointer(), 16 * sizeof(float));
  cbufdata->color[0] = 0.;
  cbufdata->color[1] = 1.;
  cbufdata->color[2] = 0.;
  cbufdata->color[3] = 0.;
  cbufdata->zn = 1.f;
  cbufdata->zf = 100.f;
  cbufferdata[1]->Unmap(0, nullptr);
  cmdlist->SetGraphicsRootDescriptorTable(0, descriptors->GetGPUDescriptorHandleForHeapStart());
  cmdlist->DrawIndexedInstanced((UINT)std::get<0>(vao->meshOffset[0]), 1, 0, 0, 0);

  {
    D3D12_RESOURCE_BARRIER_DESC barriers[2] = {
      setResourceTransitionBarrier(DepthBuffer.Get(), D3D12_RESOURCE_USAGE_DEPTH, D3D12_RESOURCE_USAGE_COPY_SOURCE),
      setResourceTransitionBarrier(DepthTexture.Get(), D3D12_RESOURCE_USAGE_GENERIC_READ, D3D12_RESOURCE_USAGE_COPY_DEST)
    };
    cmdlist->ResourceBarrier(2, barriers);
  }

  D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
  dst.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
  dst.Subresource = 0;
  dst.pResource = DepthTexture.Get();
  src.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
  src.Subresource = 0;
  src.pResource = DepthBuffer.Get();
  cmdlist->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr, D3D12_COPY_NONE);

  {
    D3D12_RESOURCE_BARRIER_DESC barriers[2] = {
      setResourceTransitionBarrier(DepthBuffer.Get(), D3D12_RESOURCE_USAGE_COPY_SOURCE, D3D12_RESOURCE_USAGE_DEPTH),
      setResourceTransitionBarrier(DepthTexture.Get(), D3D12_RESOURCE_USAGE_COPY_DEST, D3D12_RESOURCE_USAGE_GENERIC_READ)
    };
    cmdlist->ResourceBarrier(2, barriers);
  }

  cmdlist->SetGraphicsRootDescriptorTable(1, depth_tex_descriptors->GetGPUDescriptorHandleForHeapStart());
  cmdlist->SetGraphicsRootDescriptorTable(2, SamplerHeap->GetGPUDescriptorHandleForHeapStart());
  cmdlist->ClearRenderTargetView(Context::getInstance()->getCurrentBackBufferDescriptor(), tmp, nullptr, 0);
  cmdlist->SetPipelineState(LinearizeDepthShader::getInstance()->pso.Get());
  cmdlist->SetVertexBuffers(0, &ScreenQuadView, 1);
  cmdlist->DrawInstanced(3, 1, 0, 0);

  cmdlist->ResourceBarrier(1, &setResourceTransitionBarrier(Context::getInstance()->getCurrentBackBuffer(), D3D12_RESOURCE_USAGE_RENDER_TARGET, D3D12_RESOURCE_USAGE_PRESENT));

  cmdlist->Close();
  Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());
  Context::getInstance()->Swap();
}


void Clean()
{
  Context::getInstance()->kill();
  Object::getInstance()->kill();
  LinearizeDepthShader::getInstance()->kill();
  delete vao;
  RS::getInstance()->kill();
}


int WINAPI WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  HWND hwnd = WindowUtil::Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
  Init(hwnd);
  // this struct holds Windows event messages
  MSG msg;

  // wait for the next message in the queue, store the result in 'msg'
  while (GetMessage(&msg, NULL, 0, 0))
  {
    // translate keystroke messages into the right format
    TranslateMessage(&msg);

    // send the message to the WindowProc function
    DispatchMessage(&msg);
    Draw();
  }

  Clean();
  // return this part of the WM_QUIT message to Windows
  return (int)msg.wParam;
}

