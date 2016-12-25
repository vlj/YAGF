// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <Scene\ssao.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

const auto gaussian_h_code = std::vector<uint32_t>
#include <generatedShaders\gaussian_h.h>
;

const auto gaussian_v_code = std::vector<uint32_t>
#include <generatedShaders\gaussian_v.h>
;

const auto sunlight_vert_code = std::vector<uint32_t>
#include <generatedShaders\screenquad.h>
;

const auto ssao_code = std::vector<uint32_t>
#include <generatedShaders\ssao.h>
;

const auto linearize_depth_code = std::vector<uint32_t>
#include <generatedShaders\linearize_depth.h>
;

namespace
{
	struct linearize_input_constant_data
	{
		float zn;
		float zf;
	};

	struct ssao_input_constant_data
	{
		float ProjectionMatrix00;
		float ProjectionMatrix11;
		float width;
		float height;
		float radius;
		float tau;
		float beta;
		float epsilon;
	};

	const auto linearize_input_set_type = descriptor_set(
		{ range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 1, 1) },
		shader_stage::fragment_shader
	);

	const auto ssao_input_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 2, 1) },
		shader_stage::fragment_shader);

	const auto gaussian_input_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 1, 1),
		range_of_descriptors(RESOURCE_VIEW::UAV_IMAGE, 2, 1) },
		shader_stage::all);

	// Bilinear and nearest
	const auto samplers_set_type = descriptor_set(
		{ range_of_descriptors(RESOURCE_VIEW::SAMPLER, 3, 1),
		range_of_descriptors(RESOURCE_VIEW::SAMPLER, 4, 1)},
		shader_stage::all);

	auto get_linearize_pso(device_t &dev, pipeline_layout_t &layout, render_pass_t& rp)
	{
		auto attribs = std::vector<pipeline_vertex_attributes>{
			{ 0, irr::video::ECF_R32G32F, 0, 4 * sizeof(float), 0 },
			{ 1, irr::video::ECF_R32G32F, 0, 4 * sizeof(float), 2 * sizeof(float) },
		};
		auto pso_desc = graphic_pipeline_state_description::get()
			.set_vertex_shader(sunlight_vert_code)
			.set_fragment_shader(linearize_depth_code)
			.set_vertex_attributes(attribs)
			.set_color_outputs(std::vector<color_output>{ {false} });
#ifdef D3D12
		pipeline_state_t result;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc(get_pipeline_state_desc(pso_desc));
		psodesc.pRootSignature = layout.Get();

		psodesc.NumRenderTargets = 1;
		psodesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
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
		return dev.create_graphic_pso(pso_desc, rp, layout, 0);
/*	const blend_state blend = blend_state::get();

		VkPipelineTessellationStateCreateInfo tesselation_info{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
		VkPipelineViewportStateCreateInfo viewport_info{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewport_info.viewportCount = 1;
		viewport_info.scissorCount = 1;
		std::vector<VkDynamicState> dynamic_states{ VK_DYNAMIC_STATE_VIEWPORT , VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamic_state_info{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0, static_cast<uint32_t>(dynamic_states.size()), dynamic_states.data() };


		vulkan_wrapper::shader_module module_vert(dev, "..\\..\\..\\sunlight_vert.spv");
		vulkan_wrapper::shader_module module_frag(dev, "..\\..\\..\\linearize_depth.spv");

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

		return std::make_shared<vulkan_wrapper::pipeline>(dev, 0, shader_stages, vertex_input, get_pipeline_input_assembly_state_info(pso_desc), tesselation_info, viewport_info, get_pipeline_rasterization_state_create_info(pso_desc), get_pipeline_multisample_state_create_info(pso_desc), get_pipeline_depth_stencil_state_create_info(pso_desc), blend, dynamic_state_info, layout->object, rp, 1, VkPipeline(VK_NULL_HANDLE), 0);*/
#endif
	}

	auto get_ssao_pso(device_t &dev, pipeline_layout_t &layout, render_pass_t &rp)
	{
		auto attribs = std::vector<pipeline_vertex_attributes>{
			{ 0, irr::video::ECF_R32G32F, 0, 4 * sizeof(float), 0 },
			{ 1, irr::video::ECF_R32G32F, 0, 4 * sizeof(float), 2 * sizeof(float) },
		};
		auto pso_desc = graphic_pipeline_state_description::get()
			.set_vertex_shader(sunlight_vert_code)
			.set_fragment_shader(ssao_code)
			.set_vertex_attributes(attribs)
			.set_color_outputs(std::vector<color_output>{ {false} });
		return dev.create_graphic_pso(pso_desc, rp, layout, 1);
	}

	auto get_gaussian_h_pso(device_t& dev, pipeline_layout_t& layout)
	{
		auto pso_desc = compute_pipeline_state_description{}
			.set_compute_shader(gaussian_h_code);
		return dev.create_compute_pso(pso_desc, layout);
	}

	auto get_gaussian_v_pso(device_t& dev, pipeline_layout_t& layout)
	{
		auto pso_desc = compute_pipeline_state_description{}
		.set_compute_shader(gaussian_v_code);
		return dev.create_compute_pso(pso_desc, layout);
	}

	auto create_render_pass(device_t& dev)
	{
		return dev.create_ssao_pass();
	}
}

