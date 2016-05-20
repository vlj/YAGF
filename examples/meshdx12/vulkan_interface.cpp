#include "mesh.h"
#include <Windows.h>

extern "C"
{
	__declspec(dllexport) void* create_vulkan_mesh(HWND window, HINSTANCE hInstance);
}

void*  create_vulkan_mesh(HWND window, HINSTANCE hInstance)
{
	std::unique_ptr<device_t> dev;
	std::unique_ptr<swap_chain_t> chain;
	std::unique_ptr<command_queue_t> cmd_queue;
	uint32_t width;
	uint32_t height;
	irr::video::ECOLOR_FORMAT format;
	std::tie(dev, chain, cmd_queue, width, height, format) = create_device_swapchain_and_graphic_presentable_queue(hInstance, window);
	return (void*)new MeshSample(std::move(dev), std::move(chain), std::move(cmd_queue), width, height, format);
}
