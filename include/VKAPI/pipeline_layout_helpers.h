// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once
#include <vulkan\vulkan.h>
#include "../API/GfxApi.h"


constexpr auto get_descriptor_type(const RESOURCE_VIEW type)
{
	return (type == RESOURCE_VIEW::CONSTANTS_BUFFER) ? vk::DescriptorType::eUniformBuffer :
		(type == RESOURCE_VIEW::SHADER_RESOURCE) ? vk::DescriptorType::eSampledImage :
		(type == RESOURCE_VIEW::SAMPLER) ? vk::DescriptorType::eSampler :
		(type == RESOURCE_VIEW::INPUT_ATTACHMENT) ? vk::DescriptorType::eInputAttachment :
		(type == RESOURCE_VIEW::UAV_BUFFER) ? vk::DescriptorType::eStorageBuffer :
		(type == RESOURCE_VIEW::UAV_IMAGE) ? vk::DescriptorType::eStorageImage :
		(type == RESOURCE_VIEW::TEXEL_BUFFER) ? vk::DescriptorType::eUniformTexelBuffer : throw;
}

constexpr auto get_shader_stage(const shader_stage stage)
{
	return (stage == shader_stage::vertex_shader) ? VK_SHADER_STAGE_VERTEX_BIT :
		(stage == shader_stage::fragment_shader) ? VK_SHADER_STAGE_FRAGMENT_BIT :
		(stage == shader_stage::all) ? VK_SHADER_STAGE_ALL : throw;
}
