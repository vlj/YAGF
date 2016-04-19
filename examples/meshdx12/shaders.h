#pragma once

#ifdef D3D12
#include <API/d3dapi.h>
#else
#include <API/vkapi.h>
#endif


pipeline_layout_t get_skinned_object_pipeline_layout(device_t dev);
pipeline_state_t get_skinned_object_pipeline_state(device_t dev, pipeline_layout_t layout, render_pass_t rp);
