#include <assimp/Importer.hpp>
#include <Maths/matrix4.h>
#include <assimp/scene.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <array>
#include <unordered_map>

#include "../helper/SampleClass.h"
#include "shaders.h"
#include "geometry.h"
#include "textures.h"

#ifdef D3D12
#include <API/d3dapi.h>
#include <d3dx12.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")
#else
#include <API/vkapi.h>
#endif

#define CHECK_HRESULT(cmd) {HRESULT hr = cmd; if (hr != 0) throw;}

static float timer = 0.;

struct Matrixes
{
	float Model[16];
	float ViewProj[16];
};

struct JointTransform
{
	float Model[16 * 48];
};


struct MeshSample : Sample
{
	MeshSample(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) : Sample(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
	{}

	~MeshSample()
	{
		wait_for_command_queue_idle(dev, cmdqueue);
	}

private:
	size_t width;
	size_t height;

	device_t dev;
	command_queue_t cmdqueue;
	swap_chain_t chain;
	std::vector<image_t> back_buffer;
	std::vector<command_list_t> command_list_for_back_buffer;

	command_list_storage_t command_allocator;
	buffer_t cbuffer;
	buffer_t jointbuffer;
	buffer_t big_triangle;
	descriptor_storage_t cbv_srv_descriptors_heap;

	std::vector<image_t> Textures;
#ifndef D3D12
	VkDescriptorSet cbuffer_descriptor_set;
	std::vector<VkDescriptorSet> texture_descriptor_set;
	std::vector<std::shared_ptr<vulkan_wrapper::image_view> > Textures_views;
	std::shared_ptr<vulkan_wrapper::sampler> sampler;
	VkDescriptorSet sampler_descriptors;
	VkDescriptorSet input_attachment_descriptors;
	std::shared_ptr<vulkan_wrapper::image_view> diffuse_color_view;
	std::shared_ptr<vulkan_wrapper::image_view> normal_roughness_metalness_view;
#endif
	descriptor_storage_t sampler_heap;
	image_t depth_buffer;
	image_t diffuse_color;
	image_t normal_roughness_metalness;

	std::unique_ptr<object> xue;


	render_pass_t render_pass;
	framebuffer_t fbo[2];
	pipeline_layout_t object_sig;
	pipeline_state_t objectpso;
	pipeline_layout_t sunlight_sig;
	pipeline_state_t sunlightpso;

protected:

	virtual void Init(HINSTANCE hinstance, HWND window) override
	{
		irr::video::ECOLOR_FORMAT swap_chain_format;
		std::tie(dev, chain, cmdqueue, width, height, swap_chain_format) = create_device_swapchain_and_graphic_presentable_queue(hinstance, window);
		back_buffer = get_image_view_from_swap_chain(dev, chain);

		command_allocator = create_command_storage(dev);
		command_list_t command_list = create_command_list(dev, command_allocator);
		start_command_list_recording(dev, command_list, command_allocator);
		object_sig = get_skinned_object_pipeline_layout(dev);
		sunlight_sig = get_sunlight_pipeline_layout(dev);
		cbuffer = create_buffer(dev, sizeof(Matrixes));
		jointbuffer = create_buffer(dev, sizeof(JointTransform));

		cbv_srv_descriptors_heap = create_descriptor_storage(dev, 100, { { RESOURCE_VIEW::CONSTANTS_BUFFER, 2 }, {RESOURCE_VIEW::SHADER_RESOURCE, 1000}, {RESOURCE_VIEW::INPUT_ATTACHMENT, 2 },{ RESOURCE_VIEW::SAMPLER, 10 } });
		sampler_heap = create_descriptor_storage(dev, 1, { {RESOURCE_VIEW::SAMPLER, 10 } });

		clear_value_structure_t clear_val = {};
#ifndef D3D12
		set_pipeline_barrier(dev, command_list, back_buffer[0], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);
		set_pipeline_barrier(dev, command_list, back_buffer[1], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);

		VkAttachmentDescription attachment{};
		attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
		attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

		VkAttachmentDescription depth_att{};
		depth_att.format = VK_FORMAT_D24_UNORM_S8_UINT;
		depth_att.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth_att.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth_att.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_att.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		render_pass.reset(new vulkan_wrapper::render_pass(dev->object,
		{ attachment, attachment, attachment, depth_att },
		{
			subpass_description::generate_subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS)
				.set_color_attachments({ VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } })
				.set_depth_stencil_attachment(VkAttachmentReference{ 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }),
			subpass_description::generate_subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS)
				.set_color_attachments({ VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } })
				.set_input_attachments({ VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } })
		}, { get_subpass_dependency(0, 1, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT) }));
