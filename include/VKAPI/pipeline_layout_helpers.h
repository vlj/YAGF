// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once
#include <vulkan\vulkan.h>

enum class RESOURCE_VIEW
{
	CONSTANTS_BUFFER,
	SHADER_RESOURCE,
	SAMPLER,
	UAV,
};



struct range_of_descriptors
{
	const RESOURCE_VIEW range_type;
	const uint32_t bind_point;
	const uint32_t count;

	constexpr range_of_descriptors(const RESOURCE_VIEW rt, const uint32_t bindpoint, const uint32_t range_size)
		: range_type(rt), bind_point(bindpoint), count(range_size)
	{}

	constexpr operator VkDescriptorSetLayoutBinding() const
	{
		return{ bind_point, get_descriptor_type(range_type), count, VK_SHADER_STAGE_ALL_GRAPHICS };
	}

private:
	static constexpr VkDescriptorType get_descriptor_type(const RESOURCE_VIEW type)
	{
		return (type == RESOURCE_VIEW::CONSTANTS_BUFFER) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
			(type == RESOURCE_VIEW::SHADER_RESOURCE) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE :
				(type == RESOURCE_VIEW::SAMPLER) ? VK_DESCRIPTOR_TYPE_SAMPLER : throw;
	}
};

struct descriptor_set
{
	const std::vector<VkDescriptorSetLayoutBinding> descriptors_ranges;

	descriptor_set(std::vector<VkDescriptorSetLayoutBinding> &&dr) : descriptors_ranges(dr)
	{ }

	operator VkDescriptorSetLayoutCreateInfo() const
	{
		return{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, static_cast<uint32_t>(descriptors_ranges.size()), descriptors_ranges.data() };
	}
};

struct pipeline_layout_description
{
	const std::vector<descriptor_set> descriptor_sets;

	pipeline_layout_description(std::vector<descriptor_set> &&descriptor) : descriptor_sets(descriptor)
	{}

	pipeline_layout_description(const pipeline_layout_description&) = delete;
	pipeline_layout_description(pipeline_layout_description&&) = delete;

	VkPipelineLayout get(VkDevice dev)
	{
		VkPipelineLayout result;
		VkPipelineLayoutCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		// TODO: Store externally
		std::vector<VkDescriptorSetLayout> set_layouts;
		for (const descriptor_set &ds : descriptor_sets)
		{
			VkDescriptorSetLayoutCreateInfo set_info(ds);
			VkDescriptorSetLayout sl;
			CHECK_VKRESULT(vkCreateDescriptorSetLayout(dev, &set_info, nullptr, &sl));
			set_layouts.push_back(sl);
		}

		info.setLayoutCount = static_cast<uint32_t>(set_layouts.size());
		info.pSetLayouts = set_layouts.data();
		CHECK_VKRESULT(vkCreatePipelineLayout(dev, &info, nullptr, &result));
		return result;
	}
};
