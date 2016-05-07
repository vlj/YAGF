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
	MeshSample(std::unique_ptr<device_t> &&_dev, std::unique_ptr<swap_chain_t> &&_chain, std::unique_ptr<command_queue_t> &&_cmdqueue, uint32_t _w, uint32_t _h, irr::video::ECOLOR_FORMAT format)
		: dev(std::move(_dev)), chain(std::move(_chain)), cmdqueue(std::move(_cmdqueue)), width(_w), height(_h), swap_chain_format(format)
	{
		Init();
	}

	~MeshSample()
	{
		wait_for_command_queue_idle(*dev, *cmdqueue);
	}

private:
	size_t width;
	size_t height;

	std::unique_ptr<device_t> dev;
	std::unique_ptr<command_queue_t> cmdqueue;
	irr::video::ECOLOR_FORMAT swap_chain_format;
	std::unique_ptr<swap_chain_t> chain;
	std::vector<std::unique_ptr<image_t>> back_buffer;


	std::unique_ptr<command_list_storage_t> command_allocator;
	std::vector<std::unique_ptr<command_list_t>> command_list_for_back_buffer;

	std::unique_ptr<buffer_t> sun_data;
	std::unique_ptr<buffer_t> scene_matrix;
	std::unique_ptr<buffer_t> big_triangle;
	std::vector<std::tuple<buffer_t&, uint64_t, uint32_t, uint32_t> > big_triangle_info;
	std::unique_ptr<descriptor_storage_t> cbv_srv_descriptors_heap;

	std::unique_ptr<image_t> skybox_texture;
	std::unique_ptr<buffer_t> sh_coefficients;
	std::unique_ptr<image_t> specular_cube;
	std::unique_ptr<image_t> dfg_lut;

	std::shared_ptr<descriptor_set_layout> object_set;
	std::shared_ptr<descriptor_set_layout> scene_set;
	std::shared_ptr<descriptor_set_layout> sampler_set;
	std::shared_ptr<descriptor_set_layout> rtt_set;
	std::shared_ptr<descriptor_set_layout> model_set;
	std::shared_ptr<descriptor_set_layout> ibl_set;

#ifndef D3D12
	std::shared_ptr<vulkan_wrapper::sampler> sampler;
	std::shared_ptr<vulkan_wrapper::image_view> skybox_view;
	std::shared_ptr<vulkan_wrapper::image_view> diffuse_color_view;
	std::shared_ptr<vulkan_wrapper::image_view> normal_roughness_metalness_view;
	std::shared_ptr<vulkan_wrapper::image_view> depth_view;
	std::shared_ptr<vulkan_wrapper::image_view> specular_cube_view;
	std::shared_ptr<vulkan_wrapper::image_view> dfg_lut_view;
#endif
	std::unique_ptr<descriptor_storage_t> sampler_heap;

	allocated_descriptor_set ibl_descriptor;
	allocated_descriptor_set sampler_descriptors;
	allocated_descriptor_set rtt_descriptors;
	allocated_descriptor_set scene_descriptor;

	std::unique_ptr<image_t> depth_buffer;
	std::unique_ptr<image_t> diffuse_color;
	std::unique_ptr<image_t> normal_roughness_metalness;

	std::unique_ptr<irr::scene::IMeshSceneNode> xue;

	std::unique_ptr<render_pass_t> render_pass;
	framebuffer_t fbo[2];
	pipeline_layout_t object_sig;
	pipeline_state_t objectpso;
	pipeline_layout_t sunlight_sig;
	pipeline_state_t sunlightpso;
	pipeline_layout_t skybox_sig;
	pipeline_state_t skybox_pso;
	pipeline_layout_t ibl_sig;
	pipeline_state_t ibl_pso;
	void fill_draw_commands();
	void Init();

	void fill_descriptor_set();
	void load_program_and_pipeline_layout();
public:
	void Draw();
};

