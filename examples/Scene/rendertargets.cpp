// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <RenderTargets.h>


RenderTargets::RenderTargets(size_t w, size_t h) : Width(w), Height(h)
{
  depthbuffer = GlobalGFXAPI->createDepthStencilTexture(Width, Height);

  float color[] = { 0., 0., 0., 0. };
  RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH] = GlobalGFXAPI->createRTT(irr::video::ECF_R16G16B16A16F, Width, Height, color);
  RenderTargetTextures[GBUFFER_BASE_COLOR] = GlobalGFXAPI->createRTT(irr::video::ECF_R8G8B8A8_UNORM_SRGB, Width, Height, color);
  RenderTargetTextures[COLORS] = GlobalGFXAPI->createRTT(irr::video::ECF_R16G16B16A16F, Width, Height, color);

  RTTSets[FBO_GBUFFER] = GlobalGFXAPI->createRTTSet(
  { RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH] , RenderTargetTextures[GBUFFER_BASE_COLOR] },
  { irr::video::ECF_R16G16B16A16F, irr::video::ECF_R8G8B8A8_UNORM_SRGB },
  Width, Height,
  depthbuffer);
  RTTSets[FBO_COLORS] = GlobalGFXAPI->createRTTSet(
  { RenderTargetTextures[COLORS] },
  { irr::video::ECF_R16G16B16A16F },
  Width, Height, nullptr);
  RTTSets[FBO_COLOR_WITH_DEPTH] = GlobalGFXAPI->createRTTSet(
  { RenderTargetTextures[COLORS] },
  { irr::video::ECF_R16G16B16A16F },
  Width, Height, depthbuffer);
}

RenderTargets::~RenderTargets()
{
  for (unsigned i = 0; i < FBO_COUNT; i++)
    GlobalGFXAPI->releaseRTTSet(RTTSets[i]);
  for (unsigned i = 0; i < RTT_COUNT; i++)
    GlobalGFXAPI->releaseRTTOrDepthStencilTexture(RenderTargetTextures[i]);
  GlobalGFXAPI->releaseRTTOrDepthStencilTexture(depthbuffer);
}

WrapperResource* RenderTargets::getRTT(enum RTT tp)
{
  return RenderTargetTextures[tp];
}

WrapperRTTSet* RenderTargets::getRTTSet(enum RTTSet tp)
{
  return RTTSets[tp];
}

WrapperResource *RenderTargets::getDepthBuffer()
{
  return depthbuffer;
}