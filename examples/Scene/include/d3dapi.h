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
  Microsoft::WRL::ComPtr<ID3D12Resource> Texture;
  virtual void nothing() override
  {}
  DXGI_FORMAT Format;
  Microsoft::WRL::ComPtr<ID3D12Resource> &operator()(void)
  {
    return Texture;
  }
};

// Wrapper around RTTSet
class WrapperD3DRTTSet : public WrapperRTTSet
{
public:
  D3DRTTSet RttSet;
  virtual void nothing() override
  {}
  D3DRTTSet &operator()(void)
  {
    return RttSet;
  }
};

template <typename T>
struct TypeWrapTrait;

template <>
struct TypeWrapTrait<WrapperRTT>
{
  typedef WrapperD3DRTT D3DWrappingType;
};

template <>
struct TypeWrapTrait<WrapperRTTSet>
{
  typedef WrapperD3DRTTSet D3DWrappingType;
};

template <typename T>
struct TypeUnwrap
{
  typedef typename decltype(TypeWrapTrait<T>::D3DWrappingType()()) Type;

  static Type& get(T *ptr)
  {
    return (*dynamic_cast<TypeWrapTrait<T>::D3DWrappingType *>(ptr))();
  }
};

template <typename T>
typename TypeUnwrap<T>::Type unwrap(T *ptr)
{
  return TypeUnwrap<T>::get(ptr);
}



class D3DAPI : public GFXAPI
{
public:
  virtual std::shared_ptr<WrapperRTT> createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4]) override;
  virtual std::shared_ptr<WrapperRTTSet> createRTTSet(std::vector<WrapperRTT*> RTTs, size_t Width, size_t Height) override;
};