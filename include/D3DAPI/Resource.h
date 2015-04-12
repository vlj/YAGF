// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#pragma once
#include <d3d12.h>
#include <D3DAPI/Context.h>
#include <wrl/client.h>

inline D3D12_RESOURCE_BARRIER_DESC setResourceTransitionBarrier(ID3D12Resource *res, UINT before, UINT after)
{
  D3D12_RESOURCE_BARRIER_DESC barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = res;
  barrier.Transition.StateBefore = before;
  barrier.Transition.StateAfter = after;
  return barrier;
}

inline HANDLE getCPUSyncHandle(ID3D12CommandQueue *cmdqueue)
{
  Microsoft::WRL::ComPtr<ID3D12Fence> datauploadfence;
  HRESULT hr = Context::getInstance()->dev->CreateFence(0, D3D12_FENCE_MISC_NONE, IID_PPV_ARGS(&datauploadfence));
  HANDLE cpudatauploadevent = CreateEvent(0, FALSE, FALSE, 0);
  datauploadfence->SetEventOnCompletion(1, cpudatauploadevent);
  cmdqueue->Signal(datauploadfence.Get(), 1);
  return cpudatauploadevent;
}