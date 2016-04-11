// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <D3DAPI/Context.h>
#include <d3d12.h>
#include <vector>
#include <wrl/client.h>
#include <Core/IImage.h>

inline DXGI_FORMAT getDXGIFormatFromColorFormat(irr::video::ECOLOR_FORMAT fmt)
{
  switch (fmt)
  {
  case irr::video::ECF_R8G8B8A8_UNORM:
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  case irr::video::ECF_R8G8B8A8_UNORM_SRGB:
    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  case irr::video::ECF_R16G16B16A16F:
    return DXGI_FORMAT_R16G16B16A16_FLOAT;
  case irr::video::ECF_R32G32B32A32F:
    return DXGI_FORMAT_R32G32B32A32_FLOAT;
  case irr::video::ECF_A8R8G8B8:
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  case irr::video::ECF_BC1_UNORM:
    return DXGI_FORMAT_BC1_UNORM;
  case irr::video::ECF_BC1_UNORM_SRGB:
    return DXGI_FORMAT_BC1_UNORM_SRGB;
  case irr::video::ECF_BC2_UNORM:
    return DXGI_FORMAT_BC2_UNORM;
  case irr::video::ECF_BC2_UNORM_SRGB:
    return DXGI_FORMAT_BC2_UNORM_SRGB;
  case irr::video::ECF_BC3_UNORM:
    return DXGI_FORMAT_BC3_UNORM;
  case irr::video::ECF_BC3_UNORM_SRGB:
    return DXGI_FORMAT_BC3_UNORM_SRGB;
  case irr::video::ECF_BC4_UNORM:
    return DXGI_FORMAT_BC4_UNORM;
  case irr::video::ECF_BC4_SNORM:
    return DXGI_FORMAT_BC4_SNORM;
  case irr::video::ECF_BC5_UNORM:
    return DXGI_FORMAT_BC5_UNORM;
  case irr::video::ECF_BC5_SNORM:
    return DXGI_FORMAT_BC5_SNORM;
  default:
    return DXGI_FORMAT_UNKNOWN;
  }
}

struct MipLevelData
{
    size_t Offset;
    size_t Width;
    size_t Height;
    size_t RowPitch;
};

