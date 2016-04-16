// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#include <vulkan\vk_platform.h>
#include <memory>
#include <vector>
#include "..\Core\SColor.h"
#include <fstream>


#define CHECK_VKRESULT(cmd) { VkResult res = (cmd); if (res != VK_SUCCESS) throw; }

namespace vulkan_wrapper
{
	struct device
	{
		VkDevice object;
		const std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		const std::vector<const char*> layers;
		const std::vector<const char*> extensions;
		VkPhysicalDevice physical_device;
		uint32_t upload_memory_index;
		uint32_t default_memory_index;
		const VkDeviceCreateInfo info;

		device(VkPhysicalDevice phys_dev, const std::vector<VkDeviceQueueCreateInfo> &qci, const std::vector<const char*> &l, const std::vector<const char*> &ext)
			: physical_device(phys_dev), queue_create_infos(qci), layers(l), extensions(ext),
			info({ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, nullptr, 0,
				static_cast<uint32_t>(queue_create_infos.size()), queue_create_infos.data(),
				static_cast<uint32_t>(layers.size()), layers.data(),
				static_cast<uint32_t>(extensions.size()), extensions.data(),
				nullptr })
		{
			CHECK_VKRESULT(vkCreateDevice(physical_device, &info, nullptr, &object));
		}

		~device()
		{
			vkDestroyDevice(object, nullptr);
		}

		device(device&&) = delete;
		device(const device&) = delete;
	};

	struct command_pool
	{
		VkCommandPool object;
		const VkCommandPoolCreateInfo info;

		command_pool(VkDevice dev, VkCommandPoolCreateFlags flags, uint32_t queue_family)
			: info({ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, flags, queue_family }), m_device(dev)
		{
			CHECK_VKRESULT(vkCreateCommandPool(m_device, &info, nullptr, &object));
		}

		~command_pool()
		{
			vkDestroyCommandPool(m_device, object, nullptr);
		}

		command_pool(command_pool&&) = delete;
		command_pool(const command_pool&) = delete;
	private:
		VkDevice m_device;
	};

	struct command_buffer
	{
		VkCommandBuffer object;
		const VkCommandBufferAllocateInfo info;

		command_buffer(VkDevice dev, VkCommandPool command_pool, VkCommandBufferLevel level)
			: info({ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, command_pool, level, 1 }), m_device(dev)
		{
			CHECK_VKRESULT(vkAllocateCommandBuffers(m_device, &info, &object));
		}

		~command_buffer()
		{
			vkFreeCommandBuffers(m_device, info.commandPool, 1, &object);
		}

		command_buffer(command_buffer&&) = delete;
		command_buffer(const command_buffer&) = delete;
	private:
		VkDevice m_device;
	};

	struct queue
	{
		VkQueue object;
		struct {
			uint32_t queue_family;
			uint32_t queue_index;
		} info;

		queue()
		{}

		~queue()
		{}

		queue(queue&&) = delete;
		queue(const queue&) = delete;
	private:
		VkDevice m_device;
	};

	struct memory
	{
		VkDeviceMemory object;
		const VkMemoryAllocateInfo info;

		memory(VkDevice dev, uint64_t size, uint32_t memory_type)
			: info({ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, size, memory_type }), m_device(dev)
		{
			CHECK_VKRESULT(vkAllocateMemory(dev, &info, nullptr, &object));
		}

		~memory()
		{
			vkFreeMemory(m_device, object, nullptr);
		}

		memory(memory&&) = delete;
		memory(const memory&) = delete;
	private:
		VkDevice m_device;
	};

	struct buffer
	{
		VkBuffer object;
		const VkBufferCreateInfo info;
		std::shared_ptr<memory> baking_memory;

		buffer(VkDevice dev, VkDeviceSize size, VkBufferCreateFlags flags, VkBufferUsageFlags usage)
			: info({ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, flags, size, usage, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr }), m_device(dev)
		{
			CHECK_VKRESULT(vkCreateBuffer(m_device, &info, nullptr, &object));
		}

