#include "mesh.h"
#include <Scene\IBL.h>

#define CHECK_HRESULT(cmd) {HRESULT hr = cmd; if (hr != 0) throw;}

#define SAMPLE_PATH "..\\..\\..\\examples\\assets\\"

struct SceneData
{
	float ViewMatrix[16];
	float InverseViewMatrix[16];
	float ProjectionMatrix[16];
	float InverseProjectionMatrix[16];
};

/*struct JointTransform
{
	float Model[16 * 48];
};*/

namespace
{

	// (inv)modelmatrix, jointmatrix
	constexpr auto object_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1),
		range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 1, 1) },
		shader_stage::all);

	constexpr auto model_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 2, 1) },
		shader_stage::fragment_shader);

	// anisotropic, bilinear
	constexpr auto sampler_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::SAMPLER, 3, 1),
		range_of_descriptors(RESOURCE_VIEW::SAMPLER, 13, 1) },
		shader_stage::fragment_shader);

	// color, normal, roughness+metalness, depth
	constexpr auto input_attachments_descriptor_set_type = descriptor_set({
			range_of_descriptors(RESOURCE_VIEW::INPUT_ATTACHMENT, 4, 1),
			range_of_descriptors(RESOURCE_VIEW::INPUT_ATTACHMENT, 5, 1),
			range_of_descriptors(RESOURCE_VIEW::INPUT_ATTACHMENT, 6, 1),
			range_of_descriptors(RESOURCE_VIEW::INPUT_ATTACHMENT, 14, 1) },
			shader_stage::fragment_shader);

	// color, normal,roughness+metalness, depth, ssao
	constexpr auto rtt_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 4, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 5, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 6, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 14, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 15, 1) },
		shader_stage::fragment_shader);

	// view/proj matrixes, sunlight data, skybox
	constexpr auto scene_descriptor_set_type = descriptor_set({
			range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 7, 1),
			range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 8, 1),
			range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 9, 1) },
			shader_stage::all);

	// IBL data
	constexpr auto ibl_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 10, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 11, 1),
		range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 12, 1) },
		shader_stage::fragment_shader);

	std::unique_ptr<render_pass_t> create_object_sunlight_pass(device_t* dev)
	{
		std::unique_ptr<render_pass_t> result;
#ifndef D3D12
		result.reset(new render_pass_t(dev->object,
		{
			// color
			structures::attachment_description(VK_FORMAT_R8G8B8A8_UNORM, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
			// normal
			structures::attachment_description(VK_FORMAT_R16G16_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
			// roughness and metalness
			structures::attachment_description(VK_FORMAT_R8G8B8A8_UNORM, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
			// final surface
			structures::attachment_description(VK_FORMAT_R8G8B8A8_UNORM, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
			// depth
			structures::attachment_description(VK_FORMAT_D24_UNORM_S8_UINT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		},
		{
			// Object pass
			subpass_description::generate_subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS)
				.set_color_attachments({ VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } })
				.set_depth_stencil_attachment(VkAttachmentReference{ 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }),
			// Sunlight pass IBL pass
			subpass_description::generate_subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS)
				.set_color_attachments({ VkAttachmentReference{ 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } })
				.set_input_attachments({ VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VkAttachmentReference{ 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL } }),
		},
		{
			get_subpass_dependency(0, 1, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT)
		}));
#endif // !D3D12
		return result;
	}

	std::unique_ptr<render_pass_t> create_ibl_sky_pass(device_t& dev)
	{
		std::unique_ptr<render_pass_t> result;
#ifndef D3D12
		result.reset(new render_pass_t(dev,
		{
			// final surface
			structures::attachment_description(VK_FORMAT_R8G8B8A8_UNORM, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
			// depth
			structures::attachment_description(VK_FORMAT_D24_UNORM_S8_UINT, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		},
		{
			// IBL pass
			subpass_description::generate_subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS)
				.set_color_attachments({ VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } })
				.set_input_attachments({ VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL } }),
			// Draw skybox
			subpass_description::generate_subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS)
				.set_color_attachments({ VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } })
				.set_depth_stencil_attachment(VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }),
		},
		{
			get_subpass_dependency(0, 1, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)
		}));
