// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once
#include <vulkan\vulkan.h>
#include <array>
#include <initializer_list>
#include "../API/GfxApi.h"


constexpr VkPolygonMode get_polygon_mode(const irr::video::E_POLYGON_MODE epm)
{
	return (epm == irr::video::E_POLYGON_MODE::EPM_FILL) ? VK_POLYGON_MODE_FILL :
		(epm == irr::video::E_POLYGON_MODE::EPM_LINE) ? VK_POLYGON_MODE_LINE :
		(epm == irr::video::E_POLYGON_MODE::EPM_POINT) ? VK_POLYGON_MODE_POINT : throw;
}

constexpr VkCullModeFlags get_cull_mode(const irr::video::E_CULL_MODE ecm)
{
	return (ecm == irr::video::E_CULL_MODE::ECM_NONE) ? VK_CULL_MODE_NONE :
		(ecm == irr::video::E_CULL_MODE::ECM_BACK) ? VK_CULL_MODE_BACK_BIT :
		(ecm == irr::video::E_CULL_MODE::ECM_FRONT) ? VK_CULL_MODE_FRONT_BIT :
		(ecm == irr::video::E_CULL_MODE::ECM_FRONT_AND_BACK) ? VK_CULL_MODE_FRONT_AND_BACK : throw;
}

constexpr VkFrontFace get_front_face(const irr::video::E_FRONT_FACE eff)
{
	return (eff == irr::video::E_FRONT_FACE::EFF_CCW) ? VK_FRONT_FACE_COUNTER_CLOCKWISE :
		(eff == irr::video::E_FRONT_FACE::EFF_CW) ? VK_FRONT_FACE_CLOCKWISE : throw;
}

constexpr VkPipelineRasterizationStateCreateInfo get_pipeline_rasterization_state_create_info(const pipeline_state_description desc)
{
	return { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr,
		0,
		desc.rasterization_depth_clamp_enable,
		desc.rasterization_discard_enable,
		get_polygon_mode(desc.rasterization_polygon_mode),
		get_cull_mode(desc.rasterization_cull_mode),
		get_front_face(desc.rasterization_front_face),
		desc.rasterization_depth_bias_enable,
		desc.rasterization_depth_bias_constant_factor,
		desc.rasterization_depth_bias_clamp,
		desc.rasterization_depth_bias_slope_factor,
		desc.rasterization_line_width };
}

constexpr VkSampleCountFlagBits get_sample_count(const irr::video::E_SAMPLE_COUNT esc)
{
	return (esc == irr::video::E_SAMPLE_COUNT::ESC_1) ? VK_SAMPLE_COUNT_1_BIT :
		(esc == irr::video::E_SAMPLE_COUNT::ESC_2) ? VK_SAMPLE_COUNT_2_BIT :
		(esc == irr::video::E_SAMPLE_COUNT::ESC_4) ? VK_SAMPLE_COUNT_4_BIT :
		(esc == irr::video::E_SAMPLE_COUNT::ESC_8) ? VK_SAMPLE_COUNT_8_BIT : throw;
}

constexpr VkPipelineMultisampleStateCreateInfo get_pipeline_multisample_state_create_info(const pipeline_state_description desc)
{
	return{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0,
		get_sample_count(desc.multisample_sample_count),
		desc.multisample_multisample_enable,
		desc.multisample_min_sample_shading,
		nullptr,
		desc.multisample_alpha_to_coverage,
		desc.multisample_alpha_to_one
	};
}

constexpr VkCompareOp get_compare_op(const irr::video::E_COMPARE_FUNCTION ecf)
{
	return (ecf == irr::video::E_COMPARE_FUNCTION::ECF_LESS) ? VK_COMPARE_OP_LESS :
		(ecf == irr::video::E_COMPARE_FUNCTION::ECF_NEVER) ? VK_COMPARE_OP_NEVER : throw;
}

constexpr VkStencilOp get_stencil_op(const irr::video::E_STENCIL_OP eso)
{
	return (eso == irr::video::E_STENCIL_OP::ESO_KEEP) ? VK_STENCIL_OP_KEEP : throw;
}

constexpr VkPipelineDepthStencilStateCreateInfo get_pipeline_depth_stencil_state_create_info(const pipeline_state_description desc)
{
	return{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0,
		desc.depth_stencil_depth_test,
		desc.depth_stencil_depth_write,
		get_compare_op(desc.depth_stencil_depth_compare_op),
		desc.depth_stencil_depth_clip_enable,
		desc.depth_stencil_stencil_test,
		{ get_stencil_op(desc.depth_stencil_front_stencil_fail_op), get_stencil_op(desc.depth_stencil_front_stencil_pass_op), get_stencil_op(desc.depth_stencil_front_stencil_depth_fail_op), get_compare_op(desc.depth_stencil_front_stencil_compare_op) },
		{ get_stencil_op(desc.depth_stencil_back_stencil_fail_op), get_stencil_op(desc.depth_stencil_back_stencil_pass_op), get_stencil_op(desc.depth_stencil_back_stencil_depth_fail_op), get_compare_op(desc.depth_stencil_back_stencil_compare_op) },
		desc.depth_stencil_min_depth_clip,
		desc.depth_stencil_max_depth_clip };
}

constexpr VkPrimitiveTopology get_primitive_topology(const irr::video::E_PRIMITIVE_TYPE ept)
{
	return (ept == irr::video::E_PRIMITIVE_TYPE::EPT_LINES) ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST :
		(ept == irr::video::E_PRIMITIVE_TYPE::EPT_POINTS) ? VK_PRIMITIVE_TOPOLOGY_POINT_LIST :
		(ept == irr::video::E_PRIMITIVE_TYPE::EPT_LINE_STRIP) ? VK_PRIMITIVE_TOPOLOGY_LINE_STRIP :
		(ept == irr::video::E_PRIMITIVE_TYPE::EPT_TRIANGLE_STRIP) ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP :
		(ept == irr::video::E_PRIMITIVE_TYPE::EPT_TRIANGLES) ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : throw;
}

constexpr VkPipelineInputAssemblyStateCreateInfo get_pipeline_input_assembly_state_info(const pipeline_state_description desc)
{
	return{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0,
		get_primitive_topology(desc.input_assembly_topology), desc.input_assembly_primitive_restart };
}


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
