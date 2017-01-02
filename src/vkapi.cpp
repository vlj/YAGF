// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include "../include/API/vkapi.h"
#include <sstream>
#include <set>

#include <locale>
#include <codecvt>
#include <string>

#include <GLFW\glfw3.h>


namespace
{
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
		MessageBox(NULL, message.str().c_str(), "Alert", MB_OK);
#else
		std::cout << message.str() << std::endl;
#endif
		return false;
	}

	constexpr auto is_device_local_memory(uint32_t properties)
	{
		return !!(properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	constexpr auto is_host_visible(uint32_t properties)
	{
		return !!(properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	}

	constexpr auto is_host_coherent(uint32_t properties)
	{
		return !!(properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	auto get_vk_format(irr::video::ECOLOR_FORMAT format)
	{
		switch (format)
		{
		case irr::video::ECF_R8G8B8A8_UNORM: return vk::Format::eR8G8B8A8Unorm;
		case irr::video::ECF_R8G8B8A8_UNORM_SRGB: return vk::Format::eR8G8B8A8Srgb;
		case irr::video::ECF_R16F: return vk::Format::eR16Sfloat;
		case irr::video::ECF_R16G16F: return vk::Format::eR16G16Sfloat;
		case irr::video::ECF_R16G16B16A16F: return vk::Format::eR16G16B16A16Sfloat;
		case irr::video::ECF_R32F: return vk::Format::eR32Sfloat;
		case irr::video::ECF_R32G32F: return vk::Format::eR32G32Sfloat;;
		case irr::video::ECF_R32G32B32A32F: return vk::Format::eR32G32B32A32Sfloat;
		case irr::video::ECF_A8R8G8B8: return vk::Format::eA8B8G8R8UnormPack32;
		case irr::video::ECF_BC1_UNORM: return vk::Format::eBc1RgbaUnormBlock;
		case irr::video::ECF_BC1_UNORM_SRGB: return vk::Format::eBc1RgbaSrgbBlock;
		case irr::video::ECF_BC2_UNORM: return vk::Format::eBc2UnormBlock;
		case irr::video::ECF_BC2_UNORM_SRGB: return vk::Format::eBc2SrgbBlock;
		case irr::video::ECF_BC3_UNORM: return vk::Format::eBc3UnormBlock;
		case irr::video::ECF_BC3_UNORM_SRGB: return vk::Format::eBc3SrgbBlock;
		case irr::video::ECF_BC4_UNORM: return vk::Format::eBc4UnormBlock;
		case irr::video::ECF_BC4_SNORM: return vk::Format::eBc4SnormBlock;
		case irr::video::ECF_BC5_UNORM: return vk::Format::eBc5UnormBlock;
		case irr::video::ECF_BC5_SNORM: return vk::Format::eBc5SnormBlock;
		case irr::video::D24U8: return vk::Format::eD24UnormS8Uint;
		case irr::video::D32U8: return vk::Format::eD32SfloatS8Uint;
		case irr::video::ECF_B8G8R8A8: return vk::Format::eB8G8R8A8Uint;
		case irr::video::ECF_B8G8R8A8_UNORM: return vk::Format::eB8G8R8A8Unorm;
		case irr::video::ECF_B8G8R8A8_UNORM_SRGB: return vk::Format::eB8G8R8A8Srgb;
		case irr::video::ECF_R32G32B32F: return vk::Format::eR32G32B32Sfloat;
		}
		throw;
	}
}

std::tuple<std::unique_ptr<device_t>, std::unique_ptr<swap_chain_t>, std::unique_ptr<command_queue_t>, uint32_t, uint32_t, irr::video::ECOLOR_FORMAT> create_device_swapchain_and_graphic_presentable_queue(GLFWwindow *window)
{
	std::vector<const char*> layers = { 
#ifndef NDEBUG
		"VK_LAYER_LUNARG_standard_validation"
#endif
	};
	std::vector<const char*> instance_extension = {
#ifndef NDEBUG
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
		VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };

	const auto app_info = vk::ApplicationInfo{}
		.setApiVersion(VK_MAKE_VERSION(1, 0, 0))
		.setPApplicationName("Nameless app")
		.setPEngineName("Nameless engine")
		.setApplicationVersion(1)
		.setEngineVersion(1);

	const auto instance = vk::createInstance(
		vk::InstanceCreateInfo{}
			.setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
			.setPpEnabledLayerNames(layers.data())
			.setEnabledExtensionCount(static_cast<uint32_t>(instance_extension.size()))
			.setPpEnabledExtensionNames(instance_extension.data())
			.setPApplicationInfo(&app_info)
	);

#ifndef NDEBUG
	PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback{};
	PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback{};
	VkDebugReportCallbackEXT debug_report_callback{};

	dbgCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	assert(dbgCreateDebugReportCallback);
	dbgDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	assert(dbgDestroyDebugReportCallback);

	VkDebugReportCallbackCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	create_info.pNext = nullptr;
	create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	create_info.pfnCallback = &dbgFunc;
	create_info.pUserData = nullptr;

	CHECK_VKRESULT(dbgCreateDebugReportCallback(instance, &create_info, NULL, &debug_report_callback));

	/* Clean up callback */
//	dbgDestroyDebugReportCallback(instance, debug_report_callback, NULL);
#endif

	const auto& devices = instance.enumeratePhysicalDevices();
	const auto& queue_family_properties = devices[0].getQueueFamilyProperties();

	VkSurfaceKHR _surface;
	glfwCreateWindowSurface(instance, window, nullptr, &_surface);
	vk::SurfaceKHR surface(_surface);

	const auto queue_family_index = [&]()
	{
		for (unsigned int i = 0; i < queue_family_properties.size(); i++) {
			if (queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
				if (devices[0].getSurfaceSupportKHR(i, surface)) return i;
			}
		}
		throw;
	}();
	float queue_priorities = 0.f;
	const auto queue_infos = std::array<vk::DeviceQueueCreateInfo, 1> {
		vk::DeviceQueueCreateInfo{}
			.setQueueFamilyIndex(queue_family_index)
			.setQueueCount(1)
			.setPQueuePriorities(&queue_priorities)
	};

	const auto& extensions_list = devices[0].enumerateDeviceExtensionProperties(nullptr);

	const auto& has_extension = [&]() {
		const auto& It = std::find_if(extensions_list.begin(), extensions_list.end(), [](const vk::ExtensionProperties& ext) { return strcmp(ext.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME); });
		return It != extensions_list.end();
	}();

	auto&& device_extension = std::vector<const char*>{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	if (has_extension) device_extension.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
	auto dev = devices[0].createDevice(
		vk::DeviceCreateInfo{}
		.setEnabledExtensionCount(static_cast<uint32_t>(device_extension.size()))
		.setPpEnabledExtensionNames(device_extension.data())
		.setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
		.setPpEnabledLayerNames(layers.data())
		.setPQueueCreateInfos(queue_infos.data())
		.setQueueCreateInfoCount(static_cast<uint32_t>(queue_infos.size()))
	);

	auto mem_properties = devices[0].getMemoryProperties();

	auto surface_format = devices[0].getSurfaceFormatsKHR(surface);
	auto surface_capabilities = devices[0].getSurfaceCapabilitiesKHR(surface);
	auto present_modes = devices[0].getSurfacePresentModesKHR(surface);

	auto swap_chain = vk::SwapchainCreateInfoKHR{}
		.setSurface(surface)
		.setMinImageCount(2)
		.setImageFormat(surface_format[0].format)
		.setImageExtent(surface_capabilities.currentExtent)
		.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setImageArrayLayers(1)
		.setPresentMode(vk::PresentModeKHR::eFifo)
		.setClipped(true)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
		.setImageSharingMode(vk::SharingMode::eExclusive);

	auto chain = dev.createSwapchainKHR(swap_chain);

	auto&& wrapped_dev = std::make_unique<vk_device_t>(dev);
	wrapped_dev->mem_properties = devices[0].getMemoryProperties();
	wrapped_dev->queue_family_index = queue_family_index;

	auto queue = dev.getQueue(queue_infos[0].queueFamilyIndex, 0);
	const auto& fmt = [&]() {
		switch (surface_format[0].format)
		{
		case vk::Format::eB8G8R8A8Unorm: return irr::video::ECF_B8G8R8A8_UNORM;
		case vk::Format::eR8G8B8A8Unorm: return irr::video::ECF_R8G8B8A8_UNORM;
		case vk::Format::eB8G8R8A8Srgb: return irr::video::ECF_B8G8R8A8_UNORM_SRGB;
		case vk::Format::eR8G8B8A8Srgb: return irr::video::ECF_R8G8B8A8_UNORM_SRGB;
		}
		throw;
	}();
	return std::make_tuple(
		std::move(wrapped_dev),
		std::unique_ptr<swap_chain_t>(new vk_swap_chain_t(dev, chain)),
		std::unique_ptr<command_queue_t>(new vk_command_queue_t(queue)),
		surface_capabilities.currentExtent.width, surface_capabilities.currentExtent.height, fmt);
}

std::vector<std::unique_ptr<image_t>> vk_swap_chain_t::get_image_view_from_swap_chain()
{
	auto swapchain_image = dev.getSwapchainImagesKHR(object);
	auto result = std::vector<std::unique_ptr<image_t>>{};
	std::transform(swapchain_image.begin(), swapchain_image.end(), std::back_inserter(result),
		[this](auto&& img) { return std::unique_ptr<image_t>(new vk_image_t(dev, img, vk::DeviceMemory(), 1)); });
	return result;
}

std::unique_ptr<command_list_t> vk_command_list_storage_t::create_command_list()
{
	const auto& buffers = dev.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo{}
		.setCommandBufferCount(1)
		.setCommandPool(object)
		.setLevel(vk::CommandBufferLevel::ePrimary)
	);
	return std::unique_ptr<command_list_t>(new vk_command_list_t(dev, buffers[0]));
}

std::unique_ptr<command_list_storage_t> vk_device_t::create_command_storage()
{
	return std::unique_ptr<command_list_storage_t>(new vk_command_list_storage_t(
		object, object.createCommandPool(
			vk::CommandPoolCreateInfo{}
				.setQueueFamilyIndex(queue_family_index)
				.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		)
	));
}

namespace
{
	uint32_t getMemoryTypeIndex(uint32_t typeBits, const vk::PhysicalDeviceMemoryProperties &memprops, vk::MemoryPropertyFlags properties)
	{
		// Iterate over all memory types available for the device used in this example
		for (uint32_t i = 0; i < memprops.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((memprops.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}
			typeBits >>= 1;
		}
		throw "Could not find a suitable memory type!";
	}

	auto get_buffer_usage_flags(uint32_t flags)
	{
		auto result = vk::BufferUsageFlags();
		if (flags & usage_uniform)
			result |= vk::BufferUsageFlagBits::eUniformBuffer;
		if (flags & usage_uav)
			result |= vk::BufferUsageFlagBits::eStorageBuffer;
		if (flags & usage_transfer_dst)
			result |= vk::BufferUsageFlagBits::eTransferDst;
		if (flags & usage_texel_buffer)
			result |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
		if (flags & usage_buffer_transfer_src)
			result |= vk::BufferUsageFlagBits::eTransferSrc;
		if (flags & usage_index)
			result |= vk::BufferUsageFlagBits::eIndexBuffer;
		if (flags & usage_vertex)
			result |= vk::BufferUsageFlagBits::eVertexBuffer;
		return result;
	}
}

std::unique_ptr<buffer_t> vk_device_t::create_buffer(size_t size, irr::video::E_MEMORY_POOL memory_pool, uint32_t flags)
{
	const auto& buffer = object.createBuffer(
		vk::BufferCreateInfo{}
			.setSize(size)
			.setUsage(get_buffer_usage_flags(flags))
	);
	const auto& mem_req = object.getBufferMemoryRequirements(buffer);
	const auto& memory = object.allocateMemory(
		vk::MemoryAllocateInfo{}
			.setAllocationSize(mem_req.size)
			.setMemoryTypeIndex(getMemoryTypeIndex(mem_req.memoryTypeBits, mem_properties, vk::MemoryPropertyFlagBits::eHostCoherent))
	);
	object.bindBufferMemory(buffer, memory, 0);
	return  std::unique_ptr<buffer_t>(
		new vk_buffer_t(object, buffer, memory));
}

std::unique_ptr<descriptor_storage_t> vk_device_t::create_descriptor_storage(uint32_t num_sets, const std::vector<std::tuple<RESOURCE_VIEW, uint32_t> > &num_descriptors)
{
	std::vector<vk::DescriptorPoolSize> poolSizes;

	std::transform(num_descriptors.begin(), num_descriptors.end(), std::back_inserter(poolSizes),
		[](auto&& set) { return vk::DescriptorPoolSize(get_descriptor_type(std::get<0>(set)), std::get<1>(set)); });
	return std::unique_ptr<descriptor_storage_t>(
		new vk_descriptor_storage_t(object,
			object.createDescriptorPool(
				vk::DescriptorPoolCreateInfo{}
					.setMaxSets(num_sets)
					.setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
					.setPPoolSizes(poolSizes.data())
			)
		)
	);
}

void* vk_buffer_t::map_buffer()
{
	return dev.mapMemory(memory, 0, -1);
}

void vk_buffer_t::unmap_buffer()
{
	dev.unmapMemory(memory);
}

std::unique_ptr<buffer_view_t> vk_device_t::create_buffer_view(buffer_t& buffer, irr::video::ECOLOR_FORMAT format, uint64_t offset, uint32_t size)
{
	return std::unique_ptr<buffer_view_t>(new vk_buffer_view_t(object, object.createBufferView(
		vk::BufferViewCreateInfo{}
			.setBuffer(dynamic_cast<vk_buffer_t&>(buffer).object)
			.setFormat(get_vk_format(format))
			.setOffset(offset)
			.setRange(size)
		)
	));
}

void vk_device_t::set_uniform_texel_buffer_view(const allocated_descriptor_set& descriptor_set, uint32_t, uint32_t binding_location, buffer_view_t& buffer_view)
{
	const auto bv = dynamic_cast<vk_buffer_view_t&>(buffer_view).object;
	object.updateDescriptorSets({
		vk::WriteDescriptorSet{}
			.setDstSet(static_cast<const vk_allocated_descriptor_set&>(descriptor_set).object)
			.setDstBinding(binding_location)
			.setDescriptorCount(1)
			.setPTexelBufferView(&bv)
			.setDescriptorType(vk::DescriptorType::eUniformTexelBuffer)
	}, {});
}

void vk_device_t::set_uav_buffer_view(const allocated_descriptor_set& descriptor_set, uint32_t, uint32_t binding_location, buffer_t& buffer, uint64_t offset, uint32_t size)
{
	const auto buffer_descriptor = vk::DescriptorBufferInfo(dynamic_cast<vk_buffer_t&>(buffer).object, 0, size);
	object.updateDescriptorSets({
		vk::WriteDescriptorSet{}
			.setDstSet(static_cast<const vk_allocated_descriptor_set&>(descriptor_set).object)
			.setDstBinding(binding_location)
			.setDescriptorCount(1)
			.setPBufferInfo(&buffer_descriptor)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
	}, {});
}

void vk_device_t::set_constant_buffer_view(const allocated_descriptor_set& descriptor_set, uint32_t offset_in_set, uint32_t binding_location, buffer_t& buffer, uint32_t buffer_size, uint64_t offset)
{
	const auto buffer_descriptor = vk::DescriptorBufferInfo(dynamic_cast<vk_buffer_t&>(buffer).object, offset, buffer_size);
	object.updateDescriptorSets({
		vk::WriteDescriptorSet{}
			.setDstSet(static_cast<const vk_allocated_descriptor_set&>(descriptor_set).object)
			.setDstBinding(binding_location)
			.setDescriptorCount(1)
			.setPBufferInfo(&buffer_descriptor)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
	}, {});
}

clear_value_t get_clear_value(irr::video::ECOLOR_FORMAT format, float depth, uint8_t stencil)
{
	return std::make_tuple(depth, stencil);
}

clear_value_t get_clear_value(irr::video::ECOLOR_FORMAT format, const std::array<float, 4> &color)
{
	return color;
}

std::unique_ptr<image_t> vk_device_t::create_image(irr::video::ECOLOR_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, uint32_t layers, uint32_t flags, clear_value_t*)
{
	const auto& get_image_create_flag = [](auto flags)
	{
		auto result = vk::ImageCreateFlags();
		if (flags & usage_cube)
			result |= vk::ImageCreateFlagBits::eCubeCompatible;
		return result;
	};

	const auto& get_image_usage = [&]()
	{
		auto&& result = vk::ImageUsageFlags();
		if (flags & usage_transfer_src)
			result |= vk::ImageUsageFlagBits::eTransferSrc;
		if (flags & usage_depth_stencil)
			result |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
		if (flags & usage_sampled)
			result |= vk::ImageUsageFlagBits::eSampled;
		if (flags & usage_transfer_dst)
			result |= vk::ImageUsageFlagBits::eTransferDst;
		if (flags & usage_render_target)
			result |= vk::ImageUsageFlagBits::eColorAttachment;
		if (flags & usage_input_attachment)
			result |= vk::ImageUsageFlagBits::eInputAttachment;
		if (flags & usage_uav)
			result |= vk::ImageUsageFlagBits::eStorage;
		return result;
	};

	const auto& image = object.createImage(
		vk::ImageCreateInfo{}
			.setArrayLayers(layers)
			.setMipLevels(mipmap)
			.setTiling(vk::ImageTiling::eOptimal)
			.setExtent(vk::Extent3D(width, height, 1))
			.setImageType(vk::ImageType::e2D)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFormat(get_vk_format(format))
			.setFlags(get_image_create_flag(flags))
			.setUsage(get_image_usage())
			.setSamples(vk::SampleCountFlagBits::e1)
	);
	const auto& mem_req = object.getImageMemoryRequirements(image);
	const auto& memory = object.allocateMemory(
		vk::MemoryAllocateInfo{}
			.setAllocationSize(mem_req.size)
			.setMemoryTypeIndex(getMemoryTypeIndex(mem_req.memoryTypeBits, mem_properties, vk::MemoryPropertyFlagBits::eDeviceLocal))
	);
	object.bindImageMemory(image, memory, 0);
	return std::unique_ptr<image_t>(new vk_image_t(object, image, memory, mipmap));
}

namespace
{
	auto get_image_type(irr::video::E_TEXTURE_TYPE texture_type)
	{
		switch (texture_type)
		{
		case irr::video::E_TEXTURE_TYPE::ETT_2D: return vk::ImageViewType::e2D;
		case irr::video::E_TEXTURE_TYPE::ETT_CUBE: return vk::ImageViewType::eCube;
		}
		throw;
	}

	vk::ImageAspectFlags get_image_aspect(irr::video::E_ASPECT aspect)
	{
		switch (aspect)
		{
		case irr::video::E_ASPECT::EA_COLOR: return vk::ImageAspectFlagBits::eColor;
		case irr::video::E_ASPECT::EA_DEPTH: return vk::ImageAspectFlagBits::eDepth;
		case irr::video::E_ASPECT::EA_STENCIL: return vk::ImageAspectFlagBits::eStencil;
		case irr::video::E_ASPECT::EA_DEPTH_STENCIL: return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		}
		throw;
	}
}

std::unique_ptr<image_view_t> vk_device_t::create_image_view(image_t& img, irr::video::ECOLOR_FORMAT fmt, uint16_t base_mipmap, uint16_t mipmap_count, uint16_t base_layer, uint16_t layer_count, irr::video::E_TEXTURE_TYPE texture_type, irr::video::E_ASPECT aspect)
{
	const auto& image_view = object.createImageView(
		vk::ImageViewCreateInfo{}
			.setViewType(get_image_type(texture_type))
			.setFormat(get_vk_format(fmt))
			.setImage(dynamic_cast<vk_image_t&>(img).object)
			.setComponents(vk::ComponentMapping())
			.setSubresourceRange(vk::ImageSubresourceRange(get_image_aspect(aspect), base_mipmap, mipmap_count, base_layer, layer_count))
	);
	return std::unique_ptr<image_view_t>(new vk_image_view_t(object, image_view));
}

void vk_device_t::set_image_view(const allocated_descriptor_set& descriptor_set, uint32_t offset, uint32_t binding_location, image_view_t& img_view)
{
	const auto image_descriptor = vk::DescriptorImageInfo(vk::Sampler(), dynamic_cast<vk_image_view_t&>(img_view).object, vk::ImageLayout::eShaderReadOnlyOptimal);
	object.updateDescriptorSets({
		vk::WriteDescriptorSet{}
		.setDstSet(static_cast<const vk_allocated_descriptor_set&>(descriptor_set).object)
		.setDstBinding(binding_location)
		.setDescriptorCount(1)
		.setPImageInfo(&image_descriptor)
		.setDescriptorType(vk::DescriptorType::eSampledImage)
	}, {});
}

void vk_device_t::set_input_attachment(const allocated_descriptor_set& descriptor_set, uint32_t offset, uint32_t binding_location, image_view_t& img_view)
{
	const auto image_descriptor = vk::DescriptorImageInfo(vk::Sampler(), dynamic_cast<vk_image_view_t&>(img_view).object, vk::ImageLayout::eShaderReadOnlyOptimal);
	object.updateDescriptorSets({
		vk::WriteDescriptorSet{}
		.setDstSet(static_cast<const vk_allocated_descriptor_set&>(descriptor_set).object)
		.setDstBinding(binding_location)
		.setDescriptorCount(1)
		.setPImageInfo(&image_descriptor)
		.setDescriptorType(vk::DescriptorType::eInputAttachment)
	}, {});
}

void vk_device_t::set_uav_image_view(const allocated_descriptor_set& descriptor_set, uint32_t offset, uint32_t binding_location, image_view_t& img_view)
{
	const auto image_descriptor = vk::DescriptorImageInfo(vk::Sampler(), dynamic_cast<vk_image_view_t&>(img_view).object, vk::ImageLayout::eGeneral);
	object.updateDescriptorSets({
		vk::WriteDescriptorSet{}
		.setDstSet(static_cast<const vk_allocated_descriptor_set&>(descriptor_set).object)
		.setDstBinding(binding_location)
		.setDescriptorCount(1)
		.setPImageInfo(&image_descriptor)
		.setDescriptorType(vk::DescriptorType::eStorageImage)
	}, {});
}

std::unique_ptr<sampler_t> vk_device_t::create_sampler(SAMPLER_TYPE sampler_type)
{
	const auto& get_sampler_desc = [](auto&& sampler_type)
	{
		switch (sampler_type)
		{
		case SAMPLER_TYPE::ANISOTROPIC:
			return vk::SamplerCreateInfo{}
				.setMinFilter(vk::Filter::eLinear)
				.setMagFilter(vk::Filter::eLinear)
				.setMipmapMode(vk::SamplerMipmapMode::eLinear)
				.setAddressModeU(vk::SamplerAddressMode::eRepeat)
				.setAddressModeV(vk::SamplerAddressMode::eRepeat)
				.setAddressModeW(vk::SamplerAddressMode::eRepeat)
				.setMaxLod(100.f)
				.setAnisotropyEnable(true)
				.setMaxAnisotropy(16.f);
		case SAMPLER_TYPE::BILINEAR_CLAMPED:
			return vk::SamplerCreateInfo{}
				.setMinFilter(vk::Filter::eLinear)
				.setMagFilter(vk::Filter::eLinear)
				.setMipmapMode(vk::SamplerMipmapMode::eNearest)
				.setAddressModeU(vk::SamplerAddressMode::eRepeat)
				.setAddressModeV(vk::SamplerAddressMode::eRepeat)
				.setAddressModeW(vk::SamplerAddressMode::eRepeat)
				.setMaxLod(0.f)
				.setAnisotropyEnable(false);
		case SAMPLER_TYPE::TRILINEAR:
			return vk::SamplerCreateInfo{}
				.setMinFilter(vk::Filter::eLinear)
				.setMagFilter(vk::Filter::eLinear)
				.setMipmapMode(vk::SamplerMipmapMode::eLinear)
				.setAddressModeU(vk::SamplerAddressMode::eRepeat)
				.setAddressModeV(vk::SamplerAddressMode::eRepeat)
				.setAddressModeW(vk::SamplerAddressMode::eRepeat)
				.setMaxLod(100.f)
				.setAnisotropyEnable(false);
		case SAMPLER_TYPE::NEAREST:
			return vk::SamplerCreateInfo{}
				.setMinFilter(vk::Filter::eNearest)
				.setMagFilter(vk::Filter::eNearest)
				.setMipmapMode(vk::SamplerMipmapMode::eNearest)
				.setAddressModeU(vk::SamplerAddressMode::eRepeat)
				.setAddressModeV(vk::SamplerAddressMode::eRepeat)
				.setAddressModeW(vk::SamplerAddressMode::eRepeat)
				.setMaxLod(0.f)
				.setAnisotropyEnable(false);
		}
		throw;
	};
	return std::unique_ptr<sampler_t>(new vk_sampler_t(object, object.createSampler(get_sampler_desc(sampler_type))));
}

void vk_device_t::set_sampler(const allocated_descriptor_set& descriptor_set, uint32_t offset, uint32_t binding_location, sampler_t& sampler)
{
	const auto sampler_descriptor = vk::DescriptorImageInfo(dynamic_cast<vk_sampler_t&>(sampler).object, vk::ImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	object.updateDescriptorSets({
		vk::WriteDescriptorSet{}
		.setDstSet(static_cast<const vk_allocated_descriptor_set&>(descriptor_set).object)
		.setDstBinding(binding_location)
		.setDescriptorCount(1)
		.setPImageInfo(&sampler_descriptor)
		.setDescriptorType(vk::DescriptorType::eSampler)
	}, {});
}

void vk_command_list_t::copy_buffer_to_image_subresource(image_t& destination_image, uint32_t destination_subresource, buffer_t& source, uint64_t offset_in_buffer, uint32_t width, uint32_t height, uint32_t row_pitch, irr::video::ECOLOR_FORMAT format)
{
	const auto& mipLevel = destination_subresource % max(dynamic_cast<vk_image_t&>(destination_image).mip_levels, 1);
	const auto& baseArrayLayer = destination_subresource / max(dynamic_cast<vk_image_t&>(destination_image).mip_levels, 1);

	object.copyBufferToImage(dynamic_cast<vk_buffer_t&>(source).object, dynamic_cast<vk_image_t&>(destination_image).object, vk::ImageLayout::eTransferDstOptimal,
		{
			vk::BufferImageCopy(offset_in_buffer, width, height, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mipLevel, baseArrayLayer, 1), vk::Offset3D(), vk::Extent3D(width, height, 1))
	});
}

void vk_command_list_t::start_command_list_recording(command_list_storage_t&)
{
	object.begin(
		vk::CommandBufferBeginInfo{}
			.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
	);
}

struct clear_value_visitor {
	auto operator()(const std::array<float, 4> &colors) const {
		return vk::ClearValue(vk::ClearColorValue(colors));
	}

	auto operator()(const std::tuple<float, uint8_t> &depth_stencil) const {
		return vk::ClearValue(vk::ClearDepthStencilValue(std::get<0>(depth_stencil), std::get<1>(depth_stencil)));
	}
};

void vk_command_list_t::begin_renderpass(render_pass_t& rp, framebuffer_t &fbo, gsl::span<clear_value_t> clear_values, uint32_t width, uint32_t height)
{
	std::vector<vk::ClearValue> clearValues;
	std::transform(clear_values.begin(), clear_values.end(), std::back_inserter(clearValues),
		[&](auto&& v) { return std::visit(clear_value_visitor(), v); });
	object.beginRenderPass(
		vk::RenderPassBeginInfo{}
			.setFramebuffer(dynamic_cast<vk_framebuffer&>(fbo).object)
			.setRenderPass(dynamic_cast<vk_render_pass_t&>(rp).object)
			.setRenderArea(vk::Rect2D(vk::Offset2D(), vk::Extent2D(width, height)))
			.setPClearValues(clearValues.data())
			.setClearValueCount(static_cast<uint32_t>(clearValues.size())),
		vk::SubpassContents::eInline
	);
}

void vk_command_list_t::clear_depth_stencil(image_t & img, float depth)
{
	vk::ClearDepthStencilValue clear_values{};
	clear_values.depth = depth;
	object.clearDepthStencilImage(dynamic_cast<vk_image_t&>(img).object, vk::ImageLayout::eTransferDstOptimal,
		clear_values, { vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1) });
}

void vk_command_list_t::clear_depth_stencil(image_t & img, uint8_t stencil)
{
	vk::ClearDepthStencilValue clear_values{};
	clear_values.stencil = stencil;
	object.clearDepthStencilImage(dynamic_cast<vk_image_t&>(img).object, vk::ImageLayout::eTransferDstOptimal,
		clear_values, { vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1) });
}

void vk_command_list_t::clear_depth_stencil(image_t & img, float depth, uint8_t stencil)
{
	vk::ClearDepthStencilValue clear_values{};
	clear_values.depth = depth;
	clear_values.stencil = stencil;
	object.clearDepthStencilImage(dynamic_cast<vk_image_t&>(img).object, vk::ImageLayout::eTransferDstOptimal,
		clear_values, { vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1) });
}

void vk_command_list_t::clear_color(image_t &img, const std::array<float, 4>& clear_colors)
{
	vk::ClearColorValue clear_values{};
	clear_values.setFloat32(clear_colors);
	object.clearColorImage(dynamic_cast<vk_image_t&>(img).object, vk::ImageLayout::eTransferDstOptimal,
		clear_values, { vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1) });
}

void vk_command_list_storage_t::reset_command_list_storage()
{
	dev.resetCommandPool(object, vk::CommandPoolResetFlags());
}

void vk_command_list_t::make_command_list_executable()
{
	object.end();
}

void vk_command_queue_t::wait_for_command_queue_idle()
{
	object.waitIdle();
}

void vk_command_list_t::set_pipeline_barrier(image_t& resource, RESOURCE_USAGE before, RESOURCE_USAGE after, uint32_t subresource, irr::video::E_ASPECT aspect)
{
	const auto& baseMipLevel = subresource % max(dynamic_cast<vk_image_t&>(resource).mip_levels, 1);
	const auto& baseArrayLayer = subresource / max(dynamic_cast<vk_image_t&>(resource).mip_levels, 1);

	const auto& get_image_layout = [](auto&& usage)
	{
		switch (usage)
		{
		case RESOURCE_USAGE::PRESENT: return vk::ImageLayout::ePresentSrcKHR;
		case RESOURCE_USAGE::RENDER_TARGET: return vk::ImageLayout::eColorAttachmentOptimal;
		case RESOURCE_USAGE::READ_GENERIC: return vk::ImageLayout::eShaderReadOnlyOptimal;
		case RESOURCE_USAGE::DEPTH_WRITE: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
		case RESOURCE_USAGE::COPY_DEST: return vk::ImageLayout::eTransferDstOptimal;
		case RESOURCE_USAGE::uav: return vk::ImageLayout::eGeneral;
		case RESOURCE_USAGE::undefined: return vk::ImageLayout::eUndefined;
		}
		throw;
	};

	auto&& srcAccess = [=]() -> vk::AccessFlags
	{
		switch (get_image_layout(before))
		{
		case vk::ImageLayout::eTransferDstOptimal:
			return vk::AccessFlagBits::eTransferWrite;
		case vk::ImageLayout::eTransferSrcOptimal:
			return vk::AccessFlagBits::eTransferRead;
		case vk::ImageLayout::ePresentSrcKHR:
			return vk::AccessFlagBits::eMemoryRead;
		case vk::ImageLayout::eColorAttachmentOptimal:
			return vk::AccessFlagBits::eColorAttachmentWrite;
		case vk::ImageLayout::eDepthStencilAttachmentOptimal:
			return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		case vk::ImageLayout::eShaderReadOnlyOptimal:
			return vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
		}
		return vk::AccessFlags();
	};

	auto&& dstAccess = [=]() -> vk::AccessFlags
	{
		switch (get_image_layout(after))
		{
		case vk::ImageLayout::eTransferDstOptimal:
			return vk::AccessFlagBits::eTransferWrite;
		case vk::ImageLayout::eTransferSrcOptimal:
			return vk::AccessFlagBits::eTransferRead;
		case vk::ImageLayout::ePresentSrcKHR:
			return vk::AccessFlagBits::eMemoryRead;
		case vk::ImageLayout::eColorAttachmentOptimal:
			return vk::AccessFlagBits::eColorAttachmentWrite;
		case vk::ImageLayout::eDepthStencilAttachmentOptimal:
			return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		case vk::ImageLayout::eShaderReadOnlyOptimal:
			return vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
		}
		return vk::AccessFlags();
	};

	auto dst = dstAccess();
	auto src = srcAccess();

	object.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::DependencyFlags(), {}, {},
		{
			vk::ImageMemoryBarrier{}
				.setOldLayout(get_image_layout(before))
				.setNewLayout(get_image_layout(after))
				.setSrcAccessMask(src)
				.setDstAccessMask(dst)
				.setImage(dynamic_cast<vk_image_t&>(resource).object)
				.setSubresourceRange(vk::ImageSubresourceRange(get_image_aspect(aspect), baseMipLevel, 1, baseArrayLayer, 1))
		});
}

