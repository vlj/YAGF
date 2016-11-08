// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include "../include/API/vkapi.h"
#include <sstream>

#include <locale>
#include <codecvt>
#include <string>


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
		}
		throw;
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
//		if (flags & usage_uav)
//			result |= VK_IMAGE_USAGE_STORAGE_BIT;
		return result;
	}
}

std::tuple<std::unique_ptr<device_t>, std::unique_ptr<swap_chain_t>, std::unique_ptr<command_queue_t>, size_t, size_t, irr::video::ECOLOR_FORMAT> create_device_swapchain_and_graphic_presentable_queue(HINSTANCE hinstance, HWND window)
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

	auto app_info = vk::ApplicationInfo{}
		.setApiVersion(VK_MAKE_VERSION(1, 0, 0))
		.setPApplicationName("Nameless app")
		.setPEngineName("Nameless engine")
		.setApplicationVersion(1)
		.setEngineVersion(1);

	auto instance = vk::createInstance(
		vk::InstanceCreateInfo{}
			.setEnabledExtensionCount(layers.size())
			.setPpEnabledLayerNames(layers.data())
			.setEnabledExtensionCount(instance_extension.size())
			.setPpEnabledExtensionNames(instance_extension.data())
			.setPApplicationInfo(&app_info)
	);

#ifndef NDEBUG
/*	PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback{};
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
	create_info.pfnCallback = dbgFunc;
	create_info.pUserData = nullptr;

	CHECK_VKRESULT(dbgCreateDebugReportCallback(instance, &create_info, NULL, &debug_report_callback));*/

	/* Clean up callback */
//	dbgDestroyDebugReportCallback(instance, debug_report_callback, NULL);
#endif

	const auto& devices = instance.enumeratePhysicalDevices();
	const auto& queue_family_properties = devices[0].getQueueFamilyProperties();

	auto surface = instance.createWin32SurfaceKHR(
		vk::Win32SurfaceCreateInfoKHR{}
			.setHinstance(hinstance)
			.setHwnd(window)
	);

	auto&& find_graphic_queue_family = [&]()
	{
		for (unsigned int i = 0; i < queue_family_properties.size(); i++) {
			if (queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
				if (devices[0].getSurfaceSupportKHR(i, surface)) return i;
			}
		}
		throw;
	};

	float queue_priorities = 0.f;
	std::array<vk::DeviceQueueCreateInfo, 1> queue_infos{
		vk::DeviceQueueCreateInfo{}
			.setQueueFamilyIndex(find_graphic_queue_family())
			.setQueueCount(1)
			.setPQueuePriorities(&queue_priorities)
	};

	std::vector<const char*> device_extension = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	auto dev = devices[0].createDevice(
		vk::DeviceCreateInfo{}
			.setEnabledExtensionCount(device_extension.size())
			.setPpEnabledExtensionNames(device_extension.data())
			.setEnabledLayerCount(layers.size())
			.setPpEnabledLayerNames(layers.data())
			.setPQueueCreateInfos(queue_infos.data())
			.setQueueCreateInfoCount(queue_infos.size())
	);

	auto mem_properties = devices[0].getMemoryProperties();
/*	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
	{
		if (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			dev->default_memory_index = i;
		if (is_host_visible(mem_properties.memoryTypes[i].propertyFlags) & is_host_coherent(mem_properties.memoryTypes[i].propertyFlags))
			dev->upload_memory_index = i;
	}*/

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
		.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
		.setImageSharingMode(vk::SharingMode::eExclusive);

	auto chain = dev.createSwapchainKHR(swap_chain);

	auto queue = dev.getQueue(queue_infos[0].queueFamilyIndex, 0);
	return std::make_tuple(
		std::unique_ptr<device_t>(new vk_device_t(dev)),
		std::unique_ptr<swap_chain_t>(new vk_swap_chain_t(chain)),
		std::unique_ptr<command_queue_t>(new vk_command_queue_t(queue)),
		surface_capabilities.currentExtent.width, surface_capabilities.currentExtent.height, irr::video::ECF_B8G8R8A8_UNORM);
}

std::vector<std::unique_ptr<image_t>> vk_swap_chain_t::get_image_view_from_swap_chain()
{
	auto swapchain_image = dev.getSwapchainImagesKHR(object);
	auto result = std::vector<std::unique_ptr<image_t>>{};
	std::transform(swapchain_image.begin(), swapchain_image.end(), std::back_inserter(result),
		[this](auto&& img) { return std::unique_ptr<image_t>(new vk_image_t(dev, img)); });
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
	uint32_t get_memory_index(irr::video::E_MEMORY_POOL memory_pool, device_t *dev)
	{
		switch (memory_pool)
		{
		case irr::video::E_MEMORY_POOL::EMP_GPU_LOCAL: return  dev->default_memory_index;
		case irr::video::E_MEMORY_POOL::EMP_CPU_READABLE:
		case irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE: return  dev->upload_memory_index;
		}
		throw;
	}

	auto get_buffer_usage_flags(uint32_t flags)
	{
		auto result = vk::BufferUsageFlags();
		if (flags & usage_uav)
			result |= vk::BufferUsageFlagBits::eStorageBuffer;
		if (flags & usage_transfer_dst)
			result |= vk::BufferUsageFlagBits::eTransferDst;
		if (flags & usage_texel_buffer)
			result |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
		if (flags & usage_buffer_transfer_src)
			result |= vk::BufferUsageFlagBits::eTransferSrc;
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
			.setMemoryTypeIndex(upload_memory_index)
	);
	object.bindBufferMemory(buffer, memory, 0);
	return  std::unique_ptr<buffer_t>(
		new vk_buffer_t(object, buffer, memory));
}

