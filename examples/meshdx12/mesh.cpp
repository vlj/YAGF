#include "mesh.h"

#define CHECK_HRESULT(cmd) {HRESULT hr = cmd; if (hr != 0) throw;}

#ifdef D3D12
#define SAMPLE_PATH ""
#else
#define SAMPLE_PATH "..\\..\\..\\examples\\assets\\"
#endif

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

	constexpr auto sampler_descriptor_set_type = descriptor_set({
		range_of_descriptors(RESOURCE_VIEW::SAMPLER, 3, 1) },
	shader_stage::fragment_shader);

	// color, normal, depth
	constexpr auto rtt_descriptor_set_type = descriptor_set({
			range_of_descriptors(RESOURCE_VIEW::INPUT_ATTACHMENT, 4, 1),
			range_of_descriptors(RESOURCE_VIEW::INPUT_ATTACHMENT, 5, 1),
			range_of_descriptors(RESOURCE_VIEW::INPUT_ATTACHMENT, 6, 1) },
			shader_stage::fragment_shader);

	// view/proj matrixes, sunlight data, skybox
	constexpr auto scene_descriptor_set_type = descriptor_set({
			range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 7, 1),
			range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 8, 1),
			range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 9, 1) },
			shader_stage::all);
}


void MeshSample::Init()
{
	back_buffer = get_image_view_from_swap_chain(dev, chain.get());

	command_allocator = create_command_storage(dev);
	std::unique_ptr<command_list_t> command_list = create_command_list(dev, command_allocator.get());
	start_command_list_recording(dev, command_list.get(), command_allocator.get());

#ifndef D3D12
	sampler_set = get_object_descriptor_set(dev, sampler_descriptor_set_type);
	object_set = get_object_descriptor_set(dev, object_descriptor_set_type);
	scene_set = get_object_descriptor_set(dev, scene_descriptor_set_type);
	rtt_set = get_object_descriptor_set(dev, rtt_descriptor_set_type);
	model_set = get_object_descriptor_set(dev, model_descriptor_set_type);

	object_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev->object, 0, std::vector<VkDescriptorSetLayout>{ model_set->object, object_set->object, scene_set->object, sampler_set->object }, std::vector<VkPushConstantRange>());
	sunlight_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev->object, 0, std::vector<VkDescriptorSetLayout>{ rtt_set->object, scene_set->object }, std::vector<VkPushConstantRange>());
	skybox_sig = std::make_shared<vulkan_wrapper::pipeline_layout>(dev->object, 0, std::vector<VkDescriptorSetLayout>{ scene_set->object, sampler_set->object }, std::vector<VkPushConstantRange>());
#else
	object_sig = get_pipeline_layout_from_desc(dev, { model_descriptor_set_type, object_descriptor_set_type, scene_descriptor_set_type, sampler_descriptor_set_type });
	sunlight_sig = get_pipeline_layout_from_desc(dev, { rtt_descriptor_set_type, scene_descriptor_set_type });
	skybox_sig = get_pipeline_layout_from_desc(dev, { scene_descriptor_set_type, sampler_descriptor_set_type });
#endif // !D3D12

	scene_matrix = create_buffer(dev, sizeof(SceneData));
	sun_data = create_buffer(dev, 7 * sizeof(float));

	cbv_srv_descriptors_heap = create_descriptor_storage(dev, 100, { { RESOURCE_VIEW::CONSTANTS_BUFFER, 10 }, {RESOURCE_VIEW::SHADER_RESOURCE, 1000}, {RESOURCE_VIEW::INPUT_ATTACHMENT, 3 } });
	sampler_heap = create_descriptor_storage(dev, 10, { {RESOURCE_VIEW::SAMPLER, 10 } });

	clear_value_structure_t clear_val = {};