void vk_command_list_t::set_uav_flush(image_t& resource)
{
	object.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::DependencyFlags(), {}, {},
		{
			vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eGeneral)
				.setNewLayout(vk::ImageLayout::eGeneral)
				.setImage(dynamic_cast<vk_image_t&>(resource).object)
				.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead)
				.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead)
				.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
		});
}

void vk_command_list_t::set_graphic_pipeline(pipeline_state_t& pipeline)
{
	object.bindPipeline(vk::PipelineBindPoint::eGraphics, static_cast<vk_pipeline_state_t&>(pipeline).object);
}

void vk_command_list_t::set_compute_pipeline(compute_pipeline_state_t& pipeline)
{
	object.bindPipeline(vk::PipelineBindPoint::eCompute, static_cast<vk_compute_pipeline_state_t&>(pipeline).object);
}

void vk_command_list_t::set_graphic_pipeline_layout(pipeline_layout_t&) {}

void vk_command_list_t::set_compute_pipeline_layout(pipeline_layout_t&) {}

void vk_command_list_t::set_descriptor_storage_referenced(descriptor_storage_t&, descriptor_storage_t*) {}

void vk_command_list_t::set_viewport(float x, float width, float y, float height, float min_depth, float max_depth)
{
	object.setViewport(0, { vk::Viewport(x, y, width, height, min_depth, max_depth) });
}

