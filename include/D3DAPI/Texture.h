// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <D3DAPI/Context.h>
#include <d3d12.h>
#include <vector>
#include <wrl/client.h>
#include <Core/IImage.h>

class Texture
{
private:
  size_t Width, Height;
  size_t RowPitch;
  Microsoft::WRL::ComPtr<ID3D12Resource> texinram;
  std::vector<MipMapLevel> Mips;
public:
  Texture(const IImage& image) : Width(image.getWidth()), Height(image.getHeight()), RowPitch(image.getWidth() * 4), Mips(image.Mips)
  {
    HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(4 * sizeof(char) * Width * Height * 2),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&texinram)
      );

    void* pointer;
    hr = texinram->Map(0, nullptr, &pointer);


    if (!irr::video::isCompressed(image.getFormat()))
    {
        for (unsigned i = 0; i < Mips.size(); i++)
        {
            MipMapLevel miplevel = Mips[i];
            memcpy(((char*)pointer) + miplevel.Offset, ((char*)image.getPointer()) + miplevel.Offset, miplevel.Size);
        }
    }
    else
    {
    }
    texinram->Unmap(0, nullptr);
  }

  ~Texture()
  {

  }

  size_t getWidth() const
  {
    return Width;
  }

  size_t getHeight() const
  {
    return Height;
  }

  void CreateUploadCommandToResourceInDefaultHeap(ID3D12GraphicsCommandList *cmdlist, ID3D12Resource *DestResource, UINT subresource) const
  {
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
    dst.pResource = DestResource;
    dst.Subresource = subresource;

    D3D12_RESOURCE_BARRIER_DESC barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = DestResource;
    barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_GENERIC_READ;
    barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_COPY_DEST;
    cmdlist->ResourceBarrier(1, &barrier);

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.Type = D3D12_SUBRESOURCE_VIEW_PLACED_PITCHED_SUBRESOURCE;
    src.pResource = texinram.Get();
    src.PlacedTexture.Offset = 0;
    src.PlacedTexture.Placement.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    src.PlacedTexture.Placement.Width = (UINT)Width;
    src.PlacedTexture.Placement.Height = (UINT)Height;
    src.PlacedTexture.Placement.Depth = 1;
    src.PlacedTexture.Placement.RowPitch = (UINT)RowPitch;
    cmdlist->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr, D3D12_COPY_NONE);

    barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_GENERIC_READ;
    cmdlist->ResourceBarrier(1, &barrier);
  }

  D3D12_SHADER_RESOURCE_VIEW_DESC getResourceViewDesc() const
  {
    D3D12_SHADER_RESOURCE_VIEW_DESC resdesc = {};
    resdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    resdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    resdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    resdesc.Texture2D.MipLevels = Mips.size();
    return resdesc;
  }
};