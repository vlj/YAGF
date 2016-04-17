#include <assimp/Importer.hpp>
#include <Maths/matrix4.h>
#include <assimp/scene.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <array>
#include <unordered_map>

#include "../helper/SampleClass.h"

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

//constexpr auto test = pipeline_layout_description(descriptor_set({ range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1) }));


constexpr auto skinned_mesh_layout = pipeline_layout_description(
	descriptor_set({ range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 1), range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 1, 1) }),
	descriptor_set({ range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 2, 1) }),
	descriptor_set({ range_of_descriptors(RESOURCE_VIEW::SAMPLER, 3, 1) })
);

pipeline_state_t createSkinnedObjectShader(device_t dev, pipeline_layout_t layout, render_pass_t rp)
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


	vulkan_wrapper::shader_module module_vert(dev->object, "..\\..\\vert.spv");
	vulkan_wrapper::shader_module module_frag(dev->object, "..\\..\\frag.spv");

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
#ifdef D3D12
DXGI_FORMAT get_dxgi_format(irr::video::ECOLOR_FORMAT fmt)
{
	switch (fmt)
	{
	case irr::video::ECF_R8G8B8A8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case irr::video::ECF_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	case irr::video::ECF_R16G16B16A16F:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case irr::video::ECF_R32G32B32A32F:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case irr::video::ECF_A8R8G8B8:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case irr::video::ECF_BC1_UNORM:
		return DXGI_FORMAT_BC1_UNORM;
	case irr::video::ECF_BC1_UNORM_SRGB:
		return DXGI_FORMAT_BC1_UNORM_SRGB;
	case irr::video::ECF_BC2_UNORM:
		return DXGI_FORMAT_BC2_UNORM;
	case irr::video::ECF_BC2_UNORM_SRGB:
		return DXGI_FORMAT_BC2_UNORM_SRGB;
	case irr::video::ECF_BC3_UNORM:
		return DXGI_FORMAT_BC3_UNORM;
	case irr::video::ECF_BC3_UNORM_SRGB:
		return DXGI_FORMAT_BC3_UNORM_SRGB;
	case irr::video::ECF_BC4_UNORM:
		return DXGI_FORMAT_BC4_UNORM;
	case irr::video::ECF_BC4_SNORM:
		return DXGI_FORMAT_BC4_SNORM;
	case irr::video::ECF_BC5_UNORM:
		return DXGI_FORMAT_BC5_UNORM;
	case irr::video::ECF_BC5_SNORM:
		return DXGI_FORMAT_BC5_SNORM;
	case irr::video::D24U8:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	return DXGI_FORMAT_UNKNOWN;
}
#endif

struct MeshSample : Sample
{
	MeshSample(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) : Sample(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
	{}

	~MeshSample()
	{
		wait_for_command_queue_idle(dev, cmdqueue);
	}

private:
	device_t dev;
	command_queue_t cmdqueue;
	swap_chain_t chain;
	std::vector<image_t> back_buffer;
	descriptor_storage_t BackBufferDescriptorsHeap;

#ifndef D3D12
	VkDescriptorSet cbuffer_descriptor_set;
	std::vector<VkDescriptorSet> texture_descriptor_set;
#endif // !D3D12

	command_list_storage_t command_allocator;
	command_list_t command_list;
	buffer_t cbuffer;
	buffer_t jointbuffer;
	descriptor_storage_t cbv_srv_descriptors_heap;

	std::vector<image_t> Textures;
	std::vector<buffer_t> upload_buffers;
	descriptor_storage_t sampler_heap;
	image_t depth_buffer;

	std::vector<uint32_t> texture_mapping;
	std::unordered_map<std::string, uint32_t> textureSet;

	pipeline_layout_t sig;
	render_pass_t render_pass;
	framebuffer_t fbo[2];
	pipeline_state_t objectpso;

	const aiScene* model;

	std::vector<std::tuple<size_t, size_t, size_t> > meshOffset;

	buffer_t vertex_pos;
	buffer_t vertex_uv0;
	buffer_t vertex_normal;
	buffer_t index_buffer;
	size_t total_index_cnt;
	std::vector<std::tuple<buffer_t, uint64_t, uint32_t, uint32_t> > vertex_buffers_info;

protected:

	virtual void Init(HINSTANCE hinstance, HWND window) override
	{
		std::tie(dev, chain, cmdqueue) = create_device_swapchain_and_graphic_presentable_queue(hinstance, window);
		back_buffer = get_image_view_from_swap_chain(dev, chain);


		command_allocator = create_command_storage(dev);
		command_list = create_command_list(dev, command_allocator);
		sig = get_pipeline_layout_from_desc(dev, skinned_mesh_layout);

#ifndef D3D12
		start_command_list_recording(dev, command_list, command_allocator);
		set_pipeline_barrier(dev, command_list, back_buffer[0], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0);
		set_pipeline_barrier(dev, command_list, back_buffer[1], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0);

#endif // !D3D12

		cbuffer = create_buffer(dev, sizeof(Matrixes));
		jointbuffer = create_buffer(dev, sizeof(JointTransform));


		clear_value_structure_t clear_val = {};
#ifdef D3D12
		cbv_srv_descriptors_heap = create_descriptor_storage(dev, 1000);

		create_constant_buffer_view(dev, cbv_srv_descriptors_heap, 0, cbuffer, sizeof(Matrixes));
		create_constant_buffer_view(dev, cbv_srv_descriptors_heap, 1, jointbuffer, sizeof(JointTransform));

		sampler_heap = create_sampler_heap(dev, 1);
		create_sampler(dev, sampler_heap, 0, SAMPLER_TYPE::TRILINEAR);

		clear_val = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1., 0);
#endif

		depth_buffer = create_image(dev, irr::video::D24U8, 1024, 1024, 1, usage_depth_stencil, RESOURCE_USAGE::DEPTH_WRITE, &clear_val);

#ifndef D3D12
		std::vector<VkDescriptorPoolSize> size = { VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 }, VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER, 1 }, VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10 } };
		cbv_srv_descriptors_heap = std::make_shared<vulkan_wrapper::descriptor_pool>(dev->object, 0, 3, size);

		VkDescriptorSetAllocateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, cbv_srv_descriptors_heap->object, 1, sig->info.pSetLayouts };
		CHECK_VKRESULT(vkAllocateDescriptorSets(dev->object, &info, &cbuffer_descriptor_set));
		VkDescriptorBufferInfo cbuffer_info{ cbuffer->object, 0, sizeof(Matrixes) };
		VkWriteDescriptorSet update_info{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, cbuffer_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &cbuffer_info, nullptr };
		vkUpdateDescriptorSets(dev->object, 1, &update_info, 0, nullptr);

		VkDescriptorBufferInfo cbuffer2_info{ jointbuffer->object, 0, sizeof(JointTransform) };
		VkWriteDescriptorSet update2_info{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, cbuffer_descriptor_set, 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &cbuffer2_info, nullptr };
		vkUpdateDescriptorSets(dev->object, 1, &update2_info, 0, nullptr);

		VkAttachmentDescription attachment{};
		attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
		attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

		render_pass.reset(new vulkan_wrapper::render_pass(dev->object,
		{ attachment },
		{
			subpass_description::generate_subpass_description(VK_PIPELINE_BIND_POINT_GRAPHICS)
				.set_color_attachments({ VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } })
		}, { VkSubpassDependency() }));
