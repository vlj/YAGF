// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once
#include <vulkan\vulkan.h>
#include "../API/GfxApi.h"


constexpr VkDescriptorType get_descriptor_type(const RESOURCE_VIEW type)
{
	return (type == RESOURCE_VIEW::CONSTANTS_BUFFER) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
		(type == RESOURCE_VIEW::SHADER_RESOURCE) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE :
		(type == RESOURCE_VIEW::SAMPLER) ? VK_DESCRIPTOR_TYPE_SAMPLER :
		(type == RESOURCE_VIEW::INPUT_ATTACHMENT) ? VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT : throw;
}

template<size_t N>
pipeline_layout_t get_pipeline_layout_from_desc(device_t dev, const pipeline_layout_description_<N> desc)
{
	pipeline_layout_t result;
	std::vector<vulkan_wrapper::pipeline_descriptor_set> set_layouts;
	set_layouts.reserve(N);
	for (const descriptor_set_ &ds : desc.descriptors_sets)
	{
		std::vector<VkDescriptorSetLayoutBinding> descriptor_range_storage;
		descriptor_range_storage.reserve(ds.count);
		for (uint32_t i = 0; i < ds.count; i++)
		{
			const range_of_descriptors &rod = ds.descriptors_ranges[i];
			VkDescriptorSetLayoutBinding range{};
			range.binding = rod.bind_point;
			range.descriptorCount = rod.count;
			range.descriptorType = get_descriptor_type(rod.range_type);
			range.stageFlags = VK_SHADER_STAGE_ALL;
			descriptor_range_storage.emplace_back(range);

		}
		set_layouts.emplace_back(dev->object, descriptor_range_storage);
	}

	return std::make_shared<vulkan_wrapper::pipeline_layout>(dev->object, 0, std::move(set_layouts), std::vector<VkPushConstantRange>());
}