class D3DTexture
{
private:
  size_t Width, Height;
  size_t layerCount;
  size_t mipmapLevelCount;
  Microsoft::WRL::ComPtr<ID3D12Resource> texinram;
  std::vector<MipLevelData> Mips;
  irr::video::ECOLOR_FORMAT Format;
public:
  D3DTexture(const IImage& image) : Format(image.Format)
  {
    Width = image.Layers[0][0].Width;
    Height = image.Layers[0][0].Height;
    mipmapLevelCount = image.Layers[0].size();

    size_t block_width, block_height;
    size_t block_size;

    switch (Format)
    {
    case irr::video::ECF_R8G8B8A8_UNORM:
    case irr::video::ECF_R8G8B8A8_UNORM_SRGB:
      block_width = 1, block_height = 1;
      block_size = 4;
      break;
    case irr::video::ECF_R32G32B32A32F:
      block_width = 1, block_height = 1;
      block_size = 16;
      break;
    case irr::video::ECF_BC1_UNORM:
    case irr::video::ECF_BC1_UNORM_SRGB:
      block_width = 4, block_height = 4;
      block_size = 8;
      break;
    case irr::video::ECF_BC2_UNORM:
    case irr::video::ECF_BC2_UNORM_SRGB:
    case irr::video::ECF_BC3_UNORM:
    case irr::video::ECF_BC3_UNORM_SRGB:
    case irr::video::ECF_BC4_UNORM:
    case irr::video::ECF_BC4_SNORM:
    case irr::video::ECF_BC5_UNORM:
    case irr::video::ECF_BC5_SNORM:
      block_width = 4, block_height = 4;
      block_size = 16;
      break;
    }

    size_t bufferSize = ((Width + block_width - 1) / block_width) * ((Height + block_height - 1) / block_height) * block_size;

    HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Buffer(bufferSize * 6 * 3), // 3 to have some extra room despite mipmaps
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&texinram)
      );

    void* pointer;
    hr = texinram->Map(0, nullptr, &pointer);

    switch (image.Type)
    {
    case TextureType::TEXTURE2D:
      layerCount = 1;
      break;
    case TextureType::CUBEMAP:
      layerCount = 6;
      break;
    }

    size_t offset_in_texram = 0;
    for (unsigned face = 0; face < layerCount; face++)
    {
      for (unsigned i = 0; i < mipmapLevelCount; i++)
      {
        struct PackedMipMapLevel miplevel = image.Layers[face][i];
        // Offset needs to be aligned to 512 bytes
        offset_in_texram = (offset_in_texram + 511) & -0x200;
        // Row pitch is always a multiple of 256
        size_t height_in_blocks = (image.Layers[face][i].Height + block_height - 1) / block_height;
        size_t width_in_blocks = (image.Layers[face][i].Width + block_width - 1) / block_width;
        size_t height_in_texram = height_in_blocks * block_height;
        size_t width_in_texram = width_in_blocks * block_width;
        size_t rowPitch = max(width_in_blocks * block_size, 256);
        MipLevelData mml = { offset_in_texram, width_in_texram, height_in_texram, rowPitch };
        Mips.push_back(mml);
        for (unsigned row = 0; row < height_in_blocks; row++)
        {
          memcpy(((char*)pointer) + offset_in_texram, ((char*)miplevel.Data) + row * width_in_blocks * block_size, width_in_blocks * block_size);
          offset_in_texram += rowPitch;
        }
      }
    }
    texinram->Unmap(0, nullptr);
  }

  D3DTexture(const D3DTexture &)
  {

  }

  ~D3DTexture()
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

  DXGI_FORMAT getFormat() const
  {
    return getDXGIFormatFromColorFormat(Format);
  }

  size_t getMipLevelsCount() const
  {
    return mipmapLevelCount;
  }

  void CreateUploadCommandToResourceInDefaultHeap(ID3D12GraphicsCommandList *cmdlist, ID3D12Resource *DestResource) const
  {
    for (unsigned face = 0; face < layerCount; face++)
    {
      for (unsigned miplevel = 0; miplevel < mipmapLevelCount; miplevel++)
      {
        MipLevelData mipmapData = Mips[face * mipmapLevelCount + miplevel];
        D3D12_RESOURCE_BARRIER_DESC barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = DestResource;
        barrier.Transition.Subresource = (UINT)(face * mipmapLevelCount + miplevel);
        barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_GENERIC_READ;
        barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_COPY_DEST;
        cmdlist->ResourceBarrier(1, &barrier);

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
        dst.pResource = DestResource;
        dst.Subresource = (UINT)(face * mipmapLevelCount + miplevel);

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.Type = D3D12_SUBRESOURCE_VIEW_PLACED_PITCHED_SUBRESOURCE;
        src.pResource = texinram.Get();
        src.PlacedTexture.Offset = mipmapData.Offset;
        src.PlacedTexture.Placement.Format = getDXGIFormatFromColorFormat(Format);
        src.PlacedTexture.Placement.Width = (UINT)mipmapData.Width;
        src.PlacedTexture.Placement.Height = (UINT)mipmapData.Height;
        src.PlacedTexture.Placement.Depth = 1;
        src.PlacedTexture.Placement.RowPitch = (UINT)mipmapData.RowPitch;
        cmdlist->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr, D3D12_COPY_NONE);

        barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_GENERIC_READ;
        cmdlist->ResourceBarrier(1, &barrier);
      }
    }
  }

  D3D12_SHADER_RESOURCE_VIEW_DESC getResourceViewDesc() const
  {
    D3D12_SHADER_RESOURCE_VIEW_DESC resdesc = {};
    resdesc.Format = getDXGIFormatFromColorFormat(Format);
    resdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    resdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    resdesc.Texture2D.MipLevels = (UINT)Mips.size();
    return resdesc;
  }
};