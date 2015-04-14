// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <d3dapi.h>
#include <D3DAPI/Texture.h>
#include <D3DAPI/Resource.h>

std::shared_ptr<WrapperResource> D3DAPI::createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4])
{
  DXGI_FORMAT Fmt = getDXGIFormatFromColorFormat(Format);
  Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
  HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Tex2D(Fmt, (UINT)Width, (UINT)Height, 1, 0, 1, 0, D3D12_RESOURCE_MISC_ALLOW_RENDER_TARGET),
    D3D12_RESOURCE_USAGE_RENDER_TARGET,
    &CD3D12_CLEAR_VALUE(Fmt, fastColor),
    IID_PPV_ARGS(&Resource));

  WrapperD3DResource *result = new WrapperD3DResource();
  result->Texture = Resource;

  std::shared_ptr<WrapperResource> wrappedresult(result);
  return wrappedresult;
}

std::shared_ptr<WrapperRTTSet> D3DAPI::createRTTSet(const std::vector<WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height)
{
  WrapperD3DRTTSet *result = new WrapperD3DRTTSet();
  std::vector<ID3D12Resource *> resources;
  std::vector<DXGI_FORMAT> dxgi_formats;
  for (unsigned i = 0; i < RTTs.size(); i++) {
    resources.push_back(unwrap(RTTs[i]).Get());
    dxgi_formats.push_back(getDXGIFormatFromColorFormat(formats[i]));
  }
  result->RttSet = D3DRTTSet(resources, dxgi_formats, Width, Height);
  std::shared_ptr<WrapperRTTSet> wrappedresult(result);
  return wrappedresult;
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

void D3DAPI::writeResourcesTransitionBarrier(WrapperCommandList* wrappedCmdList, const std::vector<std::tuple<WrapperResource *, enum RESOURCE_USAGE, enum RESOURCE_USAGE> > &barriers)
{
  std::vector<D3D12_RESOURCE_BARRIER_DESC> barriersDesc;
  ID3D12GraphicsCommandList *CmdList = unwrap(wrappedCmdList).Get();
  for (auto barrier : barriers)
  {
    ID3D12Resource* unwrappedResource = unwrap(std::get<0>(barrier)).Get();
    barriersDesc.push_back(setResourceTransitionBarrier(unwrappedResource, convertResourceUsage(std::get<1>(barrier)), convertResourceUsage(std::get<2>(barrier))));
  }
  CmdList->ResourceBarrier((UINT)barriersDesc.size(), barriersDesc.data());
}

std::shared_ptr<WrapperCommandList> D3DAPI::createCommandList()
{
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdalloc;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdlist;
  HRESULT hr = Context::getInstance()->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));
  hr = Context::getInstance()->dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), nullptr, IID_PPV_ARGS(&cmdlist));
  WrapperD3DCommandList *wrappedResult = new WrapperD3DCommandList();
  wrappedResult->CommandAllocator = cmdalloc;
  wrappedResult->CommandList = cmdlist;
  std::shared_ptr<WrapperCommandList> result(wrappedResult);
  return result;
}