ssao_utility::ssao_utility(device_t & dev, image_t* _depth_input, uint32_t w, uint32_t h) : depth_input(_depth_input), width(w), height(h)
{
	linearize_input_set = dev.get_object_descriptor_set(linearize_input_set_type);
	samplers_set = dev.get_object_descriptor_set(samplers_set_type);
	linearize_depth_sig = dev.create_pipeline_layout(std::vector<const descriptor_set_layout*>{ linearize_input_set.get(), samplers_set.get() });
	ssao_input_set = dev.get_object_descriptor_set(ssao_input_set_type);
	ssao_sig = dev.create_pipeline_layout(std::vector<const descriptor_set_layout*>{ ssao_input_set.get(), samplers_set.get() });
	gaussian_input_set = dev.get_object_descriptor_set(gaussian_input_set_type);
	gaussian_input_sig = dev.create_pipeline_layout(std::vector<const descriptor_set_layout*>{ gaussian_input_set.get(), samplers_set.get() });

	render_pass = create_render_pass(dev);
	linearize_depth_pso = get_linearize_pso(dev, *linearize_depth_sig, *render_pass);
	ssao_pso = get_ssao_pso(dev, *ssao_sig, *render_pass);
	gaussian_h_pso = get_gaussian_h_pso(dev, *gaussian_input_sig);
	gaussian_v_pso = get_gaussian_v_pso(dev, *gaussian_input_sig);

	heap = dev.create_descriptor_storage(5, { {RESOURCE_VIEW::CONSTANTS_BUFFER, 10}, { RESOURCE_VIEW::SHADER_RESOURCE, 10 }, { RESOURCE_VIEW::UAV_IMAGE, 10 } });
	sampler_heap = dev.create_descriptor_storage(1, { { RESOURCE_VIEW::SAMPLER, 10 } });
	linearize_input = heap->allocate_descriptor_set_from_cbv_srv_uav_heap(0, { linearize_input_set.get() }, 2);
	ssao_input = heap->allocate_descriptor_set_from_cbv_srv_uav_heap(2, { ssao_input_set.get() }, 2);
	sampler_input = sampler_heap->allocate_descriptor_set_from_sampler_heap(0, { samplers_set.get() }, 2);
	gaussian_input_h = heap->allocate_descriptor_set_from_cbv_srv_uav_heap(4, { gaussian_input_set.get() }, 3);
	gaussian_input_v = heap->allocate_descriptor_set_from_cbv_srv_uav_heap(7, { gaussian_input_set.get() }, 3);

	linearize_constant_data = dev.create_buffer(sizeof(linearize_input_constant_data), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_uniform);
	ssao_constant_data = dev.create_buffer(sizeof(ssao_input_constant_data), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_uniform);
	clear_value_t clear_value = get_clear_value(irr::video::ECF_R32F, { 0., 0., 0., 0. });
	linear_depth_buffer = dev.create_image(irr::video::ECF_R32F, width, height, 1, 1, usage_render_target | usage_sampled, &clear_value);
	clear_value = get_clear_value(irr::video::ECF_R16F, { 0., 0., 0., 0. });
	ssao_result = dev.create_image(irr::video::ECF_R16F, width, height, 1, 1, usage_render_target | usage_sampled, &clear_value);
	gaussian_blurring_buffer = dev.create_image(irr::video::ECF_R16F, width, height, 1, 1, usage_uav | usage_sampled, nullptr);
	ssao_bilinear_result = dev.create_image(irr::video::ECF_R16F, width, height, 1, 1, usage_uav | usage_sampled, nullptr);

	dev.set_constant_buffer_view(*linearize_input, 0, 0, *linearize_constant_data, sizeof(linearize_input_constant_data));
	dev.set_constant_buffer_view(*ssao_input, 0, 0, *ssao_constant_data, sizeof(ssao_input_constant_data));

	depth_image_view = dev.create_image_view(*depth_input, irr::video::D24U8, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D, irr::video::E_ASPECT::EA_DEPTH);
	linear_depth_buffer_view = dev.create_image_view(*linear_depth_buffer, irr::video::ECF_R32F, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);
	ssao_result_view = dev.create_image_view(*ssao_result, irr::video::ECF_R16F, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);
	gaussian_blurring_buffer_view = dev.create_image_view(*gaussian_blurring_buffer, irr::video::ECF_R16F, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);
	ssao_bilinear_result_view = dev.create_image_view(*ssao_bilinear_result, irr::video::ECF_R16F, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);

	linear_depth_fbo = dev.create_frame_buffer(std::vector<const image_view_t*>{ linear_depth_buffer_view.get(), ssao_result_view.get() }, width, height, render_pass.get());

	dev.set_image_view(*linearize_input, 1, 1, *depth_image_view);
	dev.set_image_view(*ssao_input, 1, 2, *linear_depth_buffer_view);
	dev.set_image_view(*gaussian_input_h, 1, 1, *ssao_result_view);
	dev.set_image_view(*gaussian_input_v, 1, 1, *gaussian_blurring_buffer_view);
	bilinear_clamped_sampler = dev.create_sampler(SAMPLER_TYPE::BILINEAR_CLAMPED);
	nearest_sampler = dev.create_sampler(SAMPLER_TYPE::NEAREST);
	dev.set_sampler(*sampler_input, 0, 3, *bilinear_clamped_sampler);
	dev.set_sampler(*sampler_input, 1, 4, *nearest_sampler);
	dev.set_uav_image_view(*gaussian_input_h, 2, 2, *gaussian_blurring_buffer_view);
	dev.set_uav_image_view(*gaussian_input_v, 2, 2, *ssao_bilinear_result_view);
}

