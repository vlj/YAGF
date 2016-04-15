// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once
#include <vulkan\vulkan.h>
#include <array>
#include <initializer_list>

struct rasterisation_state
{
	const VkPipelineRasterizationStateCreateInfo info;

	static constexpr rasterisation_state get()
	{
		return rasterisation_state(false, false, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE,
			false, 0.f, 0.f, 0.f, 1.f);
	}

	constexpr operator VkPipelineRasterizationStateCreateInfo()
	{
		return info;
	}
private:
	constexpr rasterisation_state(bool depth_clamp_enable, bool rasterizer_discard_enable, VkPolygonMode polygon_mode,
		VkCullModeFlags vk_cull_mode, VkFrontFace front_face, bool depth_bias_enable,
		float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor, float line_width)
		: info{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0, depth_clamp_enable, rasterizer_discard_enable, polygon_mode, vk_cull_mode, front_face,
		depth_bias_enable, depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor, line_width }
	{ }
};

struct depth_stencil_state
{
	const VkPipelineDepthStencilStateCreateInfo info;

	static constexpr depth_stencil_state get()
	{
		return depth_stencil_state(true, true, VK_COMPARE_OP_LESS, false, false, {}, {}, 0.f, 1.f);
	}

	constexpr operator VkPipelineDepthStencilStateCreateInfo()
	{
		return info;
	}
private:
	constexpr depth_stencil_state(bool depth_test_enable, bool depth_write_enable, VkCompareOp depth_compare_op, bool depth_bound_test_enable,
		bool stencil_test_enable, VkStencilOpState front, VkStencilOpState back, float min_depth_bound, float max_depth_bound)
		: info{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0, depth_test_enable, depth_write_enable, depth_compare_op, depth_bound_test_enable,
		stencil_test_enable, front, back, min_depth_bound, max_depth_bound }
	{ }
};

struct blend_state
{
	const VkPipelineColorBlendStateCreateInfo info;
	const std::initializer_list<VkPipelineColorBlendAttachmentState> blend_attachments;

	static constexpr blend_state get()
	{
		return blend_state(false, VK_LOGIC_OP_NO_OP, {
			{ false,
			VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
			VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT }
		}, { 1., 1., 1., 1. });
	}

	constexpr operator VkPipelineColorBlendStateCreateInfo()
	{
		return info;
	}
private:
	constexpr blend_state(bool logicop_enable, VkLogicOp op, const std::initializer_list<VkPipelineColorBlendAttachmentState> att, const std::array<float, 4> blend_constants)
		: blend_attachments(att),
		info{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0, logicop_enable, op, static_cast<uint32_t> (blend_attachments.size()), blend_attachments.begin(),
		{ blend_constants[0], blend_constants[1], blend_constants[2], blend_constants[3] } }
	{ }
};
