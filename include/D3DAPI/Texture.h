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
  struct MipLevelData
  {
    size_t Offset;
    size_t Width;
    size_t Height;
    size_t RowPitch;
  };
  size_t Width, Height;
  size_t RowPitch;
  Microsoft::WRL::ComPtr<ID3D12Resource> texinram;
  std::vector<MipLevelData> Mips;
public:
  Texture(const IImage& image) : Width(image.getWidth()), Height(image.getHeight()), RowPitch(image.getWidth() * 4)
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


    size_t block_width = 4, block_height = 4;
    size_t block_size = 8;

    if (!irr::video::isCompressed(image.getFormat()))
    {
      block_width = 1, block_height = 1;
      block_size = 4;
    }
    else
    {
      // For BC1_UNORM
      block_width = 4, block_height = 4;
      block_size = 8;
    }

    size_t offset_in_texram = 0;
    for (unsigned i = 0; i < image.Mips.size(); i++)
    {
      MipMapLevel miplevel = image.Mips[i];
      // Offset needs to be aligned to 512 bytes
      offset_in_texram = (offset_in_texram + 511) & -0x200;
      // Row pitch is always a multiple of 256
      size_t height_in_blocks = (image.Mips[i].Height + block_height - 1) / block_height;
      size_t width_in_blocks = (image.Mips[i].Width + block_width - 1) / block_width;
      size_t height_in_texram = height_in_blocks * block_height;
      size_t width_in_texram = width_in_blocks * block_width;
      size_t rowPitch = max(width_in_blocks * block_size, 256);
      MipLevelData mml = { offset_in_texram, width_in_texram, height_in_texram, rowPitch };
      Mips.push_back(mml);
      for (unsigned row = 0; row < height_in_blocks; row++)
      {
        memcpy(((char*)pointer) + offset_in_texram, ((char*)image.getPointer()) + image.Mips[i].Offset + row * width_in_blocks * block_size, rowPitch);
        offset_in_texram += rowPitch;
      }
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

  void CreateUploadCommandToResourceInDefaultHeap(ID3D12GraphicsCommandList *cmdlist, ID3D12Resource *DestResource) const
  {
    for (unsigned i = 0; i < Mips.size(); i++)
    {
      MipLevelData miplevel = Mips[i];
      D3D12_RESOURCE_BARRIER_DESC barrier = {};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = DestResource;
      barrier.Transition.Subresource = i;
      barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_GENERIC_READ;
      barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_COPY_DEST;
      cmdlist->ResourceBarrier(1, &barrier);

      D3D12_TEXTURE_COPY_LOCATION dst = {};
      dst.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
      dst.pResource = DestResource;
      dst.Subresource = i;

      D3D12_TEXTURE_COPY_LOCATION src = {};
      src.Type = D3D12_SUBRESOURCE_VIEW_PLACED_PITCHED_SUBRESOURCE;
      src.pResource = texinram.Get();
      src.PlacedTexture.Offset = Mips[i].Offset;
      src.PlacedTexture.Placement.Format = DXGI_FORMAT_BC1_UNORM;
      src.PlacedTexture.Placement.Width = (UINT)miplevel.Width;
      src.PlacedTexture.Placement.Height = (UINT)miplevel.Height;
      src.PlacedTexture.Placement.Depth = 1;
      src.PlacedTexture.Placement.RowPitch = (UINT)miplevel.RowPitch;
      cmdlist->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr, D3D12_COPY_NONE);

      barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_COPY_DEST;
      barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_GENERIC_READ;
      cmdlist->ResourceBarrier(1, &barrier);
    }
  }

  D3D12_SHADER_RESOURCE_VIEW_DESC getResourceViewDesc() const
  {
    D3D12_SHADER_RESOURCE_VIEW_DESC resdesc = {};
    resdesc.Format = DXGI_FORMAT_BC1_UNORM;
    resdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    resdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    resdesc.Texture2D.MipLevels = Mips.size();
    return resdesc;
  }
};