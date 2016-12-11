// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Scene/pso.h>

#include <tuple>
#include <array>
#include <unordered_map>
#include <assimp\Importer.hpp>

const auto skinnedobject_code = std::vector<uint32_t>
#include <generatedShaders\object.h>
;

const auto object_gbuffer_code = std::vector<uint32_t>
#include <generatedShaders\object_gbuffer.h>
;

const auto sunlight_vert_code = std::vector<uint32_t>
#include <generatedShaders\skybox_vert.h>
;

const auto sunlight_frag_code = std::vector<uint32_t>
#include <generatedShaders\sunlight.h>
;

const auto ibl_frag_code = std::vector<uint32_t>
#include <generatedShaders\ibl.h>
;

const auto skybox_vert_code = std::vector<uint32_t>
#include <generatedShaders\skybox_vert.h>
;

const auto skybox_frag_code = std::vector<uint32_t>
#include <generatedShaders\skybox_frag.h>
;

std::unique_ptr<pipeline_state_t> get_skinned_object_pipeline_state(device_t& dev, pipeline_layout_t& layout, render_pass_t& rp)
{
#ifdef D3D12
	pipeline_state_t result;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc(get_pipeline_state_desc(pso_desc));
	psodesc.pRootSignature = layout.Get();

	psodesc.NumRenderTargets = 3;
	psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psodesc.RTVFormats[1] = DXGI_FORMAT_R16G16_FLOAT;
	psodesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psodesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	psodesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psodesc.BlendState.RenderTarget[1].RenderTargetWriteMask = -1;

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
	CHECK_HRESULT(dev->object->CreateGraphicsPipelineState(&psodesc, IID_PPV_ARGS(result.GetAddressOf())));
	return result;
#endif

	graphic_pipeline_state_description pso_desc = graphic_pipeline_state_description::get()
		.set_vertex_shader(skinnedobject_code)
		.set_fragment_shader(object_gbuffer_code)
		.set_vertex_attributes(std::vector<pipeline_vertex_attributes>{
		pipeline_vertex_attributes{ 0, irr::video::ECF_R32G32B32F, 0, sizeof(aiVector3D), 0 },
			pipeline_vertex_attributes{ 1, irr::video::ECF_R32G32B32F, 1, sizeof(aiVector3D), 0 },
			pipeline_vertex_attributes{ 2, irr::video::ECF_R32G32F, 2, sizeof(aiVector3D), 0 }
	});

	return dev.create_graphic_pso(pso_desc, rp, layout);

	/*	const std::vector<VkPipelineColorBlendAttachmentState> pipeline_attachments
	{
	VkPipelineColorBlendAttachmentState{false, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_COMPONENT_SWIZZLE_R | VK_COMPONENT_SWIZZLE_G | VK_COMPONENT_SWIZZLE_B| VK_COMPONENT_SWIZZLE_A },
	VkPipelineColorBlendAttachmentState{false, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_COMPONENT_SWIZZLE_R | VK_COMPONENT_SWIZZLE_G | VK_COMPONENT_SWIZZLE_B | VK_COMPONENT_SWIZZLE_A },
	VkPipelineColorBlendAttachmentState{ false, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_COMPONENT_SWIZZLE_R | VK_COMPONENT_SWIZZLE_G | VK_COMPONENT_SWIZZLE_B | VK_COMPONENT_SWIZZLE_A },
	};
	VkPipelineColorBlendStateCreateInfo blend{
	VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0, false, VK_LOGIC_OP_NO_OP, 3, pipeline_attachments.data()
	};*/
}


std::unique_ptr<pipeline_state_t> get_sunlight_pipeline_state(device_t& dev, pipeline_layout_t& layout, render_pass_t& rp)
{

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

	CHECK_HRESULT(D3DReadFileToBlob(L"sunlight.cso", pxshaderblob.GetAddressOf()));
	psodesc.PS.BytecodeLength = pxshaderblob->GetBufferSize();
	psodesc.PS.pShaderBytecode = pxshaderblob->GetBufferPointer();

	std::vector<D3D12_INPUT_ELEMENT_DESC> IAdesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 2 * sizeof(float), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	psodesc.InputLayout.pInputElementDescs = IAdesc.data();
	psodesc.InputLayout.NumElements = IAdesc.size();
	psodesc.NodeMask = 1;
	CHECK_HRESULT(dev->object->CreateGraphicsPipelineState(&psodesc, IID_PPV_ARGS(result.GetAddressOf())));
	return result;
#endif
	graphic_pipeline_state_description pso_desc = graphic_pipeline_state_description::get()
		.set_vertex_shader(sunlight_vert_code)
		.set_fragment_shader(sunlight_frag_code)
		.set_vertex_attributes(std::vector<pipeline_vertex_attributes>{
			{ 0, irr::video::ECF_R32G32F, 0, 4 * sizeof(float), 0 },
			{ 1, irr::video::ECF_R32G32F, 0, 4 * sizeof(float), 2 * sizeof(float) }
	});
	return dev.create_graphic_pso(pso_desc, rp, layout);
}

