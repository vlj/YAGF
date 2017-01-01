// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <API/GfxApi.h>

std::unique_ptr<pipeline_state_t> get_skinned_object_pipeline_state(device_t& dev, pipeline_layout_t& layout, render_pass_t& rp);
std::unique_ptr<pipeline_state_t> get_sunlight_pipeline_state(device_t& dev, pipeline_layout_t& layout, render_pass_t& rp);
std::unique_ptr<pipeline_state_t> get_skybox_pipeline_state(device_t& dev, pipeline_layout_t& layout, render_pass_t& rp);
std::unique_ptr<pipeline_state_t> get_ibl_pipeline_state(device_t& dev, pipeline_layout_t& layout, render_pass_t& rp);
