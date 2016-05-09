// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <Scene\ssao.h>
#include "..\include\Scene\ssao.h"

namespace
{

	struct linearize_input_constant_data
	{
		float zn;
		float zf;
	};

	constexpr auto linearize_input_set_type = descriptor_set(
		{ range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 1, 1) },
		shader_stage::fragment_shader
	);

	pipeline_state_t get_linearize_pso(device_t &dev, pipeline_layout_t &layout)
	{

		constexpr pipeline_state_description pso_desc = pipeline_state_description::get();
#ifdef D3D12
		pipeline_state_t result;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc(get_pipeline_state_desc(pso_desc));
		psodesc.pRootSignature = layout.Get();

		psodesc.NumRenderTargets = 1;
		psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psodesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

		Microsoft::WRL::ComPtr<ID3DBlob> vtxshaderblob, pxshaderblob;
		CHECK_HRESULT(D3DReadFileToBlob(L"screenquad.cso", vtxshaderblob.GetAddressOf()));
		psodesc.VS.BytecodeLength = vtxshaderblob->GetBufferSize();
		psodesc.VS.pShaderBytecode = vtxshaderblob->GetBufferPointer();

		CHECK_HRESULT(D3DReadFileToBlob(L"linearize_depth.cso", pxshaderblob.GetAddressOf()));
		psodesc.PS.BytecodeLength = pxshaderblob->GetBufferSize();
		psodesc.PS.pShaderBytecode = pxshaderblob->GetBufferPointer();

		std::vector<D3D12_INPUT_ELEMENT_DESC> IAdesc =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 2 * sizeof(float), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		psodesc.InputLayout.pInputElementDescs = IAdesc.data();
		psodesc.InputLayout.NumElements = static_cast<uint32_t>(IAdesc.size());
		psodesc.NodeMask = 1;
		CHECK_HRESULT(dev->CreateGraphicsPipelineState(&psodesc, IID_PPV_ARGS(result.GetAddressOf())));
		return result;
#else
		const blend_state blend = blend_state::get();

		VkPipelineTessellationStateCreateInfo tesselation_info{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
		VkPipelineViewportStateCreateInfo viewport_info{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewport_info.viewportCount = 1;
		viewport_info.scissorCount = 1;
		std::vector<VkDynamicState> dynamic_states{ VK_DYNAMIC_STATE_VIEWPORT , VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamic_state_info{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0, static_cast<uint32_t>(dynamic_states.size()), dynamic_states.data() };


		vulkan_wrapper::shader_module module_vert(dev->object, "..\\..\\..\\screenquad.spv");
		vulkan_wrapper::shader_module module_frag(dev->object, "..\\..\\..\\linearize_depth.spv");

		const std::vector<VkPipelineShaderStageCreateInfo> shader_stages{
			{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, module_vert.object, "main", nullptr },
			{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, module_frag.object, "main", nullptr }
		};

		VkPipelineVertexInputStateCreateInfo vertex_input{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

		const std::vector<VkVertexInputBindingDescription> vertex_buffers{
			{ 0, static_cast<uint32_t>(4 * sizeof(float)), VK_VERTEX_INPUT_RATE_VERTEX },
		};
		vertex_input.pVertexBindingDescriptions = vertex_buffers.data();
		vertex_input.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_buffers.size());

		const std::vector<VkVertexInputAttributeDescription> attribute{
			{ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },
			{ 1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float) },
		};
		vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute.size());
		vertex_input.pVertexAttributeDescriptions = attribute.data();

		return std::make_shared<vulkan_wrapper::pipeline>(dev, 0, shader_stages, vertex_input, get_pipeline_input_assembly_state_info(pso_desc), tesselation_info, viewport_info, get_pipeline_rasterization_state_create_info(pso_desc), get_pipeline_multisample_state_create_info(pso_desc), get_pipeline_depth_stencil_state_create_info(pso_desc), blend, dynamic_state_info, layout->object, rp->object, 1, VkPipeline(VK_NULL_HANDLE), 0);

#endif
	}
}

ssao_utility::ssao_utility(device_t & dev)
{
#ifdef D3D12
	linearize_depth_sig = get_pipeline_layout_from_desc(dev, { linearize_input_set_type });
#endif
	linearize_depth_pso = get_linearize_pso(dev, linearize_depth_sig);

	heap = create_descriptor_storage(dev, 3, { {RESOURCE_VIEW::CONSTANTS_BUFFER, 10}, { RESOURCE_VIEW::SHADER_RESOURCE, 10 } });
	linearize_input = allocate_descriptor_set_from_cbv_srv_uav_heap(dev, *heap, 0, { nullptr });

	constant_data = create_buffer(dev, sizeof(linearize_input_constant_data), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);

#ifdef D3D12
	create_constant_buffer_view(dev, linearize_input, 0, *constant_data, sizeof(linearize_input_constant_data));
#endif
}

void ssao_utility::fill_command_list(device_t & dev, command_list_t & cmd_list, image_t & depth_buffer, float zn, float zf,
	const std::vector<std::tuple<buffer_t&, uint64_t, uint32_t, uint32_t> > &big_triangle_info)
{
	linearize_input_constant_data* ptr = reinterpret_cast<linearize_input_constant_data*>(map_buffer(dev, *constant_data));
	ptr->zn = zn;
	ptr->zf = zf;
	unmap_buffer(dev, *constant_data);
#ifdef D3D12

	std::array<ID3D12DescriptorHeap*, 1> descriptors = { heap->object };
	cmd_list->SetDescriptorHeaps(1, descriptors.data());

	create_image_view(dev, linearize_input, 1, depth_buffer, 1, irr::video::D24U8, D3D12_SRV_DIMENSION_TEXTURE2D);
	cmd_list->SetGraphicsRootSignature(linearize_depth_sig.Get());
#endif
	set_graphic_pipeline(cmd_list, linearize_depth_pso);
	bind_graphic_descriptor(cmd_list, 0, linearize_input, linearize_depth_sig);
	bind_vertex_buffers(cmd_list, 0, big_triangle_info);
	draw_non_indexed(cmd_list, 3, 1, 0, 0);
}
