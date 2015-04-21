// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <d3dapi.h>
#include <D3DAPI/Texture.h>
#include <D3DAPI/Resource.h>
#include <D3DAPI/Sampler.h>

struct WrapperResource* D3DAPI::createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4])
{
  WrapperResource *result = (WrapperResource*)malloc(sizeof(WrapperResource));
  DXGI_FORMAT Fmt = getDXGIFormatFromColorFormat(Format);
  Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
  HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Tex2D(Fmt, (UINT)Width, (UINT)Height, 1, 0, 1, 0, D3D12_RESOURCE_MISC_ALLOW_RENDER_TARGET),
    D3D12_RESOURCE_USAGE_RENDER_TARGET,
    &CD3D12_CLEAR_VALUE(Fmt, fastColor),
    IID_PPV_ARGS(&result->D3DValue.resource));

  D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
  srv.Format = Fmt;
  srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv.Texture2D.MipLevels = 1;

  result->D3DValue.description.SRV = srv;

  return result;
}

struct WrapperResource* D3DAPI::createDepthStencilTexture(size_t Width, size_t Height)
{
  WrapperResource *result = (WrapperResource*)malloc(sizeof(WrapperResource));
  HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, (UINT)Width, (UINT)Height, 1, 0, 1, 0, D3D12_RESOURCE_MISC_ALLOW_DEPTH_STENCIL),
    D3D12_RESOURCE_USAGE_DEPTH,
    &CD3D12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1., 0),
    IID_PPV_ARGS(&result->D3DValue.resource));

  D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
  dsv.Format = DXGI_FORMAT_D32_FLOAT;
  dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  dsv.Texture2D.MipSlice = 0;
  result->D3DValue.description.DSV = dsv;

  return result;
}

struct WrapperRTTSet* D3DAPI::createRTTSet(const std::vector<WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height, WrapperResource *DepthStencil)
{
  WrapperRTTSet *result = (WrapperRTTSet*) malloc(sizeof(WrapperRTTSet));
  std::vector<ID3D12Resource *> resources;
  std::vector<DXGI_FORMAT> dxgi_formats;
  for (unsigned i = 0; i < RTTs.size(); i++) {
    resources.push_back(RTTs[i]->D3DValue.resource);
    dxgi_formats.push_back(getDXGIFormatFromColorFormat(formats[i]));
  }
  if (DepthStencil)
    new(result) D3DRTTSet(resources, dxgi_formats, Width, Height, DepthStencil->D3DValue.resource, &DepthStencil->D3DValue.description.DSV);
  else
    new(result) D3DRTTSet(resources, dxgi_formats, Width, Height, nullptr, nullptr);

  return result;
}

static D3D12_RESOURCE_USAGE convertResourceUsage(enum class RESOURCE_USAGE ru)
{
  switch (ru)
  {
  default:
    abort();
  case RESOURCE_USAGE::READ_GENERIC:
    return D3D12_RESOURCE_USAGE_GENERIC_READ;
  case RESOURCE_USAGE::COPY_DEST:
    return D3D12_RESOURCE_USAGE_COPY_DEST;
  case RESOURCE_USAGE::COPY_SRC:
    return D3D12_RESOURCE_USAGE_COPY_SOURCE;
  case RESOURCE_USAGE::PRESENT:
    return D3D12_RESOURCE_USAGE_PRESENT;
  case RESOURCE_USAGE::RENDER_TARGET:
    return D3D12_RESOURCE_USAGE_RENDER_TARGET;
  }
}

void D3DAPI::writeResourcesTransitionBarrier(struct WrapperCommandList* wrappedCmdList, const std::vector<std::tuple<WrapperResource *, enum class RESOURCE_USAGE, enum class RESOURCE_USAGE> > &barriers)
{
  std::vector<D3D12_RESOURCE_BARRIER_DESC> barriersDesc;
  ID3D12GraphicsCommandList *CmdList = wrappedCmdList->D3DValue.CommandList;
  for (auto barrier : barriers)
  {
    ID3D12Resource* unwrappedResource = std::get<0>(barrier)->D3DValue.resource;
    barriersDesc.push_back(setResourceTransitionBarrier(unwrappedResource, convertResourceUsage(std::get<1>(barrier)), convertResourceUsage(std::get<2>(barrier))));
  }
  CmdList->ResourceBarrier((UINT)barriersDesc.size(), barriersDesc.data());
}

void D3DAPI::clearRTTSet(struct WrapperCommandList* wrappedCmdList, struct WrapperRTTSet* RTTSet, float color[4])
{
  RTTSet->D3DValue.Clear(wrappedCmdList->D3DValue.CommandList, color);
}

