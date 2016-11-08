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
