// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <Core/Singleton.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <D3DAPI/Context.h>
#include <D3DAPI/D3DS3DVertex.h>

template<typename T, typename VTXLayout>
class PipelineStateObject : public Singleton<T>
{
public:
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;

  PipelineStateObject(const wchar_t *VertexShaderCSO, const wchar_t *PixelShaderCSO)
  {
    Microsoft::WRL::ComPtr<ID3DBlob> vtxshaderblob, pxshaderblob;
    HRESULT hr;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
    if (VertexShaderCSO)
    {
      hr = D3DReadFileToBlob(VertexShaderCSO, &vtxshaderblob);
      psodesc.VS.BytecodeLength = vtxshaderblob->GetBufferSize();
      psodesc.VS.pShaderBytecode = vtxshaderblob->GetBufferPointer();
    }
    if (PixelShaderCSO)
    {
      hr = D3DReadFileToBlob(PixelShaderCSO, &pxshaderblob);
      psodesc.PS.BytecodeLength = pxshaderblob->GetBufferSize();
      psodesc.PS.pShaderBytecode = pxshaderblob->GetBufferPointer();
    }
    psodesc.InputLayout.pInputElementDescs = VTXLayout::getInputAssemblyLayout();
    psodesc.InputLayout.NumElements = (UINT)VTXLayout::getInputAssemblySize();
    psodesc.SampleDesc.Count = 1;
    psodesc.SampleMask = UINT_MAX;
    psodesc.NodeMask = 1;

    T::SetRasterizerAndBlendStates(psodesc);

    hr = Context::getInstance()->dev->CreateGraphicsPipelineState(&psodesc, IID_PPV_ARGS(&pso));
  }
};