void D3DAPI::clearDepthStencilFromRTTSet(struct WrapperCommandList* wrappedCmdList, struct WrapperRTTSet* RTTSet, float Depth, unsigned Stencil)
{
  RTTSet->D3DValue.ClearDepthStencil(wrappedCmdList->D3DValue.CommandList, Depth, Stencil);
}

void D3DAPI::setRTTSet(struct WrapperCommandList* wrappedCmdList, struct WrapperRTTSet*RTTSet)
{
  RTTSet->D3DValue.Bind(wrappedCmdList->D3DValue.CommandList);
}

struct WrapperDescriptorHeap* D3DAPI::createCBVSRVUAVDescriptorHeap(const std::vector<std::tuple<struct WrapperResource *, RESOURCE_VIEW, size_t> > &Resources)
{
  WrapperDescriptorHeap *result = (WrapperDescriptorHeap*)malloc(sizeof(WrapperDescriptorHeap));
  D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
  heapdesc.NumDescriptors = (UINT)Resources.size();
  heapdesc.Type = D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP;
  heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
  HRESULT hr = Context::getInstance()->dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&result->D3DValue));
  size_t Index = 0, Increment = Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP);

  for (const std::tuple<struct WrapperResource *, enum class RESOURCE_VIEW, size_t> &Resource : Resources)
  {
    D3D12_CPU_DESCRIPTOR_HANDLE Handle = result->D3DValue->GetCPUDescriptorHandleForHeapStart().MakeOffsetted((INT) (Index * Increment));
    switch (std::get<1>(Resource))
    {
    case RESOURCE_VIEW::CONSTANTS_BUFFER:
      Context::getInstance()->dev->CreateConstantBufferView(&std::get<0>(Resource)->D3DValue.description.CBV, Handle);
      break;
    case RESOURCE_VIEW::SHADER_RESOURCE:
      Context::getInstance()->dev->CreateShaderResourceView(std::get<0>(Resource)->D3DValue.resource, &std::get<0>(Resource)->D3DValue.description.SRV, Handle);
      break;
    }
    Index++;
  }
  return result;
}

struct WrapperDescriptorHeap* D3DAPI::createSamplerHeap(const std::vector<std::pair<enum class SAMPLER_TYPE, size_t>> &SamplersDesc)
{
  WrapperDescriptorHeap *result = (WrapperDescriptorHeap*)malloc(sizeof(WrapperDescriptorHeap));
  D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
  heapdesc.NumDescriptors = (UINT)SamplersDesc.size();
  heapdesc.Type = D3D12_SAMPLER_DESCRIPTOR_HEAP;
  heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
  HRESULT hr = Context::getInstance()->dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&result->D3DValue));
  size_t Index = 0, Increment = Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_SAMPLER_DESCRIPTOR_HEAP);

  for (auto tmp : SamplersDesc)
  {
    D3D12_SAMPLER_DESC samplerdesc = {};

    samplerdesc.AddressU = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.AddressV = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.AddressW = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.MaxAnisotropy = 1;
    samplerdesc.MinLOD = 0;
    samplerdesc.MaxLOD = 1000;
    switch (tmp.first)
    {
    case SAMPLER_TYPE::TRILINEAR:
      samplerdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
      break;
    case SAMPLER_TYPE::BILINEAR:
      samplerdesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
      samplerdesc.MaxLOD = 0;
      break;
    case SAMPLER_TYPE::NEAREST:
      samplerdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
      samplerdesc.MaxLOD = 0;
      break;
    case SAMPLER_TYPE::ANISOTROPIC:
      samplerdesc.Filter = D3D12_FILTER_ANISOTROPIC;
      samplerdesc.MaxAnisotropy = 16;
      break;
    }
    Context::getInstance()->dev->CreateSampler(&samplerdesc, result->D3DValue->GetCPUDescriptorHandleForHeapStart().MakeOffsetted((INT)(Index * Increment)));
    Index++;
  }
  return result;
}

void D3DAPI::setDescriptorHeap(struct WrapperCommandList* wrappedCmdList, size_t slot, struct WrapperDescriptorHeap *DescriptorHeap)
{
  wrappedCmdList->D3DValue.CommandList->SetGraphicsRootDescriptorTable((UINT)slot, DescriptorHeap->D3DValue->GetGPUDescriptorHandleForHeapStart());
}