		~buffer()
		{
			vkDestroyBuffer(m_device, object, nullptr);
		}

		buffer(buffer&&) = delete;
		buffer(const buffer&) = delete;
	private:
		VkDevice m_device;
	};

	struct image
	{
		VkImage object;
		const VkImageCreateInfo info = {};
		std::shared_ptr<memory> baking_memory;

		// Swap images are not created
		image(VkImage img) : object(img)
		{}

		image(VkDevice dev, VkImageCreateFlags flags, VkImageType type, VkFormat format, VkExtent3D extent, uint32_t miplevels, uint32_t arraylayers,
			VkSampleCountFlagBits samples, VkImageTiling tiling, VkImageUsageFlags usage, VkImageLayout initial_layout)
			: info({ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, flags, type, format, extent, miplevels, arraylayers, samples, tiling, usage, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr, initial_layout }), m_device(dev)
		{
			CHECK_VKRESULT(vkCreateImage(m_device, &info, nullptr, &object));
		}

		~image()
		{
			// This image was not created
			if (info.sType)
				vkDestroyImage(m_device, object, nullptr);
		}

		image(image&&) = delete;
		image(const image&) = delete;
	private:
		VkDevice m_device;
	};

	struct image_view
	{
		VkImageView object;
		const VkImageViewCreateInfo info;

		image_view(VkDevice dev, VkImage image, VkImageViewType viewtype, VkFormat format, VkComponentMapping mapping, VkImageSubresourceRange range)
			: info({ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0, image, viewtype, format, mapping, range }), m_device(dev)
		{
			CHECK_VKRESULT(vkCreateImageView(m_device, &info, nullptr, &object));
		}

		~image_view()
		{
			vkDestroyImageView(m_device, object, nullptr);
		}

		image_view(image_view&&) = delete;
		image_view(const image_view&) = delete;
	private:
		VkDevice m_device;
	};

	struct descriptor_pool
	{
		VkDescriptorPool object;
		const std::vector<VkDescriptorPoolSize> pool_size;
		const VkDescriptorPoolCreateInfo info;

		descriptor_pool(VkDevice dev, VkDescriptorPoolCreateFlags flags, uint32_t max_sets, const std::vector<VkDescriptorPoolSize> &ps)
			: pool_size(ps), info({ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr, flags, max_sets, static_cast<uint32_t>(pool_size.size()), pool_size.data() }), m_device(dev)
		{
			CHECK_VKRESULT(vkCreateDescriptorPool(m_device, &info, nullptr, &object));
		}

		~descriptor_pool()
		{
			vkDestroyDescriptorPool(m_device, object, nullptr);
		}

		descriptor_pool(descriptor_pool&&) = delete;
		descriptor_pool(const descriptor_pool&) = delete;
	private:
		VkDevice m_device;
	};



	struct render_pass
	{
		VkRenderPass object;
		const std::vector<VkAttachmentDescription> attachments;
		const std::vector<VkSubpassDescription> subpasses;
		const std::vector<VkSubpassDependency> dependencies;
		const VkRenderPassCreateInfo info;

		render_pass(VkDevice dev, std::vector<VkAttachmentDescription> attachments_desc, std::vector<VkSubpassDescription> subpass_desc, std::vector<VkSubpassDependency> dependency)
			: attachments(attachments_desc), subpasses(subpass_desc), dependencies(dependency),
			info({VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0,
				static_cast<uint32_t>(attachments.size()), attachments.data(),
				static_cast<uint32_t>(subpasses.size()), subpasses.data(),
				static_cast<uint32_t>(dependencies.size()), dependencies.data(),
		}), m_device(dev)
		{
			CHECK_VKRESULT(vkCreateRenderPass(m_device, &info, nullptr, &object))
		}

		~render_pass()
		{
			vkDestroyRenderPass(m_device, object, nullptr);
		}

		render_pass(render_pass&&) = delete;
		render_pass(const render_pass&) = delete;
	private:
		VkDevice m_device;
	};