#endif

		fbo[0] = create_frame_buffer(dev, { { back_buffer[0], irr::video::ECOLOR_FORMAT::ECF_A8R8G8B8 } }, { depth_buffer, irr::video::ECOLOR_FORMAT::D24U8 }, 1024, 1024, render_pass);
		fbo[1] = create_frame_buffer(dev, { { back_buffer[1], irr::video::ECOLOR_FORMAT::ECF_A8R8G8B8 } }, { depth_buffer, irr::video::ECOLOR_FORMAT::D24U8 }, 1024, 1024, render_pass);

		Assimp::Importer importer;
		model = importer.ReadFile("..\\..\\examples\\assets\\xue.b3d", 0);


		// Format Weight

/*        std::vector<std::vector<irr::video::SkinnedVertexData> > weightsList;
		for (auto weightbuffer : loader->AnimatedMesh.WeightBuffers)
		{
			std::vector<irr::video::SkinnedVertexData> weights;
			for (unsigned j = 0; j < weightbuffer.size(); j += 4)
			{
				irr::video::SkinnedVertexData tmp = {
					weightbuffer[j].Index, weightbuffer[j].Weight,
					weightbuffer[j + 1].Index, weightbuffer[j + 1].Weight,
					weightbuffer[j + 2].Index, weightbuffer[j + 2].Weight,
					weightbuffer[j + 3].Index, weightbuffer[j + 3].Weight,
				};
				weights.push_back(tmp);
			}
			weightsList.push_back(weights);
		}*/


		size_t total_vertex_cnt = 0;
		total_index_cnt = 0;
		for (int i = 0; i < model->mNumMeshes; ++i)
		{
			const aiMesh *mesh = model->mMeshes[i];
			total_vertex_cnt += mesh->mNumVertices;
			total_index_cnt += mesh->mNumFaces * 3;
		}
		//vao = new FormattedVertexStorage(Context::getInstance()->cmdqueue.Get(), reorg, weightsList);

		index_buffer = create_buffer(dev, total_index_cnt * sizeof(uint16_t));
		uint16_t *indexmap = (uint16_t *)map_buffer(dev, index_buffer);
		vertex_pos = create_buffer(dev, total_vertex_cnt * sizeof(aiVector3D));
		aiVector3D *vertex_pos_map = (aiVector3D*)map_buffer(dev, vertex_pos);
		vertex_normal = create_buffer(dev, total_vertex_cnt * sizeof(aiVector3D));
		aiVector3D *vertex_normal_map = (aiVector3D*)map_buffer(dev, vertex_normal);
		vertex_uv0 = create_buffer(dev, total_vertex_cnt * sizeof(aiVector3D));
		aiVector3D *vertex_uv_map = (aiVector3D*)map_buffer(dev, vertex_uv0);

		size_t basevertex = 0, baseindex = 0;

		for (int i = 0; i < model->mNumMeshes; ++i)
		{
			const aiMesh *mesh = model->mMeshes[i];
			for (int vtx = 0; vtx < mesh->mNumVertices; ++vtx)
			{
				vertex_pos_map[basevertex + vtx] = mesh->mVertices[vtx];
				vertex_normal_map[basevertex + vtx] = mesh->mNormals[vtx];
				vertex_uv_map[basevertex + vtx] = mesh->mTextureCoords[0][vtx];
			}
			for (int idx = 0; idx < mesh->mNumFaces; ++idx)
			{
				indexmap[baseindex + 3 * idx] = mesh->mFaces[idx].mIndices[0];
				indexmap[baseindex + 3 * idx + 1] = mesh->mFaces[idx].mIndices[1];
				indexmap[baseindex + 3 * idx + 2] = mesh->mFaces[idx].mIndices[2];
			}
			meshOffset.push_back(std::make_tuple(mesh->mNumFaces * 3, basevertex, baseindex));
			basevertex += mesh->mNumVertices;
			baseindex += mesh->mNumFaces * 3;
			texture_mapping.push_back(mesh->mMaterialIndex);
		}

		unmap_buffer(dev, index_buffer);
		unmap_buffer(dev, vertex_pos);
		unmap_buffer(dev, vertex_normal);
		unmap_buffer(dev, vertex_uv0);
		// TODO: Upload to GPUmem

		vertex_buffers_info.emplace_back(vertex_pos, 0, static_cast<uint32_t>(sizeof(aiVector3D)), total_vertex_cnt * sizeof(aiVector3D));
		vertex_buffers_info.emplace_back(vertex_normal, 0, static_cast<uint32_t>(sizeof(aiVector3D)), total_vertex_cnt * sizeof(aiVector3D));
		vertex_buffers_info.emplace_back(vertex_uv0, 0, static_cast<uint32_t>(sizeof(aiVector3D)), total_vertex_cnt * sizeof(aiVector3D));

		// Upload to gpudata

		// Texture
		for (int texture_id = 0; texture_id < model->mNumMaterials; ++texture_id)
		{
			aiString path;
			model->mMaterials[texture_id]->GetTexture(aiTextureType_DIFFUSE, 0, &path);
			std::string texture_path(path.C_Str());
			const std::string &fixed = "..\\..\\examples\\assets\\" + texture_path.substr(0, texture_path.find_last_of('.')) + ".DDS";
			std::ifstream DDSFile(fixed, std::ifstream::binary);
			irr::video::CImageLoaderDDS DDSPic(DDSFile);

			uint32_t width = DDSPic.getLoadedImage().Layers[0][0].Width;
			uint32_t height = DDSPic.getLoadedImage().Layers[0][0].Height;
			uint16_t mipmap_count = DDSPic.getLoadedImage().Layers[0].size();
			uint16_t layer_count = 1;

			buffer_t upload_buffer = create_buffer(dev, width * height * 3 * 6);
			upload_buffers.push_back(upload_buffer);
			void *pointer = map_buffer(dev, upload_buffer);

			size_t offset_in_texram = 0;

			size_t block_height = 4;
			size_t block_width = 4;
			size_t block_size = 8;
			std::vector<MipLevelData> Mips;
			for (unsigned face = 0; face < layer_count; face++)
			{
				for (unsigned i = 0; i < mipmap_count; i++)
				{
					const IImage &image = DDSPic.getLoadedImage();
					struct PackedMipMapLevel miplevel = image.Layers[face][i];
					// Offset needs to be aligned to 512 bytes
					offset_in_texram = (offset_in_texram + 511) & ~511;
					// Row pitch is always a multiple of 256
					size_t height_in_blocks = (image.Layers[face][i].Height + block_height - 1) / block_height;
					size_t width_in_blocks = (image.Layers[face][i].Width + block_width - 1) / block_width;
					size_t height_in_texram = height_in_blocks * block_height;
					size_t width_in_texram = width_in_blocks * block_width;
					size_t rowPitch = width_in_blocks * block_size;
					rowPitch = (rowPitch + 255) & ~255;
					MipLevelData mml = { offset_in_texram, width_in_texram, height_in_texram, rowPitch };
					Mips.push_back(mml);
					for (unsigned row = 0; row < height_in_blocks; row++)
					{
						memcpy(((char*)pointer) + offset_in_texram, ((char*)miplevel.Data) + row * width_in_blocks * block_size, width_in_blocks * block_size);
						offset_in_texram += rowPitch;
					}
				}
			}
			unmap_buffer(dev, upload_buffer);

			image_t texture = create_image(dev, DDSPic.getLoadedImage().Format,
				width, height, mipmap_count, usage_sampled | usage_transfer_dst,
				RESOURCE_USAGE::READ_GENERIC,
				nullptr);


			Textures.push_back(texture);

			uint32_t miplevel = 0;
			for (const MipLevelData mipmapData : Mips)
			{
#ifdef D3D12
				set_pipeline_barrier(dev, command_list, texture, RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::COPY_DEST, miplevel);

				command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(texture.Get(), miplevel), 0, 0, 0,
					&CD3DX12_TEXTURE_COPY_LOCATION(upload_buffer.Get(), { mipmapData.Offset,{ get_dxgi_format(DDSPic.getLoadedImage().Format), (UINT)mipmapData.Width, (UINT)mipmapData.Height, 1, (UINT16)mipmapData.RowPitch } }),
					&CD3DX12_BOX(0, 0, (UINT)mipmapData.Width, (UINT)mipmapData.Height));
#else
				set_pipeline_barrier(dev, command_list, texture, RESOURCE_USAGE::undefined, RESOURCE_USAGE::COPY_DEST, miplevel);
				VkBufferImageCopy info{};
				info.bufferOffset = mipmapData.Offset;
				info.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				info.imageSubresource.mipLevel = miplevel;
				info.imageSubresource.layerCount = 1;
				info.imageExtent.width = mipmapData.Width;
				info.imageExtent.height = mipmapData.Height;
				info.imageExtent.depth = 1;
				vkCmdCopyBufferToImage(command_list->object, upload_buffer->object, texture->object, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &info);

#endif // !D3D12
				set_pipeline_barrier(dev, command_list, texture, RESOURCE_USAGE::COPY_DEST, RESOURCE_USAGE::READ_GENERIC, miplevel);
				miplevel++;
			}
			textureSet[fixed] = 2 + texture_id;
