// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include "../include/API/vkapi.h"
#include <sstream>

#include <locale>
#include <codecvt>
#include <string>


namespace
{
	uint32_t find_graphic_queue_family(VkPhysicalDevice phys_dev, VkSurfaceKHR surface, const std::vector<VkQueueFamilyProperties> &queue_family_properties)
	{
		for (unsigned int i = 0; i < queue_family_properties.size(); i++) {
			if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				VkBool32 support_presents;
				vkGetPhysicalDeviceSurfaceSupportKHR(phys_dev, i, surface, &support_presents);
				if (support_presents) return i;
			}
		}
		throw;
	}


	VKAPI_ATTR VkBool32 VKAPI_CALL
		dbgFunc(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType,
			uint64_t srcObject, size_t location, int32_t msgCode,
			const char *pLayerPrefix, const char *pMsg, void *pUserData) {
		std::ostringstream message;

		if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
			message << "ERROR: ";
		}
		else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
			message << "WARNING: ";
		}
		else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
			message << "PERFORMANCE WARNING: ";
		}
		else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
			message << "INFO: ";
		}
		else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
			message << "DEBUG: ";
		}
		message << "[" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg;

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring message_w = converter.from_bytes(message.str());

#ifdef _WIN32
		MessageBox(NULL, message_w.c_str(), L"Alert", MB_OK);
#else
		std::cout << message.str() << std::endl;
#endif
		return false;
	}

}

std::tuple<device_t, swap_chain_t> create_device_and_swapchain(HINSTANCE hinstance, HWND window)
{
	std::vector<const char*> layers = { "VK_LAYER_LUNARG_standard_validation" };
	std::vector<const char*> instance_extension = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };

	VkInstance instance;

	VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.pNext = NULL;
	app_info.pApplicationName = "Nameless app";
	app_info.applicationVersion = 1;
	app_info.pEngineName = "Nameless app";
	app_info.engineVersion = 1;
	app_info.apiVersion = VK_MAKE_VERSION(1, 0, 0);

	VkInstanceCreateInfo info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, &app_info, static_cast<uint32_t>(layers.size()), layers.data(), static_cast<uint32_t>(instance_extension.size()), instance_extension.data() };
	CHECK_VKRESULT(vkCreateInstance(&info, nullptr, &instance));

#ifndef NDEBUG
	PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback;
	PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback;
	VkDebugReportCallbackEXT debug_report_callback;

	dbgCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	assert(dbgCreateDebugReportCallback);
	dbgDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	assert(dbgDestroyDebugReportCallback);

	VkDebugReportCallbackCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	create_info.pNext = nullptr;
	create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	create_info.pfnCallback = dbgFunc;
	create_info.pUserData = nullptr;

	CHECK_VKRESULT(dbgCreateDebugReportCallback(instance, &create_info, NULL, &debug_report_callback));

	/* Clean up callback */
//	dbgDestroyDebugReportCallback(instance, debug_report_callback, NULL);
#endif

	uint32_t device_count;
	CHECK_VKRESULT(vkEnumeratePhysicalDevices(instance, &device_count, nullptr));
	std::vector<VkPhysicalDevice> devices(device_count);
	CHECK_VKRESULT(vkEnumeratePhysicalDevices(instance, &device_count, devices.data()));

	uint32_t queue_count;
	vkGetPhysicalDeviceQueueFamilyProperties(devices[0], &queue_count, nullptr);
	std::vector<VkQueueFamilyProperties> queue_family_properties(queue_count);
	vkGetPhysicalDeviceQueueFamilyProperties(devices[0], &queue_count, queue_family_properties.data());

	VkSurfaceKHR surface;
	VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, 0, hinstance, window };
	CHECK_VKRESULT(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface));

	float queue_priorities = 0.f;
	VkDeviceQueueCreateInfo queue_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, find_graphic_queue_family(devices[0], surface, queue_family_properties), 1, &queue_priorities };
	std::vector<VkDeviceQueueCreateInfo> queue_infos{ queue_info };

	std::vector<const char*> device_extension = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	device_t dev= std::make_shared<vulkan_wrapper::device>(devices[0], queue_infos, layers, device_extension);

	// Get the list of VkFormats that are supported:
	uint32_t format_count;
	CHECK_VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(devices[0], surface, &format_count, nullptr));
	std::vector<VkSurfaceFormatKHR> surface_format(format_count);
	CHECK_VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(devices[0], surface, &format_count, surface_format.data()));

	VkSurfaceCapabilitiesKHR surface_capabilities;
	CHECK_VKRESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[0], surface, &surface_capabilities));

	uint32_t present_mode_count;
	CHECK_VKRESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(devices[0], surface, &present_mode_count, nullptr));
	std::vector<VkPresentModeKHR> present_modes(present_mode_count);
	CHECK_VKRESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(devices[0], surface, &present_mode_count, present_modes.data()));

	VkSwapchainCreateInfoKHR swap_chain = {};
	swap_chain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swap_chain.pNext = nullptr;
	swap_chain.surface = surface;
	swap_chain.minImageCount = 2;
	swap_chain.imageFormat = surface_format[0].format;
	swap_chain.imageExtent.width = surface_capabilities.currentExtent.width;
	swap_chain.imageExtent.height = surface_capabilities.currentExtent.height;
	swap_chain.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swap_chain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swap_chain.imageArrayLayers = 1;
	swap_chain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	swap_chain.oldSwapchain = nullptr;
	swap_chain.clipped = true;
	swap_chain.imageColorSpace = surface_format[0].colorSpace;
	swap_chain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swap_chain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swap_chain.queueFamilyIndexCount = 0;
	swap_chain.pQueueFamilyIndices = nullptr;

	swap_chain_t result = std::make_shared<vulkan_wrapper::swapchain>(dev->object);
	VkSwapchainKHR swap_chain_object;
	VkResult res = (vkCreateSwapchainKHR(dev->object, &swap_chain, nullptr, &swap_chain_object));
	result->object = swap_chain_object;
	result->info = swap_chain;
	return std::make_tuple(dev, result);
}

