// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt


#pragma once

#include <Core/Singleton.h>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <cassert>

class Context : public Singleton<Context>
{
private:
  bool Initialised;
  Microsoft::WRL::ComPtr<IDXGISwapChain3> chain;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> BackBufferDescriptorsHeap;
public:
  Microsoft::WRL::ComPtr<ID3D12Device> dev;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> cmdqueue;
  Microsoft::WRL::ComPtr<ID3D12Resource> buffer[2];
  D3D12_CPU_DESCRIPTOR_HANDLE BackBufferDescriptors[2];

  Context() : Initialised(false)
  {}

  void InitD3D(HWND hWnd)
  {
    assert(!Initialised);
    HRESULT hr = D3D12CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, D3D12_CREATE_DEVICE_DEBUG, D3D_FEATURE_LEVEL_11_1, D3D12_SDK_VERSION, IID_PPV_ARGS(&dev));

    D3D12_COMMAND_QUEUE_DESC cmddesc = {};
    cmddesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = dev->CreateCommandQueue(&cmddesc, IID_PPV_ARGS(&cmdqueue));

    DXGI_SWAP_CHAIN_DESC swapChain = {};
    swapChain.BufferCount = 2;
    swapChain.Windowed = true;
    swapChain.OutputWindow = hWnd;
    swapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChain.SampleDesc.Count = 1;
    swapChain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    Microsoft::WRL::ComPtr<IDXGIFactory> fact;
    hr = CreateDXGIFactory(IID_PPV_ARGS(&fact));
    hr = fact->CreateSwapChain(cmdqueue.Get(), &swapChain, (IDXGISwapChain**)chain.GetAddressOf());

    D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
    heapdesc.NumDescriptors = 2;
    heapdesc.Type = D3D12_RTV_DESCRIPTOR_HEAP;
    hr = dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&BackBufferDescriptorsHeap));

    BackBufferDescriptors[0] = BackBufferDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
    BackBufferDescriptors[1] = BackBufferDescriptorsHeap->GetCPUDescriptorHandleForHeapStart().MakeOffsetted(dev->GetDescriptorHandleIncrementSize(D3D12_RTV_DESCRIPTOR_HEAP));
    hr = chain->GetBuffer(0, IID_PPV_ARGS(&buffer[0]));

    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Texture2D.MipSlice = 0;
    rtv_desc.Texture2D.PlaneSlice = 0;

    dev->CreateRenderTargetView(buffer[0].Get(), &rtv_desc, BackBufferDescriptors[0]);
    hr = chain->GetBuffer(1, IID_PPV_ARGS(&buffer[1]));
    dev->CreateRenderTargetView(buffer[1].Get(), &rtv_desc, BackBufferDescriptors[1]);
    buffer[0]->SetName(L"BackBuffer0");
    buffer[1]->SetName(L"BackBuffer1");

    Initialised = true;
    {
      Microsoft::WRL::ComPtr<ID3D12Debug> pDebug;
      HRESULT hr = dev.Get()->QueryInterface(IID_PPV_ARGS(&pDebug));
      pDebug->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
    }
  }

  ID3D12Resource *getCurrentBackBuffer() const
  {
    assert(Initialised);
    return buffer[chain->GetCurrentBackBufferIndex()].Get();
  }

  D3D12_CPU_DESCRIPTOR_HANDLE getCurrentBackBufferDescriptor() const
  {
    assert(Initialised);
    return BackBufferDescriptors[chain->GetCurrentBackBufferIndex()];
  }

  void Swap()
  {
    assert(Initialised);
    HRESULT hr = chain->Present(1, 0);
  }
};