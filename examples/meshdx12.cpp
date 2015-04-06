#include <D3DAPI/Texture.h>
#include <D3DAPI/Context.h>
#include <D3DAPI/RootSignature.h>

#include <D3DAPI/Misc.h>
#include <D3DAPI/VAO.h>
#include <D3DAPI/S3DVertex.h>
#include <Loaders/B3D.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <D3DAPI/PSO.h>


#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

using namespace Microsoft::WRL; // For ComPtr

ComPtr<ID3D12GraphicsCommandList> cmdlist;
ComPtr<ID3D12Fence> fence;
HANDLE handle;
ComPtr<ID3D12Resource> cbuffer;
ComPtr<ID3D12DescriptorHeap> ReadResourceHeaps;
ComPtr<ID3D12Resource> Tex;
ComPtr<ID3D12DescriptorHeap> Sampler;
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdalloc;

FormattedVertexStorage<irr::video::S3DVertex2TCoords> *vao;

RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, DescriptorTable<ConstantsBufferResource<0>, ShaderResource<0> >, DescriptorTable<SamplerResource<0>> > *rs;

class Object : public PipelineStateObject<Object, irr::video::S3DVertex2TCoords>
{
public:
  Object() : PipelineStateObject<Object, irr::video::S3DVertex2TCoords>(L"Debug\\vtx.cso", L"Debug\\pix.cso")
  {}

  static void SetRasterizerAndBlendStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psodesc)
  {
    psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
    psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psodesc.NumRenderTargets = 1;
    psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
  }
};

