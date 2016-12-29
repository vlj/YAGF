#pragma once
#define VULKAN
#include <Api/Vkapi.h>
#include <AMD_TressFX.h>
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

struct sample
{
    AMD::TressFX_Desc tressfx_helper;
    std::unique_ptr<device_t> dev;
    std::unique_ptr<command_queue_t> queue;
    std::unique_ptr<swap_chain_t> chain;

    std::vector<std::unique_ptr<image_t>> back_buffer;
	std::vector<std::unique_ptr<image_view_t>> back_buffer_view;
    std::array<std::unique_ptr<framebuffer_t>, 2> fbo;

    std::unique_ptr<image_t> depth_texture;
    std::unique_ptr<image_view_t> depth_texture_view;
    std::unique_ptr<image_t> color_texture;
    std::unique_ptr<image_view_t> color_texture_view;

    std::unique_ptr<command_list_storage_t> command_storage;
    std::unique_ptr<command_list_t> draw_command_buffer;
    std::array<std::unique_ptr<command_list_t>, 2> blit_command_buffer;

    std::unique_ptr<render_pass_t> blit_render_pass;
    std::unique_ptr<descriptor_set_layout> blit_layout_set;
    std::unique_ptr<pipeline_layout_t> blit_layout;
    std::unique_ptr<pipeline_state_t> blit_pso;

    std::unique_ptr<descriptor_storage_t> descriptor_pool;
	std::unique_ptr<allocated_descriptor_set> descriptor;
    std::unique_ptr<buffer_t> big_triangle;
    std::unique_ptr<sampler_t> bilinear_sampler;

    sample(GLFWwindow *window);
    ~sample();
    void draw();
};

