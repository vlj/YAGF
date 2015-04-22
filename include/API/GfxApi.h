// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#ifndef __GFX_API_H__
#define __GFX_API_H__

#include <Core/SColor.h>
#include <vector>
#include <tuple>

#ifdef GLBUILD
#include <GL/glew.h>
#include <GLAPI/GLRTTSet.h>
#endif

#ifdef DXBUILD
#include <D3DAPI/D3DRTTSet.h>
#include <D3DAPI/VAO.h>
#include <Core/BasicVertexLayout.h>
#endif

enum class RESOURCE_USAGE
{
  PRESENT,
  COPY_DEST,
  COPY_SRC,
  RENDER_TARGET,
  READ_GENERIC,
};
enum class RESOURCE_VIEW
{
  CONSTANTS_BUFFER,
  SHADER_RESOURCE,
  SAMPLER,
  UAV,
};

enum class SAMPLER_TYPE
{
  NEAREST,
  BILINEAR,
  TRILINEAR,
  ANISOTROPIC,
};

struct WrapperRTTSet;

struct WrapperCommandList;

struct WrapperResource;

struct WrapperIndexVertexBuffersSet;

struct WrapperPipelineState;

struct WrapperDescriptorHeap;

class GFXAPI
{
public:
  virtual struct WrapperResource* createRTT(irr::video::ECOLOR_FORMAT, size_t Width, size_t Height, float fastColor[4]) = 0;
  virtual struct WrapperResource* createDepthStencilTexture(size_t Width, size_t Height) = 0;
  virtual void releaseRTTOrDepthStencilTexture(struct WrapperResource* res) = 0;
  virtual struct WrapperRTTSet* createRTTSet(const std::vector<struct WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height, WrapperResource *DepthStencil) = 0;
  virtual void releaseRTTSet(struct WrapperRTTSet *RTTSet) = 0;
  virtual void releasePSO(struct WrapperPipelineState *pso) = 0;
  virtual void clearRTTSet(struct WrapperCommandList* wrappedCmdList, struct WrapperRTTSet*, float color[4]) = 0;
  virtual void clearDepthStencilFromRTTSet(struct WrapperCommandList* wrappedCmdList, struct WrapperRTTSet*, float Depth, unsigned stencil) = 0;
  virtual void setRTTSet(struct WrapperCommandList* wrappedCmdList, struct WrapperRTTSet*) = 0;
  virtual void setBackbufferAsRTTSet(struct WrapperCommandList* wrappedCmdList, size_t width, size_t height) = 0;
  virtual void setBackbufferAsPresent(struct WrapperCommandList* wrappedCmdList) = 0;
  virtual struct WrapperDescriptorHeap* createCBVSRVUAVDescriptorHeap(const std::vector<std::tuple<struct WrapperResource *, enum class RESOURCE_VIEW, size_t> > &Resources) = 0;
  virtual void releaseCBVSRVUAVDescriptorHeap(struct WrapperDescriptorHeap* Heap) = 0;
  virtual struct WrapperDescriptorHeap* createSamplerHeap(const std::vector<std::pair<enum class SAMPLER_TYPE, size_t>> &SamplersDesc) = 0;
  virtual void releaseSamplerHeap(struct WrapperDescriptorHeap* Heap) = 0;
  virtual void setDescriptorHeap(struct WrapperCommandList* wrappedCmdList, size_t slot, struct WrapperDescriptorHeap *DescriptorHeap) = 0;
  virtual void setPipelineState(struct WrapperCommandList* wrappedCmdList, struct WrapperPipelineState* pipelineState) = 0;
  virtual void setIndexVertexBuffersSet(struct WrapperCommandList* wrappedCmdList, WrapperIndexVertexBuffersSet*) = 0;
  virtual struct WrapperResource *createConstantsBuffer(size_t) = 0;
  virtual void releaseConstantsBuffers(struct WrapperResource *cbuf) = 0;
  virtual void *mapConstantsBuffer(struct WrapperResource *) = 0;
  virtual void unmapConstantsBuffers(struct WrapperResource *wrappedConstantsBuffer) = 0;
  virtual void writeResourcesTransitionBarrier(struct WrapperCommandList* wrappedCmdList, const std::vector<std::tuple<struct WrapperResource *, enum class RESOURCE_USAGE, enum class RESOURCE_USAGE> > &) = 0;
  virtual struct WrapperCommandList* createCommandList() = 0;
  virtual void openCommandList(struct WrapperCommandList* wrappedCmdList) = 0;
  virtual void closeCommandList(struct WrapperCommandList* wrappedCmdList) = 0;
  virtual void releaseCommandList(struct WrapperCommandList* wrappedCmdList) = 0;
  virtual void drawIndexedInstanced(struct WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t indexOffset, size_t vertexOffset, size_t instanceOffset) = 0;
  virtual void drawInstanced(struct WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t vertexOffset, size_t instanceOffset) = 0;
  virtual void submitToQueue(struct WrapperCommandList *wrappedCmdList) = 0;
  // Uploaded in the direct queue
  virtual struct WrapperIndexVertexBuffersSet* createFullscreenTri() = 0;
  virtual void releaseIndexVertexBuffersSet(struct WrapperIndexVertexBuffersSet *res) = 0;
};

//Global
extern GFXAPI* GlobalGFXAPI;

#endif