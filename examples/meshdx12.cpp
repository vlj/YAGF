#include <windows.h>
#include <windowsx.h>

#include <D3DAPI/Context.h>
#include <D3DAPI/RootSignature.h>
#include <D3DAPI/Texture.h>
#include <d3dcompiler.h>

#include <Loaders/B3D.h>
#include <Loaders/PNG.h>
#include <tuple>

std::vector<std::tuple<size_t, size_t, size_t> > CountBaseIndexVTX;

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

using namespace Microsoft::WRL; // For ComPtr

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam);

ComPtr<ID3D12CommandAllocator> cmdalloc;

ComPtr<ID3D12GraphicsCommandList> cmdlist;
ComPtr<ID3D12Fence> fence;
ComPtr<ID3D12PipelineState> pso;
ComPtr<ID3D12Resource> vertexbuffer;
ComPtr<ID3D12Resource> indexbuffer;
HANDLE handle;
ComPtr<ID3D12Resource> cbuffer;
ComPtr<ID3D12DescriptorHeap> ReadResourceHeaps;
D3D12_VERTEX_BUFFER_VIEW vtxb = {};
D3D12_INDEX_BUFFER_VIEW idxb = {};
ComPtr<ID3D12Resource> Tex;
ComPtr<ID3D12DescriptorHeap> Sampler;
Texture *TextureInRam;

RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, DescriptorTable<ConstantsBufferResource<0>, ShaderResource<0> >, DescriptorTable<SamplerResource<0>> > *rs;

void Init(HWND hWnd)
{
  Context::getInstance()->InitD3D(hWnd);
  ID3D12Device *dev = Context::getInstance()->dev.Get();

  HRESULT hr = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));
  hr = dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), 0, IID_PPV_ARGS(&cmdlist));

  handle = CreateEvent(0, FALSE, FALSE, 0);
  hr = dev->CreateFence(0, D3D12_FENCE_MISC_NONE, IID_PPV_ARGS(&fence));

  // Create PSO
  {
    ComPtr<ID3DBlob> vtxshaderblob;
    hr = D3DReadFileToBlob(L"Debug\\vtx.cso", &vtxshaderblob);
    ComPtr<ID3DBlob> pxshaderblob;
    hr = D3DReadFileToBlob(L"Debug\\pix.cso", &pxshaderblob);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
    psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
    psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    D3D12_INPUT_ELEMENT_DESC IAdesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 2, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D12_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_PER_VERTEX_DATA, 0 },
        { "SECONDTEXCOORD", 4, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_PER_VERTEX_DATA, 0 },
    };
    psodesc.InputLayout.pInputElementDescs = IAdesc;
    psodesc.InputLayout.NumElements = 5;
    psodesc.NumRenderTargets = 1;
    psodesc.VS.BytecodeLength = vtxshaderblob->GetBufferSize();
    psodesc.VS.pShaderBytecode = vtxshaderblob->GetBufferPointer();
    psodesc.PS.BytecodeLength = pxshaderblob->GetBufferSize();
    psodesc.PS.pShaderBytecode = pxshaderblob->GetBufferPointer();
    psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psodesc.SampleDesc.Count = 1;
    psodesc.SampleMask = UINT_MAX;
    psodesc.NodeMask = 1;
    psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);

    hr = dev->CreateGraphicsPipelineState(&psodesc, IID_PPV_ARGS(&pso));
  }

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
  {
    rs = new RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, DescriptorTable<ConstantsBufferResource<0>, ShaderResource<0> >, DescriptorTable<SamplerResource<0>> >();
  }
  irr::io::CReadFile reader("..\\examples\\anchor.b3d");
  irr::scene::CB3DMeshFileLoader loader(&reader);
  std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > buffers = loader.AnimatedMesh;

  for (auto tmp : buffers)
  {
    const irr::scene::SMeshBufferLightMap &tmpbuf = tmp.first;
    //		std::pair<size_t, size_t> BaseIndexVtx = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords> >::getInstance()->getBase(&tmpbuf);
    CountBaseIndexVTX.push_back(std::make_tuple(tmpbuf.getIndexCount(), 0, 0));
  }

  std::vector<IImage *> imgs;
  for (auto tmp : loader.Textures)
  {
    irr::io::CReadFile texreader("..\\examples\\anchor.png");
    imgs.push_back(irr::video::CImageLoaderPng::loadImage(&texreader));
  }

  D3D12_RESOURCE_BARRIER_DESC barrier = {};
  // Upload to gpudata
  {
    // Vertex and Index Buffer
    ComPtr<ID3D12Resource> vertexdata, indexdata;
    hr = dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(buffers[0].first.getVertexCount() * sizeof(irr::video::S3DVertex2TCoords)),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&vertexdata));

    void *tmp;
    hr = vertexdata->Map(0, nullptr, &tmp);
    memcpy(tmp, buffers[0].first.getVertices(), buffers[0].first.getVertexCount() * sizeof(irr::video::S3DVertex2TCoords));
    vertexdata->Unmap(0, nullptr);

    hr = dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(buffers[0].first.getIndexCount() * sizeof(unsigned short)),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&indexdata));

    hr = indexdata->Map(0, nullptr, &tmp);
    memcpy(tmp, buffers[0].first.getIndices(), buffers[0].first.getIndexCount() * sizeof(unsigned short));
    indexdata->Unmap(0, nullptr);

    hr = dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(buffers[0].first.getVertexCount() * sizeof(irr::video::S3DVertex2TCoords)),
      D3D12_RESOURCE_USAGE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&vertexbuffer));

    hr = dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(buffers[0].first.getIndexCount() * sizeof(unsigned short)),
      D3D12_RESOURCE_USAGE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&indexbuffer));

    cmdlist->CopyBufferRegion(vertexbuffer.Get(), 0, vertexdata.Get(), 0, buffers[0].first.getVertexCount() * sizeof(irr::video::S3DVertex2TCoords), D3D12_COPY_NONE);
    cmdlist->CopyBufferRegion(indexbuffer.Get(), 0, indexdata.Get(), 0, buffers[0].first.getIndexCount() * sizeof(unsigned short), D3D12_COPY_NONE);

    // Texture
    TextureInRam = new Texture(imgs[0]->getWidth(), imgs[0]->getHeight(), 4 * sizeof(char));
    TextureInRam->texinram->Map(0, nullptr, &tmp);
    memcpy(tmp, imgs[0]->getPointer(), 4 * sizeof(char) * imgs[0]->getHeight() * imgs[0]->getWidth());
    TextureInRam->texinram->Unmap(0, nullptr);

    hr = dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_B8G8R8A8_UNORM, imgs[0]->getWidth(), imgs[0]->getHeight(), 1, 1, 1, 0, D3D12_RESOURCE_MISC_NONE),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&Tex)
      );

    TextureInRam->CreateUploadCommandToResourceInDefaultHeap(cmdlist.Get(), Tex.Get());

    dev->CreateShaderResourceView(Tex.Get(), &TextureInRam->getResourceViewDesc(), ReadResourceHeaps->GetCPUDescriptorHandleForHeapStart().MakeOffsetted(dev->GetDescriptorHandleIncrementSize(D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP)));

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

    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = vertexbuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_GENERIC_READ;
    cmdlist->ResourceBarrier(1, &barrier);
    barrier.Transition.pResource = indexbuffer.Get();
    cmdlist->ResourceBarrier(1, &barrier);

    ComPtr<ID3D12Fence> datauploadfence;
    hr = dev->CreateFence(0, D3D12_FENCE_MISC_NONE, IID_PPV_ARGS(&datauploadfence));
    HANDLE cpudatauploadevent = CreateEvent(0, FALSE, FALSE, 0);
    datauploadfence->SetEventOnCompletion(1, cpudatauploadevent);
    cmdlist->Close();
    Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());
    Context::getInstance()->cmdqueue->Signal(datauploadfence.Get(), 1);
    WaitForSingleObject(cpudatauploadevent, INFINITE);
    cmdalloc->Reset();
    cmdlist->Reset(cmdalloc.Get(), pso.Get());

    vtxb.BufferLocation = vertexbuffer->GetGPUVirtualAddress();
    vtxb.SizeInBytes = buffers[0].first.getVertexCount() * sizeof(irr::video::S3DVertex2TCoords);
    vtxb.StrideInBytes = sizeof(irr::video::S3DVertex2TCoords);

    idxb.BufferLocation = indexbuffer->GetGPUVirtualAddress();
    idxb.Format = DXGI_FORMAT_R16_UINT;
    idxb.SizeInBytes = buffers[0].first.getIndexCount() * sizeof(unsigned short);
  }
}

