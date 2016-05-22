#include "sample.h"
#include <Api/Vkapi.h>

sample::sample(HINSTANCE hinstance, HWND hwnd)
{
    auto dev_swapchain_queue = create_device_swapchain_and_graphic_presentable_queue(hinstance, hwnd);
    tressfx_helper.pvkDevice = *std::get<0>(dev_swapchain_queue);
    tressfx_helper.texture_memory_index = std::get<0>(dev_swapchain_queue)->default_memory_index;
    TressFX_Initialize(tressfx_helper);
}