void vk_command_list_t::set_scissor(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom)
{
	object.setScissor(0, { vk::Rect2D(vk::Offset2D(left, top), vk::Extent2D(right - left, bottom - top)) });
}

std::unique_ptr<allocated_descriptor_set> vk_descriptor_storage_t::allocate_descriptor_set_from_cbv_srv_uav_heap(uint32_t, const std::vector<descriptor_set_layout*> layout, uint32_t)
{
	std::vector<vk::DescriptorSetLayout> set_layouts;
	std::transform(layout.begin(), layout.end(), std::back_inserter(set_layouts),
		[](auto&& tmp) { return dynamic_cast<vk_descriptor_set_layout*>(tmp)->object; });
	return std::unique_ptr<allocated_descriptor_set>(
		new vk_allocated_descriptor_set(
			dev.allocateDescriptorSets(
				vk::DescriptorSetAllocateInfo{}
					.setDescriptorPool(object)
					.setDescriptorSetCount(static_cast<uint32_t>(set_layouts.size()))
					.setPSetLayouts(set_layouts.data()))[0])
		);
}

std::unique_ptr<allocated_descriptor_set> vk_descriptor_storage_t::allocate_descriptor_set_from_sampler_heap(uint32_t, const std::vector<descriptor_set_layout*> layouts, uint32_t)
{
	return allocate_descriptor_set_from_cbv_srv_uav_heap(0, layouts, 0);
}

