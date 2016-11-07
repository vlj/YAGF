// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once
#include <vulkan\vulkan.h>
#include <array>
#include <gsl/gsl>

#define CHECK_VKRESULT(cmd) { VkResult res = (cmd); if (res != VK_SUCCESS) throw; }


namespace vulkan_wrapper
{
	template<typename T>
	struct wrapper
	{
		gsl::owner<T> object{};

		operator T()
		{
			return object;
		}
	};

	struct device : public wrapper<VkDevice>
	{
		VkPhysicalDevice physical_device;
		uint32_t upload_memory_index;
		uint32_t default_memory_index;
		uint32_t queue_family_index;

		// TODO: Make queue create info, layers and extensions member when SA is fixed
		device(VkPhysicalDevice phys_dev, const std::vector<VkDeviceQueueCreateInfo> &qci, const std::vector<const char*> &l, const std::vector<const char*> &ext)
			: physical_device(phys_dev)
		{
			const VkDeviceCreateInfo info =
			{
				VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, nullptr, 0,
				static_cast<uint32_t>(qci.size()), qci.data(),
				static_cast<uint32_t>(l.size()), l.data(),
				static_cast<uint32_t>(ext.size()), ext.data(),
				nullptr
			};
			CHECK_VKRESULT(vkCreateDevice(physical_device, &info, nullptr, &object));
			queue_family_index = qci[0].queueFamilyIndex;
		}

		~device()
		{
			vkDestroyDevice(object, nullptr);
		}

		device(device&&) = delete;
		device(const device&) = delete;
	};

	struct command_pool : public wrapper<VkCommandPool>
	{
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

	struct command_buffer : public wrapper<VkCommandBuffer>
	{
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

	struct queue : public wrapper<VkQueue>
	{
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

	struct semaphore : public wrapper<VkSemaphore>
	{
		const VkSemaphoreCreateInfo info;

		semaphore(VkDevice dev)
			: info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 }, m_device(dev)
		{
			CHECK_VKRESULT(vkCreateSemaphore(m_device, &info, nullptr, &object));
		}

		~semaphore()
		{
			vkDestroySemaphore(m_device, object, nullptr);
		}

		semaphore(semaphore&&) = delete;
		semaphore(const semaphore&) = delete;
	private:
		VkDevice m_device;
	};

	struct fence : public wrapper<VkFence>
	{
		const VkFenceCreateInfo info;

		fence(VkDevice dev)
			: info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 }, m_device(dev)
		{
			CHECK_VKRESULT(vkCreateFence(m_device, &info, nullptr, &object));
		}

		~fence()
		{
			vkDestroyFence(m_device, object, nullptr);
		}

		fence(semaphore&&) = delete;
		fence(const semaphore&) = delete;
	private:
		VkDevice m_device;
	};

	struct render_pass : public wrapper<VkRenderPass>
	{
		// TODO: Make info/attachments_desc/subpass_desc/dependency member when Static Analysis is fixed
		render_pass(VkDevice dev, std::vector<VkAttachmentDescription> &&attachments_desc, std::vector<VkSubpassDescription> &&subpass_desc, std::vector<VkSubpassDependency> &&dependency)
			: m_device(dev)
		{
			const VkRenderPassCreateInfo info =
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0,
				gsl::narrow_cast<uint32_t>(attachments_desc.size()), attachments_desc.data(),
				gsl::narrow_cast<uint32_t>(subpass_desc.size()), subpass_desc.data(),
				gsl::narrow_cast<uint32_t>(dependency.size()), dependency.data(),
			};
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


	struct shader_module : public wrapper<VkShaderModule>
	{
		const std::vector<uint32_t> spirv_code;
		const VkShaderModuleCreateInfo info;


		shader_module(VkDevice dev, const std::string &filename) :
			spirv_code(load_binary_file(filename)),
			info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, spirv_code.size() * sizeof(uint32_t), spirv_code.data() },
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


	struct pipeline : public wrapper<VkPipeline>
	{
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
			vkDestroyPipeline(m_device, object, nullptr);
		}

		pipeline(pipeline&&) = delete;
		pipeline(const pipeline&) = delete;
	private:
		VkDevice m_device;
	};

	struct compute_pipeline : public wrapper<VkPipeline>
	{
		const VkComputePipelineCreateInfo info;

		compute_pipeline(VkDevice dev, const VkPipelineShaderStageCreateInfo &shader_stage,
			VkPipelineLayout layout, VkPipeline base_pipeline, int32_t base_pipeline_index)
			: info({ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0, shader_stage, layout, base_pipeline, base_pipeline_index }),
			m_device(dev)
		{
			CHECK_VKRESULT(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &info, nullptr, &object));
		}

		~compute_pipeline()
		{
			vkDestroyPipeline(m_device, object, nullptr);
		}

		compute_pipeline(compute_pipeline&&) = delete;
		compute_pipeline(const compute_pipeline&) = delete;
	private:
		VkDevice m_device;
	};

	struct pipeline_descriptor_set : public wrapper<VkDescriptorSetLayout>
	{
		const std::vector<VkDescriptorSetLayoutBinding> bindings;
		const VkDescriptorSetLayoutCreateInfo info;

