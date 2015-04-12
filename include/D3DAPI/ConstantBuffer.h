// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <d3d12.h>
#include <D3DAPI/Context.h>
#include <wrl/client.h>

template<typename T>
class ConstantBuffer
{
private:
  Microsoft::WRL::ComPtr<ID3D12Resource> cbuffer;
  size_t size;
public:
  ConstantBuffer() : size(sizeof(T) > 256 ? sizeof(T) : 256)
  {
    HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer( size ),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&cbuffer));
  }

  T* map()
  {
    T* pointer;
    cbuffer->Map(0, nullptr, (void**)&pointer);
    return pointer;
  }

  void unmap()
  {
    cbuffer->Unmap(0, nullptr);
  }

  D3D12_CONSTANT_BUFFER_VIEW_DESC getDesc() const
  {
    D3D12_CONSTANT_BUFFER_VIEW_DESC bufdesc = {};
    bufdesc.BufferLocation = cbuffer->GetGPUVirtualAddress();
    bufdesc.SizeInBytes = (UINT)size;
    return bufdesc;
  }
};