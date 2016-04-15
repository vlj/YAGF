#include <Loaders/B3D.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <array>
#include <unordered_map>

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


class WindowUtil
{
private:
    // this is the main message handler for the program
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        // sort through and find what code to run for the message given
        switch (message)
        {
            // this message is read when the window is closed
        case WM_DESTROY:
        {
            // close the application entirely
            PostQuitMessage(0);
            return 0;
        } break;
        }

        // Handle any messages the switch statement didn't
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

public:
    static HWND Create(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
    {
        // the handle for the window, filled by a function
        HWND hWnd;
        // this struct holds information for the window class
        WNDCLASSEX wc = {};

        // fill in the struct with the needed information
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wc.lpszClassName = L"WindowClass1";

        // register the window class
        RegisterClassEx(&wc);

        // create the window and use the result as the handle
        hWnd = CreateWindowEx(NULL,
            L"WindowClass1",    // name of the window class
            L"MeshDX12",   // title of the window
            WS_OVERLAPPEDWINDOW,    // window style
            300,    // x-position of the window
            300,    // y-position of the window
            1024,    // width of the window
            1024,    // height of the window
            NULL,    // we have no parent window, NULL
            NULL,    // we aren't using menus, NULL
            hInstance,    // application handle
            NULL);    // used with multiple windows, NULL
        ShowWindow(hWnd, nCmdShow);
        return hWnd;
    }
};



auto tmp = pipeline_layout_description({
	descriptor_set({ range_of_descriptors(RESOURCE_VIEW::CONSTANTS_BUFFER, 0, 2), range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 1, 1) }),
	descriptor_set({ range_of_descriptors(RESOURCE_VIEW::SAMPLER, 2, 1)})
});


