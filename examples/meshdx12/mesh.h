#pragma once
#include <assimp/Importer.hpp>
#include <Maths/matrix4.h>
#include <assimp/scene.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <array>
#include <unordered_map>

#include "shaders.h"
#include "geometry.h"
#include "textures.h"

#ifdef D3D12
#include <API/d3dapi.h>
#include <d3dx12.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")
#else
#include <API/vkapi.h>
#endif


struct MeshSample
{
	MeshSample(device_t _dev, swap_chain_t _chain, command_queue_t _cmdqueue, uint32_t _w, uint32_t _h, irr::video::ECOLOR_FORMAT format)
		: dev(_dev), chain(_chain), cmdqueue(_cmdqueue), width(_w), height(_h), swap_chain_format(format)
	{
		Init();
	}

	~MeshSample()
	{
		wait_for_command_queue_idle(dev, cmdqueue);
	}

private:
	size_t width;
	size_t height;

	device_t dev;
	command_queue_t cmdqueue;
	irr::video::ECOLOR_FORMAT swap_chain_format;
	swap_chain_t chain;
	std::vector<image_t> back_buffer;
	std::vector<command_list_t> command_list_for_back_buffer;

	command_list_storage_t command_allocator;
	buffer_t sun_data;
	buffer_t view_matrixes;
	buffer_t cbuffer;
	buffer_t jointbuffer;
	buffer_t big_triangle;
	std::vector<std::tuple<buffer_t, uint64_t, uint32_t, uint32_t> > big_triangle_info;
	descriptor_storage_t cbv_srv_descriptors_heap;

	std::vector<image_t> Textures;
#ifndef D3D12
	VkDescriptorSet cbuffer_descriptor_set;
	std::vector<VkDescriptorSet> texture_descriptor_set;
	std::vector<std::shared_ptr<vulkan_wrapper::image_view> > Textures_views;
	std::shared_ptr<vulkan_wrapper::sampler> sampler;
	VkDescriptorSet sampler_descriptors;
	VkDescriptorSet input_attachment_descriptors;
	std::shared_ptr<vulkan_wrapper::image_view> diffuse_color_view;
	std::shared_ptr<vulkan_wrapper::image_view> normal_roughness_metalness_view;
	std::shared_ptr<vulkan_wrapper::image_view> depth_view;
#else
	framebuffer_t g_buffer;
#endif
	descriptor_storage_t sampler_heap;
	image_t depth_buffer;
	image_t diffuse_color;
	image_t normal_roughness_metalness;

	std::unique_ptr<object> xue;

	render_pass_t render_pass;
	framebuffer_t fbo[2];
	pipeline_layout_t object_sig;
	pipeline_state_t objectpso;
	pipeline_layout_t sunlight_sig;
	pipeline_state_t sunlightpso;
	void fill_draw_commands();
	void Init();
public:
	void Draw();
};
