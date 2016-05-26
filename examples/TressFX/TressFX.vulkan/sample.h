#pragma once
#define VULKAN
#include <Api/Vkapi.h>
#include <AMD_TressFX.h>

struct sample
{
    AMD::TressFX_Desc tressfx_helper;
    std::unique_ptr<device_t> dev;
    std::unique_ptr<command_queue_t> queue;
    std::unique_ptr<swap_chain_t> chain;

    std::vector<std::unique_ptr<image_t>> back_buffer;

    std::unique_ptr<image_t> depth_texture;
    std::unique_ptr<image_view_t> depth_texture_view;
    std::unique_ptr<image_t> color_texture;
    std::unique_ptr<image_view_t> color_texture_view;
    std::unique_ptr<buffer_t> constant_buffer;

    std::unique_ptr<command_list_storage_t> command_storage;
    std::unique_ptr<command_list_t> draw_command_buffer;
    std::array<std::unique_ptr<command_list_t>, 2> blit_command_buffer;

    std::unique_ptr<render_pass_t> blit_render_pass;
    std::shared_ptr<descriptor_set_layout> blit_layout_set;
    pipeline_layout_t blit_layout;
    pipeline_state_t blit_pso;

    sample(HINSTANCE hinstance, HWND hwnd);
    void draw();
};