#else
		create_constant_buffer_view(dev, cbv_srv_descriptors_heap, 0, cbuffer, sizeof(Matrixes));
		create_constant_buffer_view(dev, cbv_srv_descriptors_heap, 1, jointbuffer, sizeof(JointTransform));
		create_sampler(dev, sampler_heap, 0, SAMPLER_TYPE::TRILINEAR);

		clear_val = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1., 0);
#endif // !D3D12
		depth_buffer = create_image(dev, irr::video::D24U8, width, height, 1, usage_depth_stencil, &clear_val);
		diffuse_color = create_image(dev, irr::video::ECF_R8G8B8A8_UNORM, width, height, 1, usage_render_target | usage_sampled, &clear_val);
		normal_roughness_metalness = create_image(dev, irr::video::ECF_R8G8B8A8_UNORM, width, height, 1, usage_render_target | usage_sampled, &clear_val);
		set_pipeline_barrier(dev, command_list, depth_buffer, RESOURCE_USAGE::undefined, RESOURCE_USAGE::DEPTH_WRITE, 0, irr::video::E_ASPECT::EA_DEPTH_STENCIL);
		set_pipeline_barrier(dev, command_list, diffuse_color, RESOURCE_USAGE::undefined, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
		set_pipeline_barrier(dev, command_list, normal_roughness_metalness, RESOURCE_USAGE::undefined, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);

		fbo[0] = create_frame_buffer(dev, { { diffuse_color, irr::video::ECF_R8G8B8A8_UNORM }, { normal_roughness_metalness, irr::video::ECF_R8G8B8A8_UNORM }, { back_buffer[0], swap_chain_format } }, { depth_buffer, irr::video::ECOLOR_FORMAT::D24U8 }, width, height, render_pass);
		fbo[1] = create_frame_buffer(dev, { { diffuse_color, irr::video::ECF_R8G8B8A8_UNORM }, { normal_roughness_metalness, irr::video::ECF_R8G8B8A8_UNORM }, { back_buffer[1], swap_chain_format } }, { depth_buffer, irr::video::ECOLOR_FORMAT::D24U8 }, width, height, render_pass);
#ifndef D3D12
		sampler = std::make_shared<vulkan_wrapper::sampler>(dev->object, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.f, true, 16.f);
		diffuse_color_view = std::make_shared<vulkan_wrapper::image_view>(dev->object, diffuse_color->object, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
			structures::component_mapping(), structures::image_subresource_range());
		normal_roughness_metalness_view = std::make_shared<vulkan_wrapper::image_view>(dev->object, normal_roughness_metalness->object, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
			structures::component_mapping(), structures::image_subresource_range());

		CHECK_VKRESULT(vkAllocateDescriptorSets(dev->object,
			&structures::descriptor_set_allocate_info(sampler_heap->object, { object_sig->info.pSetLayouts[2] }),
			&sampler_descriptors));
		CHECK_VKRESULT(vkAllocateDescriptorSets(dev->object,
			&structures::descriptor_set_allocate_info(cbv_srv_descriptors_heap->object, { object_sig->info.pSetLayouts[0] }),
			&cbuffer_descriptor_set));
		CHECK_VKRESULT(vkAllocateDescriptorSets(dev->object,
			&structures::descriptor_set_allocate_info(cbv_srv_descriptors_heap->object, { sunlight_sig->info.pSetLayouts[0] }),
			&input_attachment_descriptors));

		util::update_descriptor_sets(dev->object,
		{
			structures::write_descriptor_set(cbuffer_descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				{ VkDescriptorBufferInfo{ cbuffer->object, 0, sizeof(Matrixes) } }, 0),
			structures::write_descriptor_set(cbuffer_descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				{ VkDescriptorBufferInfo{ jointbuffer->object, 0, sizeof(JointTransform) } }, 1),
			structures::write_descriptor_set(sampler_descriptors, VK_DESCRIPTOR_TYPE_SAMPLER,
				{ VkDescriptorImageInfo{ sampler->object, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 3),
			structures::write_descriptor_set(input_attachment_descriptors, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				{ VkDescriptorImageInfo{ VK_NULL_HANDLE, diffuse_color_view->object, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 0),
			structures::write_descriptor_set(input_attachment_descriptors, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				{ VkDescriptorImageInfo{ VK_NULL_HANDLE, normal_roughness_metalness_view->object, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 1),
			structures::write_descriptor_set(input_attachment_descriptors, VK_DESCRIPTOR_TYPE_SAMPLER,
				{ VkDescriptorImageInfo{ sampler->object, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 3),
		});
#endif

		Assimp::Importer importer;
		auto model = importer.ReadFile("..\\..\\..\\examples\\assets\\xue.b3d", 0);
		xue = std::make_unique<object>(dev, model);

		// Texture
		std::vector<buffer_t> upload_buffers;
		for (int texture_id = 0; texture_id < model->mNumMaterials; ++texture_id)
		{
			aiString path;
			model->mMaterials[texture_id]->GetTexture(aiTextureType_DIFFUSE, 0, &path);
			std::string texture_path(path.C_Str());
			const std::string &fixed = "..\\..\\..\\examples\\assets\\" + texture_path.substr(0, texture_path.find_last_of('.')) + ".DDS";

			image_t texture;
			buffer_t upload_buffer;
			std::tie(texture, upload_buffer) = load_texture(dev, fixed, command_list);
			Textures.push_back(texture);
			upload_buffers.push_back(upload_buffer);
#ifdef D3D12
			create_image_view(dev, cbv_srv_descriptors_heap, 2 + texture_id, texture);
#else
			VkDescriptorSet texture_descriptor;
			CHECK_VKRESULT(vkAllocateDescriptorSets(dev->object, &structures::descriptor_set_allocate_info(cbv_srv_descriptors_heap->object, { object_sig->info.pSetLayouts[1] }), &texture_descriptor));
			texture_descriptor_set.push_back(texture_descriptor);
			auto img_view = std::make_shared<vulkan_wrapper::image_view>(dev->object, texture->object, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
				structures::component_mapping(), structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT, 0, texture->info.mipLevels));
			Textures_views.push_back(img_view);
			util::update_descriptor_sets(dev->object,
			{
				structures::write_descriptor_set(texture_descriptor, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, { VkDescriptorImageInfo{ VK_NULL_HANDLE, img_view->object, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } }, 2)
			});
#endif
		}

		objectpso = get_skinned_object_pipeline_state(dev, object_sig, render_pass);
		sunlightpso = get_sunlight_pipeline_state(dev, sunlight_sig, render_pass);


		big_triangle = create_buffer(dev, 4 * 3 * sizeof(float));
		float fullscreen_tri[]
		{
			-1., -3., 0., 2.,
			3., 1., 2., 0.,
			-1., 1., 0., 0.
		};

		memcpy(map_buffer(dev, big_triangle), fullscreen_tri, 4 * 3 * sizeof(float));
		unmap_buffer(dev, big_triangle);

		make_command_list_executable(command_list);
		submit_executable_command_list(cmdqueue, command_list);
		wait_for_command_queue_idle(dev, cmdqueue);
		fill_draw_commands();
	}


	void fill_draw_commands()
	{
		for (unsigned i = 0; i < 2; i++)
		{
			command_list_for_back_buffer.push_back(create_command_list(dev, command_allocator));
			command_list_t current_cmd_list = command_list_for_back_buffer.back();


			start_command_list_recording(dev, current_cmd_list, command_allocator);

			set_pipeline_barrier(dev, current_cmd_list, back_buffer[i], RESOURCE_USAGE::PRESENT, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);

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

			vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, object_sig->object, 0, 1, &cbuffer_descriptor_set, 0, nullptr);
			vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, object_sig->object, 2, 1, &sampler_descriptors, 0, nullptr);
#else // !D3D12
			current_cmd_list->OMSetRenderTargets(1, &(fbo[i]->rtt_heap->GetCPUDescriptorHandleForHeapStart()), true, &(fbo[i]->dsv_heap->GetCPUDescriptorHandleForHeapStart()));

			current_cmd_list->SetGraphicsRootSignature(object_sig.Get());

			std::array<ID3D12DescriptorHeap*, 2> descriptors = { cbv_srv_descriptors_heap.Get(), sampler_heap.Get() };
			current_cmd_list->SetDescriptorHeaps(2, descriptors.data());

			current_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			current_cmd_list->SetGraphicsRootDescriptorTable(0, cbv_srv_descriptors_heap->GetGPUDescriptorHandleForHeapStart());

			clear_color(dev, current_cmd_list, fbo[i], clearColor);
			clear_depth_stencil(dev, current_cmd_list, fbo[i], depth_stencil_aspect::depth_only, 1., 0);

			current_cmd_list->SetGraphicsRootDescriptorTable(2,
				CD3DX12_GPU_DESCRIPTOR_HANDLE(sampler_heap->GetGPUDescriptorHandleForHeapStart()));
#endif
			set_graphic_pipeline(current_cmd_list, objectpso);

			set_viewport(current_cmd_list, 0., 1024.f, 0., 1024.f, 0., 1.);
			set_scissor(current_cmd_list, 0, 1024, 0, 1024);

			bind_index_buffer(current_cmd_list, xue->index_buffer, 0, xue->total_index_cnt * sizeof(uint16_t), irr::video::E_INDEX_TYPE::EIT_16BIT);
			bind_vertex_buffers(current_cmd_list, 0, xue->vertex_buffers_info);


			for (unsigned i = 0; i < xue->meshOffset.size(); i++)
			{
#ifdef D3D12
				current_cmd_list->SetGraphicsRootDescriptorTable(1,
					CD3DX12_GPU_DESCRIPTOR_HANDLE(cbv_srv_descriptors_heap->GetGPUDescriptorHandleForHeapStart())
					.Offset(xue->texture_mapping[i] + 2, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
#else
				vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, object_sig->object, 1, 1, &texture_descriptor_set[xue->texture_mapping[i]], 0, nullptr);
#endif
				draw_indexed(current_cmd_list, std::get<0>(xue->meshOffset[i]), 1, std::get<2>(xue->meshOffset[i]), std::get<1>(xue->meshOffset[i]), 0);
			}
			vkCmdNextSubpass(current_cmd_list->object, VK_SUBPASS_CONTENTS_INLINE);
			set_graphic_pipeline(current_cmd_list, sunlightpso);
			size_t offsets[1] = {};
			vkCmdBindVertexBuffers(current_cmd_list->object, 0, 1, &big_triangle->object, offsets);
			set_viewport(current_cmd_list, 0., 1024.f, 0., 1024.f, 0., 1.);
			set_scissor(current_cmd_list, 0, 1024, 0, 1024);
			vkCmdBindDescriptorSets(current_cmd_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, sunlight_sig->object, 0, 1, &input_attachment_descriptors, 0, nullptr);
			draw_non_indexed(current_cmd_list, 3, 1, 0, 0);

#ifndef D3D12
			vkCmdEndRenderPass(current_cmd_list->object);
#endif // !D3D12
			set_pipeline_barrier(dev, current_cmd_list, back_buffer[i], RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);
			make_command_list_executable(current_cmd_list);
		}
	}

	virtual void Draw() override
	{
		Matrixes *cbufdata = static_cast<Matrixes*>(map_buffer(dev, cbuffer));
		irr::core::matrix4 Model, View;
		View.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
		Model.setTranslation(irr::core::vector3df(0.f, 0.f, 2.f));
		Model.setRotationDegrees(irr::core::vector3df(0.f, timer / 360.f, 0.f));

		memcpy(cbufdata->Model, Model.pointer(), 16 * sizeof(float));
		memcpy(cbufdata->ViewProj, View.pointer(), 16 * sizeof(float));
		unmap_buffer(dev, cbuffer);

		timer += 16.f;

		double intpart;
		float frame = (float)modf(timer / 10000., &intpart);
		frame *= 300.f;
		/*        loader->AnimatedMesh.animateMesh(frame, 1.f);
				loader->AnimatedMesh.skinMesh(1.f);

				memcpy(map_buffer(dev, jointbuffer), loader->AnimatedMesh.JointMatrixes.data(), loader->AnimatedMesh.JointMatrixes.size() * 16 * sizeof(float));*/
				//unmap_buffer(dev, jointbuffer);

		uint32_t current_backbuffer = get_next_backbuffer_id(dev, chain);
		submit_executable_command_list(cmdqueue, command_list_for_back_buffer[current_backbuffer]);
		wait_for_command_queue_idle(dev, cmdqueue);
		present(dev, cmdqueue, chain, current_backbuffer);
	}
};


int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	MeshSample app(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	return app.run();
}
