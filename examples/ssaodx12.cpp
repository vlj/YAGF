#include <D3DAPI/Misc.h>
#include <D3DAPI/PSO.h>
#include <Util/GeometryCreator.h>
#include <D3DAPI/VAO.h>
#include <Maths/matrix4.h>
#include <D3DAPI/RootSignature.h>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

class Object : public PipelineStateObject<Object, irr::video::S3DVertex2TCoords>
{
public:
  Object() : PipelineStateObject<Object, irr::video::S3DVertex2TCoords>(L"Debug\\ssao_vtx.cso", L"Debug\\ssao_pix.cso")
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

struct SSAOBuffer
{
  float ModelMatrix[16];
  float ViewProjectionMatrix[16];
  float ProjectionMatrix[16];
  float zn;
  float zf;
};

FormattedVertexStorage<irr::video::S3DVertex> *vao;

using namespace Microsoft::WRL;
ComPtr<ID3D12Resource> DepthTexture;
ComPtr<ID3D12Resource> cbufferdata;
ComPtr<ID3D12DescriptorHeap> descriptors;
ComPtr<ID3D12DescriptorHeap> depth_descriptors;

RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, DescriptorTable<ConstantsBufferResource<0>> > *rs;

void Init(HWND hWnd)
{
  Context::getInstance()->InitD3D(hWnd);

  irr::scene::SMeshBuffer *buffer = GeometryCreator::createCubeMeshBuffer(
    irr::core::vector3df(1., 1., 1.));
  std::vector<irr::scene::SMeshBuffer> buffers = { *buffer };

  vao = new FormattedVertexStorage<irr::video::S3DVertex>(Context::getInstance()->cmdqueue.Get(), buffers);

  // Create render targets
  Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, 1024, 1024, 1, 1, 1, 0, D3D12_RESOURCE_MISC_ALLOW_DEPTH_STENCIL),
    D3D12_RESOURCE_USAGE_DEPTH,
    &CD3D12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1., 0),
    IID_PPV_ARGS(&DepthTexture)
    );

  D3D12_DESCRIPTOR_HEAP_DESC heapdesc_depth = {};
  heapdesc_depth.Type = D3D12_DSV_DESCRIPTOR_HEAP;
  heapdesc_depth.NumDescriptors = 1;
  heapdesc_depth.Flags = D3D12_DESCRIPTOR_HEAP_NONE;
  heapdesc_depth.NodeMask = 1;
  Context::getInstance()->dev->CreateDescriptorHeap(&heapdesc_depth, IID_PPV_ARGS(&depth_descriptors));

  D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_desc = {};
  depth_stencil_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depth_stencil_desc.Texture2D.MipSlice = 0;
  Context::getInstance()->dev->CreateDepthStencilView(DepthTexture.Get(), &depth_stencil_desc, depth_descriptors->GetCPUDescriptorHandleForHeapStart());

  Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Buffer(sizeof(struct SSAOBuffer)),
    D3D12_RESOURCE_USAGE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&cbufferdata)
    );

  D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
  heapdesc.Type = D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP;
  heapdesc.NumDescriptors = 1;
  heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
  Context::getInstance()->dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&descriptors));

  D3D12_CONSTANT_BUFFER_VIEW_DESC cbvdesc = {};
  cbvdesc.BufferLocation = cbufferdata->GetGPUVirtualAddress();
  cbvdesc.SizeInBytes = 256;
  Context::getInstance()->dev->CreateConstantBufferView(&cbvdesc, descriptors->GetCPUDescriptorHandleForHeapStart());

  rs = new RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, DescriptorTable<ConstantsBufferResource<0>> >();

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

  irr::core::matrix4 Model, View;
  View.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
  Model.setTranslation(irr::core::vector3df(0.f, 0.f, 8.));

  SSAOBuffer *cbufdata;
  cbufferdata->Map(0, nullptr, (void**)&cbufdata);
  memcpy(cbufdata->ViewProjectionMatrix, View.pointer(), 16 * sizeof(float));
  memcpy(cbufdata->ProjectionMatrix, View.pointer(), 16 * sizeof(float));
  memcpy(cbufdata->ModelMatrix, Model.pointer(), 16 * sizeof(float));
  cbufdata->zn = 1.f;
  cbufdata->zf = 100.f;
  cbufferdata->Unmap(0, nullptr);

  float tmp[] = { 0., 0., 0., 0. };
  D3D12_RESOURCE_BARRIER_DESC barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = Context::getInstance()->getCurrentBackBuffer();
  barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_RENDER_TARGET;
  cmdlist->ResourceBarrier(1, &barrier);
  cmdlist->ClearRenderTargetView(Context::getInstance()->getCurrentBackBufferDescriptor(), tmp, nullptr, 0);
  cmdlist->ClearDepthStencilView(depth_descriptors->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_DEPTH, 1., 0., nullptr, 0);

  cmdlist->SetGraphicsRootSignature(rs->pRootSignature.Get());
  cmdlist->SetGraphicsRootDescriptorTable(0, descriptors->GetGPUDescriptorHandleForHeapStart());

  cmdlist->SetRenderTargets(&Context::getInstance()->getCurrentBackBufferDescriptor(), true, 1, &depth_descriptors->GetCPUDescriptorHandleForHeapStart());
  cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmdlist->SetVertexBuffers(0, &vao->getVertexBufferView(), 1);
  cmdlist->SetIndexBuffer(&vao->getIndexBufferView());
  cmdlist->DrawIndexedInstanced(std::get<0>(vao->meshOffset[0]), 1, 0, 0, 0);

  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = Context::getInstance()->getCurrentBackBuffer();
  barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_PRESENT;
  cmdlist->ResourceBarrier(1, &barrier);

  cmdlist->Close();
  Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());
  Context::getInstance()->Swap();
}


void Clean()
{
  Context::getInstance()->kill();
  Object::getInstance()->kill();
  delete vao;
  delete rs;
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