	struct shader_module
	{
		VkShaderModule object;
		const std::vector<uint32_t> spirv_code;
		const VkShaderModuleCreateInfo info;


		shader_module(VkDevice dev, const std::string &filename) :
			spirv_code(load_binary_file(filename)),
			info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, spirv_code.size() * sizeof(uint32_t), spirv_code.data()},
			m_device(dev)
		{
			CHECK_VKRESULT(vkCreateShaderModule(dev, &info, nullptr, &object));
		}

		~shader_module()
		{
			vkDestroyShaderModule(m_device, object, nullptr);
		}

		shader_module(shader_module&&) = delete;
		shader_module(const shader_module&) = delete;

	private:
		VkDevice m_device;

		static std::vector<uint32_t> load_binary_file(const std::string &filename)
		{
			std::ifstream file(filename, std::ios::binary | std::ios::in | std::ios::ate);
			if (!file.is_open()) throw;

			std::vector<uint32_t> code(file.tellg() / sizeof(uint32_t));
			// go back to the beginning
			file.seekg(0, std::ios::beg);
			file.read((char*)code.data(), code.size() * sizeof(uint32_t));
			file.close();
			return code;
		}
	};


	struct pipeline
	{
		VkPipeline object;
		const std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
		const VkPipelineVertexInputStateCreateInfo vertex_input_state;
		const VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
		const VkPipelineTessellationStateCreateInfo tessellation_state;
		const VkPipelineViewportStateCreateInfo viewport_state;
		const VkPipelineRasterizationStateCreateInfo rasterization_state;
		const VkPipelineMultisampleStateCreateInfo multisample_state;
		const VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
		const VkPipelineColorBlendStateCreateInfo color_blend_state;
		const VkPipelineDynamicStateCreateInfo dynamic_state;
		const VkGraphicsPipelineCreateInfo info;


		pipeline(VkDevice dev, VkPipelineCreateFlags flags, const std::vector<VkPipelineShaderStageCreateInfo> &shaders,
			const VkPipelineVertexInputStateCreateInfo &vis,
			const VkPipelineInputAssemblyStateCreateInfo &ias,
			const VkPipelineTessellationStateCreateInfo &ts,
			const VkPipelineViewportStateCreateInfo &vs,
			const VkPipelineRasterizationStateCreateInfo &rs,
			const VkPipelineMultisampleStateCreateInfo &ms,
			const VkPipelineDepthStencilStateCreateInfo &dss,
			const VkPipelineColorBlendStateCreateInfo &cbs,
			const VkPipelineDynamicStateCreateInfo &ds,
			VkPipelineLayout layout, VkRenderPass render_pass, uint32_t subpass, VkPipeline base_pipeline, int32_t base_pipeline_index)
			: shader_stages(shaders), vertex_input_state(vis), input_assembly_state(ias), tessellation_state(ts), viewport_state(vs), rasterization_state(rs),
			multisample_state(ms), depth_stencil_state(dss), color_blend_state(cbs), dynamic_state(ds),
			info(
				{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, flags, static_cast<uint32_t>(shader_stages.size()), shader_stages.data(),
				&vertex_input_state, &input_assembly_state, &tessellation_state, &viewport_state, &rasterization_state, &multisample_state, &depth_stencil_state, &color_blend_state, &dynamic_state,
				layout, render_pass, subpass, base_pipeline, base_pipeline_index }
			), m_device(dev)
		{
			CHECK_VKRESULT(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &info, nullptr, &object));
		}

		~pipeline()
		{
			vkDestroyPipeline(m_device, VK_NULL_HANDLE, nullptr);
		}

		pipeline(pipeline&&) = delete;
		pipeline(const pipeline&) = delete;
	private:
		VkDevice m_device;
	};


	struct pipeline_layout
	{
		VkPipelineLayout object;
		const std::vector<VkDescriptorSetLayout> set_layouts;
		const std::vector<VkPushConstantRange> push_constant_ranges;
		const VkPipelineLayoutCreateInfo info;

		pipeline_layout(VkDevice dev, VkPipelineLayoutCreateFlags flags, const std::vector<VkDescriptorSetLayout> &sets, const std::vector<VkPushConstantRange> &pcr)
			: set_layouts(sets), push_constant_ranges(pcr),
			info({ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, flags, static_cast<uint32_t>(set_layouts.size()), set_layouts.data(), static_cast<uint32_t>(push_constant_ranges.size()), push_constant_ranges.data() }),
			m_device(dev)
		{
			CHECK_VKRESULT(vkCreatePipelineLayout(m_device, &info, nullptr, &object));
		}

		~pipeline_layout()
		{
			vkDestroyPipelineLayout(m_device, object, nullptr);
		}


		pipeline_layout(const pipeline_layout&) = delete;
	private:
		VkDevice m_device;
	};

	struct swapchain
	{
		VkSwapchainKHR object;
		VkSwapchainCreateInfoKHR info;

		swapchain(VkDevice dev) : m_device(dev)
		{}

		~swapchain()
		{
			vkDestroySwapchainKHR(m_device, object, nullptr);
		}

		swapchain(swapchain&&) = delete;
		swapchain(const swapchain&) = delete;
	private:
		VkDevice m_device;
	};

	struct framebuffer
	{
		VkFramebuffer object;
		const std::vector<VkImageView> attachements;
		const VkFramebufferCreateInfo info;

		framebuffer(VkDevice dev, VkRenderPass render_pass, const std::vector<VkImageView> &att, uint32_t width, uint32_t height, uint32_t layers)
			: attachements(att),
			info({ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, render_pass, static_cast<uint32_t>(attachements.size()), attachements.data(), width, height, layers }),
			m_device(dev)
		{
			CHECK_VKRESULT(vkCreateFramebuffer(m_device, &info, nullptr, &object));
		}

		~framebuffer()
		{
			vkDestroyFramebuffer(m_device, object, nullptr);
		}

		framebuffer(framebuffer&&) = delete;
		framebuffer(const framebuffer&) = delete;
	private:
		VkDevice m_device;
	};
}

