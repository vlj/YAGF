// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#pragma once

#include <GfxApi.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <D3DAPI/D3DRTTSet.h>
#include <D3DAPI/VAO.h>
#include <Core/BasicVertexLayout.h>

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

class WrapperD3DResource : public WrapperResource
{
public:
  Microsoft::WRL::ComPtr<ID3D12Resource> Texture;
  virtual void nothing() override
  {}
  Microsoft::WRL::ComPtr<ID3D12Resource> &operator()(void)
  {
    return Texture;
  }
};

class WrapperD3DCommandList : public WrapperCommandList
{
public:
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
  virtual void nothing() override
  {}
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &operator()(void)
  {
    return CommandList;
  }
};

class WrapperD3DIndexVertexBuffersSet : public WrapperIndexVertexBuffersSet
{
public:
  virtual void nothing() override
  {}
  FormattedVertexStorage<irr::video::S3DVertex2TCoords> *vao;
  FormattedVertexStorage<irr::video::S3DVertex2TCoords> &operator()(void)
  {
    return *vao;
  }
};

template <typename T>
struct TypeWrapTrait;

template <>
struct TypeWrapTrait<WrapperResource>
{
  typedef WrapperD3DResource D3DWrappingType;
};

template <>
struct TypeWrapTrait<WrapperRTTSet>
{
  typedef WrapperD3DRTTSet D3DWrappingType;
};

template<>
struct TypeWrapTrait<WrapperCommandList>
{
  typedef WrapperD3DCommandList D3DWrappingType;
};

template<>
struct TypeWrapTrait<WrapperIndexVertexBuffersSet>
{
  typedef WrapperD3DIndexVertexBuffersSet D3DWrappingType;
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
  virtual WrapperResource* createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4]) override;
  virtual WrapperRTTSet* createRTTSet(const std::vector<WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height) override;
  virtual void clearRTTSet(WrapperCommandList* wrappedCmdList, WrapperRTTSet*, float color[4]) override;
  virtual void setRTTSet(WrapperCommandList* wrappedCmdList, WrapperRTTSet*) override;
  virtual WrapperResource *createConstantsBuffer(size_t) override;
  virtual void setIndexVertexBuffersSet(WrapperCommandList* wrappedCmdList, WrapperIndexVertexBuffersSet*) override;
  virtual void writeResourcesTransitionBarrier(WrapperCommandList* wrappedCmdList, const std::vector<std::tuple<WrapperResource *, enum RESOURCE_USAGE, enum RESOURCE_USAGE> > &barriers) override;
  virtual WrapperCommandList* createCommandList() override;
  virtual void closeCommandList(WrapperCommandList* wrappedCmdList) override;
  virtual void drawIndexedInstanced(WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t indexOffset, size_t vertexOffset, size_t instanceOffset) override;
};