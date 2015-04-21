// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <Core/Singleton.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <D3DAPI/Context.h>
#include <D3DAPI/D3DS3DVertex.h>

template<typename VTXLayout>
struct PipelineStateObject
{
  static ID3D12PipelineState *get(D3D12_GRAPHICS_PIPELINE_STATE_DESC prefilledPSODesc, const wchar_t *VertexShaderCSO, const wchar_t *PixelShaderCSO)
  {
    ID3D12PipelineState *pso;
    Microsoft::WRL::ComPtr<ID3DBlob> vtxshaderblob, pxshaderblob;
    HRESULT hr;
    if (VertexShaderCSO)
    {
      hr = D3DReadFileToBlob(VertexShaderCSO, &vtxshaderblob);
      prefilledPSODesc.VS.BytecodeLength = vtxshaderblob->GetBufferSize();
      prefilledPSODesc.VS.pShaderBytecode = vtxshaderblob->GetBufferPointer();
    }
    if (PixelShaderCSO)
    {
      hr = D3DReadFileToBlob(PixelShaderCSO, &pxshaderblob);
      prefilledPSODesc.PS.BytecodeLength = pxshaderblob->GetBufferSize();
      prefilledPSODesc.PS.pShaderBytecode = pxshaderblob->GetBufferPointer();
    }
    prefilledPSODesc.InputLayout.pInputElementDescs = VTXLayout::getInputAssemblyLayout();
    prefilledPSODesc.InputLayout.NumElements = (UINT)VTXLayout::getInputAssemblySize();
    prefilledPSODesc.SampleDesc.Count = 1;
    prefilledPSODesc.SampleMask = UINT_MAX;
    prefilledPSODesc.NodeMask = 1;

    hr = Context::getInstance()->dev->CreateGraphicsPipelineState(&prefilledPSODesc, IID_PPV_ARGS(&pso));
    return pso;
  }
};