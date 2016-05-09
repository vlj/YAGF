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

struct ssao_utility
{
	std::shared_ptr<descriptor_set_layout> linearize_input_set;
	pipeline_state_t linearize_depth_pso;
	pipeline_layout_t linearize_depth_sig;
	std::shared_ptr<descriptor_set_layout> ssao_input_set;
	pipeline_state_t ssao_pso;
	pipeline_layout_t ssao_sig;

	std::unique_ptr<descriptor_storage_t> heap;
	allocated_descriptor_set linearize_input;

	std::unique_ptr<buffer_t> constant_data;

	ssao_utility(device_t &dev);

	/**
	 * Need to set render target before filling cmd list atm
	 * Also implicitly use previously set scissor/viewport
	 */
	void fill_command_list(device_t &dev, command_list_t& cmd_list, image_t &depth_buffer, float zn, float zf,
		const std::vector<std::tuple<buffer_t&, uint64_t, uint32_t, uint32_t> > &big_triangle_info);
};
