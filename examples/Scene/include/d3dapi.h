// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#pragma once

#include <GfxApi.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <D3DAPI/D3DRTTSet.h>

// Wrapper around RTT
class WrapperD3DRTT : public WrapperRTT
{
public:
  virtual void nothing() override
  {}
  Microsoft::WRL::ComPtr<ID3D12Resource> Texture;
  DXGI_FORMAT Format;
};

// Wrapper around RTTSet
class WrapperD3DRTTSet : public WrapperRTTSet
{
public:
  virtual void nothing() override
  {}
  D3DRTTSet RttSet;
};

class D3DAPI : public GFXAPI
{
public:
  virtual std::shared_ptr<WrapperRTT> createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4]) override;
  virtual std::shared_ptr<WrapperRTTSet> createRTTSet(std::vector<WrapperRTT*> RTTs, size_t Width, size_t Height) override;
};