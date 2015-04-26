// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __FULLSCREEN_PASS_H__
#define __FULLSCREEN_PASS_H__

#include <Scene/RenderTargets.h>
#include <API/GfxApi.h>

class FullscreenPassManager
{
private:
  RenderTargets &RTT;
  // Fix for D3D
  WrapperResource *depthtexturecopy;
  WrapperIndexVertexBuffersSet *screentri;
  WrapperDescriptorHeap *Samplers;
  // IBL
  WrapperPipelineState *IBLPSO;
  WrapperDescriptorHeap *IBLInputs;
  WrapperDescriptorHeap *IBLSamplers;
  // Sunlights
  WrapperPipelineState *SunlightPSO;
  WrapperDescriptorHeap *SunlightInputs;
  // Skybox
  WrapperPipelineState *SkyboxPSO;
  WrapperDescriptorHeap *SkyboxInputs;
  WrapperDescriptorHeap *SkyboxSamplers;
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
  void renderSky(WrapperDescriptorHeap *skyboxtextureheap);
  void renderIBL(WrapperDescriptorHeap *probeHeap);
};

#endif