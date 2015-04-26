// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __SHADERS_H__
#define __SHADERS_H__

struct WrapperPipelineState *createSunlightShader();
struct WrapperPipelineState *createTonemapShader();
struct WrapperPipelineState *createObjectShader();
struct WrapperPipelineState *createSkinnedObjectShader();
struct WrapperPipelineState *createSkyboxShader();
struct WrapperPipelineState *createIBLShader();

struct WrapperPipelineState *ImportanceSamplingForSpecularCubemap();
#endif