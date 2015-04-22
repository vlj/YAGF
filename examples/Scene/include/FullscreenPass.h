// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __FULLSCREEN_PASS_H__
#define __FULLSCREEN_PASS_H__

#include <RenderTargets.h>
#include <API/GfxApi.h>

class FullscreenPassManager
{
private:
  RenderTargets &RTT;
  // Fix for D3D
  WrapperResource *depthtexturecopy;
  WrapperIndexVertexBuffersSet *screentri;
  WrapperDescriptorHeap *Samplers;
  // Sunlights
  WrapperPipelineState *SunlightPSO;
  WrapperDescriptorHeap *SunlightInputs;
  // should be param of rendersunlight
  WrapperResource *viewdata;
  WrapperResource *lightdata;
  // Tonemap
  WrapperPipelineState *TonemapPSO;
  WrapperDescriptorHeap *TonemapInputs;
public:
  WrapperCommandList *CommandList;

  FullscreenPassManager(RenderTargets &);
  ~FullscreenPassManager();

  void renderSunlight();
  void renderTonemap();
};

#endif