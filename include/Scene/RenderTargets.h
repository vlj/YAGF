// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __RENDER_TARGETS_H__
#define __RENDER_TARGETS_H__

#include <API/GfxApi.h>

class RenderTargets
{
public:
  enum RTT
  {
    GBUFFER_NORMAL_AND_DEPTH = 0,
    GBUFFER_BASE_COLOR,
//    GBUFFER_EMIT_VALUE,
    COLORS,
//    LINEAR_DEPTH,
    RTT_COUNT,
  };

  enum RTTSet
  {
    FBO_GBUFFER,
    FBO_COLORS,
    FBO_COLOR_WITH_DEPTH,
//    FBO_LINEAR_DEPTH,
    FBO_COUNT,
  };

private:
  size_t Width, Height;
  WrapperResource* RenderTargetTextures[RTT_COUNT];
  WrapperRTTSet* RTTSets[FBO_COUNT];
  WrapperResource *depthbuffer;
public:
  RenderTargets(size_t w, size_t h);
  ~RenderTargets();
  WrapperResource* getRTT(enum RTT tp);
  WrapperRTTSet* getRTTSet(enum RTTSet tp);
  WrapperResource *getDepthBuffer();
};

#endif