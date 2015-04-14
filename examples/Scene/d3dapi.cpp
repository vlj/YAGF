// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <d3dapi.h>
#include <D3DAPI/Texture.h>

std::shared_ptr<WrapperRTT> D3DAPI::createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4])
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

  WrapperD3DRTT *result = new WrapperD3DRTT();
  result->Texture = Resource;
  result->Format = Fmt;

  std::shared_ptr<WrapperRTT> wrappedresult(result);
  return wrappedresult;
}

std::shared_ptr<WrapperRTTSet> D3DAPI::createRTTSet(std::vector<WrapperRTT*> RTTs, size_t Width, size_t Height)
{
  WrapperD3DRTTSet *result = new WrapperD3DRTTSet();
  std::vector<ID3D12Resource *> resources;
  std::vector<DXGI_FORMAT> formats;
  for (WrapperRTT* wrappedrtt : RTTs)
  {
    WrapperD3DRTT *unwrappedrtt = dynamic_cast<WrapperD3DRTT*>(wrappedrtt);
    resources.push_back(unwrappedrtt->Texture.Get());
    formats.push_back(unwrappedrtt->Format);
  }
  result->RttSet = D3DRTTSet(resources, formats, Width, Height);
  std::shared_ptr<WrapperRTTSet> wrappedresult(result);
  return wrappedresult;
}