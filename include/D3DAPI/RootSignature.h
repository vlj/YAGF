// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#pragma once

#include <d3d12.h>
#include <vector>
#include <wrl/client.h>
#include <Core/Singleton.h>

template <size_t BindingPoint>
struct ConstantsBufferResource
{
  static void Build(std::vector<D3D12_DESCRIPTOR_RANGE>& RP)
  {
    RP.push_back(D3D12_DESCRIPTOR_RANGE());
    RP.back().Init(D3D12_DESCRIPTOR_RANGE_CBV, 1, BindingPoint);
  }
};

template <size_t BindingPoint>
struct ShaderResource
{
  static void Build(std::vector<D3D12_DESCRIPTOR_RANGE>& RP)
  {
    RP.push_back(D3D12_DESCRIPTOR_RANGE());
    RP.back().Init(D3D12_DESCRIPTOR_RANGE_SRV, 1, BindingPoint);
  }
};

template <size_t BindingPoint>
struct SamplerResource
{
  static void Build(std::vector<D3D12_DESCRIPTOR_RANGE>& RP)
  {
    RP.push_back(D3D12_DESCRIPTOR_RANGE());
    RP.back().Init(D3D12_DESCRIPTOR_RANGE_SAMPLER, 1, BindingPoint);
  }
};

template<typename... Args>
struct DescriptorRangeBuilder;

template<>
struct DescriptorRangeBuilder<>
{
  static void build(std::vector<D3D12_DESCRIPTOR_RANGE>&)
  { }
};

template<typename T, typename... Args>
struct DescriptorRangeBuilder<T, Args...>
{
  static void build(std::vector<D3D12_DESCRIPTOR_RANGE>& DescriptorRangeWIP)
  {
    T::Build(DescriptorRangeWIP);
    DescriptorRangeBuilder<Args...>::build(DescriptorRangeWIP);
  }
};

template <typename...Args>
struct DescriptorTable
{
  static void buildTableRanges(std::vector<D3D12_DESCRIPTOR_RANGE>& lst)
  {
    DescriptorRangeBuilder<Args...>::build(lst);
  }
};

template<typename... Args>
struct DescriptorTableBuilder;

template<>
struct DescriptorTableBuilder<>
{
  static void build(std::vector<std::vector<D3D12_DESCRIPTOR_RANGE> >& lst)
  { }
};

template<typename T, typename... Args>
struct DescriptorTableBuilder<T, Args...>
{
  static void build(std::vector<std::vector<D3D12_DESCRIPTOR_RANGE> >& lst)
  {
    lst.push_back(std::vector<D3D12_DESCRIPTOR_RANGE>());
    T::buildTableRanges(lst.back());
    DescriptorTableBuilder<Args...>::build(lst);
  }
};

template<D3D12_ROOT_SIGNATURE_FLAGS flags, typename... T>
class RootSignature : public Singleton<RootSignature<flags, T...>>
{
private:
  std::vector<std::vector<D3D12_DESCRIPTOR_RANGE> > getDescriptorRanges() const
  {
    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE> > result;
    DescriptorTableBuilder<T...>::build(result);
    return result;
  }

public:
  Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature;

  RootSignature()
  {
    std::vector<D3D12_ROOT_PARAMETER> RootParameters;

    const std::vector<std::vector<D3D12_DESCRIPTOR_RANGE> >& descriptorRanges = getDescriptorRanges();
    for (const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges : descriptorRanges)
    {
      RootParameters.push_back(D3D12_ROOT_PARAMETER());
      RootParameters.back().InitAsDescriptorTable((UINT)ranges.size(), ranges.data());
    }

    D3D12_ROOT_SIGNATURE RootSig = D3D12_ROOT_SIGNATURE((UINT)RootParameters.size(), RootParameters.data());
    RootSig.Flags = flags;

    ComPtr<ID3DBlob> pSerializedRootSig;
    HRESULT hr = D3D12SerializeRootSignature(&RootSig, D3D_ROOT_SIGNATURE_V1, &pSerializedRootSig, nullptr);

    hr = Context::getInstance()->dev->CreateRootSignature(1,
      pSerializedRootSig->GetBufferPointer(), pSerializedRootSig->GetBufferSize(),
      IID_PPV_ARGS(&pRootSignature));
  }
};