		pipeline_descriptor_set(VkDevice dev, const std::vector<VkDescriptorSetLayoutBinding> &b)
			: bindings(b),
			info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, static_cast<uint32_t>(bindings.size()), bindings.data() },
			m_device(dev)
		{
			CHECK_VKRESULT(vkCreateDescriptorSetLayout(dev, &info, nullptr, &object));
		}

		~pipeline_descriptor_set()
		{
			vkDestroyDescriptorSetLayout(m_device, object, nullptr);
		}
	private:
		VkDevice m_device;
	};


	struct swapchain : public wrapper<VkSwapchainKHR>
	{
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

	struct framebuffer : public wrapper<VkFramebuffer>
	{
		const std::vector<VkImageView> attachements;
		const VkFramebufferCreateInfo info;

		framebuffer(VkDevice dev, VkRenderPass render_pass, const std::vector<VkImageView> &att, uint32_t width, uint32_t height, uint32_t layers)
			: attachements(att),
			info({ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, render_pass, gsl::narrow_cast<uint32_t>(attachements.size()), attachements.data(), width, height, layers }),
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

namespace structures
{
	constexpr VkComponentMapping component_mapping(const VkComponentSwizzle r = VK_COMPONENT_SWIZZLE_R, const VkComponentSwizzle g = VK_COMPONENT_SWIZZLE_G, const VkComponentSwizzle b = VK_COMPONENT_SWIZZLE_B, const VkComponentSwizzle a = VK_COMPONENT_SWIZZLE_A)
	{
		return{ r, g, b, a };
	}

	constexpr VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT, uint32_t base_mip_level = 0, uint32_t mip_level_count = 1, uint32_t base_layer = 0, uint32_t layer_count = 1)
	{
		return{ aspect, base_mip_level, mip_level_count, base_layer, layer_count };
	}

	inline VkWriteDescriptorSet write_descriptor_set(VkDescriptorSet dst_set, VkDescriptorType descriptor_type, const std::vector<VkDescriptorImageInfo> &image_descriptors,
		uint32_t dst_binding, uint32_t dst_array_element = 0)
	{
		return{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, dst_set, dst_binding, dst_array_element, gsl::narrow_cast<uint32_t>(image_descriptors.size()), descriptor_type, image_descriptors.data(), nullptr, nullptr };
	}

	inline VkWriteDescriptorSet write_descriptor_set(VkDescriptorSet dst_set, VkDescriptorType descriptor_type, const std::vector<VkDescriptorBufferInfo> &buffer_descriptors,
		uint32_t dst_binding, uint32_t dst_array_element = 0)
	{
		return{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, dst_set, dst_binding, dst_array_element, gsl::narrow_cast<uint32_t>(buffer_descriptors.size()), descriptor_type, nullptr, buffer_descriptors.data(), nullptr };
	}

	inline VkWriteDescriptorSet write_descriptor_set(VkDescriptorSet dst_set, VkDescriptorType descriptor_type, const std::vector<VkBufferView> &texel_buffer_view_descriptors,
		uint32_t dst_binding, uint32_t dst_array_element = 0)
	{
		return{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, dst_set, dst_binding, dst_array_element, gsl::narrow_cast<uint32_t>(texel_buffer_view_descriptors.size()), descriptor_type, nullptr, nullptr, texel_buffer_view_descriptors.data() };
	}

	inline VkClearValue clear_value(const std::array<float, 4> &rgba)
	{
		VkClearValue result;
		result.color.float32[0] = rgba[0];
		result.color.float32[1] = rgba[1];
		result.color.float32[2] = rgba[2];
		result.color.float32[3] = rgba[3];
		return result;
	}

	inline VkClearValue clear_value(float depth, uint8_t stencil)
	{
		VkClearValue result;
		result.depthStencil.depth = depth;
		result.depthStencil.stencil = stencil;
		return result;
	}

	constexpr VkAttachmentDescription attachment_description(
		VkFormat format,
		VkAttachmentLoadOp load_op,
		VkAttachmentStoreOp store_op,
		VkImageLayout initial_layout,
		VkImageLayout final_layout,
		VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT,
		VkAttachmentLoadOp stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VkAttachmentStoreOp stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VkAttachmentDescriptionFlags flag = 0)
	{
		return{ flag, format, sample_count, load_op, store_op, stencil_load_op, stencil_store_op, initial_layout, final_layout };
	}

	constexpr VkBufferCopy buffer_copy(uint64_t src_offset, uint64_t dst_offset, uint64_t size)
	{
		return{ src_offset, dst_offset, size };
	}

	constexpr VkDescriptorBufferInfo descriptor_buffer_info(VkBuffer buffer, uint64_t offset, uint64_t size)
	{
		return{ buffer, offset, size };
	}


	constexpr VkDescriptorImageInfo descriptor_sampler_info(VkSampler sampler)
	{
		return{ sampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	}
}

namespace util
{
	inline VkDescriptorSet allocate_descriptor_sets(VkDevice dev, VkDescriptorPool descriptor_pool, const std::vector<VkDescriptorSetLayout> &set_layouts)
	{
		VkDescriptorSetAllocateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, descriptor_pool, gsl::narrow_cast<uint32_t>(set_layouts.size()), set_layouts.data() };
		VkDescriptorSet result{};
		CHECK_VKRESULT(vkAllocateDescriptorSets(dev, &info, &result));
		return result;
	}

	inline void update_descriptor_sets(VkDevice dev, const std::vector<VkWriteDescriptorSet> &update_info)
	{
	}
}