#ifndef D3D12
	set_pipeline_barrier(dev, command_list.get(), back_buffer[0].get(), RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);
	set_pipeline_barrier(dev, command_list.get(), back_buffer[1].get(), RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);

	render_pass.reset(new vulkan_wrapper::render_pass(dev->object,
	{
		structures::attachment_description(VK_FORMAT_R8G8B8A8_UNORM, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
		structures::attachment_description(VK_FORMAT_R8G8B8A8_UNORM, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
		structures::attachment_description(VK_FORMAT_R8G8B8A8_UNORM, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
		structures::attachment_description(VK_FORMAT_D24_UNORM_S8_UINT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	},
	{
		subpass_description::generate_subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS)
			.set_color_attachments({ VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } })
			.set_depth_stencil_attachment(VkAttachmentReference{ 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }),
		subpass_description::generate_subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS)
			.set_color_attachments({ VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } })
			.set_input_attachments({ VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VkAttachmentReference{ 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL } }),
		subpass_description::generate_subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS)
			.set_color_attachments({ VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } })
			.set_depth_stencil_attachment(VkAttachmentReference{ 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }),
	},
	{
		get_subpass_dependency(0, 1, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT),
		get_subpass_dependency(0, 2, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT),
		get_subpass_dependency(1, 2, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
	}));
#else

	clear_val = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1., 0);
#endif // !D3D12
	depth_buffer = create_image(dev, irr::video::D24U8, width, height, 1, 1, usage_depth_stencil | usage_sampled | usage_input_attachment, &clear_val);

#ifdef D3D12
	float clear_color[4] = {};
	clear_val = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clear_color);