void Draw()
{
  D3D12_RECT rect = {};
  rect.left = 0;
  rect.top = 0;
  rect.bottom = 300;
  rect.right = 300;

  D3D12_VIEWPORT view = {};
  view.Height = 300;
  view.Width = 300;
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
  cmdlist->SetIndexBuffer(&idxb);
  cmdlist->SetVertexBuffers(0, &vtxb, 1);
  cmdlist->DrawIndexedInstanced(idxb.SizeInBytes / sizeof(unsigned short), 1, 0, 0, 0);

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
  cmdlist->Reset(cmdalloc.Get(), pso.Get());
  Context::getInstance()->Swap();
}

void Clean()
{
  Context::getInstance()->kill();
  delete rs;
  delete TextureInRam;
}

int WINAPI WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  // the handle for the window, filled by a function
  HWND hWnd;
  // this struct holds information for the window class
  WNDCLASSEX wc = {};

  // fill in the struct with the needed information
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
  wc.lpszClassName = "WindowClass1";

  // register the window class
  RegisterClassEx(&wc);

  // create the window and use the result as the handle
  hWnd = CreateWindowEx(NULL,
    "WindowClass1",    // name of the window class
    "MeshDX12",   // title of the window
    WS_OVERLAPPEDWINDOW,    // window style
    300,    // x-position of the window
    300,    // y-position of the window
    1024,    // width of the window
    1024,    // height of the window
    NULL,    // we have no parent window, NULL
    NULL,    // we aren't using menus, NULL
    hInstance,    // application handle
    NULL);    // used with multiple windows, NULL
  ShowWindow(hWnd, nCmdShow);

  // this struct holds Windows event messages
  MSG msg;

  Init(hWnd);

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
  return msg.wParam;
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  // sort through and find what code to run for the message given
  switch (message)
  {
    // this message is read when the window is closed
  case WM_DESTROY:
  {
    // close the application entirely
    PostQuitMessage(0);
    return 0;
  } break;
  }

  // Handle any messages the switch statement didn't
  return DefWindowProc(hWnd, message, wParam, lParam);
}