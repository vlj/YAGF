#include "shaders.h"

#include <tuple>
#include <array>
#include <unordered_map>
#include <assimp/Importer.hpp>

constexpr auto skinned_mesh_layout = pipeline_layout_description(
	descriptor_set({ range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1), range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 1, 1) }),
	descriptor_set({ range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 2, 1) }),
	descriptor_set({ range_of_descriptors(RESOURCE_VIEW::SAMPLER, 3, 1) })
);

pipeline_layout_t get_skinned_object_pipeline_layout(device_t dev)
{
	return get_pipeline_layout_from_desc(dev, skinned_mesh_layout);
}

pipeline_state_t get_skinned_object_pipeline_state(device_t dev, pipeline_layout_t layout, render_pass_t rp)
{
	constexpr pipeline_state_description pso_desc = pipeline_state_description::get();
#ifdef D3D12
	pipeline_state_t result;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc(get_pipeline_state_desc(pso_desc));
	psodesc.pRootSignature = layout.Get();

	psodesc.NumRenderTargets = 1;
	psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//    psodesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psodesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	psodesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	Microsoft::WRL::ComPtr<ID3DBlob> vtxshaderblob, pxshaderblob;
	CHECK_HRESULT(D3DReadFileToBlob(L"skinnedobject.cso", vtxshaderblob.GetAddressOf()));
	psodesc.VS.BytecodeLength = vtxshaderblob->GetBufferSize();
	psodesc.VS.pShaderBytecode = vtxshaderblob->GetBufferPointer();

	CHECK_HRESULT(D3DReadFileToBlob(L"object_gbuffer.cso", pxshaderblob.GetAddressOf()));
	psodesc.PS.BytecodeLength = pxshaderblob->GetBufferSize();
	psodesc.PS.pShaderBytecode = pxshaderblob->GetBufferPointer();

	std::vector<D3D12_INPUT_ELEMENT_DESC> IAdesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		/*        { "TEXCOORD", 1, DXGI_FORMAT_R32_SINT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 1, 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 3, DXGI_FORMAT_R32_SINT, 1, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 4, DXGI_FORMAT_R32_FLOAT, 1, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 5, DXGI_FORMAT_R32_SINT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 6, DXGI_FORMAT_R32_FLOAT, 1, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 7, DXGI_FORMAT_R32_SINT, 1, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 8, DXGI_FORMAT_R32_FLOAT, 1, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }*/
	};

	psodesc.InputLayout.pInputElementDescs = IAdesc.data();
	psodesc.InputLayout.NumElements = IAdesc.size();
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


	vulkan_wrapper::shader_module module_vert(dev->object, "..\\..\\..\\vert.spv");
	vulkan_wrapper::shader_module module_frag(dev->object, "..\\..\\..\\frag.spv");

	const std::vector<VkPipelineShaderStageCreateInfo> shader_stages{
		{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, module_vert.object, "main", nullptr },
		{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, module_frag.object, "main", nullptr }
	};

	VkPipelineVertexInputStateCreateInfo vertex_input{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	const std::vector<VkVertexInputBindingDescription> vertex_buffers{
		{ 0, static_cast<uint32_t>(sizeof(aiVector3D)), VK_VERTEX_INPUT_RATE_VERTEX },
		{ 1, static_cast<uint32_t>(sizeof(aiVector3D)), VK_VERTEX_INPUT_RATE_VERTEX },
		{ 2, static_cast<uint32_t>(sizeof(aiVector3D)), VK_VERTEX_INPUT_RATE_VERTEX }
	};
	vertex_input.pVertexBindingDescriptions = vertex_buffers.data();
	vertex_input.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_buffers.size());

	const std::vector<VkVertexInputAttributeDescription> attribute{
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
		{ 1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0 },
		{ 2, 2, VK_FORMAT_R32G32_SFLOAT, 0 },
	};
	vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute.size());
	vertex_input.pVertexAttributeDescriptions = attribute.data();

	return std::make_shared<vulkan_wrapper::pipeline>(dev->object, 0, shader_stages, vertex_input, get_pipeline_input_assembly_state_info(pso_desc), tesselation_info, viewport_info, get_pipeline_rasterization_state_create_info(pso_desc), get_pipeline_multisample_state_create_info(pso_desc), get_pipeline_depth_stencil_state_create_info(pso_desc), blend, dynamic_state_info, layout->object, rp->object, 0, VkPipeline(VK_NULL_HANDLE), 0);

#endif
}