#ifdef D3D12
			create_image_view(dev, cbv_srv_descriptors_heap, 2 + texture_id, texture);
#else
			VkDescriptorSetAllocateInfo allocate_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, cbv_srv_descriptors_heap->object, 1, &sig->info.pSetLayouts[1] };
			VkDescriptorSet texture_descriptor;
			CHECK_VKRESULT(vkAllocateDescriptorSets(dev->object, &allocate_info, &texture_descriptor));
			texture_descriptor_set.push_back(texture_descriptor);
			VkDescriptorImageInfo image_view{ VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

			VkWriteDescriptorSet write_info{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, texture_descriptor, 2, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &image_view, nullptr, nullptr};
			vkUpdateDescriptorSets(dev->object, 1, &write_info, 0, nullptr);
#endif
		}

		objectpso = createSkinnedObjectShader(dev, sig, render_pass);
		make_command_list_executable(command_list);
		submit_executable_command_list(cmdqueue, command_list);
		wait_for_command_queue_idle(dev, cmdqueue);
	}

	virtual void Draw() override
	{
		start_command_list_recording(dev, command_list, command_allocator);
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

		set_pipeline_barrier(dev, command_list, back_buffer[current_backbuffer], RESOURCE_USAGE::PRESENT, RESOURCE_USAGE::RENDER_TARGET, 0);

		std::array<float, 4> clearColor = { .25f, .25f, 0.35f, 1.0f };
#ifndef D3D12
		VkRenderPassBeginInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		info.renderPass = render_pass->object;
		info.framebuffer = fbo[current_backbuffer]->fbo.object;
		info.clearValueCount = 1;
		VkClearValue clear_values{};
		memcpy(clear_values.color.float32, clearColor.data(), 4 * sizeof(float));
		info.pClearValues = &clear_values;
		info.renderArea.extent.width = 900;
		info.renderArea.extent.height = 900;
		vkCmdBeginRenderPass(command_list->object, &info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(command_list->object, VK_PIPELINE_BIND_POINT_GRAPHICS, sig->object, 0, 1, &cbuffer_descriptor_set, 0, nullptr);
#else // !D3D12
		command_list->OMSetRenderTargets(1, &(fbo[current_backbuffer]->rtt_heap->GetCPUDescriptorHandleForHeapStart()), true, &(fbo[current_backbuffer]->dsv_heap->GetCPUDescriptorHandleForHeapStart()));

		command_list->SetGraphicsRootSignature(sig.Get());

		std::array<ID3D12DescriptorHeap*, 2> descriptors = { cbv_srv_descriptors_heap.Get(), sampler_heap.Get() };
		command_list->SetDescriptorHeaps(2, descriptors.data());

		command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		command_list->SetGraphicsRootDescriptorTable(0, cbv_srv_descriptors_heap->GetGPUDescriptorHandleForHeapStart());

		clear_color(dev, command_list, fbo[current_backbuffer], clearColor);
		clear_depth_stencil(dev, command_list, fbo[current_backbuffer], depth_stencil_aspect::depth_only, 1., 0);

		command_list->SetGraphicsRootDescriptorTable(2,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(sampler_heap->GetGPUDescriptorHandleForHeapStart()));
#endif
		set_graphic_pipeline(command_list, objectpso);

		set_viewport(command_list, 0., 1024.f, 0., 1024.f, 0., 1.);
		set_scissor(command_list, 0, 1024, 0, 1024);

		bind_index_buffer(command_list, index_buffer, 0, total_index_cnt * sizeof(uint16_t), irr::video::E_INDEX_TYPE::EIT_16BIT);
		bind_vertex_buffers(command_list, 0, vertex_buffers_info);


		//        std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > buffers = loader->AnimatedMesh.getMeshBuffers();
		for (unsigned i = 0; i < meshOffset.size(); i++)
		{
#ifdef D3D12
			command_list->SetGraphicsRootDescriptorTable(1,
				CD3DX12_GPU_DESCRIPTOR_HANDLE(cbv_srv_descriptors_heap->GetGPUDescriptorHandleForHeapStart())
				.Offset(texture_mapping[i] + 2, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
#endif
			draw_indexed(command_list, std::get<0>(meshOffset[i]), 1, std::get<2>(meshOffset[i]), std::get<1>(meshOffset[i]), 0);
		}

#ifndef D3D12
		vkCmdEndRenderPass(command_list->object);
#endif // !D3D12
		set_pipeline_barrier(dev, command_list, back_buffer[current_backbuffer], RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::PRESENT, 0);
		make_command_list_executable(command_list);
		submit_executable_command_list(cmdqueue, command_list);
		wait_for_command_queue_idle(dev, cmdqueue);
		reset_command_list_storage(dev, command_allocator);
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
