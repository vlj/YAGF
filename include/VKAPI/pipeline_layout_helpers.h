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
		(type == RESOURCE_VIEW::INPUT_ATTACHMENT) ? VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT :
		(type == RESOURCE_VIEW::UAV_BUFFER) ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
		(type == RESOURCE_VIEW::UAV_IMAGE) ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE :
		(type == RESOURCE_VIEW::TEXEL_BUFFER) ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : throw;
}

constexpr VkShaderStageFlags get_shader_stage(const shader_stage stage)
{
	return (stage == shader_stage::vertex_shader) ? VK_SHADER_STAGE_VERTEX_BIT :
		(stage == shader_stage::fragment_shader) ? VK_SHADER_STAGE_FRAGMENT_BIT :
		(stage == shader_stage::all) ? VK_SHADER_STAGE_ALL : throw;
}
