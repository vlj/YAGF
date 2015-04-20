// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <D3DAPI/Misc.h>
#include <vector>
#include <D3DAPI/Context.h>

class D3DRTTSet
{
private:
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DepthDescriptorHeap;
  size_t width, height;
  size_t NumRTT;
  bool hasDepthStencil;
public:
  D3DRTTSet() {}

  D3DRTTSet(const std::vector<ID3D12Resource*> &RTTs, const std::vector<DXGI_FORMAT> &format, size_t w, size_t h, ID3D12Resource *DepthStencil, D3D12_DEPTH_STENCIL_VIEW_DESC *DSVDescription)
    : width(w), height(h), NumRTT(RTTs.size()), hasDepthStencil(DepthStencil != nullptr)
  {
    DescriptorHeap = createDescriptorHeap(Context::getInstance()->dev.Get(), RTTs.size(), D3D12_RTV_DESCRIPTOR_HEAP, false);
    for (unsigned i = 0; i < RTTs.size(); i++)
    {
      D3D12_RENDER_TARGET_VIEW_DESC rttvd = {};
      rttvd.Format = format[i];
      rttvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
      rttvd.Texture2D.MipSlice = 0;
      rttvd.Texture2D.PlaneSlice = 0;
      Context::getInstance()->dev->CreateRenderTargetView(RTTs[i], &rttvd, DescriptorHeap->GetCPUDescriptorHandleForHeapStart().MakeOffsetted(i * Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_RTV_DESCRIPTOR_HEAP)));
    }
    if (DepthStencil)
    {
      DepthDescriptorHeap = createDescriptorHeap(Context::getInstance()->dev.Get(), 1, D3D12_DSV_DESCRIPTOR_HEAP, false);

      Context::getInstance()->dev->CreateDepthStencilView(DepthStencil, DSVDescription, DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    }
  }

  ~D3DRTTSet()
  {

  }

  void Bind(ID3D12GraphicsCommandList *cmdlist)
  {
    D3D12_RECT rect = {};
    rect.left = 0;
    rect.top = 0;
    rect.bottom = (LONG)width;
    rect.right = (LONG)height;

    D3D12_VIEWPORT view = {};
    view.Height = (FLOAT)width;
    view.Width = (FLOAT)height;
    view.TopLeftX = 0;
    view.TopLeftY = 0;
    view.MinDepth = 0;
    view.MaxDepth = 1.;

    cmdlist->RSSetViewports(1, &view);
    cmdlist->RSSetScissorRects(1, &rect);

    if (hasDepthStencil)
      cmdlist->SetRenderTargets(&DescriptorHeap->GetCPUDescriptorHandleForHeapStart(), true, (UINT)NumRTT, &DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    else
      cmdlist->SetRenderTargets(&DescriptorHeap->GetCPUDescriptorHandleForHeapStart(), true, (UINT)NumRTT, nullptr);
  }

  void Clear(ID3D12GraphicsCommandList *cmdlist, float clearColor[4])
  {
    UINT Increment = Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_RTV_DESCRIPTOR_HEAP);
    for (unsigned i = 0; i < NumRTT; i++)
    {
      cmdlist->ClearRenderTargetView(DescriptorHeap->GetCPUDescriptorHandleForHeapStart().MakeOffsetted(i * Increment), clearColor, 0, 0);
    }
  }

  void ClearDepthStencil(ID3D12GraphicsCommandList *cmdlist, float Depth, unsigned Stencil)
  {
    cmdlist->ClearDepthStencilView(DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_DEPTH, Depth, Stencil, nullptr, 0);
  }

  void Bind(ID3D12GraphicsCommandList *cmdlist, D3D12_CPU_DESCRIPTOR_HANDLE DepthDescriptor)
  {
    D3D12_RECT rect = {};
    rect.left = 0;
    rect.top = 0;
    rect.bottom = (LONG)width;
    rect.right = (LONG)height;

    D3D12_VIEWPORT view = {};
    view.Height = (FLOAT)width;
    view.Width = (FLOAT)height;
    view.TopLeftX = 0;
    view.TopLeftY = 0;
    view.MinDepth = 0;
    view.MaxDepth = 1.;

    cmdlist->RSSetViewports(1, &view);
    cmdlist->RSSetScissorRects(1, &rect);

    if (NumRTT > 0)
      cmdlist->SetRenderTargets(&DescriptorHeap->GetCPUDescriptorHandleForHeapStart(), true, (UINT)NumRTT, &DepthDescriptor);
    else
      cmdlist->SetRenderTargets(nullptr, false, 0, &DepthDescriptor);
  }

  void BindLayer(unsigned i)
  {

  }


  size_t getWidth() const
  {
    return width;
  }

  size_t getHeight() const
  {
    return height;
  }
};
