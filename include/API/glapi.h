// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __GLAPI_H__
#define __GLAPI_H__

#include <API/GfxApi.h>

struct WrapperResource
{
  GLuint GLValue;
};

struct WrapperRTTSet
{
  GLRTTSet GLValue;
};

struct WrapperPipelineState
{
  struct {
    GLuint Program;
    void(*StateSetter)(void);
  } GLValue;
};

struct WrapperIndexVertexBuffersSet
{
  struct {
    GLuint VAO;
    std::vector<GLuint> VertexBuffers;
  } GLValue;
};

struct WrapperDescriptorHeap
{
  std::vector<std::tuple<GLuint, RESOURCE_VIEW, unsigned>> GLValue;
};

class GLAPI : public GFXAPI
{
public:
  virtual struct WrapperResource* createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4]) override;
  virtual struct WrapperResource* createDepthStencilTexture(size_t Width, size_t Height) override;
  virtual void releaseRTTOrDepthStencilTexture(struct WrapperResource* res) override;
  virtual struct WrapperRTTSet* createRTTSet(const std::vector<struct WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height, WrapperResource *DepthStencil) override;
  virtual void releaseRTTSet(struct WrapperRTTSet *RTTSet) override;
  virtual void releasePSO(struct WrapperPipelineState *pso) override;
  virtual void clearRTTSet(struct WrapperCommandList* wrappedCmdList, struct WrapperRTTSet*, float color[4]) override;
  virtual void clearDepthStencilFromRTTSet(struct WrapperCommandList* wrappedCmdList, struct WrapperRTTSet*, float Depth, unsigned stencil) override;
  virtual void setRTTSet(struct WrapperCommandList* wrappedCmdList, WrapperRTTSet*) override;
  virtual void setBackbufferAsRTTSet(struct WrapperCommandList* wrappedCmdList, size_t width, size_t height) override;
  virtual void setBackbufferAsPresent(struct WrapperCommandList* wrappedCmdList) override;
  virtual struct WrapperDescriptorHeap* createCBVSRVUAVDescriptorHeap(const std::vector<std::tuple<struct WrapperResource *, enum class RESOURCE_VIEW, size_t> > &Resources) override;
  virtual void releaseCBVSRVUAVDescriptorHeap(struct WrapperDescriptorHeap* Heap) override;
  virtual struct WrapperDescriptorHeap* createSamplerHeap(const std::vector<std::pair<enum class SAMPLER_TYPE, size_t>> &SamplersDesc) override;
  virtual void releaseSamplerHeap(struct WrapperDescriptorHeap* Heap) override;
  virtual void setDescriptorHeap(struct WrapperCommandList* wrappedCmdList, size_t slot, struct WrapperDescriptorHeap *DescriptorHeap) override;
  virtual struct WrapperResource *createConstantsBuffer(size_t) override;
  virtual void releaseConstantsBuffers(struct WrapperResource *cbuf) override;
  virtual void *mapConstantsBuffer(struct WrapperResource *) override;
  virtual void unmapConstantsBuffers(struct WrapperResource *wrappedConstantsBuffer) override;
  virtual void setIndexVertexBuffersSet(struct WrapperCommandList* wrappedCmdList, WrapperIndexVertexBuffersSet*) override;
  virtual void setPipelineState(struct WrapperCommandList* wrappedCmdList, struct WrapperPipelineState* pipelineState) override;
  virtual void writeResourcesTransitionBarrier(struct WrapperCommandList* wrappedCmdList, const std::vector<std::tuple<struct WrapperResource *, enum class RESOURCE_USAGE, enum class RESOURCE_USAGE> > &barriers) override;
  virtual struct WrapperCommandList* createCommandList() override;
  virtual void releaseCommandList(struct WrapperCommandList* wrappedCmdList) override;
  virtual void closeCommandList(struct WrapperCommandList* wrappedCmdList) override;
  virtual void openCommandList(struct WrapperCommandList* wrappedCmdList) override;
  virtual void drawIndexedInstanced(struct WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t indexOffset, size_t vertexOffset, size_t instanceOffset) override;
  virtual void drawInstanced(struct WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t vertexOffset, size_t instanceOffset) override;
  virtual void submitToQueue(struct WrapperCommandList *wrappedCmdList) override;
  virtual struct WrapperIndexVertexBuffersSet* createFullscreenTri() override;
  virtual void releaseIndexVertexBuffersSet(struct WrapperIndexVertexBuffersSet *res) override;
};

#endif