void ssao_utility::fill_command_list(device_t & dev, command_list_t & cmd_list, image_t & depth_buffer, float zn, float zf,
	const std::vector<std::tuple<buffer_t&, uint64_t, uint32_t, uint32_t> > &big_triangle_info)
{
	linearize_input_constant_data* ptr = static_cast<linearize_input_constant_data*>(linearize_constant_data->map_buffer());
	ptr->zn = zn;
	ptr->zf = zf;
	linearize_constant_data->unmap_buffer();

	cmd_list.set_graphic_pipeline_layout(*linearize_depth_sig);
	cmd_list.set_descriptor_storage_referenced(*heap, sampler_heap.get());
#ifdef D3D12
	cmd_list->OMSetRenderTargets(1, &(linear_depth_fbo->rtt_heap->GetCPUDescriptorHandleForHeapStart()), false, nullptr);
#endif
	cmd_list.begin_renderpass(*render_pass, *linear_depth_fbo,
		std::vector<clear_value_t>{std::array<float, 4>{}, std::array<float, 4>{}},
		width, height);
	cmd_list.set_graphic_pipeline(*linearize_depth_pso);
	cmd_list.bind_graphic_descriptor(0, *linearize_input, *linearize_depth_sig);
	cmd_list.bind_graphic_descriptor(1, *sampler_input, *linearize_depth_sig);
	cmd_list.bind_vertex_buffers(0, big_triangle_info);
	cmd_list.draw_non_indexed(3, 1, 0, 0);

	glm::mat4 Perspective = glm::perspective(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
	ssao_input_constant_data* ssao_ptr = static_cast<ssao_input_constant_data*>(ssao_constant_data->map_buffer());
	float *tmp = reinterpret_cast<float*>(&Perspective);
	ssao_ptr->ProjectionMatrix00 = tmp[0];
	ssao_ptr->ProjectionMatrix11 = tmp[5];
	ssao_ptr->width = static_cast<float>(width);
	ssao_ptr->height = static_cast<float>(height);
	ssao_ptr->radius = 100.f;
	ssao_ptr->tau = 7.f;
	ssao_ptr->beta = .1f;
	ssao_ptr->epsilon = .1f;
	ssao_constant_data->unmap_buffer();
#ifdef D3D12
	cmd_list->OMSetRenderTargets(1, &(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(linear_depth_fbo->rtt_heap->GetCPUDescriptorHandleForHeapStart())
			.Offset(1, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV))), false, nullptr);
#endif
	cmd_list.next_subpass();
	cmd_list.set_graphic_pipeline_layout(*ssao_sig);
	cmd_list.set_graphic_pipeline(*ssao_pso);
	cmd_list.bind_graphic_descriptor(0, *ssao_input, *ssao_sig);
	cmd_list.bind_graphic_descriptor(1, *sampler_input, *ssao_sig);
	cmd_list.bind_vertex_buffers(0, big_triangle_info);
	cmd_list.draw_non_indexed(3, 1, 0, 0);
	cmd_list.end_renderpass();
	cmd_list.set_pipeline_barrier(*gaussian_blurring_buffer, RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::uav, 0, irr::video::E_ASPECT::EA_COLOR);
	cmd_list.set_compute_pipeline_layout(*gaussian_input_sig);
	cmd_list.bind_compute_descriptor(0, *gaussian_input_h, *gaussian_input_sig);
	cmd_list.bind_compute_descriptor(1, *sampler_input, *gaussian_input_sig);
	cmd_list.set_compute_pipeline(*gaussian_h_pso);
	cmd_list.dispatch(width, height, 1);
	cmd_list.set_pipeline_barrier(*gaussian_blurring_buffer, RESOURCE_USAGE::uav, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);

	cmd_list.set_pipeline_barrier(*ssao_bilinear_result, RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::uav, 0, irr::video::E_ASPECT::EA_COLOR);
	cmd_list.bind_compute_descriptor(0, *gaussian_input_v, *gaussian_input_sig);
	cmd_list.bind_compute_descriptor(1, *sampler_input, *gaussian_input_sig);
	cmd_list.set_compute_pipeline(*gaussian_v_pso);
	cmd_list.dispatch(width, height, 1);
	cmd_list.set_pipeline_barrier(*ssao_bilinear_result, RESOURCE_USAGE::uav, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
}
