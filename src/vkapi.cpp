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

	bool is_device_local_memory(uint32_t properties)
	{
		return !!(properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	bool is_host_visible(uint32_t properties)
	{
		return !!(properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	}

	bool is_host_coherent(uint32_t properties)
	{
		return !!(properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
}

std::tuple<device_t, swap_chain_t, command_queue_t> create_device_swapchain_and_graphic_presentable_queue(HINSTANCE hinstance, HWND window)
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
	device_t dev = std::make_shared<vulkan_wrapper::device>(devices[0], queue_infos, layers, device_extension);

	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(devices[0], &mem_properties);
	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
	{
		if (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			dev->default_memory_index = i;
		if (is_host_visible(mem_properties.memoryTypes[i].propertyFlags) & is_host_coherent(mem_properties.memoryTypes[i].propertyFlags))
			dev->upload_memory_index = i;
	}

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

	swap_chain_t chain = std::make_shared<vulkan_wrapper::swapchain>(dev->object);
	CHECK_VKRESULT(vkCreateSwapchainKHR(dev->object, &swap_chain, nullptr, &(chain->object)));
	chain->info = swap_chain;

	command_queue_t queue = std::make_shared<vulkan_wrapper::queue>();
	queue->info.queue_family = dev->queue_create_infos[0].queueFamilyIndex;
	queue->info.queue_index = 0;
	vkGetDeviceQueue(dev->object, queue->info.queue_family, queue->info.queue_index, &(queue->object));

	return std::make_tuple(dev, chain, queue);
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
	return std::make_shared<vulkan_wrapper::command_pool>(dev->object, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, dev->queue_create_infos[0].queueFamilyIndex);
}

command_list_t create_command_list(device_t dev, command_list_storage_t storage)
{
	return std::make_shared<vulkan_wrapper::command_buffer>(dev->object, storage->object, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

buffer_t create_buffer(device_t dev, size_t size)
{
	// TODO: Usage
	auto buffer = std::make_shared<vulkan_wrapper::buffer>(dev->object, size, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	VkMemoryRequirements mem_req;
	vkGetBufferMemoryRequirements(dev->object, buffer->object, &mem_req);
	buffer->baking_memory = std::make_shared<vulkan_wrapper::memory>(dev->object, mem_req.size, dev->upload_memory_index);
	vkBindBufferMemory(dev->object, buffer->object, buffer->baking_memory->object, 0);
	return buffer;
}

void* map_buffer(device_t dev, buffer_t buffer)
{
	void* ptr;
	CHECK_VKRESULT(vkMapMemory(dev->object, buffer->baking_memory->object, 0, buffer->baking_memory->info.allocationSize, 0, &ptr));
	return ptr;
}

void unmap_buffer(device_t dev, buffer_t buffer)
{
	vkUnmapMemory(dev->object, buffer->baking_memory->object);
}

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

image_t create_image(device_t dev, irr::video::ECOLOR_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, uint32_t flags, clear_value_structure_t*)
{
	VkExtent3D extent{ width, height, 1 };
	auto image = std::make_shared<vulkan_wrapper::image>(dev->object, get_image_create_flag(flags), VK_IMAGE_TYPE_2D, get_vk_format(format), extent, mipmap, 1,
		VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, get_image_usage_flag(flags), VK_IMAGE_LAYOUT_UNDEFINED);
	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(dev->object, image->object, &mem_req);
	image->baking_memory = std::make_shared<vulkan_wrapper::memory>(dev->object, mem_req.size, dev->default_memory_index);
	vkBindImageMemory(dev->object, image->object, image->baking_memory->object, 0);
	return image;
}

void copy_buffer_to_image_subresource(command_list_t list, image_t destination_image, uint32_t destination_subresource, buffer_t source, uint64_t offset_in_buffer, uint32_t width, uint32_t height, uint32_t row_pitch, irr::video::ECOLOR_FORMAT format)
{
	VkBufferImageCopy info{};
	info.bufferOffset = offset_in_buffer;
	info.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	info.imageSubresource.mipLevel = destination_subresource;
	info.imageSubresource.layerCount = 1;
	info.imageExtent.width = width;
	info.imageExtent.height = height;
	info.imageExtent.depth = 1;
	vkCmdCopyBufferToImage(list->object, source->object, destination_image->object, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &info);
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
void start_command_list_recording(device_t dev, command_list_t command_list, command_list_storage_t storage)
{
	VkCommandBufferInheritanceInfo inheritance_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };

	VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	info.pInheritanceInfo = &inheritance_info;
	info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	CHECK_VKRESULT(vkBeginCommandBuffer(command_list->object, &info));
}

void reset_command_list_storage(device_t dev, command_list_storage_t storage)
{
	vkResetCommandPool(dev->object, storage->object, 0);
}


framebuffer_t create_frame_buffer(device_t dev, std::vector<std::tuple<image_t, irr::video::ECOLOR_FORMAT>> render_targets, std::tuple<image_t, irr::video::ECOLOR_FORMAT> depth_stencil_texture, uint32_t width, uint32_t height, render_pass_t render_pass)
{
	return std::make_shared<vk_framebuffer>(dev, render_pass, render_targets, depth_stencil_texture, width, height, 1);
}

void make_command_list_executable(command_list_t command_list)
{
	CHECK_VKRESULT(vkEndCommandBuffer(command_list->object));
}

void wait_for_command_queue_idle(device_t dev, command_queue_t command_queue)
{
	CHECK_VKRESULT(vkQueueWaitIdle(command_queue->object));
}

namespace
{
	VkImageLayout get_image_layout(RESOURCE_USAGE usage)
	{
		switch (usage)
		{
		case RESOURCE_USAGE::PRESENT: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		case RESOURCE_USAGE::RENDER_TARGET: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case RESOURCE_USAGE::READ_GENERIC: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case RESOURCE_USAGE::DEPTH_WRITE: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case RESOURCE_USAGE::COPY_DEST: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case RESOURCE_USAGE::undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
		}
		throw;
	}
}

void set_pipeline_barrier(device_t dev, command_list_t command_list, image_t resource, RESOURCE_USAGE before, RESOURCE_USAGE after, uint32_t subresource)
{
	//Prepare an image to match the new layout..
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.newLayout = get_image_layout(after);
	barrier.oldLayout = get_image_layout(before);
	barrier.image = resource->object;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = subresource;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;

	switch (get_image_layout(after))
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT; break;
	}

	switch (get_image_layout(before))
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT; break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT; break;
	}

	vkCmdPipelineBarrier(command_list->object, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void clear_color(device_t dev, command_list_t command_list, framebuffer_t framebuffer, const std::array<float, 4> &color)
{
	VkClearValue clear_val = {};
	memcpy(clear_val.color.float32, color.data(), 4 * sizeof(float));
	VkClearAttachment attachments = { VK_IMAGE_ASPECT_COLOR_BIT, 0, clear_val };

	VkClearRect clear_rect = {};
	clear_rect.rect.extent.width = 1024;
	clear_rect.rect.extent.height = 1024;
	clear_rect.layerCount = 1;

	vkCmdClearAttachments(command_list->object, static_cast<uint32_t>(framebuffer->fbo.attachements.size()), &attachments, 1, &clear_rect);
}

void set_graphic_pipeline(command_list_t command_list, pipeline_state_t pipeline)
{
	vkCmdBindPipeline(command_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->object);
}


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

uint32_t get_next_backbuffer_id(device_t dev, swap_chain_t chain)
{
	uint32_t index;
	VkFenceCreateInfo info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
	VkFence fence;
	CHECK_VKRESULT(vkCreateFence(dev->object, &info, nullptr, &fence));
	CHECK_VKRESULT(vkAcquireNextImageKHR(dev->object, chain->object, UINT64_MAX, VK_NULL_HANDLE, fence, &index));
	CHECK_VKRESULT(vkWaitForFences(dev->object, 1, &fence, true, UINT64_MAX));
	// TODO: Reuse accross call to gnbi
	vkDestroyFence(dev->object, fence, nullptr);
	return index;
}

void present(device_t dev, command_queue_t cmdqueue, swap_chain_t chain, uint32_t backbuffer_index)
{
	VkPresentInfoKHR info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	info.swapchainCount = 1;
	info.pSwapchains = &(chain->object);
	info.pImageIndices = &backbuffer_index;

	CHECK_VKRESULT(vkQueuePresentKHR(cmdqueue->object, &info));
}

namespace
{
	std::vector<VkImageView> get_image_view_vector(const std::vector<std::shared_ptr<vulkan_wrapper::image_view> > &vectors)
	{
		std::vector<VkImageView> result;
		for (const auto& iv : vectors)
		{
			result.emplace_back(iv->object);
		}
		return result;
	}
}

vk_framebuffer::vk_framebuffer(device_t dev, render_pass_t render_pass, std::vector<std::tuple<image_t, irr::video::ECOLOR_FORMAT>> render_targets, uint32_t width, uint32_t height, uint32_t layers)
	: image_views(build_image_views(dev->object, render_targets)),
	fbo(dev->object, render_pass->object, get_image_view_vector(image_views), width, height, layers)
{
}

vk_framebuffer::vk_framebuffer(device_t dev, render_pass_t render_pass, const std::vector<std::tuple<image_t, irr::video::ECOLOR_FORMAT>> &render_targets,
	const std::tuple<image_t, irr::video::ECOLOR_FORMAT> &depth_stencil, uint32_t width, uint32_t height, uint32_t layers)
	: image_views(build_image_views(dev->object, render_targets, depth_stencil)),
	fbo(dev->object, render_pass->object, get_image_view_vector(image_views), width, height, layers)
{
}

std::vector<std::shared_ptr<vulkan_wrapper::image_view> > vk_framebuffer::build_image_views(VkDevice dev, const std::vector<std::tuple<image_t, irr::video::ECOLOR_FORMAT>> &render_targets)
{
	std::vector<std::shared_ptr<vulkan_wrapper::image_view> >  result;
	for (const auto & rtt_info : render_targets)
	{
		VkComponentMapping default_mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		VkImageSubresourceRange ranges{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		result.emplace_back(
			std::make_shared<vulkan_wrapper::image_view>(dev, std::get<0>(rtt_info)->object, VK_IMAGE_VIEW_TYPE_2D, get_vk_format(std::get<1>(rtt_info)), default_mapping, ranges)
		);
	}
	return result;
}

std::vector<std::shared_ptr<vulkan_wrapper::image_view> > vk_framebuffer::build_image_views(VkDevice dev, const std::vector<std::tuple<image_t, irr::video::ECOLOR_FORMAT>> &render_targets,
	const std::tuple<image_t, irr::video::ECOLOR_FORMAT> &depth_stencil)
{
	std::vector<std::shared_ptr<vulkan_wrapper::image_view> >  result = build_image_views(dev, render_targets);
	VkImageSubresourceRange ranges{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };
	VkComponentMapping default_mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	result.emplace_back(std::make_shared<vulkan_wrapper::image_view>(dev, std::get<0>(depth_stencil)->object, VK_IMAGE_VIEW_TYPE_2D, get_vk_format(std::get<1>(depth_stencil)), default_mapping, ranges));
	return result;
}