#endif
	diffuse_color = create_image(dev, irr::video::ECF_R8G8B8A8_UNORM, width, height, 1, 1, usage_render_target | usage_sampled | usage_input_attachment, &clear_val);
	normal_roughness_metalness = create_image(dev, irr::video::ECF_R8G8B8A8_UNORM, width, height, 1, 1, usage_render_target | usage_sampled | usage_input_attachment, &clear_val);
	set_pipeline_barrier(dev, command_list.get(), depth_buffer.get(), RESOURCE_USAGE::undefined, RESOURCE_USAGE::DEPTH_WRITE, 0, irr::video::E_ASPECT::EA_DEPTH_STENCIL);
	set_pipeline_barrier(dev, command_list.get(), diffuse_color.get(), RESOURCE_USAGE::undefined, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
	set_pipeline_barrier(dev, command_list.get(), normal_roughness_metalness.get(), RESOURCE_USAGE::undefined, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);

	fbo[0] = create_frame_buffer(dev, { { diffuse_color.get(), irr::video::ECF_R8G8B8A8_UNORM },{ normal_roughness_metalness.get(), irr::video::ECF_R8G8B8A8_UNORM },{ back_buffer[0].get(), swap_chain_format } }, { depth_buffer.get(), irr::video::ECOLOR_FORMAT::D24U8 }, width, height, render_pass.get());
	fbo[1] = create_frame_buffer(dev, { { diffuse_color.get(), irr::video::ECF_R8G8B8A8_UNORM },{ normal_roughness_metalness.get(), irr::video::ECF_R8G8B8A8_UNORM },{ back_buffer[1].get(), swap_chain_format } }, { depth_buffer.get(), irr::video::ECOLOR_FORMAT::D24U8 }, width, height, render_pass.get());
#ifndef D3D12
	sampler = std::make_shared<vulkan_wrapper::sampler>(dev->object, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.f, true, 16.f);
	diffuse_color_view = std::make_shared<vulkan_wrapper::image_view>(dev->object, diffuse_color->object, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
		structures::component_mapping(), structures::image_subresource_range());
	normal_roughness_metalness_view = std::make_shared<vulkan_wrapper::image_view>(dev->object, normal_roughness_metalness->object, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
		structures::component_mapping(), structures::image_subresource_range());
	depth_view = std::make_shared<vulkan_wrapper::image_view>(dev->object, depth_buffer->object, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_D32_SFLOAT_S8_UINT,
		structures::component_mapping(), structures::image_subresource_range(VK_IMAGE_ASPECT_DEPTH_BIT));

	sampler_descriptors = util::allocate_descriptor_sets(dev->object, sampler_heap->object, { sampler_set->object });
	scene_descriptor = util::allocate_descriptor_sets(dev->object, cbv_srv_descriptors_heap->object, { scene_set->object });
	rtt = util::allocate_descriptor_sets(dev->object, cbv_srv_descriptors_heap->object, { rtt_set->object });

	util::update_descriptor_sets(dev->object,
	{
		structures::write_descriptor_set(sampler_descriptors, VK_DESCRIPTOR_TYPE_SAMPLER,
			{ VkDescriptorImageInfo{ sampler->object, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 3),
		structures::write_descriptor_set(rtt, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			{ VkDescriptorImageInfo{ VK_NULL_HANDLE, diffuse_color_view->object, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 4),
		structures::write_descriptor_set(rtt, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			{ VkDescriptorImageInfo{ VK_NULL_HANDLE, normal_roughness_metalness_view->object, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 5),
		structures::write_descriptor_set(rtt, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			{ VkDescriptorImageInfo{ VK_NULL_HANDLE, depth_view->object, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 6),
		structures::write_descriptor_set(scene_descriptor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			{ VkDescriptorBufferInfo{ scene_matrix->object, 0, sizeof(SceneData) } }, 7),
		structures::write_descriptor_set(scene_descriptor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			{ VkDescriptorBufferInfo{ sun_data->object, 0, 7 * sizeof(float) } }, 8)
	});
#endif // !D3D12

	Assimp::Importer importer;
	auto model = importer.ReadFile(std::string(SAMPLE_PATH) + "xue.b3d", 0);
	xue = std::make_unique<irr::scene::IMeshSceneNode>(dev, model, command_list.get(), cbv_srv_descriptors_heap.get(),
#ifndef D3D12
		object_set.get(), model_set.get(),
#endif // !D3D12
		nullptr);

	std::unique_ptr<buffer_t> upload_buffer;
	std::tie(skybox_texture, upload_buffer) = load_texture(dev, SAMPLE_PATH + std::string("w_sky_1BC1.DDS"), command_list.get());

#ifndef D3D12
	skybox_view = std::make_shared<vulkan_wrapper::image_view>(dev->object, skybox_texture->object, VK_IMAGE_VIEW_TYPE_CUBE, VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
		structures::component_mapping(), structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT, 0, 11, 0, 6));

	util::update_descriptor_sets(dev->object,
	{
		structures::write_descriptor_set(scene_descriptor, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		{ VkDescriptorImageInfo{ VK_NULL_HANDLE, skybox_view->object, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 9),
	});
#else
	// scene
	create_constant_buffer_view(dev, cbv_srv_descriptors_heap.get(), 0, scene_matrix.get(), sizeof(SceneData));
	create_constant_buffer_view(dev, cbv_srv_descriptors_heap.get(), 1, sun_data.get(), sizeof(7 * sizeof(float)));
	create_image_view(dev, cbv_srv_descriptors_heap.get(), 2, skybox_texture.get(), 1, irr::video::ECF_BC1_UNORM_SRGB, D3D12_SRV_DIMENSION_TEXTURECUBE);

	// rtt
	create_image_view(dev, cbv_srv_descriptors_heap.get(), 5, diffuse_color.get(), 1, irr::video::ECF_R8G8B8A8_UNORM, D3D12_SRV_DIMENSION_TEXTURE2D);
	create_image_view(dev, cbv_srv_descriptors_heap.get(), 6, normal_roughness_metalness.get(), 1, irr::video::ECF_R8G8B8A8_UNORM, D3D12_SRV_DIMENSION_TEXTURE2D);
	create_image_view(dev, cbv_srv_descriptors_heap.get(), 7, depth_buffer.get(), 1, irr::video::ECOLOR_FORMAT::D24U8, D3D12_SRV_DIMENSION_TEXTURE2D);


	create_sampler(dev, sampler_heap.get(), 0, SAMPLER_TYPE::TRILINEAR);
#endif // !D3D12


	objectpso = get_skinned_object_pipeline_state(dev, object_sig, render_pass.get());
	sunlightpso = get_sunlight_pipeline_state(dev, sunlight_sig, render_pass.get());
	skybox_pso = get_skybox_pipeline_state(dev, skybox_sig, render_pass.get());

	big_triangle = create_buffer(dev, 4 * 3 * sizeof(float));
	float fullscreen_tri[]
	{
		-1., -3., 0., 2.,
		3., 1., 2., 0.,
		-1., 1., 0., 0.
	};

	memcpy(map_buffer(dev, big_triangle.get()), fullscreen_tri, 4 * 3 * sizeof(float));
	unmap_buffer(dev, big_triangle.get());
	big_triangle_info = { std::make_tuple(big_triangle.get(), 0, 4 * sizeof(float), 4 * 3 * sizeof(float)) };

	make_command_list_executable(command_list.get());
	submit_executable_command_list(cmdqueue.get(), command_list.get());
	wait_for_command_queue_idle(dev, cmdqueue.get());
	fill_draw_commands();
}


void MeshSample::fill_draw_commands()
{
	for (unsigned i = 0; i < 2; i++)
	{
		command_list_for_back_buffer.push_back(create_command_list(dev, command_allocator.get()));
		command_list_t* current_cmd_list = command_list_for_back_buffer.back().get();


		start_command_list_recording(dev, current_cmd_list, command_allocator.get());

		set_pipeline_barrier(dev, current_cmd_list, back_buffer[i].get(), RESOURCE_USAGE::PRESENT, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);

		std::array<float, 4> clearColor = { .25f, .25f, 0.35f, 1.0f };
#ifndef D3D12
		VkRenderPassBeginInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		info.renderPass = render_pass->object;
		info.framebuffer = fbo[i]->fbo.object;
		info.clearValueCount = 4;
		VkClearValue clear_values[4] = {
			structures::clear_value(clearColor),
			structures::clear_value(clearColor),
			structures::clear_value(clearColor),
			structures::clear_value(1.f, 0)
		};
		info.pClearValues = clear_values;
		info.renderArea.extent.width = width;
		info.renderArea.extent.height = height;
		vkCmdBeginRenderPass(current_cmd_list->object, &info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, object_sig->object, 2, 1, &scene_descriptor, 0, nullptr);
		vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, object_sig->object, 3, 1, &sampler_descriptors, 0, nullptr);
#else // !D3D12
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtt_to_use = {
			CD3DX12_CPU_DESCRIPTOR_HANDLE(fbo[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart())
				.Offset(0, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
			CD3DX12_CPU_DESCRIPTOR_HANDLE(fbo[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart())
				.Offset(1, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
		};
		current_cmd_list->object->OMSetRenderTargets(rtt_to_use.size(), rtt_to_use.data(), false, &(fbo[i]->dsv_heap->GetCPUDescriptorHandleForHeapStart()));
		clear_color(dev, current_cmd_list, fbo[i], clearColor);
		clear_depth_stencil(dev, current_cmd_list, fbo[i], depth_stencil_aspect::depth_only, 1., 0);

		current_cmd_list->object->SetGraphicsRootSignature(object_sig.Get());

		std::array<ID3D12DescriptorHeap*, 2> descriptors = { cbv_srv_descriptors_heap->object, sampler_heap->object };
		current_cmd_list->object->SetDescriptorHeaps(2, descriptors.data());

		current_cmd_list->object->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		current_cmd_list->object->SetGraphicsRootDescriptorTable(2, cbv_srv_descriptors_heap->object->GetGPUDescriptorHandleForHeapStart());
		current_cmd_list->object->SetGraphicsRootDescriptorTable(3,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(sampler_heap->object->GetGPUDescriptorHandleForHeapStart()));
#endif
		set_graphic_pipeline(current_cmd_list, objectpso);

		set_viewport(current_cmd_list, 0., 1024.f, 0., 1024.f, 0., 1.);
		set_scissor(current_cmd_list, 0, 1024, 0, 1024);

		xue->fill_draw_command(dev, current_cmd_list, object_sig, cbv_srv_descriptors_heap.get());

#ifndef D3D12
		vkCmdNextSubpass(current_cmd_list->object, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, sunlight_sig->object, 0, 1, &rtt, 0, nullptr);
		vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, sunlight_sig->object, 1, 1, &scene_descriptor, 0, nullptr);
#else

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> present_rtt = {
			CD3DX12_CPU_DESCRIPTOR_HANDLE(fbo[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart())
			.Offset(2, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
		};

		current_cmd_list->object->OMSetRenderTargets(present_rtt.size(), present_rtt.data(), false, nullptr);
		current_cmd_list->object->SetGraphicsRootSignature(sunlight_sig.Get());
		current_cmd_list->object->SetGraphicsRootDescriptorTable(0,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(cbv_srv_descriptors_heap->object->GetGPUDescriptorHandleForHeapStart())
			.Offset(5, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
		current_cmd_list->object->SetGraphicsRootDescriptorTable(1,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(cbv_srv_descriptors_heap->object->GetGPUDescriptorHandleForHeapStart()));

		set_pipeline_barrier(dev, current_cmd_list, diffuse_color.get(), RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
		set_pipeline_barrier(dev, current_cmd_list, normal_roughness_metalness.get(), RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
		set_pipeline_barrier(dev, current_cmd_list, depth_buffer.get(), RESOURCE_USAGE::DEPTH_WRITE, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_DEPTH);
#endif // !D3D12
		set_graphic_pipeline(current_cmd_list, sunlightpso);
		bind_vertex_buffers(current_cmd_list, 0, big_triangle_info);
		set_viewport(current_cmd_list, 0., 1024.f, 0., 1024.f, 0., 1.);
		set_scissor(current_cmd_list, 0, 1024, 0, 1024);
		draw_non_indexed(current_cmd_list, 3, 1, 0, 0);
#ifndef D3D12
		vkCmdNextSubpass(current_cmd_list->object, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_sig->object, 0, 1, &scene_descriptor, 0, nullptr);
		vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_sig->object, 1, 1, &sampler_descriptors, 0, nullptr);
#else
		current_cmd_list->object->OMSetRenderTargets(present_rtt.size(), present_rtt.data(), false, &(fbo[i]->dsv_heap->GetCPUDescriptorHandleForHeapStart()));
		set_pipeline_barrier(dev, current_cmd_list, depth_buffer.get(), RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::DEPTH_WRITE, 0, irr::video::E_ASPECT::EA_DEPTH);
		current_cmd_list->object->SetGraphicsRootSignature(skybox_sig.Get());
		current_cmd_list->object->SetGraphicsRootDescriptorTable(0,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(cbv_srv_descriptors_heap->object->GetGPUDescriptorHandleForHeapStart()));
		current_cmd_list->object->SetGraphicsRootDescriptorTable(1,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(sampler_heap->object->GetGPUDescriptorHandleForHeapStart()));
#endif
		set_graphic_pipeline(current_cmd_list, skybox_pso);
		bind_vertex_buffers(current_cmd_list, 0, big_triangle_info);
		set_viewport(current_cmd_list, 0., 1024.f, 0., 1024.f, 0., 1.);
		set_scissor(current_cmd_list, 0, 1024, 0, 1024);
		draw_non_indexed(current_cmd_list, 3, 1, 0, 0);
#ifndef D3D12
		vkCmdEndRenderPass(current_cmd_list->object);
#endif // !D3D12
		set_pipeline_barrier(dev, current_cmd_list, back_buffer[i].get(), RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);
		make_command_list_executable(current_cmd_list);
	}
}

void MeshSample::Draw()
{
	xue->update_constant_buffers(dev);

	SceneData * tmp = static_cast<SceneData*>(map_buffer(dev, scene_matrix.get()));
	irr::core::matrix4 Perspective;
	irr::core::matrix4 InvPerspective;
	irr::core::matrix4 identity;
	Perspective.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
	Perspective.getInverse(InvPerspective);
	memcpy(tmp->ProjectionMatrix, Perspective.pointer(), 16 * sizeof(float));
	memcpy(tmp->InverseProjectionMatrix, InvPerspective.pointer(), 16 * sizeof(float));
	memcpy(tmp->ViewMatrix, identity.pointer(), 16 * sizeof(float));
	memcpy(tmp->InverseViewMatrix, identity.pointer(), 16 * sizeof(float));
	unmap_buffer(dev, scene_matrix.get());

	float * sun_tmp = (float*)map_buffer(dev, sun_data.get());
	sun_tmp[0] = 0.;
	sun_tmp[1] = 1.;
	sun_tmp[2] = 0.;
	sun_tmp[3] = .5f;
	sun_tmp[4] = 10.;
	sun_tmp[5] = 10.;
	sun_tmp[6] = 10.;
	unmap_buffer(dev, sun_data.get());

//	double intpart;
//	float frame = (float)modf(timer / 10000., &intpart);
//	frame *= 300.f;
	/*        loader->AnimatedMesh.animateMesh(frame, 1.f);
			loader->AnimatedMesh.skinMesh(1.f);

			memcpy(map_buffer(dev, jointbuffer), loader->AnimatedMesh.JointMatrixes.data(), loader->AnimatedMesh.JointMatrixes.size() * 16 * sizeof(float));*/
			//unmap_buffer(dev, jointbuffer);

	uint32_t current_backbuffer = get_next_backbuffer_id(dev, chain.get());
	submit_executable_command_list(cmdqueue.get(), command_list_for_back_buffer[current_backbuffer].get());
	wait_for_command_queue_idle(dev, cmdqueue.get());
	present(dev, cmdqueue.get(), chain.get(), current_backbuffer);
}
