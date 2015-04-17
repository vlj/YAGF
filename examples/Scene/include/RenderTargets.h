// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __RENDER_TARGETS_H__
#define __RENDER_TARGETS_H__

#include <GfxApi.h>

#ifdef GLBUILD
#include <GLAPI/GLRTTSet.h>
#endif

#ifdef DXBUILD
#include <d3d12.h>
#include <wrl/client.h>
#endif

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
#ifdef GLBUILD
  GLuint DepthStencilTexture;
#endif

public:
  RenderTargets(size_t w, size_t h): Width(w), Height(h)
  {
    WrapperResource *depthbuffer = GlobalGFXAPI->createDepthStencilTexture(Width, Height);

    float color[] = { 0., 0., 0., 0. };
    RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH] = GlobalGFXAPI->createRTT(irr::video::ECF_R16G16B16A16F, Width, Height, color);
    RenderTargetTextures[GBUFFER_BASE_COLOR] = GlobalGFXAPI->createRTT(irr::video::ECF_R8G8B8A8_UNORM_SRGB, Width, Height, color);
    RenderTargetTextures[COLORS] = GlobalGFXAPI->createRTT(irr::video::ECF_R16G16B16A16F, Width, Height, color);

    RTTSets[FBO_GBUFFER] = GlobalGFXAPI->createRTTSet(
    { RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH] , RenderTargetTextures[GBUFFER_BASE_COLOR] },
    { irr::video::ECF_R16G16B16A16F, irr::video::ECF_R8G8B8A8_UNORM_SRGB },
    Width, Height,
    depthbuffer);


#ifdef GLBUILD
    glGenTextures(1, &DepthStencilTexture);
    glBindTexture(GL_TEXTURE_2D, DepthStencilTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, (GLsizei)Width, (GLsizei)Height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);

/*glGenTextures(RTT_COUNT, RenderTargetTextures);
    RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH] = generateRTT(w, h , GL_RGBA16F, GL_RGBA, GL_FLOAT);
    RenderTargetTextures[GBUFFER_BASE_COLOR] = generateRTT(w, h, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    RenderTargetTextures[COLORS] = generateRTT(w, h, GL_RGBA16F, GL_BGRA, GL_FLOAT);

    FrameBuffers.push_back(GLRTTSet({ RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH], RenderTargetTextures[GBUFFER_BASE_COLOR]}, DepthStencilTexture, w, h));
    FrameBuffers.push_back(GLRTTSet({ RenderTargetTextures[COLORS], RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH] }, DepthStencilTexture, w, h));
    FrameBuffers.push_back(GLRTTSet({ RenderTargetTextures[LINEAR_DEPTH], RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH] }, DepthStencilTexture, w, h));*/
#endif
  }

  ~RenderTargets()
  {
#ifdef GLBUILD
    glDeleteTextures(1, &DepthStencilTexture);
#endif
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