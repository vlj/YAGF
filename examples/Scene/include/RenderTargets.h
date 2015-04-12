// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __RENDER_TARGETS_H__
#define __RENDER_TARGETS_H__

#ifdef DXBUILD
#include <d3d12.h>
#include <wrl/client.h>
#endif

class RenderTargets
{
private:
  size_t Width, Height;
#ifdef DXBUILD
  Microsoft::WRL::ComPtr<ID3D12Resource> DepthBuffer;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DepthDescriptorHeap;
#endif
public:
  RenderTargets(size_t w, size_t h): Width(w), Height(h)
  {
#ifdef DXBUILD
    HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, Width, Height, 1, 0, 1, 0, D3D12_RESOURCE_MISC_ALLOW_DEPTH_STENCIL),
      D3D12_RESOURCE_USAGE_DEPTH,
      &CD3D12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1., 0),
      IID_PPV_ARGS(&DepthBuffer));

    D3D12_DESCRIPTOR_HEAP_DESC Depthdesc = {};
    Depthdesc.Type = D3D12_DSV_DESCRIPTOR_HEAP;
    Depthdesc.NumDescriptors = 1;
    Depthdesc.Flags = D3D12_DESCRIPTOR_HEAP_NONE;
    hr = Context::getInstance()->dev->CreateDescriptorHeap(&Depthdesc, IID_PPV_ARGS(&DepthDescriptorHeap));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    Context::getInstance()->dev->CreateDepthStencilView(DepthBuffer.Get(), &dsv, DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
#endif
  }

#ifdef DXBUILD
  ID3D12DescriptorHeap* getDepthDescriptorHeap() const
  {
    return DepthDescriptorHeap.Get();
  }
#endif

  ~RenderTargets()
  {}


};

#endif