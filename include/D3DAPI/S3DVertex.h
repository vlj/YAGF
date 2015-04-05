// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <Core/BasicVertexLayout.h>
#include <d3d12.h>

struct ScreenQuadVertex
{
};

template<typename T>
struct VertexLayout
{
  static D3D12_INPUT_ELEMENT_DESC* getInputAssemblyLayout();
  static size_t getInputAssemblySize();
};

template<>
struct VertexLayout<irr::video::S3DVertex2TCoords>
{
  static D3D12_INPUT_ELEMENT_DESC IAdesc[];

  static D3D12_INPUT_ELEMENT_DESC* getInputAssemblyLayout()
  {
    return IAdesc;
  }

  static size_t getInputAssemblySize()
  {
    return 5;
  }
};


D3D12_INPUT_ELEMENT_DESC VertexLayout<irr::video::S3DVertex2TCoords>::IAdesc[] =
{
  { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_PER_VERTEX_DATA, 0 },
  { "NORMAL", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_PER_VERTEX_DATA, 0 },
  { "COLOR", 2, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D12_INPUT_PER_VERTEX_DATA, 0 },
  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_PER_VERTEX_DATA, 0 },
  { "SECONDTEXCOORD", 4, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_PER_VERTEX_DATA, 0 },
};


template<>
struct VertexLayout<ScreenQuadVertex>
{
  static D3D12_INPUT_ELEMENT_DESC IAdesc[];

  static D3D12_INPUT_ELEMENT_DESC* getInputAssemblyLayout()
  {
    return IAdesc;
  }

  static size_t getInputAssemblySize()
  {
    return 2;
  }
};

D3D12_INPUT_ELEMENT_DESC VertexLayout<ScreenQuadVertex>::IAdesc[] =
{
  { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_PER_VERTEX_DATA, 0 },
  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_PER_VERTEX_DATA, 0 },
};