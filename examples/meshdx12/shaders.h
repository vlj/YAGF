// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#ifdef D3D12
#include <API/d3dapi.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#else
#include <API/vkapi.h>
#endif


pipeline_layout_t get_skinned_object_pipeline_layout(device_t dev);
pipeline_state_t get_skinned_object_pipeline_state(device_t dev, pipeline_layout_t layout, render_pass_t rp);

pipeline_layout_t get_sunlight_pipeline_layout(device_t dev);
pipeline_state_t get_sunlight_pipeline_state(device_t dev, pipeline_layout_t layout, render_pass_t rp);
