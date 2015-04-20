// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __FULLSCREEN_PASS_H__
#define __FULLSCREEN_PASS_H__

#include <RenderTargets.h>
#include <GfxApi.h>

class FullscreenPassManager
{
private:
  RenderTargets &RTT;
  WrapperPipelineState *SunlightPSO;
  WrapperDescriptorHeap *SunlightInputs;
  WrapperDescriptorHeap *Samplers;
  WrapperCommandList *CommandList;
  // should be param of rendersunlight
  WrapperResource *viewdata;
  WrapperResource *lightdata;
  // Fix for D3D
  WrapperResource *depthtexturecopy;
public:
  FullscreenPassManager(RenderTargets &);

  void renderSunlight();
};

#endif