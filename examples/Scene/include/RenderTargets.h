// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __RENDER_TARGETS_H__
#define __RENDER_TARGETS_H__

#include <GfxApi.h>

class RenderTargets
{
public:
  enum RTT
  {
    GBUFFER_NORMAL_AND_DEPTH = 0,
    GBUFFER_BASE_COLOR,
    GBUFFER_EMIT_VALUE,
    COLORS,
    LINEAR_DEPTH,
    RTT_COUNT,
  };

  enum RTTSet
  {
    FBO_GBUFFER,
    FBO_COLORS,
    FBO_LINEAR_DEPTH,
    FBO_COUNT,
  };

private:
  size_t Width, Height;
  WrapperResource* RenderTargetTextures[RTT_COUNT];
  WrapperRTTSet* RTTSets[FBO_COUNT];
  WrapperResource *depthbuffer;

public:
  RenderTargets(size_t w, size_t h): Width(w), Height(h)
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
  }

  ~RenderTargets()
  {
  }

  WrapperResource* getRTT(enum RTT tp)
  {
    return RenderTargetTextures[tp];
  }

  WrapperRTTSet* getRTTSet(enum RTTSet tp)
  {
    return RTTSets[tp];
  }

};

#endif