pipeline_state_t createSkinnedObjectShader(device_t dev, pipeline_layout_t layout, render_pass_t rp)
{
    pipeline_state_t result;

	constexpr rasterisation_state raster = rasterisation_state::get();
	constexpr depth_stencil_state depth_stencil = depth_stencil_state::get();
	const blend_state blend = blend_state::get();

	VkPipelineVertexInputStateCreateInfo vertex_input{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	VkPipelineTessellationStateCreateInfo tesselation_info{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
	VkPipelineViewportStateCreateInfo viewport_info{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	VkPipelineDynamicStateCreateInfo dynamic_state_info{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	VkPipelineMultisampleStateCreateInfo multisample_info{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

	VkShaderModule module;

	const std::vector<VkPipelineShaderStageCreateInfo> shader_stages{
		{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, module, "main", nullptr },
		{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, module, "main", nullptr }
	};

	vulkan_wrapper::pipeline tmp(dev->object, 0, shader_stages, vertex_input, input_assembly_info, tesselation_info, viewport_info, raster, multisample_info, depth_stencil, blend, dynamic_state_info, layout->object, rp->object, 0, VK_NULL_HANDLE, 0);

#ifdef D3D12
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
    psodesc.pRootSignature = sig;
    psodesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psodesc.NumRenderTargets = 1;
    psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
//    psodesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psodesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psodesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

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
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32_SINT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 1, 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 3, DXGI_FORMAT_R32_SINT, 1, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 4, DXGI_FORMAT_R32_FLOAT, 1, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 5, DXGI_FORMAT_R32_SINT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 6, DXGI_FORMAT_R32_FLOAT, 1, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 7, DXGI_FORMAT_R32_SINT, 1, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 8, DXGI_FORMAT_R32_FLOAT, 1, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    psodesc.InputLayout.pInputElementDescs = IAdesc.data();
    psodesc.InputLayout.NumElements = IAdesc.size();
    psodesc.SampleDesc.Count = 1;
    psodesc.SampleMask = UINT_MAX;
    psodesc.NodeMask = 1;
    CHECK_HRESULT(dev->CreateGraphicsPipelineState(&psodesc, IID_PPV_ARGS(result.GetAddressOf())));
#endif
    return result;
}


struct Sample
{
    device_t dev;
    command_queue_t cmdqueue;
    swap_chain_t chain;
    std::vector<image_t> back_buffer;
    descriptor_storage_t BackBufferDescriptorsHeap;

    command_list_storage_t command_allocator;
    command_list_t command_list;
    buffer_t cbuffer;
    buffer_t jointbuffer;
    descriptor_storage_t cbv_srv_descriptors_heap;

    std::vector<image_t> Textures;
    std::vector<buffer_t> upload_buffers;
    descriptor_storage_t sampler_heap;
    image_t depth_buffer;

    std::unordered_map<std::string, uint32_t> textureSet;

    pipeline_state_t objectpso;
    pipeline_layout_t sig;
	render_pass_t render_pass;
    framebuffer_t fbo[2];

    irr::scene::CB3DMeshFileLoader *loader;

    std::vector<std::tuple<size_t, size_t, size_t> > meshOffset;

    buffer_t vertex_buffer_attributes;
    buffer_t index_buffer;
    size_t total_index_cnt;
    std::vector<std::tuple<buffer_t, uint64_t, uint32_t, uint32_t> > vertex_buffers_info;


    void Init()
    {
        command_allocator = create_command_storage(dev);
        command_list = create_command_list(dev, command_allocator);


#ifndef D3D12
		start_command_list_recording(dev, command_list, command_allocator);
		set_pipeline_barrier(dev, command_list, back_buffer[0], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0);
		set_pipeline_barrier(dev, command_list, back_buffer[1], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0);
#endif // !D3D12

        cbuffer = create_buffer(dev, sizeof(Matrixes));
        jointbuffer = create_buffer(dev, sizeof(JointTransform));

        cbv_srv_descriptors_heap = create_descriptor_storage(dev, 1000);
/*        create_constant_buffer_view(dev, cbv_srv_descriptors_heap, 0, cbuffer, sizeof(Matrixes));
        create_constant_buffer_view(dev, cbv_srv_descriptors_heap, 1, jointbuffer, sizeof(JointTransform));

        sampler_heap = create_sampler_heap(dev, 1);
        create_sampler(dev, sampler_heap, 0, SAMPLER_TYPE::TRILINEAR);*/

		clear_value_structure_t clear_val = {};
#ifdef D3D12
		clear_val = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1., 0);
#endif

        depth_buffer = create_image(dev, irr::video::D24U8, 1024, 1024, 1, usage_depth_stencil, RESOURCE_USAGE::DEPTH_WRITE, &clear_val);

#ifndef D3D12
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
			}, {VkSubpassDependency()}));
#endif

		fbo[0] = create_frame_buffer(dev, { { back_buffer[0], irr::video::ECOLOR_FORMAT::ECF_A8R8G8B8 } }, { depth_buffer, irr::video::ECOLOR_FORMAT::D24U8 }, 1024, 1024, render_pass);
		fbo[1] = create_frame_buffer(dev, { { back_buffer[1], irr::video::ECOLOR_FORMAT::ECF_A8R8G8B8 } }, { depth_buffer, irr::video::ECOLOR_FORMAT::D24U8 }, 1024, 1024, render_pass);

        irr::io::CReadFile reader("..\\..\\examples\\assets\\xue.b3d");
        loader = new irr::scene::CB3DMeshFileLoader(&reader);
        std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > buffers = loader->AnimatedMesh.getMeshBuffers();
        std::vector<irr::scene::SMeshBufferLightMap> reorg;

        for (auto buf : buffers)
            reorg.push_back(buf.first);

        // Format Weight

        std::vector<std::vector<irr::video::SkinnedVertexData> > weightsList;
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
        }

        size_t total_vertex_cnt = 0;
        total_index_cnt = 0;
        for (const irr::scene::IMeshBuffer<irr::video::S3DVertex2TCoords>& mesh : reorg)
        {
            total_vertex_cnt += mesh.getVertexCount();
            total_index_cnt += mesh.getIndexCount();
        }
        //vao = new FormattedVertexStorage(Context::getInstance()->cmdqueue.Get(), reorg, weightsList);
        size_t bufferSize = total_vertex_cnt * sizeof(irr::video::S3DVertex2TCoords);

        vertex_buffer_attributes = create_buffer(dev, bufferSize);
        index_buffer = create_buffer(dev, total_index_cnt * sizeof(uint16_t));
        uint16_t *indexmap = (uint16_t *)map_buffer(dev, index_buffer);
        irr::video::S3DVertex2TCoords *vertexmap = (irr::video::S3DVertex2TCoords*)map_buffer(dev, vertex_buffer_attributes);

        size_t basevertex = 0, baseindex = 0;

        for (const irr::scene::IMeshBuffer<irr::video::S3DVertex2TCoords>& mesh : reorg)
        {
            memcpy(&vertexmap[basevertex], mesh.getVertices(), sizeof(irr::video::S3DVertex2TCoords) * mesh.getVertexCount());
            memcpy(&indexmap[baseindex], mesh.getIndices(), sizeof(unsigned short) * mesh.getIndexCount());
            meshOffset.push_back(std::make_tuple(mesh.getIndexCount(), basevertex, baseindex));
            basevertex += mesh.getVertexCount();
            baseindex += mesh.getIndexCount();
        }

        unmap_buffer(dev, index_buffer);
        unmap_buffer(dev, vertex_buffer_attributes);
        // TODO: Upload to GPUmem

        vertex_buffers_info.emplace_back(vertex_buffer_attributes, 0, static_cast<uint32_t>(sizeof(irr::video::S3DVertex2TCoords)), static_cast<uint32_t>(bufferSize));

        // Upload to gpudata

        // Texture
        /*command_list_t uploadcmdlist = command_list;
        uint32_t texture_id = 0;
        for (auto Tex : loader->Textures)
        {
            const std::string &fixed = "..\\..\\examples\\assets\\" + Tex.TextureName.substr(0, Tex.TextureName.find_last_of('.')) + ".DDS";
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

            size_t block_height = 1;
            size_t block_width = 1;
            size_t block_size = 4;
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
//                        memcpy(((char*)pointer) + offset_in_texram, ((char*)miplevel.Data) + row * width_in_blocks * block_size, width_in_blocks * block_size);
                        offset_in_texram += rowPitch;
                    }
                }
            }
            unmap_buffer(dev, upload_buffer);

            image_t texture = create_image(dev, DDSPic.getLoadedImage().Format,
                width, height, mipmap_count, usage_sampled | usage_transfer_dst, RESOURCE_USAGE::READ_GENERIC, nullptr);

            Textures.push_back(texture);

            uint32_t miplevel = 0;
            for (const MipLevelData mipmapData : Mips)
            {
                set_pipeline_barrier(dev, command_list, texture, RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::COPY_DEST, miplevel);

//                command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(texture.Get(), miplevel), 0, 0, 0,
//                    &CD3DX12_TEXTURE_COPY_LOCATION(upload_buffer.Get(), { mipmapData.Offset, { getDXGIFormatFromColorFormat(DDSPic.getLoadedImage().Format), (UINT)mipmapData.Width, (UINT)mipmapData.Height, 1, (UINT16)mipmapData.RowPitch } }),
//                    &CD3DX12_BOX());

                set_pipeline_barrier(dev, command_list, texture, RESOURCE_USAGE::COPY_DEST, RESOURCE_USAGE::READ_GENERIC, miplevel);
                miplevel++;
            }
            textureSet[fixed] = 2 + texture_id;
            create_image_view(dev, cbv_srv_descriptors_heap, 2 + texture_id++, texture);
        }
#ifdef D3D12
        sig = skinned_object_root_signature.get(dev);
        objectpso = createSkinnedObjectShader(dev, sig.Get());
#endif*/
        make_command_list_executable(command_list);
        submit_executable_command_list(cmdqueue, command_list);
		wait_for_command_queue_idle(dev, cmdqueue);
    }

public:
    Sample(HINSTANCE hinstance, HWND window)
    {
		std::tie(dev, chain) = create_device_and_swapchain(hinstance, window);
        cmdqueue = create_graphic_command_queue(dev);
        back_buffer = get_image_view_from_swap_chain(dev, chain);
        Init();
    }

    ~Sample()
    {
        wait_for_command_queue_idle(dev, cmdqueue);
    }

    void Draw()
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
        loader->AnimatedMesh.animateMesh(frame, 1.f);
        loader->AnimatedMesh.skinMesh(1.f);

        memcpy(map_buffer(dev, jointbuffer), loader->AnimatedMesh.JointMatrixes.data(), loader->AnimatedMesh.JointMatrixes.size() * 16 * sizeof(float));
        unmap_buffer(dev, jointbuffer);

		uint32_t current_backbuffer = get_next_backbuffer_id(dev, chain);

        set_pipeline_barrier(dev, command_list, back_buffer[current_backbuffer], RESOURCE_USAGE::PRESENT, RESOURCE_USAGE::RENDER_TARGET, 0);

		std::array<float, 4> clearColor = { .25f, .25f, 0.35f, 1.0f };
#ifndef D3D12
		VkRenderPassBeginInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		info.renderPass = render_pass->object;
		info.framebuffer = fbo[current_backbuffer]->fbo.object;
		info.clearValueCount = 1;
		VkClearValue clear_values{};
		memcpy(clear_values.color.float32, clearColor.data(), 4 * sizeof(float));
		info.pClearValues = &clear_values;
		info.renderArea.extent.width = 900;
		info.renderArea.extent.height = 900;
		vkCmdBeginRenderPass(command_list->object, &info, VK_SUBPASS_CONTENTS_INLINE);
#else // !D3D12
		command_list->OMSetRenderTargets(1, &(fbo[current_backbuffer]->rtt_heap->GetCPUDescriptorHandleForHeapStart()), true, &(fbo[current_backbuffer]->dsv_heap->GetCPUDescriptorHandleForHeapStart()));

		command_list->SetGraphicsRootSignature(sig.Get());

		std::array<ID3D12DescriptorHeap*, 2> descriptors = { cbv_srv_descriptors_heap.Get(), sampler_heap.Get() };
		command_list->SetDescriptorHeaps(2, descriptors.data());

		command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		command_list->SetGraphicsRootDescriptorTable(0, cbv_srv_descriptors_heap->GetGPUDescriptorHandleForHeapStart());

		clear_color(dev, command_list, fbo[current_backbuffer], clearColor);
		clear_depth_stencil(dev, command_list, fbo[current_backbuffer], depth_stencil_aspect::depth_only, 1., 0);
#endif
		set_graphic_pipeline(command_list, objectpso);

        set_viewport(command_list, 0., 1024.f, 0., 1024.f, 0., 1.);
        set_scissor(command_list, 0, 1024, 0, 1024);

		bind_index_buffer(command_list, index_buffer, 0, total_index_cnt * sizeof(uint16_t), irr::video::E_INDEX_TYPE::EIT_16BIT);
		bind_vertex_buffers(command_list, 0, vertex_buffers_info);

        std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > buffers = loader->AnimatedMesh.getMeshBuffers();
        for (unsigned i = 0; i < buffers.size(); i++)
        {
#ifdef D3D12
			command_list->SetGraphicsRootDescriptorTable(0,
				CD3DX12_GPU_DESCRIPTOR_HANDLE(cbv_srv_descriptors_heap->GetGPUDescriptorHandleForHeapStart())
				.Offset(textureSet[buffers[i].second.TextureNames[0]], dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
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
        present(dev, cmdqueue, chain, current_backbuffer);
    }
};


int WINAPI WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  Sample app(hInstance, WindowUtil::Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow));

  // this struct holds Windows event messages
  MSG msg = {};

  // Loop from https://msdn.microsoft.com/en-us/library/windows/apps/dn166880.aspx
  while (WM_QUIT != msg.message)
  {
    bool bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);

    if (bGotMsg)
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    app.Draw();
  }

  // return this part of the WM_QUIT message to Windows
  return (int)msg.wParam;
}