#endif // !D3D12
		return result;
	}
}


void MeshSample::Init()
{
	back_buffer = chain->get_image_view_from_swap_chain();

	command_allocator = dev->create_command_storage();
	auto command_list = command_allocator->create_command_list();
	command_list->start_command_list_recording(*command_allocator);

	cbv_srv_descriptors_heap = dev->create_descriptor_storage(100, { { RESOURCE_VIEW::CONSTANTS_BUFFER, 10 },{ RESOURCE_VIEW::SHADER_RESOURCE, 1000 },{ RESOURCE_VIEW::INPUT_ATTACHMENT, 4 },{ RESOURCE_VIEW::UAV_BUFFER, 1 } });
	sampler_heap = dev->create_descriptor_storage(10, { { RESOURCE_VIEW::SAMPLER, 10 } });
	object_sunlight_pass = create_object_sunlight_pass(dev.get());
	ibl_skyboss_pass = create_ibl_sky_pass(*dev);

	load_program_and_pipeline_layout();

	scene_matrix = dev->create_buffer(sizeof(SceneData), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);
	sun_data = dev->create_buffer(7 * sizeof(float), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);

	clear_value_structure_t clear_val = get_clear_value(irr::video::D24U8, 1., 0);
#ifndef D3D12
	command_list->set_pipeline_barrier(*back_buffer[0], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);
	command_list->set_pipeline_barrier(*back_buffer[1], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);
#endif // !D3D12
	depth_buffer = dev->create_image(irr::video::D24U8, width, height, 1, 1, usage_depth_stencil | usage_sampled | usage_input_attachment, &clear_val);
	clear_val = get_clear_value(irr::video::ECF_R8G8B8A8_UNORM, { 0., 0., 0., 0. });
	diffuse_color = dev->create_image(irr::video::ECF_R8G8B8A8_UNORM, width, height, 1, 1, usage_render_target | usage_sampled | usage_input_attachment, &clear_val);
	roughness_metalness = dev->create_image(irr::video::ECF_R8G8B8A8_UNORM, width, height, 1, 1, usage_render_target | usage_sampled | usage_input_attachment, &clear_val);
	clear_val = get_clear_value(irr::video::ECF_R16G16F, { 0., 0., 0., 0. });
	normal = dev->create_image(irr::video::ECF_R16G16F, width, height, 1, 1, usage_render_target | usage_sampled | usage_input_attachment, &clear_val);
	command_list->set_pipeline_barrier(*depth_buffer, RESOURCE_USAGE::undefined, RESOURCE_USAGE::DEPTH_WRITE, 0, irr::video::E_ASPECT::EA_DEPTH_STENCIL);
	command_list->set_pipeline_barrier(*diffuse_color, RESOURCE_USAGE::undefined, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
	command_list->set_pipeline_barrier(*normal, RESOURCE_USAGE::undefined, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
	command_list->set_pipeline_barrier(*roughness_metalness, RESOURCE_USAGE::undefined, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);

	fbo_pass1[0] = dev->create_frame_buffer({ { *diffuse_color, irr::video::ECF_R8G8B8A8_UNORM }, { *normal, irr::video::ECF_R16G16F },{ *roughness_metalness, irr::video::ECF_R8G8B8A8_UNORM }, { *back_buffer[0], swap_chain_format } }, { *depth_buffer, irr::video::ECOLOR_FORMAT::D24U8 }, width, height, object_sunlight_pass.get());
	fbo_pass1[1] = dev->create_frame_buffer({ { *diffuse_color, irr::video::ECF_R8G8B8A8_UNORM }, { *normal, irr::video::ECF_R16G16F },{ *roughness_metalness, irr::video::ECF_R8G8B8A8_UNORM }, { *back_buffer[1], swap_chain_format } }, { *depth_buffer, irr::video::ECOLOR_FORMAT::D24U8 }, width, height, object_sunlight_pass.get());

	fbo_pass2[0] = dev->create_frame_buffer({ { *back_buffer[0], swap_chain_format } }, { *depth_buffer, irr::video::ECOLOR_FORMAT::D24U8 }, width, height, ibl_skyboss_pass.get());
	fbo_pass2[1] = dev->create_frame_buffer({ { *back_buffer[1], swap_chain_format } }, { *depth_buffer, irr::video::ECOLOR_FORMAT::D24U8 }, width, height, ibl_skyboss_pass.get());

	ibl_descriptor = cbv_srv_descriptors_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(10, { ibl_set.get() }, 3);
	scene_descriptor = cbv_srv_descriptors_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(0, { scene_set.get() }, 3);
	input_attachments_descriptors = cbv_srv_descriptors_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(5, { input_attachments_set.get() }, 4);
	rtt_descriptors = cbv_srv_descriptors_heap->allocate_descriptor_set_from_cbv_srv_uav_heap(5, { rtt_set.get() }, 5);
	sampler_descriptors = sampler_heap->allocate_descriptor_set_from_sampler_heap(0, { sampler_set.get() }, 2);

	std::unique_ptr<buffer_t> upload_buffer;
	std::tie(skybox_texture, upload_buffer) = load_texture(*dev, SAMPLE_PATH + std::string("w_sky_1BC1.DDS"), *command_list);
	fill_descriptor_set();

	Assimp::Importer importer;
	auto model = importer.ReadFile(std::string(SAMPLE_PATH) + "xue.b3d", 0);

	scene = std::make_unique<irr::scene::Scene>();
	xue = scene->addMeshSceneNode(
		std::make_unique<irr::scene::IMeshSceneNode>(*dev, model, *command_list, *cbv_srv_descriptors_heap, object_set.get(), model_set.get(), nullptr),
		nullptr);

	big_triangle = dev.create_buffer(4 * 3 * sizeof(float), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, none);
	float fullscreen_tri[]
	{
		-1., -3., 0., 2.,
		3., 1., 2., 0.,
		-1., 1., 0., 0.
	};

	memcpy(big_triangle->map_buffer(), fullscreen_tri, 4 * 3 * sizeof(float));
	big_triangle->unmap_buffer();
	big_triangle_info = { { *big_triangle, 0, 4 * sizeof(float), 4 * 3 * sizeof(float) } };

	command_list->make_command_list_executable();
	cmdqueue->submit_executable_command_list(*command_list);
	cmdqueue->wait_for_command_queue_idle();
	//ibl
	ibl_utility ibl_util(*dev);
	command_list->start_command_list_recording(*command_allocator);
	sh_coefficients = ibl_util.computeSphericalHarmonics(*dev, *command_list, *skybox_view, 1024);
	specular_cube = ibl_util.generateSpecularCubemap(*dev, *command_list, *skybox_view);
	std::tie(dfg_lut, dfg_lut_view) = ibl_util.getDFGLUT(*dev, *command_list, 128);

	specular_cube_view = dev->create_image_view(*specular_cube, irr::video::ECF_R16G16B16A16F, 0, 8, 0, 6, irr::video::E_TEXTURE_TYPE::ETT_CUBE);

	dev->set_image_view(ibl_descriptor, 1, 11, *specular_cube_view);
	dev->set_image_view(ibl_descriptor, 2, 12, *dfg_lut_view);
	dev->set_constant_buffer_view(ibl_descriptor, 0, 10, *sh_coefficients, 27 * sizeof(float));
	ssao_util = std::make_unique<ssao_utility>(*dev, depth_buffer.get(), width, height);

	ssao_view = dev->create_image_view(*ssao_util->ssao_bilinear_result, irr::video::ECOLOR_FORMAT::ECF_R16F, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);
	dev->set_image_view(rtt_descriptors, 4, 15, *ssao_view);

	command_list->set_pipeline_barrier(*ssao_util->linear_depth_buffer, RESOURCE_USAGE::undefined, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
	command_list->set_pipeline_barrier(*ssao_util->ssao_result, RESOURCE_USAGE::undefined, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
	command_list->set_pipeline_barrier(*ssao_util->gaussian_blurring_buffer, RESOURCE_USAGE::undefined, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
	command_list->set_pipeline_barrier(*ssao_util->ssao_bilinear_result, RESOURCE_USAGE::undefined, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
	command_list->make_command_list_executable();
	cmdqueue->submit_executable_command_list(*command_list);
	cmdqueue->wait_for_command_queue_idle();

	fill_draw_commands();
}

void MeshSample::fill_descriptor_set()
{
	diffuse_color_view = dev->create_image_view(*diffuse_color, irr::video::ECF_R8G8B8A8_UNORM, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);
	normal_view = dev->create_image_view(*normal, irr::video::ECF_R16G16F, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);
	roughness_metalness_view = dev->create_image_view(*roughness_metalness, irr::video::ECF_R8G8B8A8_UNORM, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D);
	skybox_view = dev->create_image_view(*skybox_texture, irr::video::ECF_BC1_UNORM_SRGB, 0, 11, 0, 6, irr::video::E_TEXTURE_TYPE::ETT_CUBE);
	depth_view = dev->create_image_view(*depth_buffer, irr::video::D24U8, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D, irr::video::E_ASPECT::EA_DEPTH);

	// scene
	dev->set_image_view(scene_descriptor, 2, 9, *skybox_view);
	dev->set_constant_buffer_view(scene_descriptor, 0, 7, *scene_matrix, sizeof(SceneData));
	dev->set_constant_buffer_view(scene_descriptor, 1, 8, *sun_data, sizeof(7 * sizeof(float)));

	// rtt
	dev->set_input_attachment(input_attachments_descriptors, 0, 4, *diffuse_color_view);
	dev->set_input_attachment(input_attachments_descriptors, 1, 5, *normal_view);
	dev->set_input_attachment(input_attachments_descriptors, 2, 14, *roughness_metalness_view);
	dev->set_input_attachment(input_attachments_descriptors, 3, 6, *depth_view);

	dev->set_image_view(rtt_descriptors, 0, 4, *diffuse_color_view);
	dev->set_image_view(rtt_descriptors, 1, 5, *normal_view);
	dev->set_image_view(rtt_descriptors, 2, 14, *roughness_metalness_view);
	dev->set_image_view(rtt_descriptors, 3, 6, *depth_view);

	bilinear_clamped_sampler = dev->create_sampler(SAMPLER_TYPE::BILINEAR_CLAMPED);
	anisotropic_sampler = dev->create_sampler(SAMPLER_TYPE::ANISOTROPIC);

	dev->set_sampler(sampler_descriptors, 0, 3, *anisotropic_sampler);
	dev->set_sampler(sampler_descriptors, 1, 13, *bilinear_clamped_sampler);
}

void MeshSample::load_program_and_pipeline_layout()
{
#ifdef D3D12
	object_sig = get_pipeline_layout_from_desc(*dev, { model_descriptor_set_type, object_descriptor_set_type, scene_descriptor_set_type, sampler_descriptor_set_type });
	sunlight_sig = get_pipeline_layout_from_desc(*dev, { input_attachments_descriptor_set_type, scene_descriptor_set_type });
	skybox_sig = get_pipeline_layout_from_desc(*dev, { scene_descriptor_set_type, sampler_descriptor_set_type });
	ibl_sig = get_pipeline_layout_from_desc(*dev, { rtt_descriptor_set_type, scene_descriptor_set_type, ibl_descriptor_set_type, sampler_descriptor_set_type });
#else
	sampler_set = get_object_descriptor_set(*dev, sampler_descriptor_set_type);
	object_set = get_object_descriptor_set(*dev, object_descriptor_set_type);
	scene_set = get_object_descriptor_set(*dev, scene_descriptor_set_type);
	input_attachments_set = get_object_descriptor_set(*dev, input_attachments_descriptor_set_type);
	rtt_set = get_object_descriptor_set(*dev, rtt_descriptor_set_type);
	model_set = get_object_descriptor_set(*dev, model_descriptor_set_type);
	ibl_set = get_object_descriptor_set(*dev, ibl_descriptor_set_type);

	object_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev->object, 0, std::vector<VkDescriptorSetLayout>{ model_set->object, object_set->object, scene_set->object, sampler_set->object }, std::vector<VkPushConstantRange>());
	sunlight_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev->object, 0, std::vector<VkDescriptorSetLayout>{ input_attachments_set->object, scene_set->object }, std::vector<VkPushConstantRange>());
	skybox_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev->object, 0, std::vector<VkDescriptorSetLayout>{ scene_set->object, sampler_set->object }, std::vector<VkPushConstantRange>());
	ibl_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev->object, 0, std::vector<VkDescriptorSetLayout>{ rtt_set->object, scene_set->object, ibl_set->object, sampler_set->object }, std::vector<VkPushConstantRange>());
#endif // D3D12
	objectpso = get_skinned_object_pipeline_state(dev.get(), object_sig, object_sunlight_pass.get());
	sunlightpso = get_sunlight_pipeline_state(dev.get(), sunlight_sig, object_sunlight_pass.get());
	skybox_pso = get_skybox_pipeline_state(dev.get(), skybox_sig, ibl_skyboss_pass.get());
	ibl_pso = get_ibl_pipeline_state(dev.get(), ibl_sig, ibl_skyboss_pass.get());
}

void MeshSample::fill_draw_commands()
{
	for (unsigned i = 0; i < 2; i++)
	{
		command_list_for_back_buffer.push_back(command_allocator->create_command_list());
		command_list_t* current_cmd_list = command_list_for_back_buffer.back().get();
		current_cmd_list->start_command_list_recording(*command_allocator);
		current_cmd_list->set_pipeline_barrier(*back_buffer[i], RESOURCE_USAGE::PRESENT, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);

		std::array<float, 4> clearColor = { .25f, .25f, 0.35f, 1.0f };
#ifndef D3D12
		{
			VkRenderPassBeginInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
			info.renderPass = object_sunlight_pass->object;
			info.framebuffer = fbo_pass1[i]->fbo.object;
			info.clearValueCount = 5;
			VkClearValue clear_values[5] = {
				structures::clear_value(clearColor),
				structures::clear_value(clearColor),
				structures::clear_value(clearColor),
				structures::clear_value(clearColor),
				structures::clear_value(1.f, 0)
			};
			info.pClearValues = clear_values;
			info.renderArea.extent.width = width;
			info.renderArea.extent.height = height;
			vkCmdBeginRenderPass(current_cmd_list->object, &info, VK_SUBPASS_CONTENTS_INLINE);
		}
#else // !D3D12
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtt_to_use = {
			CD3DX12_CPU_DESCRIPTOR_HANDLE(fbo_pass1[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart())
				.Offset(0, dev->object->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
			CD3DX12_CPU_DESCRIPTOR_HANDLE(fbo_pass1[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart())
				.Offset(1, dev->object->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
			CD3DX12_CPU_DESCRIPTOR_HANDLE(fbo_pass1[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart())
				.Offset(2, dev->object->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
		};
		current_cmd_list->object->OMSetRenderTargets(rtt_to_use.size(), rtt_to_use.data(), false, &(fbo_pass1[i]->dsv_heap->GetCPUDescriptorHandleForHeapStart()));
		clear_color(*current_cmd_list, fbo_pass1[i], clearColor);
		clear_depth_stencil(*current_cmd_list, fbo_pass1[i], 1., 0);

		current_cmd_list->object->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#endif
		current_cmd_list->set_graphic_pipeline_layout(*object_sig);
		current_cmd_list->set_descriptor_storage_referenced(*cbv_srv_descriptors_heap, sampler_heap.get());
		current_cmd_list->set_graphic_pipeline(*objectpso);
		current_cmd_list->bind_graphic_descriptor(2, scene_descriptor, *object_sig);
		current_cmd_list->bind_graphic_descriptor(3, sampler_descriptors, *object_sig);
		current_cmd_list->set_viewport(0., width, 0., height, 0., 1.);
		current_cmd_list->set_scissor(0, width, 0, height);

		scene->fill_gbuffer_filling_command(*current_cmd_list, *object_sig);
#ifdef D3D12
		set_pipeline_barrier(*current_cmd_list, *diffuse_color, RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
		set_pipeline_barrier(*current_cmd_list, *normal, RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
		set_pipeline_barrier(*current_cmd_list, *roughness_metalness, RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
		set_pipeline_barrier(*current_cmd_list, *depth_buffer, RESOURCE_USAGE::DEPTH_WRITE, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_DEPTH);
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> present_rtt = {
			CD3DX12_CPU_DESCRIPTOR_HANDLE(fbo_pass1[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart())
			.Offset(3, dev->object->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
		};
		current_cmd_list->object->OMSetRenderTargets(present_rtt.size(), present_rtt.data(), false, nullptr);
#else
		vkCmdNextSubpass(current_cmd_list->object, VK_SUBPASS_CONTENTS_INLINE);
#endif
		current_cmd_list->set_graphic_pipeline_layout(*sunlight_sig);
		current_cmd_list->set_descriptor_storage_referenced(*cbv_srv_descriptors_heap, sampler_heap.get());
		current_cmd_list->bind_graphic_descriptor(0, input_attachments_descriptors, *sunlight_sig);
		current_cmd_list->bind_graphic_descriptor(1, scene_descriptor, *sunlight_sig);
		current_cmd_list->set_graphic_pipeline(*sunlightpso);
		current_cmd_list->bind_vertex_buffers(0, big_triangle_info);
		current_cmd_list->draw_non_indexed(3, 1, 0, 0);
#ifndef D3D12
		vkCmdEndRenderPass(current_cmd_list->object);
#endif // !D3D12
		ssao_util->fill_command_list(*dev, *current_cmd_list, *depth_buffer, 1.f, 100.f, big_triangle_info);
#ifdef D3D12
		current_cmd_list->object->OMSetRenderTargets(present_rtt.size(), present_rtt.data(), false, nullptr);
#else
		{
			VkRenderPassBeginInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
			info.renderPass = ibl_skyboss_pass->object;
			info.framebuffer = fbo_pass2[i]->fbo.object;
			info.renderArea.extent.width = width;
			info.renderArea.extent.height = height;
			vkCmdBeginRenderPass(current_cmd_list->object, &info, VK_SUBPASS_CONTENTS_INLINE);
		}
#endif // !D3D12
		set_graphic_pipeline_layout(*current_cmd_list, ibl_sig);
		set_descriptor_storage_referenced(*current_cmd_list, *cbv_srv_descriptors_heap, sampler_heap.get());
		bind_graphic_descriptor(*current_cmd_list, 0, rtt_descriptors, ibl_sig);
		bind_graphic_descriptor(*current_cmd_list, 1, scene_descriptor, ibl_sig);
		bind_graphic_descriptor(*current_cmd_list, 2, ibl_descriptor, ibl_sig);
		bind_graphic_descriptor(*current_cmd_list, 3, sampler_descriptors, ibl_sig);
		set_graphic_pipeline(*current_cmd_list, ibl_pso);
		bind_vertex_buffers(*current_cmd_list, 0, big_triangle_info);
		draw_non_indexed(*current_cmd_list, 3, 1, 0, 0);
#ifndef D3D12
		vkCmdNextSubpass(current_cmd_list->object, VK_SUBPASS_CONTENTS_INLINE);
#else
		current_cmd_list->object->OMSetRenderTargets(present_rtt.size(), present_rtt.data(), false, &(fbo_pass1[i]->dsv_heap->GetCPUDescriptorHandleForHeapStart()));
		set_pipeline_barrier(*current_cmd_list, *depth_buffer, RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::DEPTH_WRITE, 0, irr::video::E_ASPECT::EA_DEPTH);
#endif
		set_graphic_pipeline_layout(*current_cmd_list, skybox_sig);
		bind_graphic_descriptor(*current_cmd_list, 0, scene_descriptor, skybox_sig);
		bind_graphic_descriptor(*current_cmd_list, 1, sampler_descriptors, skybox_sig);
		set_graphic_pipeline(*current_cmd_list, skybox_pso);
		bind_vertex_buffers(*current_cmd_list, 0, big_triangle_info);
		draw_non_indexed(*current_cmd_list, 3, 1, 0, 0);
#ifndef D3D12
		vkCmdEndRenderPass(current_cmd_list->object);
#else
		set_pipeline_barrier(*current_cmd_list, *diffuse_color, RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
		set_pipeline_barrier(*current_cmd_list, *normal, RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
		set_pipeline_barrier(*current_cmd_list, *roughness_metalness, RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
#endif // !D3D12
		set_pipeline_barrier(*current_cmd_list, *back_buffer[i], RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);
		make_command_list_executable(*current_cmd_list);
	}
}

void MeshSample::Draw()
{
	scene->update(*dev);

	SceneData * tmp = static_cast<SceneData*>(map_buffer(*dev, *scene_matrix));
	irr::core::matrix4 Perspective;
	irr::core::matrix4 InvPerspective;
	irr::core::matrix4 View;
	irr::core::matrix4 InvView;
	float horizon_angle_in_radian = horizon_angle * 3.14f / 100.f;
	View.buildCameraLookAtMatrixLH(irr::core::vector3df(0., 2. * sin(horizon_angle_in_radian), -2. * cos(horizon_angle_in_radian)), irr::core::vector3df(), irr::core::vector3df(0., cos(horizon_angle_in_radian), sin(horizon_angle_in_radian)));
	View.getInverse(InvView);
	Perspective.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
	Perspective.getInverse(InvPerspective);
	memcpy(tmp->ProjectionMatrix, Perspective.pointer(), 16 * sizeof(float));
	memcpy(tmp->InverseProjectionMatrix, InvPerspective.pointer(), 16 * sizeof(float));
	memcpy(tmp->ViewMatrix, View.pointer(), 16 * sizeof(float));
	memcpy(tmp->InverseViewMatrix, InvView.pointer(), 16 * sizeof(float));
	unmap_buffer(*dev, *scene_matrix);

	float * sun_tmp = (float*)map_buffer(*dev, *sun_data);
	sun_tmp[0] = 0.;
	sun_tmp[1] = 1.;
	sun_tmp[2] = 0.;
	sun_tmp[3] = .5f;
	sun_tmp[4] = 10.;
	sun_tmp[5] = 10.;
	sun_tmp[6] = 10.;
	unmap_buffer(*dev, *sun_data);

//	double intpart;
//	float frame = (float)modf(timer / 10000., &intpart);
//	frame *= 300.f;
	/*        loader->AnimatedMesh.animateMesh(frame, 1.f);
			loader->AnimatedMesh.skinMesh(1.f);

			memcpy(map_buffer(dev, jointbuffer), loader->AnimatedMesh.JointMatrixes.data(), loader->AnimatedMesh.JointMatrixes.size() * 16 * sizeof(float));*/
			//unmap_buffer(dev, jointbuffer);

	uint32_t current_backbuffer = get_next_backbuffer_id(*dev, *chain);
	submit_executable_command_list(*cmdqueue, *command_list_for_back_buffer[current_backbuffer]);
	wait_for_command_queue_idle(*dev, *cmdqueue);
	present(*dev, *cmdqueue, *chain, current_backbuffer);
}