std::unique_ptr<pipeline_state_t> get_ibl_pipeline_state(device_t& dev, pipeline_layout_t& layout, render_pass_t& rp)
{
#ifdef D3D12
	pipeline_state_t result;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc(get_pipeline_state_desc(pso_desc));
	psodesc.pRootSignature = layout.Get();

	psodesc.NumRenderTargets = 1;
	psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psodesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psodesc.BlendState.RenderTarget[0].BlendEnable = true;
	psodesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psodesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psodesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	psodesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	psodesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psodesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;

	Microsoft::WRL::ComPtr<ID3DBlob> vtxshaderblob, pxshaderblob;
	CHECK_HRESULT(D3DReadFileToBlob(L"screenquad.cso", vtxshaderblob.GetAddressOf()));
	psodesc.VS.BytecodeLength = vtxshaderblob->GetBufferSize();
	psodesc.VS.pShaderBytecode = vtxshaderblob->GetBufferPointer();

	CHECK_HRESULT(D3DReadFileToBlob(L"ibl.cso", pxshaderblob.GetAddressOf()));
	psodesc.PS.BytecodeLength = pxshaderblob->GetBufferSize();
	psodesc.PS.pShaderBytecode = pxshaderblob->GetBufferPointer();

	std::vector<D3D12_INPUT_ELEMENT_DESC> IAdesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 2 * sizeof(float), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	psodesc.InputLayout.pInputElementDescs = IAdesc.data();
	psodesc.InputLayout.NumElements = IAdesc.size();
	psodesc.NodeMask = 1;
	CHECK_HRESULT(dev->object->CreateGraphicsPipelineState(&psodesc, IID_PPV_ARGS(result.GetAddressOf())));
	return result;
#endif
	graphic_pipeline_state_description pso_desc = graphic_pipeline_state_description::get()
		.set_depth_write(false)
		.set_depth_test(false)
		.set_vertex_shader(sunlight_vert_code)
		.set_fragment_shader(ibl_frag_code)
		.set_vertex_attributes(std::vector<pipeline_vertex_attributes>{
			{ 0, irr::video::ECF_R32G32F, 0, 4 * sizeof(float), 0 },
			{ 1, irr::video::ECF_R32G32F, 0, 4 * sizeof(float), 2 * sizeof(float) }
	});

	return dev.create_graphic_pso(pso_desc, rp, layout);

	/*	VkPipelineColorBlendAttachmentState blend_attachment_state{ true, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE , VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE , VK_BLEND_FACTOR_ONE , VK_BLEND_OP_ADD, -1 };
	VkPipelineColorBlendStateCreateInfo blend_state{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0, false, VK_LOGIC_OP_NO_OP, 1, &blend_attachment_state };*/
}

std::unique_ptr<pipeline_state_t> get_skybox_pipeline_state(device_t& dev, pipeline_layout_t& layout, render_pass_t& rp)
{
#ifdef D3D12
	pipeline_state_t result;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc(get_pipeline_state_desc(pso_desc));
	psodesc.pRootSignature = layout.Get();

	psodesc.NumRenderTargets = 1;
	psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psodesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	Microsoft::WRL::ComPtr<ID3DBlob> vtxshaderblob, pxshaderblob;
	CHECK_HRESULT(D3DReadFileToBlob(L"skyboxvert.cso", vtxshaderblob.GetAddressOf()));
	psodesc.VS.BytecodeLength = vtxshaderblob->GetBufferSize();
	psodesc.VS.pShaderBytecode = vtxshaderblob->GetBufferPointer();

	CHECK_HRESULT(D3DReadFileToBlob(L"skybox.cso", pxshaderblob.GetAddressOf()));
	psodesc.PS.BytecodeLength = pxshaderblob->GetBufferSize();
	psodesc.PS.pShaderBytecode = pxshaderblob->GetBufferPointer();

	std::vector<D3D12_INPUT_ELEMENT_DESC> IAdesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 2 * sizeof(float), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	psodesc.InputLayout.pInputElementDescs = IAdesc.data();
	psodesc.InputLayout.NumElements = IAdesc.size();
	psodesc.NodeMask = 1;
	CHECK_HRESULT(dev->object->CreateGraphicsPipelineState(&psodesc, IID_PPV_ARGS(result.GetAddressOf())));
	return result;
#endif
	graphic_pipeline_state_description pso_desc = graphic_pipeline_state_description::get()
		.set_depth_write(false)
		.set_depth_compare_function(irr::video::E_COMPARE_FUNCTION::ECF_LEQUAL)
		.set_vertex_shader(skybox_vert_code)
		.set_fragment_shader(skybox_frag_code)
		.set_vertex_attributes(std::vector<pipeline_vertex_attributes>{
			{ 0, irr::video::ECF_R32G32F, 0, 4 * sizeof(float), 0 },
			{ 1, irr::video::ECF_R32G32F, 0, 4 * sizeof(float), 2 * sizeof(float) }
	});
	return dev.create_graphic_pso(pso_desc, rp, layout);
}
