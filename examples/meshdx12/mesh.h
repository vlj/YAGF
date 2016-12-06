#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <array>
#include <unordered_map>

#include <Scene/pso.h>
#include <Scene/textures.h>
#include <Scene/Scene.h>
#include <Scene/ssao.h>

#include <API/GfxApi.h>
#include <glfw/glfw3.h>


struct MeshSample
{
	MeshSample();

	~MeshSample()
	{
		cmdqueue->wait_for_command_queue_idle();

		glfwTerminate();
	}


	float horizon_angle = 0;
	irr::scene::IMeshSceneNode* xue;
	GLFWwindow *window;

private:
	uint32_t width;
	uint32_t height;

	std::unique_ptr<device_t> dev;
	std::unique_ptr<command_queue_t> cmdqueue;
	irr::video::ECOLOR_FORMAT swap_chain_format;
	std::unique_ptr<swap_chain_t> chain;
	std::vector<std::unique_ptr<image_t>> back_buffer;
	std::vector<std::unique_ptr<image_view_t>> back_buffer_view;


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
	std::shared_ptr<descriptor_set_layout> input_attachments_set;
	std::shared_ptr<descriptor_set_layout> rtt_set;
	std::shared_ptr<descriptor_set_layout> model_set;
	std::shared_ptr<descriptor_set_layout> ibl_set;


	std::unique_ptr<sampler_t> anisotropic_sampler;
	std::unique_ptr<sampler_t> bilinear_clamped_sampler;

	std::unique_ptr<image_view_t> skybox_view;
	std::unique_ptr<image_view_t> diffuse_color_view;
	std::unique_ptr<image_view_t> normal_view;
	std::unique_ptr<image_view_t> roughness_metalness_view;
	std::unique_ptr<image_view_t> depth_view;
	std::unique_ptr<image_view_t> specular_cube_view;
	std::unique_ptr<image_view_t> dfg_lut_view;
	std::unique_ptr<image_view_t> ssao_view;

	std::unique_ptr<descriptor_storage_t> sampler_heap;

	std::unique_ptr<allocated_descriptor_set> ibl_descriptor;
	std::unique_ptr<allocated_descriptor_set> sampler_descriptors;
	std::unique_ptr<allocated_descriptor_set> input_attachments_descriptors;
	std::unique_ptr<allocated_descriptor_set> rtt_descriptors;
	std::unique_ptr<allocated_descriptor_set> scene_descriptor;

	std::unique_ptr<image_t> depth_buffer;
	std::unique_ptr<image_t> diffuse_color;
	std::unique_ptr<image_t> normal;
	std::unique_ptr<image_t> roughness_metalness;

	std::unique_ptr<irr::scene::Scene> scene;


	std::unique_ptr<ssao_utility> ssao_util;

	std::unique_ptr<render_pass_t> object_sunlight_pass;
	std::unique_ptr<render_pass_t> ibl_skyboss_pass;
	std::array<std::unique_ptr<framebuffer_t>, 2> fbo_pass1;
	std::array<std::unique_ptr<framebuffer_t>, 2> fbo_pass2;
	std::unique_ptr<pipeline_layout_t> object_sig;
	std::unique_ptr<pipeline_state_t> objectpso;
	std::unique_ptr<pipeline_layout_t> sunlight_sig;
	std::unique_ptr<pipeline_state_t> sunlightpso;
	std::unique_ptr<pipeline_layout_t> skybox_sig;
	std::unique_ptr<pipeline_state_t> skybox_pso;
	std::unique_ptr<pipeline_layout_t> ibl_sig;
	std::unique_ptr<pipeline_state_t> ibl_pso;
	void fill_draw_commands();
	void Init();

	void createDescriptorSets();

	void createTextures();

	void fill_descriptor_set();
	void load_program_and_pipeline_layout();
public:
	void Draw();
	void Loop() {
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			// Keep running
		}
		glfwDestroyWindow(window);
		return;
	}
};

