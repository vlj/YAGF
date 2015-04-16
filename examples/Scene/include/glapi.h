// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __GLAPI_H__
#define __GLAPI_H__

#include <GfxApi.h>

class GLAPI : public GFXAPI
{
public:
  virtual WrapperResource* createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4]) override;
  virtual WrapperRTTSet* createRTTSet(const std::vector<union WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height) override;
  virtual void clearRTTSet(union WrapperCommandList* wrappedCmdList, WrapperRTTSet*, float color[4]) override;
  virtual void setRTTSet(union WrapperCommandList* wrappedCmdList, WrapperRTTSet*) override;
  virtual union WrapperResource *createConstantsBuffer(size_t) override;
  virtual void setIndexVertexBuffersSet(union WrapperCommandList* wrappedCmdList, WrapperIndexVertexBuffersSet*) override;
  virtual void writeResourcesTransitionBarrier(union WrapperCommandList* wrappedCmdList, const std::vector<std::tuple<union WrapperResource *, enum RESOURCE_USAGE, enum RESOURCE_USAGE> > &barriers) override;
  virtual union WrapperCommandList* createCommandList() override;
  virtual void closeCommandList(union WrapperCommandList* wrappedCmdList) override;
  virtual void drawIndexedInstanced(union WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t indexOffset, size_t vertexOffset, size_t instanceOffset) override;
};

#endif