void vk_command_list_t::bind_graphic_descriptor(uint32_t bindpoint, const allocated_descriptor_set & descriptor_set, pipeline_layout_t& sig)
{
	object.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, dynamic_cast<vk_pipeline_layout_t&>(sig).object, bindpoint,
		{ static_cast<const vk_allocated_descriptor_set&>(descriptor_set).object }, { });
}
 
void vk_command_list_t::bind_compute_descriptor(uint32_t bindpoint, const allocated_descriptor_set & descriptor_set, pipeline_layout_t& sig)
{
	object.bindDescriptorSets(vk::PipelineBindPoint::eCompute, dynamic_cast<vk_pipeline_layout_t&>(sig).object, bindpoint,
		{ static_cast<const vk_allocated_descriptor_set&>(descriptor_set).object }, { });
}

namespace
{
	auto get_index_type(irr::video::E_INDEX_TYPE type)
	{
		switch (type)
		{
		case irr::video::E_INDEX_TYPE::EIT_16BIT: return vk::IndexType::eUint16;
		case irr::video::E_INDEX_TYPE::EIT_32BIT: return vk::IndexType::eUint32;
		}
		throw;
	}
};

void vk_command_list_t::bind_index_buffer(buffer_t& buffer, uint64_t offset, uint32_t size, irr::video::E_INDEX_TYPE type)
{
	object.bindIndexBuffer(dynamic_cast<vk_buffer_t&>(buffer).object, offset, get_index_type(type));
}

