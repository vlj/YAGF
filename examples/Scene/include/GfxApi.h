// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#ifndef __GFX_API_H__
#define __GFX_API_H__

#include <Core/SColor.h>
#include <memory>
#include <vector>
#include <tuple>

class WrapperRTTSet
{
public:
  virtual void nothing() = 0;
};

class WrapperCommandList
{
public:
  virtual void nothing() = 0;
};

class WrapperResource
{
public:
  virtual void nothing() = 0;
};

class WrapperIndexVertexBuffersSet
{
public:
  virtual void nothing() = 0;
};

class GFXAPI
{
public:
  enum RESOURCE_USAGE
  {
    PRESENT,
    COPY_DEST,
    COPY_SRC,
    RENDER_TARGET,
  };
  virtual std::shared_ptr<WrapperResource> createRTT(irr::video::ECOLOR_FORMAT, size_t Width, size_t Height, float fastColor[4]) = 0;
  virtual std::shared_ptr<WrapperRTTSet> createRTTSet(const std::vector<WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height) = 0;
  virtual void clearRTTSet(WrapperCommandList* wrappedCmdList, WrapperRTTSet*, float color[4]) = 0;
  virtual void setRTTSet(WrapperCommandList* wrappedCmdList, WrapperRTTSet*) = 0;
  virtual void setIndexVertexBuffersSet(WrapperCommandList* wrappedCmdList, WrapperIndexVertexBuffersSet*) = 0;
  virtual WrapperResource *createConstantsBuffer(size_t) = 0;
  virtual void writeResourcesTransitionBarrier(WrapperCommandList* wrappedCmdList, const std::vector<std::tuple<WrapperResource *, enum RESOURCE_USAGE, enum RESOURCE_USAGE> > &) = 0;
  virtual std::shared_ptr<WrapperCommandList> createCommandList() = 0;
  virtual void closeCommandList(WrapperCommandList* wrappedCmdList) = 0;
  virtual void drawIndexedInstanced(WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t indexOffset, size_t vertexOffset, size_t instanceOffset) = 0;
};

//Global
extern GFXAPI* GlobalGFXAPI;

#endif