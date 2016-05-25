#pragma once
#define VULKAN
#include <Api/Vkapi.h>
#include <AMD_TressFX.h>

struct sample
{
    AMD::TressFX_Desc tressfx_helper;
    std::unique_ptr<device_t> dev;
    std::unique_ptr<command_queue_t> queue;
    std::unique_ptr<image_t> depth_texture;
    std::unique_ptr<image_view_t> depth_texture_view;

    std::unique_ptr<command_list_storage_t> command_storage;

    sample(HINSTANCE hinstance, HWND hwnd);
    void draw();
};

