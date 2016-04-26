#pragma once
#include <assimp/Importer.hpp>
#include <Maths/matrix4.h>
#include <assimp/scene.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <array>
#include <unordered_map>

#include "shaders.h"
#include <Scene/textures.h>
#include <Scene/MeshSceneNode.h>

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
	MeshSample(device_t _dev, std::unique_ptr<swap_chain_t> &&_chain, std::unique_ptr<command_queue_t> &&_cmdqueue, uint32_t _w, uint32_t _h, irr::video::ECOLOR_FORMAT format)
		: dev(_dev), chain(std::move(_chain)), cmdqueue(std::move(_cmdqueue)), width(_w), height(_h), swap_chain_format(format)
	{
		Init();
	}

	~MeshSample()
	{
		wait_for_command_queue_idle(dev, cmdqueue.get());
	}

private:
	size_t width;
	size_t height;

	device_t dev;
	std::unique_ptr<command_queue_t> cmdqueue;
	irr::video::ECOLOR_FORMAT swap_chain_format;
	std::unique_ptr<swap_chain_t> chain;
	std::vector<image_t> back_buffer;
	std::vector<command_list_t> command_list_for_back_buffer;

	std::unique_ptr<command_list_storage_t> command_allocator;
	buffer_t sun_data;
	buffer_t scene_matrix;
	buffer_t big_triangle;
	std::vector<std::tuple<buffer_t, uint64_t, uint32_t, uint32_t> > big_triangle_info;
	descriptor_storage_t cbv_srv_descriptors_heap;

	image_t skybox_texture;
#ifndef D3D12
	std::shared_ptr<vulkan_wrapper::pipeline_descriptor_set> object_set;
	std::shared_ptr<vulkan_wrapper::pipeline_descriptor_set> scene_set;
	std::shared_ptr<vulkan_wrapper::pipeline_descriptor_set> sampler_set;
	std::shared_ptr<vulkan_wrapper::pipeline_descriptor_set> rtt_set;
	std::shared_ptr<vulkan_wrapper::pipeline_descriptor_set> model_set;
	std::shared_ptr<vulkan_wrapper::sampler> sampler;
	VkDescriptorSet sampler_descriptors;
	VkDescriptorSet rtt;
	VkDescriptorSet scene_descriptor;
	std::shared_ptr<vulkan_wrapper::image_view> skybox_view;
	std::shared_ptr<vulkan_wrapper::image_view> diffuse_color_view;
	std::shared_ptr<vulkan_wrapper::image_view> normal_roughness_metalness_view;
	std::shared_ptr<vulkan_wrapper::image_view> depth_view;
#endif
	descriptor_storage_t sampler_heap;
	image_t depth_buffer;
	image_t diffuse_color;
	image_t normal_roughness_metalness;

	std::unique_ptr<irr::scene::IMeshSceneNode> xue;

	render_pass_t render_pass;
	framebuffer_t fbo[2];
	pipeline_layout_t object_sig;
	pipeline_state_t objectpso;
	pipeline_layout_t sunlight_sig;
	pipeline_state_t sunlightpso;
	pipeline_layout_t skybox_sig;
	pipeline_state_t skybox_pso;
	void fill_draw_commands();
	void Init();
public:
	void Draw();
};

