// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once
#include <vulkan\vulkan.h>
#include "../API/GfxApi.h"


static constexpr VkDescriptorType get_descriptor_type(const RESOURCE_VIEW type)
{
	return (type == RESOURCE_VIEW::CONSTANTS_BUFFER) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
		(type == RESOURCE_VIEW::SHADER_RESOURCE) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE :
		(type == RESOURCE_VIEW::SAMPLER) ? VK_DESCRIPTOR_TYPE_SAMPLER : throw;
}

template<size_t N>
pipeline_layout_t get_pipeline_layout_from_desc(device_t dev, const pipeline_layout_description_<N> desc)
{
	pipeline_layout_t result;

	std::vector<std::vector<VkDescriptorSetLayoutBinding> > all_descriptor_range_storage;
	all_descriptor_range_storage.reserve(N);
	// TODO: Store externally
	std::vector<VkDescriptorSetLayout> set_layouts;
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
			descriptor_range_storage.emplace_back(range);
		}
		all_descriptor_range_storage.emplace_back(descriptor_range_storage);
		VkDescriptorSetLayoutCreateInfo set_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0,
			static_cast<uint32_t>(all_descriptor_range_storage.back().size()),
			all_descriptor_range_storage.back().data() };
		VkDescriptorSetLayout sl;
		CHECK_VKRESULT(vkCreateDescriptorSetLayout(dev->object, &set_info, nullptr, &sl));
		set_layouts.push_back(sl);
	}

	return std::make_shared<vulkan_wrapper::pipeline_layout>(dev->object, 0, set_layouts, std::vector<VkPushConstantRange>());
}