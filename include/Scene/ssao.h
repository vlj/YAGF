// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <API/GfxApi.h>

struct ssao_utility
{
	uint32_t width;
	uint32_t height;
	std::unique_ptr<descriptor_set_layout> linearize_input_set;
	std::unique_ptr<pipeline_state_t> linearize_depth_pso;
	std::unique_ptr<pipeline_layout_t> linearize_depth_sig;
	std::unique_ptr<descriptor_set_layout> ssao_input_set;
	std::unique_ptr<pipeline_state_t> ssao_pso;
	std::unique_ptr<pipeline_layout_t> ssao_sig;
	std::unique_ptr<descriptor_set_layout> samplers_set;
	std::unique_ptr<descriptor_set_layout> gaussian_input_set;
	std::unique_ptr<compute_pipeline_state_t> gaussian_h_pso;
	std::unique_ptr<compute_pipeline_state_t> gaussian_v_pso;
	std::unique_ptr<pipeline_layout_t> gaussian_input_sig;

	std::unique_ptr<descriptor_storage_t> heap;
	std::unique_ptr<descriptor_storage_t> sampler_heap;
	std::unique_ptr<allocated_descriptor_set> linearize_input;
	std::unique_ptr<allocated_descriptor_set> ssao_input;
	std::unique_ptr<allocated_descriptor_set> sampler_input;
	std::unique_ptr<allocated_descriptor_set> gaussian_input_h;
	std::unique_ptr<allocated_descriptor_set> gaussian_input_v;

	std::unique_ptr<buffer_t> linearize_constant_data;
	std::unique_ptr<buffer_t> ssao_constant_data;

	image_t* depth_input;
	std::unique_ptr<image_view_t> depth_image_view;
	std::unique_ptr<image_t> linear_depth_buffer;
	std::unique_ptr<image_view_t> linear_depth_buffer_view;
	std::unique_ptr<image_t> ssao_result;
	std::unique_ptr<image_view_t> ssao_result_view;
	std::unique_ptr<image_t> gaussian_blurring_buffer;
	std::unique_ptr<image_view_t> gaussian_blurring_buffer_view;
	std::unique_ptr<image_t> ssao_bilinear_result;
	std::unique_ptr<image_view_t> ssao_bilinear_result_view;
	std::unique_ptr<framebuffer_t> linear_depth_fbo;
	std::unique_ptr<render_pass_t> render_pass;

	std::unique_ptr<sampler_t> bilinear_clamped_sampler;
	std::unique_ptr<sampler_t> nearest_sampler;


	ssao_utility(device_t &dev, image_t* _depth_input, uint32_t w, uint32_t h);

	/**
	 * Need to set render target before filling cmd list atm
	 * Also implicitly use previously set scissor/viewport
	 */
	void fill_command_list(device_t &dev, command_list_t& cmd_list, image_t &depth_buffer, float zn, float zf,
		const std::vector<std::tuple<buffer_t&, uint64_t, uint32_t, uint32_t> > &big_triangle_info);
};
