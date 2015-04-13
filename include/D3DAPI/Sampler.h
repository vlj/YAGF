// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#pragma once

#include <d3d12.h>

struct Samplers
{
  static D3D12_SAMPLER_DESC getTrilinearSamplerDesc()
  {
    D3D12_SAMPLER_DESC samplerdesc = {};
    samplerdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerdesc.AddressU = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.AddressV = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.AddressW = D3D12_TEXTURE_ADDRESS_WRAP;
    samplerdesc.MaxAnisotropy = 1;
    samplerdesc.MinLOD = 0;
    samplerdesc.MaxLOD = 1000;
    return samplerdesc;
  }
};