void vk_command_list_t::bind_vertex_buffers(uint32_t first_bind, const std::vector<std::tuple<buffer_t&, uint64_t, uint32_t, uint32_t> > &buffer_offset_stride_size)
{
	std::vector<vk::Buffer> pbuffers(buffer_offset_stride_size.size());
	std::vector<VkDeviceSize> poffsets(buffer_offset_stride_size.size());

	size_t idx = 0;
	for (const auto &infos : buffer_offset_stride_size)
	{
		uint64_t offset;
		std::tie(std::ignore, offset, std::ignore, std::ignore) = infos;
		pbuffers[idx] = dynamic_cast<vk_buffer_t&>(std::get<0>(infos)).object;
		poffsets[idx] = offset;
		idx++;
	}
	object.bindVertexBuffers(first_bind, pbuffers, poffsets);
}

void vk_command_queue_t::submit_executable_command_list(command_list_t& command_list, semaphore_t* wait_sem)
{
	std::array<vk::CommandBuffer, 1> command_buffers{ dynamic_cast<vk_command_list_t&>(command_list).object };
	const auto& wait_semaphores = wait_sem != nullptr ?
		std::vector<vk::Semaphore>{ dynamic_cast<vk_semaphore_t*>(wait_sem)->object } :
		std::vector<vk::Semaphore>{};
	const auto& wait_stages = wait_sem != nullptr ?
		std::vector<vk::PipelineStageFlags> {vk::PipelineStageFlagBits::eTopOfPipe} :
		std::vector<vk::PipelineStageFlags>{};
	object.submit({
		vk::SubmitInfo{}
			.setCommandBufferCount(static_cast<uint32_t>(command_buffers.size()))
			.setPCommandBuffers(command_buffers.data())
			.setWaitSemaphoreCount(static_cast<uint32_t>(wait_semaphores.size()))
			.setPWaitSemaphores(wait_semaphores.data())
			.setPWaitDstStageMask(wait_stages.data())
	}, vk::Fence());
}

void vk_command_list_t::draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t base_index, int32_t base_vertex, uint32_t base_instance)
{
	object.drawIndexed(index_count, instance_count, base_index, base_vertex, base_instance);
}

void vk_command_list_t::draw_non_indexed(uint32_t vertex_count, uint32_t instance_count, int32_t base_vertex, uint32_t base_instance)
{
	object.draw(vertex_count, instance_count, base_vertex, base_instance);
}

void vk_command_list_t::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
	object.dispatch(x, y, z);
}

