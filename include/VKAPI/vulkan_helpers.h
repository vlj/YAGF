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
}

namespace structures
{
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
}