command_queue_t create_graphic_command_queue(device_t dev)
{
	command_queue_t result = std::make_shared<vulkan_wrapper::queue>();
	result->info.queue_family = dev->queue_create_infos[0].queueFamilyIndex;
	result->info.queue_index = 0;
	vkGetDeviceQueue(dev->object, result->info.queue_family, result->info.queue_index, &(result->object));
	return result;
}

std::vector<image_t> get_image_view_from_swap_chain(device_t dev, swap_chain_t chain)
{
	uint32_t swap_chain_count;
	CHECK_VKRESULT(vkGetSwapchainImagesKHR(dev->object, chain->object, &swap_chain_count, nullptr));
	std::vector<VkImage> swapchain_image(swap_chain_count);
	CHECK_VKRESULT(vkGetSwapchainImagesKHR(dev->object, chain->object, &swap_chain_count, swapchain_image.data()));
	std::vector<image_t> result;
	for (const auto& img : swapchain_image)
	{
		result.emplace_back(std::make_shared<vulkan_wrapper::image>(img));
	}
	return result;
}

command_list_storage_t create_command_storage(device_t dev)
{
	// TODO
	return std::make_shared<vulkan_wrapper::command_pool>(dev->object, 0, 0);
}

command_list_t create_command_list(device_t dev, command_list_storage_t storage)
{
	return std::make_shared<vulkan_wrapper::command_buffer>(dev->object, storage->object, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

buffer_t create_buffer(device_t dev, size_t size)
{
	// TODO: Usage
	return std::make_shared<vulkan_wrapper::buffer>(dev->object, size, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void* map_buffer(device_t dev, buffer_t buffer);
void unmap_buffer(device_t dev, buffer_t buffer);

namespace
{
	VkFormat get_vk_format(irr::video::ECOLOR_FORMAT format)
	{
		switch (format)
		{
		case irr::video::ECF_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
		case irr::video::ECF_R8G8B8A8_UNORM_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
		case irr::video::ECF_R16G16B16A16F: return VK_FORMAT_R16G16B16A16_SSCALED;
		case irr::video::ECF_R32G32B32A32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case irr::video::ECF_A8R8G8B8: return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
		case irr::video::ECF_BC1_UNORM: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case irr::video::ECF_BC1_UNORM_SRGB: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case irr::video::ECF_BC2_UNORM: return VK_FORMAT_BC2_UNORM_BLOCK;
		case irr::video::ECF_BC2_UNORM_SRGB: return VK_FORMAT_BC2_SRGB_BLOCK;
		case irr::video::ECF_BC3_UNORM: return VK_FORMAT_BC3_UNORM_BLOCK;
		case irr::video::ECF_BC3_UNORM_SRGB: return VK_FORMAT_BC3_SRGB_BLOCK;
		case irr::video::ECF_BC4_UNORM: return VK_FORMAT_BC4_UNORM_BLOCK;
		case irr::video::ECF_BC4_SNORM: return VK_FORMAT_BC4_SNORM_BLOCK;
		case irr::video::ECF_BC5_UNORM: return VK_FORMAT_BC5_UNORM_BLOCK;
		case irr::video::ECF_BC5_SNORM: return VK_FORMAT_BC5_SNORM_BLOCK;
		case irr::video::D24U8: return VK_FORMAT_D24_UNORM_S8_UINT;
		}
		return VK_FORMAT_UNDEFINED;
	}

	VkImageUsageFlags get_image_usage_flag(uint32_t flags)
	{
		VkImageUsageFlags result = 0;
		if (flags & usage_depth_stencil)
			result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (flags & usage_render_target)
			result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (flags & usage_transfer_src)
			result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (flags & usage_transfer_dst)
			result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (flags & usage_input_attachment)
			result |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		if (flags & usage_sampled)
			result |= VK_IMAGE_USAGE_SAMPLED_BIT;
		return result;
	}

	VkImageCreateFlags get_image_create_flag(uint32_t flags)
	{
		VkImageUsageFlags result = 0;
		if (flags & usage_cube)
			result |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		return result;
	}
}

image_t create_image(device_t dev, irr::video::ECOLOR_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, uint32_t flags, RESOURCE_USAGE initial_state, clear_value_structure_t*)
{
	VkExtent3D extent{ width, height, 1 };
	// TODO: Storage
	return std::make_shared<vulkan_wrapper::image>(dev->object, get_image_create_flag(flags), VK_IMAGE_TYPE_2D, get_vk_format(format), extent, mipmap, 1,
		VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, get_image_usage_flag(flags), VK_IMAGE_LAYOUT_UNDEFINED);
}


descriptor_storage_t create_descriptor_storage(device_t dev, uint32_t num_descriptors)
{
	// TODO
	std::vector<VkDescriptorPoolSize> descriptor_pool_size;
	return std::make_shared<vulkan_wrapper::descriptor_pool>(dev->object, 0, 8, descriptor_pool_size);
}

void create_constant_buffer_view(device_t dev, descriptor_storage_t storage, uint32_t index, buffer_t buffer, uint32_t buffer_size);
descriptor_storage_t create_sampler_heap(device_t dev, uint32_t num_descriptors);
void create_sampler(device_t dev, descriptor_storage_t storage, uint32_t index, SAMPLER_TYPE sampler_type);
void create_image_view(device_t dev, descriptor_storage_t storage, uint32_t index, image_t img);
void start_command_list_recording(device_t dev, command_list_t command_list, command_list_storage_t storage);

void make_command_list_executable(command_list_t command_list)
{
	CHECK_VKRESULT(vkEndCommandBuffer(command_list->object));
}

void wait_for_command_queue_idle(device_t dev, command_queue_t command_queue)
{
	CHECK_VKRESULT(vkQueueWaitIdle(command_queue->object));
}

void present(device_t dev, swap_chain_t chain);
void set_pipeline_barrier(device_t dev, command_list_t command_list, image_t resource, RESOURCE_USAGE before, RESOURCE_USAGE after, uint32_t subresource);
void clear_color(device_t dev, command_list_t command_list, framebuffer_t framebuffer, const std::array<float, 4> &color);


void clear_depth_stencil(device_t dev, command_list_t command_list, framebuffer_t framebuffer, depth_stencil_aspect aspect, float depth, uint8_t stencil);
void set_viewport(command_list_t command_list, float x, float width, float y, float height, float min_depth, float max_depth)
{
	VkViewport viewport = {};
	viewport.x = x;
	viewport.y = y;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = min_depth;
	viewport.maxDepth = max_depth;
	vkCmdSetViewport(command_list->object, 0, 1, &viewport);
}

void set_scissor(command_list_t command_list, uint32_t left, uint32_t right, uint32_t top, uint32_t bottom)
{
	VkRect2D scissor = {};
	scissor.offset.x = left;
	scissor.offset.y = top;
	scissor.extent.width = right - left;
	scissor.extent.height = bottom - top;
	vkCmdSetScissor(command_list->object, 0, 1, &scissor);
}

namespace
{
	VkIndexType get_index_type(irr::video::E_INDEX_TYPE type)
	{
		switch (type)
		{
		case irr::video::E_INDEX_TYPE::EIT_16BIT: return VK_INDEX_TYPE_UINT16;
		case irr::video::E_INDEX_TYPE::EIT_32BIT: return VK_INDEX_TYPE_UINT32;
		}
		throw;
	}
};

void bind_index_buffer(command_list_t command_list, buffer_t buffer, uint64_t offset, uint32_t size, irr::video::E_INDEX_TYPE type)
{
	vkCmdBindIndexBuffer(command_list->object, buffer->object, offset, get_index_type(type));
}

void bind_vertex_buffers(command_list_t commandlist, uint32_t first_bind, const std::vector<std::tuple<buffer_t, uint64_t, uint32_t, uint32_t> > &buffer_offset_stride_size)
{
	std::vector<VkBuffer> pbuffers(buffer_offset_stride_size.size());
	std::vector<VkDeviceSize> poffsets(buffer_offset_stride_size.size());

	size_t idx = 0;
	for (const auto &infos : buffer_offset_stride_size)
	{
		buffer_t buffer;
		uint64_t offset;
		std::tie(buffer, offset, std::ignore, std::ignore) = infos;
		pbuffers[idx] = buffer->object;
		poffsets[idx] = offset;
		idx++;
	}
	vkCmdBindVertexBuffers(commandlist->object, first_bind, static_cast<uint32_t>(buffer_offset_stride_size.size()), pbuffers.data(), poffsets.data());
}

void submit_executable_command_list(command_queue_t command_queue, command_list_t command_list)
{
	VkSubmitInfo info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	info.commandBufferCount = 1;
	info.pCommandBuffers = &(command_list->object);
	vkQueueSubmit(command_queue->object, 1, &info, VK_NULL_HANDLE);
}

void draw_indexed(command_list_t command_list, uint32_t index_count, uint32_t instance_count, uint32_t base_index, int32_t base_vertex, uint32_t base_instance)
{
	vkCmdDrawIndexed(command_list->object, index_count, instance_count, base_index, base_vertex, base_instance);
}
