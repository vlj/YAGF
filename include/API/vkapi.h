// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#include <vulkan\vk_platform.h>
#include <memory>
#include <vector>

#define CHECK_VKRESULT(cmd) { VkResult res = (cmd); if (cmd != VK_SUCCESS) throw; }

namespace vulkan_wrapper
{
	struct device
	{
		VkDevice object;
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

	struct buffer
	{
		VkBuffer object;
		const VkBufferCreateInfo info;

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
		const VkImageCreateInfo info;

		image(VkDevice dev, VkImageCreateFlags flags, VkImageType type, VkFormat format, VkExtent3D extent, uint32_t miplevels, uint32_t arraylayers,
			VkSampleCountFlagBits samples, VkImageTiling tiling, VkImageUsageFlags usage, VkImageLayout initial_layout)
			: info({ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, flags, type, format, extent, miplevels, arraylayers, samples, tiling, usage, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr, initial_layout }), m_device(dev)
		{
			CHECK_VKRESULT(vkCreateImage(m_device, &info, nullptr, &object));
		}

		~image()
		{
			vkDestroyImage(m_device, object, nullptr);
		}

		image(image&&) = delete;
		image(const image&) = delete;
	private:
		VkDevice m_device;
	};

	struct descriptor_pool
	{
		VkDescriptorPool object;
		const VkDescriptorPoolCreateInfo info;
		const std::vector<VkDescriptorPoolSize> pool_size;

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

	struct pipeline
	{
		VkPipeline object;
		const VkGraphicsPipelineCreateInfo info;
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
		const VkPipelineLayoutCreateInfo info;
		const std::vector<VkDescriptorSetLayout> set_layouts;
		const std::vector<VkPushConstantRange> push_constant_ranges;

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

		pipeline_layout(pipeline_layout&&) = delete;
		pipeline_layout(const pipeline_layout&) = delete;
	private:
		VkDevice m_device;
	};

	struct swapchain
	{
		VkSwapchainKHR object;
		VkSwapchainCreateInfoKHR info;

		swapchain()
		{}

		~swapchain()
		{}

		swapchain(swapchain&&) = delete;
		swapchain(const swapchain&) = delete;
	private:
		VkDevice m_device;
	};

	struct framebuffer
	{
		VkFramebuffer object;
		const VkFramebufferCreateInfo info;
		const std::vector<VkImageView> attachements;

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
using render_pass_t = VkRenderPass;

struct vk_framebuffer
{
	std::vector<VkImageView> image_views;
	vulkan_wrapper::framebuffer fbo;

	vk_framebuffer(device_t dev, render_pass_t render_pass, uint32_t width, uint32_t height, uint32_t layers)
		: fbo(dev->object, render_pass, image_views, width, height, layers)
	{
	}
};

using framebuffer_t = std::shared_ptr<vk_framebuffer>;

/*struct root_signature_builder
{
private:
	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE > > all_ranges;
	std::vector<D3D12_ROOT_PARAMETER> root_parameters;
	D3D12_ROOT_SIGNATURE_DESC desc = {};

	void build_root_parameter(std::vector<D3D12_DESCRIPTOR_RANGE > &&ranges, D3D12_SHADER_VISIBILITY visibility);
public:
	root_signature_builder(std::vector<std::tuple<std::vector<D3D12_DESCRIPTOR_RANGE >, D3D12_SHADER_VISIBILITY> > &&parameters, D3D12_ROOT_SIGNATURE_FLAGS flags);
	pipeline_layout_t get(device_t dev);
};*/

device_t create_device();
command_queue_t create_graphic_command_queue(device_t dev);
swap_chain_t create_swap_chain(device_t dev, command_queue_t queue, HWND window);
std::vector<image_t> get_image_view_from_swap_chain(device_t dev, swap_chain_t chain);

#include "GfxApi.h"