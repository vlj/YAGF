// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once
#include <vulkan\vulkan.h>
#include <array>
#include <initializer_list>


struct blend_state
{
	const std::vector<VkPipelineColorBlendAttachmentState> blend_attachments;
	const VkPipelineColorBlendStateCreateInfo info;

	static blend_state get()
	{
		return blend_state(false, VK_LOGIC_OP_NO_OP, {
			{ false,
			VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
			VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT }
		}, { 1., 1., 1., 1. });
	}

	operator VkPipelineColorBlendStateCreateInfo() const
	{
		return info;
	}
private:
	blend_state(bool logicop_enable, VkLogicOp op, const std::vector<VkPipelineColorBlendAttachmentState> att, const std::array<float, 4> blend_constants)
		: blend_attachments(att),
		info{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0, logicop_enable, op, static_cast<uint32_t> (blend_attachments.size()), blend_attachments.data(),
		{ blend_constants[0], blend_constants[1], blend_constants[2], blend_constants[3] } }
	{ }
};