void vk_command_list_t::copy_buffer(buffer_t& src, uint64_t src_offset, buffer_t& dst, uint64_t dst_offset, uint64_t size)
{
	object.copyBuffer(dynamic_cast<vk_buffer_t&>(src).object, dynamic_cast<vk_buffer_t&>(dst).object,
		{vk::BufferCopy(src_offset, dst_offset, size)});
}

void vk_command_list_t::next_subpass()
{
	object.nextSubpass(vk::SubpassContents::eInline);
}

uint32_t vk_swap_chain_t::get_next_backbuffer_id(semaphore_t& semaphore)
{
	const auto& casted_semaphore = dynamic_cast<vk_semaphore_t&>(semaphore);
	return dev.acquireNextImageKHR(object, UINT64_MAX, casted_semaphore.object, vk::Fence()).value;
}

void vk_swap_chain_t::present(command_queue_t& cmdqueue, uint32_t backbuffer_index)
{
	const auto presentInfo = vk::PresentInfoKHR{}
		.setPSwapchains(&object)
		.setSwapchainCount(1)
		.setPImageIndices(&backbuffer_index);
	dynamic_cast<vk_command_queue_t&>(cmdqueue).object.presentKHR(&presentInfo);
}

vk_framebuffer::vk_framebuffer(vk::Device _dev, vk::RenderPass render_pass, gsl::span<const vk::ImageView> render_targets,
	uint32_t width, uint32_t height, uint32_t layers)
	: dev(_dev)
{
	object = dev.createFramebuffer(
		vk::FramebufferCreateInfo{}
			.setAttachmentCount(static_cast<uint32_t>(render_targets.size()))
			.setPAttachments(render_targets.data())
			.setLayers(layers)
			.setWidth(width)
			.setHeight(height)
			.setRenderPass(render_pass)
	);
}

std::unique_ptr<framebuffer_t> vk_device_t::create_frame_buffer(gsl::span<const image_view_t*> render_targets, uint32_t width, uint32_t height, render_pass_t* render_pass)
{
	std::vector<vk::ImageView> attachments;
	std::transform(render_targets.begin(), render_targets.end(), std::back_inserter(attachments),
		[&](auto&& input) { return dynamic_cast<const vk_image_view_t*>(input)->object; });
	return std::unique_ptr<framebuffer_t>(new vk_framebuffer(object, static_cast<vk_render_pass_t*>(render_pass)->object, attachments, width, height, 1));
}

std::unique_ptr<framebuffer_t> vk_device_t::create_frame_buffer(gsl::span<const image_view_t*> render_targets, const image_view_t& depth_stencil_texture, uint32_t width, uint32_t height, render_pass_t* render_pass)
{
	std::vector<vk::ImageView> attachments;
	std::transform(render_targets.begin(), render_targets.end(), std::back_inserter(attachments),
		[&](const auto input) { return dynamic_cast<const vk_image_view_t*>(input)->object; });
		attachments.push_back(dynamic_cast<const vk_image_view_t&>(depth_stencil_texture).object);
	return std::unique_ptr<framebuffer_t>(new vk_framebuffer(object, static_cast<vk_render_pass_t*>(render_pass)->object, attachments, width, height, 1));
}



void vk_command_list_t::end_renderpass()
{
	object.endRenderPass();
}

std::unique_ptr<descriptor_set_layout> vk_device_t::get_object_descriptor_set(const descriptor_set &ds)
{
	std::vector<vk::DescriptorSetLayoutBinding> descriptor_range_storage;
	descriptor_range_storage.reserve(ds.descriptors_ranges.size());
	std::transform(ds.descriptors_ranges.begin(), ds.descriptors_ranges.end(), std::back_inserter(descriptor_range_storage),
		[&](auto&& rod) {
		return vk::DescriptorSetLayoutBinding{}
			.setBinding(rod.bind_point)
			.setDescriptorCount(rod.count)
			.setDescriptorType(get_descriptor_type(rod.range_type))
			.setStageFlags(get_shader_stage(ds.stage));
	});
	return std::unique_ptr<descriptor_set_layout>(new vk_descriptor_set_layout(object,
		object.createDescriptorSetLayout(
			vk::DescriptorSetLayoutCreateInfo{}
				.setBindingCount(static_cast<uint32_t>(descriptor_range_storage.size()))
				.setPBindings(descriptor_range_storage.data())
	)));
}

namespace
{
	struct shader_module
	{
		vk::ShaderModule object;
		vk::Device dev;

		shader_module(vk::Device _dev, gsl::span<const uint32_t> bin) :
			dev(_dev)
		{
			object = dev.createShaderModule(
				vk::ShaderModuleCreateInfo{}
					.setCodeSize(bin.size_bytes())
					.setPCode(bin.data()));
		}

		~shader_module()
		{
			dev.destroyShaderModule(object);
		}

		shader_module(shader_module&&) = delete;
		shader_module(const shader_module&) = delete;
	};
}

