// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <d3dapi.h>
#include <D3DAPI/Texture.h>
#include <D3DAPI/Resource.h>

WrapperResource* D3DAPI::createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4])
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

  return result;
}

union WrapperRTTSet* D3DAPI::createRTTSet(const std::vector<WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height)
{
  WrapperRTTSet *result = (WrapperRTTSet*) malloc(sizeof(WrapperRTTSet));
  std::vector<ID3D12Resource *> resources;
  std::vector<DXGI_FORMAT> dxgi_formats;
  for (unsigned i = 0; i < RTTs.size(); i++) {
    resources.push_back(RTTs[i]->D3DValue.resource);
    dxgi_formats.push_back(getDXGIFormatFromColorFormat(formats[i]));
  }
  new(result) D3DRTTSet(resources, dxgi_formats, Width, Height);

  return result;
}

static D3D12_RESOURCE_USAGE convertResourceUsage(enum GFXAPI::RESOURCE_USAGE ru)
{
  switch (ru)
  {
  case GFXAPI::COPY_DEST:
    return D3D12_RESOURCE_USAGE_COPY_DEST;
  case GFXAPI::COPY_SRC:
    return D3D12_RESOURCE_USAGE_COPY_SOURCE;
  case GFXAPI::PRESENT:
    return D3D12_RESOURCE_USAGE_PRESENT;
  case GFXAPI::RENDER_TARGET:
    return D3D12_RESOURCE_USAGE_RENDER_TARGET;
  }
}

void D3DAPI::writeResourcesTransitionBarrier(union WrapperCommandList* wrappedCmdList, const std::vector<std::tuple<WrapperResource *, enum RESOURCE_USAGE, enum RESOURCE_USAGE> > &barriers)
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

void D3DAPI::clearRTTSet(union WrapperCommandList* wrappedCmdList, union WrapperRTTSet* RTTSet, float color[4])
{
  RTTSet->D3DValue.Clear(wrappedCmdList->D3DValue.CommandList, color);
}

void D3DAPI::setRTTSet(union WrapperCommandList* wrappedCmdList, union WrapperRTTSet*RTTSet)
{
  RTTSet->D3DValue.Bind(wrappedCmdList->D3DValue.CommandList);
}

void D3DAPI::setPipelineState(union WrapperCommandList* wrappedCmdList, union WrapperPipelineState* wrappedPipelineState)
{
  wrappedCmdList->D3DValue.CommandList->SetPipelineState(wrappedPipelineState->D3DValue.pipelineStateObject);
  wrappedCmdList->D3DValue.CommandList->SetGraphicsRootSignature(wrappedPipelineState->D3DValue.rootSignature);
}

void D3DAPI::setIndexVertexBuffersSet(union WrapperCommandList* wrappedCmdList, WrapperIndexVertexBuffersSet* wrappedVAO)
{
  wrappedCmdList->D3DValue.CommandList->SetVertexBuffers(0, wrappedVAO->D3DValue.getVertexBufferView().data(), wrappedVAO->D3DValue.getVertexBufferView().size());
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
  result->D3DValue.description = bufdesc;
  return result;
}

void *D3DAPI::mapConstantsBuffer(union WrapperResource *wrappedConstantBuffer)
{
  void *ptr;
  wrappedConstantBuffer->D3DValue.resource->Map(0, nullptr, &ptr);
  return ptr;
}

void D3DAPI::unmapConstantsBuffers(union WrapperResource *wrappedConstantsBuffer)
{
  wrappedConstantsBuffer->D3DValue.resource->Unmap(0, nullptr);
}

WrapperCommandList* D3DAPI::createCommandList()
{
  WrapperCommandList *result = (WrapperCommandList*)malloc(sizeof(WrapperCommandList));
  HRESULT hr = Context::getInstance()->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&result->D3DValue.CommandAllocator));
  hr = Context::getInstance()->dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, result->D3DValue.CommandAllocator, nullptr, IID_PPV_ARGS(&result->D3DValue.CommandList));

  return result;
}

void D3DAPI::closeCommandList(union WrapperCommandList *wrappedCmdList)
{
  wrappedCmdList->D3DValue.CommandList->Close();
}

void D3DAPI::drawIndexedInstanced(union WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t indexOffset, size_t vertexOffset, size_t instanceOffset)
{
  wrappedCmdList->D3DValue.CommandList->DrawIndexedInstanced(indexCount, instanceCount, indexOffset, vertexOffset, instanceOffset);
}