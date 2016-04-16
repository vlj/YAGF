// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#include <d3d12.h>
#include "../API/GfxApi.h"


constexpr D3D12_FILL_MODE get_polygon_mode(const irr::video::E_POLYGON_MODE epm)
{
	return (epm == irr::video::E_POLYGON_MODE::EPM_FILL) ? D3D12_FILL_MODE_SOLID :
		(epm == irr::video::E_POLYGON_MODE::EPM_LINE) ? D3D12_FILL_MODE_WIREFRAME :
		(epm == irr::video::E_POLYGON_MODE::EPM_POINT) ? D3D12_FILL_MODE_WIREFRAME : throw;
}

constexpr D3D12_CULL_MODE get_cull_mode(const irr::video::E_CULL_MODE ecm)
{
	return (ecm == irr::video::E_CULL_MODE::ECM_NONE) ? D3D12_CULL_MODE_NONE :
		(ecm == irr::video::E_CULL_MODE::ECM_BACK) ? D3D12_CULL_MODE_BACK :
		(ecm == irr::video::E_CULL_MODE::ECM_FRONT) ? D3D12_CULL_MODE_FRONT : throw;
}

constexpr uint8_t get_sample_count(const irr::video::E_SAMPLE_COUNT esc)
{
	return (esc == irr::video::E_SAMPLE_COUNT::ESC_1) ? 1 :
		(esc == irr::video::E_SAMPLE_COUNT::ESC_2) ? 2 :
		(esc == irr::video::E_SAMPLE_COUNT::ESC_4) ? 4 :
		(esc == irr::video::E_SAMPLE_COUNT::ESC_8) ? 8 : throw;
}

constexpr D3D12_COMPARISON_FUNC get_compare_op(const irr::video::E_COMPARE_FUNCTION ecf)
{
	return (ecf == irr::video::E_COMPARE_FUNCTION::ECF_LESS) ? D3D12_COMPARISON_FUNC_LESS :
		(ecf == irr::video::E_COMPARE_FUNCTION::ECF_NEVER) ? D3D12_COMPARISON_FUNC_NEVER : throw;
}

constexpr D3D12_STENCIL_OP get_stencil_op(const irr::video::E_STENCIL_OP eso)
{
	return (eso == irr::video::E_STENCIL_OP::ESO_KEEP) ? D3D12_STENCIL_OP_KEEP : throw;
}

constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE get_primitive_topology(const irr::video::E_PRIMITIVE_TYPE ept)
{
	return (ept == irr::video::E_PRIMITIVE_TYPE::EPT_LINES) ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE :
		(ept == irr::video::E_PRIMITIVE_TYPE::EPT_POINTS) ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT :
		(ept == irr::video::E_PRIMITIVE_TYPE::EPT_LINE_STRIP) ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE :
		(ept == irr::video::E_PRIMITIVE_TYPE::EPT_TRIANGLE_STRIP) ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE :
		(ept == irr::video::E_PRIMITIVE_TYPE::EPT_TRIANGLES) ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE : throw;
}


constexpr D3D12_GRAPHICS_PIPELINE_STATE_DESC get_pipeline_state_desc(const pipeline_state_description desc)
{
	return{ nullptr, {}, {}, {}, {}, {}, {}, {}, (UINT)-1,
		{
			get_polygon_mode(desc.rasterization_polygon_mode),
			get_cull_mode(desc.rasterization_cull_mode),
			desc.rasterization_front_face == irr::video::E_FRONT_FACE::EFF_CCW,
			0,
			desc.rasterization_depth_bias_clamp,
			desc.rasterization_depth_bias_slope_factor,
			desc.depth_stencil_depth_clip_enable,
			desc.multisample_multisample_enable,
			false,
			0,
			desc.rasterization_conservative_enable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		},
		{
			desc.depth_stencil_depth_test,
			desc.depth_stencil_depth_write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
			get_compare_op(desc.depth_stencil_depth_compare_op),
			desc.depth_stencil_stencil_test,
			(UINT8)-1,
			(UINT8)-1,
			{ get_stencil_op(desc.depth_stencil_front_stencil_fail_op), get_stencil_op(desc.depth_stencil_front_stencil_pass_op), get_stencil_op(desc.depth_stencil_front_stencil_depth_fail_op), get_compare_op(desc.depth_stencil_front_stencil_compare_op) },
			{ get_stencil_op(desc.depth_stencil_back_stencil_fail_op), get_stencil_op(desc.depth_stencil_back_stencil_pass_op), get_stencil_op(desc.depth_stencil_back_stencil_depth_fail_op), get_compare_op(desc.depth_stencil_back_stencil_compare_op) }
		},
		{},
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
		get_primitive_topology(desc.input_assembly_topology),
		0,
		{},
		DXGI_FORMAT_UNKNOWN,
		{ get_sample_count(desc.multisample_sample_count), 0}
	};
}