using command_list_storage_t = std::shared_ptr<vulkan_wrapper::command_pool>;
using command_list_t = std::shared_ptr<vulkan_wrapper::command_buffer>;
using device_t = std::shared_ptr<vulkan_wrapper::device>;
using command_queue_t = std::shared_ptr<vulkan_wrapper::queue>;
using buffer_t = std::shared_ptr<vulkan_wrapper::buffer>;
using image_t = std::shared_ptr<vulkan_wrapper::image>;
using descriptor_storage_t = std::shared_ptr<vulkan_wrapper::descriptor_pool>;
using pipeline_state_t = std::shared_ptr<vulkan_wrapper::pipeline>;
using pipeline_layout_t = std::shared_ptr<vulkan_wrapper::pipeline_layout>;
using swap_chain_t = std::shared_ptr<vulkan_wrapper::swapchain>;
using render_pass_t = std::shared_ptr<vulkan_wrapper::render_pass>;
using clear_value_structure_t = void*;

struct vk_framebuffer
{
	const std::vector<std::shared_ptr<vulkan_wrapper::image_view> > image_views;
	vulkan_wrapper::framebuffer fbo;

	vk_framebuffer(device_t dev, render_pass_t render_pass, std::vector<std::tuple<image_t, irr::video::ECOLOR_FORMAT>> render_targets, uint32_t width, uint32_t height, uint32_t layers);
private:
	std::vector<std::shared_ptr<vulkan_wrapper::image_view> > build_image_views(VkDevice dev, const std::vector<std::tuple<image_t, irr::video::ECOLOR_FORMAT>> &render_targets);
};

using framebuffer_t = std::shared_ptr<vk_framebuffer>;


#include "GfxApi.h"
#include "../VKAPI/pipeline_helpers.h"
#include "../VKAPI/pipeline_layout_helpers.h"
#include "../VKAPI/renderpass_helpers.h"