// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#ifndef IBL_HPP
#define IBL_HPP

#ifdef D3D12
#include <API/d3dapi.h>
#include <d3dx12.h>
#else
#include <API/vkapi.h>
#endif

#include <Core/IImage.h>

struct Color
{
  float Red;
  float Green;
  float Blue;
};

struct SH
{
	float bL00;
	float bL1m1;
	float bL10;
	float bL11;
	float bL2m2;
	float bL2m1;
	float bL20;
	float bL21;
	float bL22;

	float gL00;
	float gL1m1;
	float gL10;
	float gL11;
	float gL2m2;
	float gL2m1;
	float gL20;
	float gL21;
	float gL22;

	float rL00;
	float rL1m1;
	float rL10;
	float rL11;
	float rL2m2;
	float rL2m1;
	float rL20;
	float rL21;
	float rL22;
};

struct ibl_utility
{
	std::shared_ptr<descriptor_set_layout> object_set;
	std::shared_ptr<descriptor_set_layout> sampler_set;

	std::shared_ptr<descriptor_set_layout> face_set;
	std::shared_ptr<descriptor_set_layout> mipmap_set;
	std::shared_ptr<descriptor_set_layout> uav_set;

	std::shared_ptr<descriptor_set_layout> dfg_set;

	pipeline_layout_t compute_sh_sig;
	pipeline_layout_t importance_sampling_sig;
	pipeline_layout_t dfg_building_sig;

	std::unique_ptr<compute_pipeline_state_t> compute_sh_pso;
	std::unique_ptr<compute_pipeline_state_t> importance_sampling;
	std::unique_ptr<compute_pipeline_state_t> pso;

	std::unique_ptr<descriptor_storage_t> srv_cbv_uav_heap;
	std::unique_ptr<descriptor_storage_t> sampler_heap;

	allocated_descriptor_set sampler_descriptors;

	std::unique_ptr<sampler_t> anisotropic_sampler;

	std::unique_ptr<buffer_t> compute_sh_cbuf;
	std::array<std::unique_ptr<buffer_t>, 6> permutation_matrix;

	std::unique_ptr<buffer_t> hammersley_sequence_buffer;
	std::array<std::unique_ptr<buffer_t>, 8> per_level_cbuffer{};

	std::array<std::unique_ptr<image_view_t>, 48> uav_views;
#ifndef D3D12
	std::unique_ptr<vulkan_wrapper::buffer_view> hammersley_sequence_buffer_view;
#endif

	ibl_utility(device_t &dev);

	/** Generate the 9 first SH coefficients for each color channel
	using the cubemap provided by CubemapFace.
	*  \param textures sequence of 6 square textures.
	*  \param row/columns count of textures.
	*/
	std::unique_ptr<buffer_t> computeSphericalHarmonics(device_t& dev, command_list_t& cmd_list, image_view_t& probe_view, size_t edge_size);

	std::unique_ptr<image_t> generateSpecularCubemap(device_t& dev, command_list_t& cmd_list, image_view_t& probe_view);
	std::tuple<std::unique_ptr<image_t>, std::unique_ptr<image_view_t>> getDFGLUT(device_t& dev, command_list_t& cmd_list, uint32_t DFG_LUT_size = 128);

private:
	allocated_descriptor_set get_compute_sh_descriptor(device_t &dev, buffer_t &constant_buffer, image_view_t& probe_view, buffer_t& sh_buffer);
	allocated_descriptor_set get_dfg_input_descriptor_set(device_t &dev, buffer_t& constant_buffer, image_view_t &DFG_LUT_view);
};
#endif