void D3DAPI::setPipelineState(struct WrapperCommandList* wrappedCmdList, struct WrapperPipelineState* wrappedPipelineState)
{
  wrappedCmdList->D3DValue.CommandList->SetPipelineState(wrappedPipelineState->D3DValue.pipelineStateObject);
  wrappedCmdList->D3DValue.CommandList->SetGraphicsRootSignature(wrappedPipelineState->D3DValue.rootSignature);
  wrappedCmdList->D3DValue.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void D3DAPI::setIndexVertexBuffersSet(struct WrapperCommandList* wrappedCmdList, struct WrapperIndexVertexBuffersSet* wrappedVAO)
{
  wrappedCmdList->D3DValue.CommandList->SetVertexBuffers(0, wrappedVAO->D3DValue.getVertexBufferView().data(), (UINT)wrappedVAO->D3DValue.getVertexBufferView().size());
  wrappedCmdList->D3DValue.CommandList->SetIndexBuffer(&wrappedVAO->D3DValue.getIndexBufferView());
}

WrapperResource *D3DAPI::createConstantsBuffer(size_t sizeInByte)
{
  sizeInByte = sizeInByte < 256 ? 256 : sizeInByte;
  WrapperResource *result = (WrapperResource*)malloc(sizeof(WrapperResource));
  HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Buffer(sizeInByte),
    D3D12_RESOURCE_USAGE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&result->D3DValue.resource));
  D3D12_CONSTANT_BUFFER_VIEW_DESC bufdesc = {};
  bufdesc.BufferLocation = result->D3DValue.resource->GetGPUVirtualAddress();
  bufdesc.SizeInBytes = (UINT)sizeInByte;
  result->D3DValue.description.CBV = bufdesc;
  return result;
}

void *D3DAPI::mapConstantsBuffer(struct WrapperResource *wrappedConstantBuffer)
{
  void *ptr;
  wrappedConstantBuffer->D3DValue.resource->Map(0, nullptr, &ptr);
  return ptr;
}

void D3DAPI::unmapConstantsBuffers(struct WrapperResource *wrappedConstantsBuffer)
{
  wrappedConstantsBuffer->D3DValue.resource->Unmap(0, nullptr);
}

WrapperCommandList* D3DAPI::createCommandList()
{
  WrapperCommandList *result = (WrapperCommandList*)malloc(sizeof(WrapperCommandList));
  HRESULT hr = Context::getInstance()->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&result->D3DValue.CommandAllocator));
  hr = Context::getInstance()->dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, result->D3DValue.CommandAllocator, nullptr, IID_PPV_ARGS(&result->D3DValue.CommandList));
  result->D3DValue.CommandList->Close();
  return result;
}

void D3DAPI::openCommandList(struct WrapperCommandList* wrappedCmdList)
{
  wrappedCmdList->D3DValue.CommandAllocator->Reset();
  wrappedCmdList->D3DValue.CommandList->Reset(wrappedCmdList->D3DValue.CommandAllocator, nullptr);
}

void D3DAPI::closeCommandList(struct WrapperCommandList *wrappedCmdList)
{
  wrappedCmdList->D3DValue.CommandList->Close();
}

void D3DAPI::drawIndexedInstanced(struct WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t indexOffset, size_t vertexOffset, size_t instanceOffset)
{
  wrappedCmdList->D3DValue.CommandList->DrawIndexedInstanced((UINT)indexCount, (UINT)instanceCount, (UINT)indexOffset, (UINT)vertexOffset, (UINT)instanceOffset);
}

void D3DAPI::drawInstanced(struct WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t vertexOffset, size_t instanceOffset)
{
  wrappedCmdList->D3DValue.CommandList->DrawInstanced((UINT)indexCount, (UINT)instanceCount, (UINT)vertexOffset, (UINT)instanceOffset);
}

void D3DAPI::submitToQueue(struct WrapperCommandList *wrappedCmdList)
{
  Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)&wrappedCmdList->D3DValue.CommandList);
}

// TODO: Improve this mess
static Microsoft::WRL::ComPtr<ID3D12Resource> ScreenQuad;
static D3D12_VERTEX_BUFFER_VIEW ScreenQuadView;

void D3DAPI::fullscreenSetVertexBufferAndDraw(struct WrapperCommandList *wrappedCmdList)
{
  if (ScreenQuad.Get() == nullptr)
  {
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
    hr = Context::getInstance()->dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), nullptr, IID_PPV_ARGS(&cmdlist));

    cmdlist->CopyBufferRegion(ScreenQuad.Get(), 0, ScreenQuadCPU.Get(), 0, 3 * 4 * sizeof(float), D3D12_COPY_NONE);
    cmdlist->Close();
    Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());
    // TODO: wait

    ScreenQuadView.BufferLocation = ScreenQuad->GetGPUVirtualAddress();
    ScreenQuadView.StrideInBytes = 4 * sizeof(float);
    ScreenQuadView.SizeInBytes = 3 * 4 * sizeof(float);
  }

  wrappedCmdList->D3DValue.CommandList->SetVertexBuffers(0, &ScreenQuadView, (UINT)1);
  wrappedCmdList->D3DValue.CommandList->DrawInstanced(3, 1, 0, 0);
}