std::unique_ptr<pipeline_state_t> vk_device_t::create_graphic_pso(const graphic_pipeline_state_description &pso_desc, const render_pass_t& render_pass, const pipeline_layout_t& layout, const uint32_t& subpass)
{
	const blend_state blend = blend_state::get();

	const auto& attachments = [&]() {
		const auto&& get_factor = [](const auto& op)
		{
			switch (op)
			{
			case blend_factor::one: return vk::BlendFactor::eOne;
			case blend_factor::zero: return vk::BlendFactor::eZero;
			}
			throw;
		};

		const auto&& get_op = [](const auto& op)
		{
			switch (op)
			{
			case blend_op::add: return vk::BlendOp::eAdd;
			}
			throw;
		};

		auto&& result = std::vector<vk::PipelineColorBlendAttachmentState>{};
		std::transform(pso_desc.color_outputs.begin(), pso_desc.color_outputs.end(), std::back_inserter(result),
			[&](const auto& color_output)
			{
				return vk::PipelineColorBlendAttachmentState{}
					.setBlendEnable(color_output.blend)
					.setAlphaBlendOp(get_op(color_output.op))
					.setSrcColorBlendFactor(get_factor(color_output.src_color))
					.setSrcAlphaBlendFactor(get_factor(color_output.src_alpha))
					.setDstColorBlendFactor(get_factor(color_output.dst_color))
					.setDstAlphaBlendFactor(get_factor(color_output.dst_alpha))
					.setColorWriteMask(vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eR);
			});
		return result;
	}();

	const auto color_blend = vk::PipelineColorBlendStateCreateInfo{}
		.setPAttachments(attachments.data())
		.setAttachmentCount(static_cast<uint32_t>(attachments.size()));

	auto tesselation_info = vk::PipelineTessellationStateCreateInfo{};
	auto viewport_info = vk::PipelineViewportStateCreateInfo{}
		.setViewportCount(1)
		.setScissorCount(1);
	auto dynamic_states = std::array<vk::DynamicState, 2>{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	auto dynamic_state_info = vk::PipelineDynamicStateCreateInfo{}
		.setDynamicStateCount(static_cast<uint32_t>(dynamic_states.size()))
		.setPDynamicStates(dynamic_states.data());

	shader_module module_vert(object, pso_desc.vertex_binary);
	shader_module module_frag(object, pso_desc.fragment_binary);

	auto shader_stages = std::vector<vk::PipelineShaderStageCreateInfo>{
		vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(module_vert.object)
			.setPName("main"),
		vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(module_frag.object)
			.setPName("main")
	};

	const auto& vertex_input_binding = [&]()
	{
		auto&& result = std::vector<vk::VertexInputBindingDescription>{};
		auto&& already_defined_buffer = std::set<uint32_t>{};
		for (const auto& attrib : pso_desc.attributes)
		{
			if (already_defined_buffer.find(attrib.binding) != already_defined_buffer.end())
				continue;
			result.emplace_back(attrib.binding, attrib.stride, vk::VertexInputRate::eVertex);
			already_defined_buffer.insert(attrib.binding);
		}
		return result;
	}();

	const auto& vertex_input_attributes = [&]()
	{
		auto&& result = std::vector<vk::VertexInputAttributeDescription>{};
		std::transform(pso_desc.attributes.begin(), pso_desc.attributes.end(), std::back_inserter(result),
			[&](const auto& attrib) -> vk::VertexInputAttributeDescription {
				return {attrib.location, attrib.binding, get_vk_format(attrib.format), attrib.offset};
		});
		return result;
	}();

	const auto vertex_input = vk::PipelineVertexInputStateCreateInfo{}
		.setPVertexBindingDescriptions(vertex_input_binding.data())
		.setVertexBindingDescriptionCount(static_cast<uint32_t>(vertex_input_binding.size()))
		.setPVertexAttributeDescriptions(vertex_input_attributes.data())
		.setVertexAttributeDescriptionCount(static_cast<uint32_t>(vertex_input_attributes.size()));

	const auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo{}
		.setPrimitiveRestartEnable(false)
		.setTopology(vk::PrimitiveTopology::eTriangleList);

	const auto rasterization = vk::PipelineRasterizationStateCreateInfo{}
		.setLineWidth(1.f)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(vk::CullModeFlagBits::eNone);

	const auto multisample = vk::PipelineMultisampleStateCreateInfo{}
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	const auto&& get_compare_op = [](const auto& op) {
		switch (op) {
		case irr::video::E_COMPARE_FUNCTION::ECF_LEQUAL: return vk::CompareOp::eLessOrEqual;
		case irr::video::E_COMPARE_FUNCTION::ECF_LESS: return vk::CompareOp::eLess;
		case irr::video::E_COMPARE_FUNCTION::ECF_NEVER: return vk::CompareOp::eNever;
		}
		throw;
	};

	const auto depth_stencil = vk::PipelineDepthStencilStateCreateInfo{}
		.setDepthTestEnable(pso_desc.depth_stencil_depth_test)
		.setStencilTestEnable(pso_desc.depth_stencil_stencil_test)
		.setDepthWriteEnable(pso_desc.depth_stencil_depth_write)
		.setDepthCompareOp(get_compare_op(pso_desc.depth_stencil_depth_compare_op));

	/*auto vertex_buffers = std::vector<vk::VertexInputBindingDescription>{
		vk::VertexInputBindingDescription{ 0, static_cast<uint32_t>(4 * sizeof(float)), VK_VERTEX_INPUT_RATE_VERTEX },
	};
	vertex_input.pVertexBindingDescriptions = vertex_buffers.data();
	vertex_input.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_buffers.size());

	auto attribute = std::vector<vk::VertexInputAttributeDescription>{
	{ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },
	{ 1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float) },
	};
	vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute.size());
	vertex_input.pVertexAttributeDescriptions = attribute.data();*/

//	return std::make_shared<vulkan_wrapper::pipeline>(dev, 0, shader_stages, vertex_input, get_pipeline_input_assembly_state_info(pso_desc), tesselation_info, viewport_info, get_pipeline_rasterization_state_create_info(pso_desc), get_pipeline_multisample_state_create_info(pso_desc), get_pipeline_depth_stencil_state_create_info(pso_desc), blend, dynamic_state_info, layout->object, rp, 1, VkPipeline(VK_NULL_HANDLE), 0);*/

	vk::Pipeline pso = object.createGraphicsPipeline(
		vk::PipelineCache{},
		vk::GraphicsPipelineCreateInfo{}
			.setPVertexInputState(&vertex_input)
			.setPInputAssemblyState(&input_assembly)
			.setPRasterizationState(&rasterization)
			.setPMultisampleState(&multisample)
			.setRenderPass(dynamic_cast<const vk_render_pass_t&>(render_pass).object)
			.setLayout(dynamic_cast<const vk_pipeline_layout_t&>(layout).object)
			.setPTessellationState(&tesselation_info)
			.setPDynamicState(&dynamic_state_info)
			.setSubpass(subpass)
			.setStageCount(static_cast<uint32_t>(shader_stages.size()))
			.setPStages(shader_stages.data())
			.setPVertexInputState(&vertex_input)
			.setPViewportState(&viewport_info)
			.setPColorBlendState(&color_blend)
			.setPDepthStencilState(&depth_stencil)
	);
	return std::unique_ptr<pipeline_state_t>(new vk_pipeline_state_t(object, pso));
}

std::unique_ptr<compute_pipeline_state_t> vk_device_t::create_compute_pso(const compute_pipeline_state_description &pso_desc, const pipeline_layout_t& layout)
{
	const auto& module = shader_module(object, pso_desc.compute_binary);
	auto&& result = object.createComputePipeline(vk::PipelineCache{},
		vk::ComputePipelineCreateInfo{}
			.setStage(vk::PipelineShaderStageCreateInfo{}
				.setModule(module.object)
				.setPName("main")
				.setStage(vk::ShaderStageFlagBits::eCompute)
			)
			.setLayout(dynamic_cast<const vk_pipeline_layout_t&>(layout).object)
	);
	return std::unique_ptr<compute_pipeline_state_t>(new vk_compute_pipeline_state_t(object, result));
}

std::unique_ptr<pipeline_layout_t> vk_device_t::create_pipeline_layout(gsl::span<const descriptor_set_layout*> sets)
{
	const auto& descriptor_layout = [&]() {
		std::vector<vk::DescriptorSetLayout> result;
		std::transform(sets.begin(), sets.end(), std::back_inserter(result),
			[](const auto input) { return dynamic_cast<const vk_descriptor_set_layout*>(input)->object; });
		return result;
	}();
	const auto& result = object.createPipelineLayout(
		vk::PipelineLayoutCreateInfo{}
			.setPSetLayouts(descriptor_layout.data())
			.setSetLayoutCount(static_cast<uint32_t>(descriptor_layout.size()))
	);
	return std::unique_ptr<pipeline_layout_t>(new vk_pipeline_layout_t(object, result));
}

std::unique_ptr<render_pass_t> vk_device_t::create_ibl_sky_pass(const irr::video::ECOLOR_FORMAT& fmt)
{
	const auto& attachments = std::array<vk::AttachmentDescription, 2>{
		vk::AttachmentDescription{}
			.setFormat(get_vk_format(fmt))
			.setLoadOp(vk::AttachmentLoadOp::eLoad)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
		vk::AttachmentDescription{}
			.setFormat(vk::Format::eD24UnormS8Uint)
			.setLoadOp(vk::AttachmentLoadOp::eLoad)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setStencilLoadOp(vk::AttachmentLoadOp::eLoad)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
	};

	const auto& color_attachment_ref = std::array<vk::AttachmentReference, 1>{
		vk::AttachmentReference{0, vk::ImageLayout::eColorAttachmentOptimal}
	};

	const auto& ibl_input_attachement_ref = std::array<vk::AttachmentReference, 1> {
		vk::AttachmentReference{ 1, vk::ImageLayout::eDepthStencilReadOnlyOptimal }
	};
	const auto& skybox_ds_attachment_ref = vk::AttachmentReference{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };

	const auto& subpasses = std::array<vk::SubpassDescription, 2>{
		// IBL pass
		vk::SubpassDescription{}
			.setPColorAttachments(color_attachment_ref.data())
			.setColorAttachmentCount(static_cast<uint32_t>(color_attachment_ref.size()))
			.setPInputAttachments(ibl_input_attachement_ref.data())
			.setInputAttachmentCount(static_cast<uint32_t>(ibl_input_attachement_ref.size()))
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics),
		// Draw skybox
		vk::SubpassDescription{}
			.setPColorAttachments(color_attachment_ref.data())
			.setColorAttachmentCount(static_cast<uint32_t>(color_attachment_ref.size()))
			.setPDepthStencilAttachment(&skybox_ds_attachment_ref)
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
	};

	const auto& dependencies = std::array<vk::SubpassDependency, 1>{
		vk::SubpassDependency{}
			.setSrcSubpass(0)
			.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead)
			.setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
			.setDstSubpass(1)
			.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
			.setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests)
	};

	auto&& result = object.createRenderPass(
		vk::RenderPassCreateInfo{}
			.setPAttachments(attachments.data())
			.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
			.setPSubpasses(subpasses.data())
			.setSubpassCount(static_cast<uint32_t>(subpasses.size()))
			.setPDependencies(dependencies.data())
			.setDependencyCount(static_cast<uint32_t>(dependencies.size()))
	);
	return std::unique_ptr<render_pass_t>(new vk_render_pass_t(object, result));
}