std::unique_ptr<descriptor_storage_t> vk_device_t::create_descriptor_storage(uint32_t num_sets, const std::vector<std::tuple<RESOURCE_VIEW, uint32_t> > &num_descriptors)
{
	std::vector<vk::DescriptorPoolSize> poolSizes;

	std::transform(num_descriptors.begin(), num_descriptors.end(), std::back_inserter(poolSizes),
		[](auto&& set) { return vk::DescriptorPoolSize(get_descriptor_type(std::get<0>(set_size)), std::get<1>(set_size)); });
	return std::unique_ptr<descriptor_storage_t>(
		new vk_descriptor_storage_t(object,
			object.createDescriptorPool(
				vk::DescriptorPoolCreateInfo{}
					.setMaxSets(num_sets)
					.setPoolSizeCount(poolSizes.size())
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

std::unique_ptr<buffer_view_t> create_buffer_view(device_t& dev, buffer_t& buffer, irr::video::ECOLOR_FORMAT format, uint64_t offset, uint32_t size)
{
	return std::make_unique<buffer_view_t>(dev, buffer, get_vk_format(format), offset, size);
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

clear_value_structure_t get_clear_value(irr::video::ECOLOR_FORMAT format, float depth, uint8_t stencil)
{
	return clear_value_structure_t();
}

clear_value_structure_t get_clear_value(irr::video::ECOLOR_FORMAT format, const std::array<float, 4> &color)
{
	return clear_value_structure_t();
}

std::unique_ptr<image_t> vk_device_t::create_image(irr::video::ECOLOR_FORMAT format, uint32_t width, uint32_t height, uint16_t mipmap, uint32_t layers, uint32_t flags, clear_value_structure_t*)
{
	auto&& get_image_create_flag = [](auto flags)
	{
		auto result = vk::ImageCreateFlag();
		if (flags & usage_cube)
			result |= vk::ImageCreateFlagBits::eCubeCompatible;
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
			.setUsage(get_image_create_flag(flags))
			.setSamples(vk::SampleCountFlagBits::e1)
	);
	const auto& mem_req = object.getImageMemoryRequirements(image);
	const auto& memory = object.allocateMemory(
		vk::MemoryAllocateInfo{}
			.setAllocationSize(mem_req.size)
			.setMemoryTypeIndex(default_memory_index)
	);
	object.bindImageMemory(image, memory, 0);
	return std::unique_ptr<image_t>(new vk_image_t(object, image, memory));
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
	auto&& get_sampler_desc = [](auto&& sampler_type)
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
	auto mipLevel = destination_subresource;// % max(destination_image.info.mipLevels, 1);
	auto baseArrayLayer = destination_subresource;// / max(destination_image.info.mipLevels, 1);

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
	/*switch (get_image_layout(after))
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
	}*/

	auto baseMipLevel = subresource;// % max(resource.info.mipLevels, 1);
	auto baseArrayLayer = subresource;// / max(resource.info.mipLevels, 1);

	auto get_image_layout = [](auto&& usage)
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

	object.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::DependencyFlags(), {}, {},
		{
			vk::ImageMemoryBarrier{}
				.setOldLayout(get_image_layout(before))
				.setNewLayout(get_image_layout(after))
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

void vk_command_list_t::set_graphic_pipeline(pipeline_state_t pipeline)
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
					.setDescriptorSetCount(set_layouts.size())
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
		{ static_cast<const vk_allocated_descriptor_set&>(descriptor_set).object }, { 0 });
}
 
void vk_command_list_t::bind_compute_descriptor(uint32_t bindpoint, const allocated_descriptor_set & descriptor_set, pipeline_layout_t& sig)
{
	object.bindDescriptorSets(vk::PipelineBindPoint::eCompute, dynamic_cast<vk_pipeline_layout_t&>(sig).object, bindpoint,
		{ static_cast<const vk_allocated_descriptor_set&>(descriptor_set).object }, { 0 });
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

void vk_command_queue_t::submit_executable_command_list(command_list_t& command_list)
{
	std::array<vk::CommandBuffer, 1> command_buffers{ dynamic_cast<vk_command_list_t&>(command_list).object };
	object.submit({
		vk::SubmitInfo{}
			.setCommandBufferCount(command_buffers.size())
			.setPCommandBuffers(command_buffers.data())
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

uint32_t vk_swap_chain_t::get_next_backbuffer_id()
{
	return dev.acquireNextImageKHR(object, UINT64_MAX, vk::Semaphore(), vk::Fence()).value;
}

void present(device_t& dev, command_queue_t& cmdqueue, swap_chain_t& chain, uint32_t backbuffer_index)
{
	VkPresentInfoKHR info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	info.swapchainCount = 1;
	info.pSwapchains = &(chain.object);
	info.pImageIndices = &backbuffer_index;

	CHECK_VKRESULT(vkQueuePresentKHR(cmdqueue, &info));
}

namespace
{
	std::vector<VkImageView> get_image_view_vector(const std::vector<std::unique_ptr<vulkan_wrapper::image_view> > &vectors)
	{
		std::vector<VkImageView> result;
		for (const auto& iv : vectors)
		{
			result.emplace_back(iv->object);
		}
		return result;
	}
}

vk_framebuffer::vk_framebuffer(vk::Device _dev, vk::RenderPass render_pass, std::vector<std::tuple<image_t&, irr::video::ECOLOR_FORMAT>> render_targets, uint32_t width, uint32_t height, uint32_t layers)
	: dev(_dev)
{
	std::vector<vk::ImageView> image_views;
	object = dev.createFramebuffer(
		vk::FramebufferCreateInfo{}
			.setAttachmentCount(image_views.size())
			.setPAttachments(image_views.data())
			.setLayers(layers)
			.setWidth(width)
			.setHeight(height)
			.setRenderPass(render_pass)
	);
}

vk_framebuffer::vk_framebuffer(vk::Device _dev, vk::RenderPass render_pass, const std::vector<std::tuple<image_t&, irr::video::ECOLOR_FORMAT>> &render_targets,
	const std::tuple<image_t&, irr::video::ECOLOR_FORMAT> &depth_stencil, uint32_t width, uint32_t height, uint32_t layers)
	: dev(_dev)
{
	std::vector<vk::ImageView> image_views;
	object = dev.createFramebuffer(
		vk::FramebufferCreateInfo{}
			.setAttachmentCount(image_views.size())
			.setPAttachments(image_views.data())
			.setLayers(layers)
			.setWidth(width)
			.setHeight(height)
			.setRenderPass(render_pass)
	);
}

std::vector<std::unique_ptr<vulkan_wrapper::image_view> > vk_framebuffer::build_image_views(VkDevice dev, const std::vector<std::tuple<image_t&, irr::video::ECOLOR_FORMAT>> &render_targets)
{
	std::vector<std::unique_ptr<vulkan_wrapper::image_view> >  result;
	for (const auto & rtt_info : render_targets)
	{
		VkComponentMapping default_mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		VkImageSubresourceRange ranges{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		result.emplace_back(
			std::make_unique<vulkan_wrapper::image_view>(dev, std::get<0>(rtt_info), VK_IMAGE_VIEW_TYPE_2D, get_vk_format(std::get<1>(rtt_info)), default_mapping, ranges)
		);
	}
	return result;
}

std::vector<std::unique_ptr<vulkan_wrapper::image_view> > vk_framebuffer::build_image_views(VkDevice dev, const std::vector<std::tuple<image_t&, irr::video::ECOLOR_FORMAT>> &render_targets,
	const std::tuple<image_t&, irr::video::ECOLOR_FORMAT> &depth_stencil)
{
	std::vector<std::unique_ptr<vulkan_wrapper::image_view> > result = std::move(build_image_views(dev, render_targets));
	VkImageSubresourceRange ranges{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };
	VkComponentMapping default_mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	result.emplace_back(std::make_unique<vulkan_wrapper::image_view>(dev, std::get<0>(depth_stencil), VK_IMAGE_VIEW_TYPE_2D, get_vk_format(std::get<1>(depth_stencil)), default_mapping, ranges));
	return result;
}

std::unique_ptr<framebuffer_t> vk_device_t::create_frame_buffer(std::vector<std::tuple<image_t&, irr::video::ECOLOR_FORMAT>> render_targets, uint32_t width, uint32_t height, render_pass_t* render_pass)
{
	return std::unique_ptr<framebuffer_t>(new vk_framebuffer(object, *render_pass, render_targets, width, height, 1));
}

std::unique_ptr<framebuffer_t> vk_device_t::create_frame_buffer(std::vector<std::tuple<image_t&, irr::video::ECOLOR_FORMAT>> render_targets, std::tuple<image_t&, irr::video::ECOLOR_FORMAT> depth_stencil_texture, uint32_t width, uint32_t height, render_pass_t* render_pass)
{
	return std::unique_ptr<framebuffer_t>(new vk_framebuffer(dev, *render_pass, render_targets, depth_stencil_texture, width, height, 1));
}



void vk_command_list_t::end_renderpass()
{
	object.endRenderPass();
}