void Init(HWND hWnd)
{
  Context::getInstance()->InitD3D(hWnd);
  ID3D12Device *dev = Context::getInstance()->dev.Get();

  HRESULT hr = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));
  hr = dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), 0, IID_PPV_ARGS(&cmdlist));

  handle = CreateEvent(0, FALSE, FALSE, 0);
  hr = dev->CreateFence(0, D3D12_FENCE_MISC_NONE, IID_PPV_ARGS(&fence));

  hr = dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Buffer(256),
    D3D12_RESOURCE_USAGE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&cbuffer));

  float *tmp;
  cbuffer->Map(0, nullptr, (void**)&tmp);
  float white[4] = { 1.f, 1.f, 1.f, 1.f };
  memcpy(tmp, white, 4 * sizeof(float));
  cbuffer->Unmap(0, nullptr);

  {
    D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
    heapdesc.Type = D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP;
    heapdesc.NumDescriptors = 2;
    heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
    hr = dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&ReadResourceHeaps));

    D3D12_CONSTANT_BUFFER_VIEW_DESC bufdesc = {};
    bufdesc.BufferLocation = cbuffer->GetGPUVirtualAddress();
    bufdesc.SizeInBytes = 256;
    dev->CreateConstantBufferView(&bufdesc, ReadResourceHeaps->GetCPUDescriptorHandleForHeapStart());
  }

  // Define Root Signature
  rs = new RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, DescriptorTable<ConstantsBufferResource<0>, ShaderResource<0> >, DescriptorTable<SamplerResource<0>> >();

  irr::io::CReadFile reader("..\\examples\\anchor.b3d");
  irr::scene::CB3DMeshFileLoader loader(&reader);
  std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > buffers = loader.AnimatedMesh;
  std::vector<irr::scene::SMeshBufferLightMap> reorg;

  for (auto buf : buffers)
    reorg.push_back(buf.first);

  vao = new FormattedVertexStorage<irr::video::S3DVertex2TCoords>(Context::getInstance()->cmdqueue.Get(), reorg);

  std::vector<IImage> imgs;
  for (auto tex : loader.Textures)
  {
    irr::io::CReadFile texreader("..\\examples\\anchor.DDS");
    imgs.push_back(irr::video::CImageLoaderDDS::loadImage(&texreader));
  }

  D3D12_RESOURCE_BARRIER_DESC barrier = {};
  // Upload to gpudata
  {
    // Texture
    Texture TextureInRam(imgs[0]);

    hr = dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_B8G8R8A8_UNORM, (UINT)imgs[0].getWidth(), (UINT)imgs[0].getHeight(), 1, 10),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&Tex)
      );

    TextureInRam.CreateUploadCommandToResourceInDefaultHeap(cmdlist.Get(), Tex.Get(), 0);

    dev->CreateShaderResourceView(Tex.Get(), &TextureInRam.getResourceViewDesc(), ReadResourceHeaps->GetCPUDescriptorHandleForHeapStart().MakeOffsetted(dev->GetDescriptorHandleIncrementSize(D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP)));

    D3D12_DESCRIPTOR_HEAP_DESC sampler_heap = {};
    sampler_heap.Type = D3D12_SAMPLER_DESCRIPTOR_HEAP;
    sampler_heap.NumDescriptors = 1;
    sampler_heap.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
    hr = dev->CreateDescriptorHeap(&sampler_heap, IID_PPV_ARGS(&Sampler));

    D3D12_SAMPLER_DESC samplerdesc = {};
    samplerdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerdesc.AddressU = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.AddressV = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.AddressW = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.MaxAnisotropy = 1;
    samplerdesc.MinLOD = 0;
    samplerdesc.MaxLOD = 0;
    dev->CreateSampler(&samplerdesc, Sampler->GetCPUDescriptorHandleForHeapStart());

    ComPtr<ID3D12Fence> datauploadfence;
    hr = dev->CreateFence(0, D3D12_FENCE_MISC_NONE, IID_PPV_ARGS(&datauploadfence));
    HANDLE cpudatauploadevent = CreateEvent(0, FALSE, FALSE, 0);
    datauploadfence->SetEventOnCompletion(1, cpudatauploadevent);
    cmdlist->Close();
    Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());
    Context::getInstance()->cmdqueue->Signal(datauploadfence.Get(), 1);
    WaitForSingleObject(cpudatauploadevent, INFINITE);
    cmdlist->Reset(cmdalloc.Get(), Object::getInstance()->pso.Get());
  }
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

  cmdlist->RSSetViewports(1, &view);
  cmdlist->RSSetScissorRects(1, &rect);

  D3D12_RESOURCE_BARRIER_DESC barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = Context::getInstance()->getCurrentBackBuffer();
  barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_RENDER_TARGET;
  cmdlist->ResourceBarrier(1, &barrier);

  float clearColor[] = { .25f, .25f, 0.35f, 1.0f };
  cmdlist->ClearRenderTargetView(Context::getInstance()->getCurrentBackBufferDescriptor(), clearColor, 0, 0);


  cmdlist->SetGraphicsRootSignature(rs->pRootSignature.Get());
  float c[] = { 1., 1., 1., 1. };
  cmdlist->SetGraphicsRootDescriptorTable(0, ReadResourceHeaps->GetGPUDescriptorHandleForHeapStart());
  cmdlist->SetGraphicsRootDescriptorTable(1, Sampler->GetGPUDescriptorHandleForHeapStart());
  cmdlist->SetRenderTargets(&Context::getInstance()->getCurrentBackBufferDescriptor(), true, 1, nullptr);
  cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmdlist->SetIndexBuffer(&vao->getIndexBufferView());
  cmdlist->SetVertexBuffers(0, &vao->getVertexBufferView(), 1);
  cmdlist->DrawIndexedInstanced((UINT)std::get<0>(vao->meshOffset[0]), 1, (UINT)std::get<1>(vao->meshOffset[0]), (UINT)std::get<2>(vao->meshOffset[0]), 0);

  barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_PRESENT;
  cmdlist->ResourceBarrier(1, &barrier);
  HRESULT hr = cmdlist->Close();
  Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());

  hr = fence->Signal(0);
  hr = fence->SetEventOnCompletion(1, handle);
  hr = Context::getInstance()->cmdqueue->Signal(fence.Get(), 1);
  WaitForSingleObject(handle, INFINITE);
  cmdalloc->Reset();
  cmdlist->Reset(cmdalloc.Get(), Object::getInstance()->pso.Get());
  Context::getInstance()->Swap();
}

void Clean()
{
  Context::getInstance()->kill();
  Object::getInstance()->kill();
  delete rs;
  delete vao;
}

int WINAPI WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{

  Init(WindowUtil::Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow));
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