std::unique_ptr<render_pass_t> vk_device_t::create_object_sunlight_pass(const irr::video::ECOLOR_FORMAT& fmt)
{
	const auto& attachments = std::array<vk::AttachmentDescription, 5>{
		// color
		vk::AttachmentDescription{ vk::AttachmentDescriptionFlagBits{}, vk::Format::eR8G8B8A8Unorm, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal},
		// normal
		vk::AttachmentDescription{ vk::AttachmentDescriptionFlagBits{}, vk::Format::eR16G16Sfloat, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal },
		// roughness and metalness
		vk::AttachmentDescription{ vk::AttachmentDescriptionFlagBits{}, vk::Format::eR8G8B8A8Unorm, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal },
		// final surface
		vk::AttachmentDescription{ vk::AttachmentDescriptionFlagBits{}, get_vk_format(fmt), vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eColorAttachmentOptimal },
		// depth
		vk::AttachmentDescription{ vk::AttachmentDescriptionFlagBits{}, vk::Format::eD24UnormS8Uint, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal },
	};

	const auto& object_subpass_attachments = std::array<vk::AttachmentReference, 3> {
		vk::AttachmentReference{0, vk::ImageLayout::eColorAttachmentOptimal},
		vk::AttachmentReference{1, vk::ImageLayout::eColorAttachmentOptimal},
		vk::AttachmentReference{2, vk::ImageLayout::eColorAttachmentOptimal}
	};

	const auto& ibl_subpass_attachments = std::array<vk::AttachmentReference, 1> {
		vk::AttachmentReference{ 3, vk::ImageLayout::eColorAttachmentOptimal }
	};

	const auto& ibl_subpass_input_attachements = std::array<vk::AttachmentReference, 4> {
		vk::AttachmentReference{ 0, vk::ImageLayout::eShaderReadOnlyOptimal },
		vk::AttachmentReference{ 1, vk::ImageLayout::eShaderReadOnlyOptimal },
		vk::AttachmentReference{ 2, vk::ImageLayout::eShaderReadOnlyOptimal },
		vk::AttachmentReference{ 4, vk::ImageLayout::eShaderReadOnlyOptimal }
	};

	const auto& depthAttachments = vk::AttachmentReference{ 4, vk::ImageLayout::eDepthStencilAttachmentOptimal };

	const auto& subpasses = std::array<vk::SubpassDescription, 2> {
		// Object pass
		vk::SubpassDescription{}
			.setPColorAttachments(object_subpass_attachments.data())
			.setColorAttachmentCount(static_cast<uint32_t>(object_subpass_attachments.size()))
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setPDepthStencilAttachment(&depthAttachments),
		// Sunlight pass IBL pass
		vk::SubpassDescription{}
			.setPColorAttachments(ibl_subpass_attachments.data())
			.setColorAttachmentCount(static_cast<uint32_t>(ibl_subpass_attachments.size()))
			.setPInputAttachments(ibl_subpass_input_attachements.data())
			.setInputAttachmentCount(static_cast<uint32_t>(ibl_subpass_input_attachements.size()))
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
	};

	const auto& dependencies = std::array<vk::SubpassDependency, 1> {
		vk::SubpassDependency{}
			.setSrcSubpass(0)
			.setDstSubpass(1)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
			.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
			.setDstAccessMask(vk::AccessFlagBits::eInputAttachmentRead)
	};

	auto&& result = object.createRenderPass(
		vk::RenderPassCreateInfo{}
			.setPAttachments(attachments.data())
			.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
			.setPSubpasses(subpasses.data())
			.setSubpassCount(static_cast<uint32_t>(subpasses.size()))
			.setPDependencies(dependencies.data())
			.setDependencyCount(static_cast<uint32_t>(dependencies.size()))
	);
	return std::unique_ptr<render_pass_t>(new vk_render_pass_t(object, result));
}

std::unique_ptr<render_pass_t> vk_device_t::create_ssao_pass()
{
	const auto& attachments = std::array<vk::AttachmentDescription, 2>{
		vk::AttachmentDescription{}
			.setFormat(vk::Format::eR32Sfloat)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setSamples(vk::SampleCountFlagBits::e1),
		vk::AttachmentDescription{}
			.setFormat(vk::Format::eR16Sfloat)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setSamples(vk::SampleCountFlagBits::e1)
	};

	const auto& lineardepth_subpass_attachments = std::array<vk::AttachmentReference, 1> {
		vk::AttachmentReference{ 0, vk::ImageLayout::eColorAttachmentOptimal }
	};

	const auto& ssao_subpass_attachments = std::array<vk::AttachmentReference, 1> {
		vk::AttachmentReference{ 1, vk::ImageLayout::eColorAttachmentOptimal }
	};

	const auto& ssao_subpass_input = std::array<vk::AttachmentReference, 0> {
		//vk::AttachmentReference{ 0, vk::ImageLayout::eShaderReadOnlyOptimal }
	};

	const auto& subpasses = std::array<vk::SubpassDescription, 2> {
		// Linearize depth
		vk::SubpassDescription{}
			.setPColorAttachments(lineardepth_subpass_attachments.data())
			.setColorAttachmentCount(static_cast<uint32_t>(lineardepth_subpass_attachments.size()))
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics),
		// SSAO
		vk::SubpassDescription{}
			.setPColorAttachments(ssao_subpass_attachments.data())
			.setColorAttachmentCount(static_cast<uint32_t>(ssao_subpass_attachments.size()))
			.setPInputAttachments(ssao_subpass_input.data())
			.setInputAttachmentCount(static_cast<uint32_t>(ssao_subpass_input.size()))
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
	};

	const auto& dependencies = std::array<vk::SubpassDependency, 1> {
		vk::SubpassDependency{}
			.setSrcSubpass(0)
			.setDstSubpass(1)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
			.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
	};

	const auto& result = object.createRenderPass(
		vk::RenderPassCreateInfo{}
		.setPAttachments(attachments.data())
		.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
		.setPSubpasses(subpasses.data())
		.setSubpassCount(static_cast<uint32_t>(subpasses.size()))
		.setPDependencies(dependencies.data())
		.setDependencyCount(static_cast<uint32_t>(dependencies.size()))
	);
	return std::unique_ptr<render_pass_t>(new vk_render_pass_t(object, result));
}

std::unique_ptr<render_pass_t> vk_device_t::create_blit_pass(const irr::video::ECOLOR_FORMAT& color_format)
{
	const auto& attachments = std::array<vk::AttachmentDescription, 1>{
		vk::AttachmentDescription{}
			.setFormat(get_vk_format(color_format))
			.setLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setInitialLayout(vk::ImageLayout::ePresentSrcKHR)
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
	};

	const auto& att_ref = std::array<vk::AttachmentReference, 1>{
		vk::AttachmentReference{0, vk::ImageLayout::eColorAttachmentOptimal}
	};

	const auto& subpass_desc = std::array<vk::SubpassDescription, 1>
	{
		vk::SubpassDescription{}
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setPColorAttachments(att_ref.data())
			.setColorAttachmentCount(static_cast<uint32_t>(att_ref.size()))
	};
	auto&& result = object.createRenderPass(
		vk::RenderPassCreateInfo{}
			.setPAttachments(attachments.data())
			.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
			.setPSubpasses(subpass_desc.data())
			.setSubpassCount(static_cast<uint32_t>(subpass_desc.size()))
	);
	return std::unique_ptr<render_pass_t>(new vk_render_pass_t(object, result));
}

std::unique_ptr<fence_t> vk_device_t::create_fence()
{
	return std::unique_ptr<fence_t>();
}

std::unique_ptr<semaphore_t> vk_device_t::create_semaphore()
{
	auto&& semaphore = object.createSemaphore(vk::SemaphoreCreateInfo{});
	return std::unique_ptr<semaphore_t>(new vk_semaphore_t(object, semaphore));
}
