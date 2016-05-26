#include "sample.h"
#include "TFXFileIO.h"
#include <Maths\matrix4.h>
#include <VKAPI\pipeline_layout_helpers.h>

namespace
{

    DirectX::XMVECTOR         g_lightEyePt = DirectX::XMVectorSet(-421.25043f, 306.7890949f, -343.22232f, 1.0f);
    DirectX::XMVECTOR         g_defaultEyePt = DirectX::XMVectorSet(-190.0f, 70.0f, -250.0f, 1.0f);
    DirectX::XMVECTOR         g_defaultLookAt = DirectX::XMVectorSet(0, 40, 0, 1.0f);

    // (inv)modelmatrix, jointmatrix
    constexpr auto blit_descriptor = descriptor_set({
        range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 0, 1),
        range_of_descriptors(RESOURCE_VIEW::SAMPLER, 1, 1)},
        shader_stage::all);


    pipeline_state_t get_blit_pso(device_t &dev, pipeline_layout_t layout, render_pass_t* rp)
    {
        constexpr pipeline_state_description pso_desc = pipeline_state_description::get();
        const blend_state blend = blend_state::get();

        VkPipelineTessellationStateCreateInfo tesselation_info{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
        VkPipelineViewportStateCreateInfo viewport_info{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewport_info.viewportCount = 1;
        viewport_info.scissorCount = 1;
        std::vector<VkDynamicState> dynamic_states{ VK_DYNAMIC_STATE_VIEWPORT , VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamic_state_info{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0, static_cast<uint32_t>(dynamic_states.size()), dynamic_states.data() };


        vulkan_wrapper::shader_module module_vert(dev, "..\\..\\blit_vert.spv");
        vulkan_wrapper::shader_module module_frag(dev, "..\\..\\blit_frag.spv");

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

        return std::make_shared<vulkan_wrapper::pipeline>(
            dev, 0, shader_stages, vertex_input,
            get_pipeline_input_assembly_state_info(pso_desc), tesselation_info,
            viewport_info,
            get_pipeline_rasterization_state_create_info(pso_desc),
            get_pipeline_multisample_state_create_info(pso_desc),
            get_pipeline_depth_stencil_state_create_info(pso_desc), blend,
            dynamic_state_info, layout->object, rp->object, 1,
            VkPipeline(VK_NULL_HANDLE), 0);
    }

}

sample::sample(HINSTANCE hinstance, HWND hwnd)
{
    auto dev_swapchain_queue = create_device_swapchain_and_graphic_presentable_queue(hinstance, hwnd);
    dev = std::move(std::get<0>(dev_swapchain_queue));
    queue = std::move(std::get<2>(dev_swapchain_queue));
    chain = std::move(std::get<1>(dev_swapchain_queue));
    back_buffer = get_image_view_from_swap_chain(*dev, *chain);

    blit_layout_set = get_object_descriptor_set(*dev, blit_descriptor);
    blit_layout = std::make_shared<vulkan_wrapper::pipeline_layout>(
        dev->object, 0, std::vector<VkDescriptorSetLayout>{blit_layout_set->object},
        std::vector<VkPushConstantRange>());
    blit_pso = get_blit_pso(*dev, blit_layout, blit_render_pass.get());

    tressfx_helper.pvkDevice = *dev;
    tressfx_helper.texture_memory_index = dev->default_memory_index;

    irr::core::matrix4 View, InvView, tmp, LightMatrix;
    tmp.buildCameraLookAtMatrixRH(irr::core::vector3df(-190.0f, 70.0f, -250.0f), irr::core::vector3df(0.f, 40.f, 0.f), irr::core::vector3df(0.f, 1.f, 0.f));
    //View.buildProjectionMatrixPerspectiveFovRH(70.f / 180.f * 3.14f, 1.f, 1.f, 1000.f);
    View *= tmp;
    View.getInverse(InvView);

/*    tressfx_helper.g_vEye[0] = 0.f;
    tressfx_helper.g_vEye[1] = 0.f;
    tressfx_helper.g_vEye[2] = 200.f;*/

    irr::core::matrix4 Model;

    LightMatrix.buildProjectionMatrixPerspectiveFovLH(0.6f, 1.f, 532.f, 769.f);

/*    tmp.buildCameraLookAtMatrixRH(irr::core::vector3df(cbuf.g_PointLightPos[0], cbuf.g_PointLightPos[1], cbuf.g_PointLightPos[2]),
        irr::core::vector3df(0.f, 0.f, 0.f), irr::core::vector3df(0.f, 1.f, 0.f));
    LightMatrix *= tmp;*/

    tressfx_helper.lightPosition = g_lightEyePt;
    tressfx_helper.eyePoint = g_defaultEyePt;
    tressfx_helper.mWorld = DirectX::XMMATRIX(Model.pointer());
    tressfx_helper.mViewProj = DirectX::XMMATRIX(View.pointer());
    tressfx_helper.mInvViewProj = DirectX::XMMATRIX(InvView.pointer());
    tressfx_helper.mViewProjLightFromLibrary = DirectX::XMMATRIX(LightMatrix.pointer());
    tressfx_helper.bShortCutOn = false;
    tressfx_helper.hairParams.bAntialias = false;
    tressfx_helper.hairParams.strandCopies = 1;
    tressfx_helper.backBufferHeight = 1024;
    tressfx_helper.backBufferWidth = 1024;
    tressfx_helper.hairParams.density = .1;
    tressfx_helper.hairParams.thickness = 0.3f;

    depth_texture = create_image(*dev, irr::video::D24U8, 1024, 1024, 1, 1, usage_depth_stencil, nullptr);
    depth_texture_view = create_image_view(*dev, *depth_texture, irr::video::D24U8, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D, irr::video::E_ASPECT::EA_DEPTH_STENCIL);
    color_texture = create_image(*dev, irr::video::ECF_R8G8B8A8_UNORM_SRGB, 1024, 1024, 1, 1, usage_render_target | usage_transfer_src, nullptr);
    color_texture_view = create_image_view(*dev, *color_texture, irr::video::ECF_R8G8B8A8_UNORM_SRGB, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D, irr::video::E_ASPECT::EA_COLOR);

    TressFX_Initialize(tressfx_helper, *depth_texture_view, *color_texture_view);

    TFXProjectFile tfxproject;
    bool success = tfxproject.Read(L"..\\..\\..\\TressFX\\amd_tressfx_viewer\\media\\testhair1\\TestHair1.tfxproj");

    AMD::TressFX_HairBlob hairBlob;

    const int numHairSectionsTotal = tfxproject.CountTFXFiles();
    std::ifstream tfxFile;

    for (int i = 0; i < numHairSectionsTotal; ++i)
    {
        tfxFile.clear();
        tfxFile.open(tfxproject.mTFXFile[i].tfxFileName, std::ios::binary | std::ifstream::in);

        if (tfxFile.fail())
        {
            break;
        }

        std::filebuf* pbuf = tfxFile.rdbuf();
        size_t size = pbuf->pubseekoff(0, tfxFile.end, tfxFile.in);
        pbuf->pubseekpos(0, tfxFile.in);
        hairBlob.pHair = (void *) new char[size];
        tfxFile.read((char *)hairBlob.pHair, size);
        tfxFile.close();
        hairBlob.size = (unsigned)size;

        hairBlob.animationSize = 0;
        hairBlob.pAnimation = NULL;

        AMD::TressFX_GuideFollowParams guideFollowParams;
        guideFollowParams.numFollowHairsPerGuideHair = tfxproject.mTFXFile[i].numFollowHairsPerGuideHair;
        guideFollowParams.radiusForFollowHair = tfxproject.mTFXFile[i].radiusForFollowHair;
        guideFollowParams.tipSeparationFactor = tfxproject.mTFXFile[i].tipSeparationFactor;

        TressFX_LoadRawAsset(tressfx_helper, guideFollowParams, &hairBlob);
    }
    command_storage = create_command_storage(*dev);
    std::unique_ptr<command_list_t> upload_command_buffer = create_command_list(*dev, *command_storage);
    start_command_list_recording(*upload_command_buffer, *command_storage);
    std::unique_ptr<buffer_t> upload_buffer = create_buffer(*dev, 1024 * 1024 * 128, irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_buffer_transfer_src);
    TressFX_CreateProcessedAsset(tressfx_helper, nullptr, nullptr, nullptr, 0, *upload_command_buffer, *upload_buffer, *upload_buffer->baking_memory);

    set_pipeline_barrier(*upload_command_buffer, *depth_texture, RESOURCE_USAGE::undefined, RESOURCE_USAGE::DEPTH_WRITE, 0, irr::video::E_ASPECT::EA_DEPTH_STENCIL);
    set_pipeline_barrier(*upload_command_buffer, *color_texture, RESOURCE_USAGE::undefined, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_DEPTH_STENCIL);
    VkClearDepthStencilValue clear_values{};
    clear_values.depth = 1.f;
    vkCmdClearDepthStencilImage(*upload_command_buffer, *depth_texture, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &clear_values, 1, &structures::image_subresource_range(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT));
    set_pipeline_barrier(*upload_command_buffer, *back_buffer[0], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);
    set_pipeline_barrier(*upload_command_buffer, *back_buffer[1], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);

    VkClearColorValue color_clear{};
    float ctmp[4] = { .5, 1., 1., 1. };
    memcpy(color_clear.float32, ctmp, 4 * sizeof(float));
    vkCmdClearColorImage(*upload_command_buffer, *back_buffer[0], VK_IMAGE_LAYOUT_GENERAL, &color_clear, 1, &structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT));
    vkCmdClearColorImage(*upload_command_buffer, *back_buffer[1], VK_IMAGE_LAYOUT_GENERAL, &color_clear, 1, &structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT));
    ctmp[0] = 0.;
    memcpy(color_clear.float32, ctmp, 4 * sizeof(float));
    vkCmdClearColorImage(*upload_command_buffer, *color_texture, VK_IMAGE_LAYOUT_GENERAL, &color_clear, 1, &structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT));

    make_command_list_executable(*upload_command_buffer);
    submit_executable_command_list(*queue, *upload_command_buffer);
    wait_for_command_queue_idle(*dev, *queue);

    constant_buffer = create_buffer(*dev, 1024 * 1024 * 10, irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_buffer_transfer_src);
    draw_command_buffer = create_command_list(*dev, *command_storage);
    start_command_list_recording(*draw_command_buffer, *command_storage);

    set_scissor(*draw_command_buffer, 0, 0, 1024, 1024);
    set_viewport(*draw_command_buffer, 0, 1024., 0., 1024, 0., 1.);

    TressFX_Begin(tressfx_helper, *constant_buffer, *constant_buffer->baking_memory, 0);
    TressFX_Render(tressfx_helper, *draw_command_buffer, *constant_buffer, 0);
    TressFX_End(tressfx_helper);

    make_command_list_executable(*draw_command_buffer);

    for (int i = 0; i < 2; i++)
    {
        blit_command_buffer[i] = create_command_list(*dev, *command_storage);
        start_command_list_recording(*blit_command_buffer[i], *command_storage);
        set_pipeline_barrier(*blit_command_buffer[i], *back_buffer[i], RESOURCE_USAGE::PRESENT, RESOURCE_USAGE::COPY_DEST, 0, irr::video::E_ASPECT::EA_COLOR);
        VkImageBlit Regions{};
        Regions.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Regions.srcSubresource.layerCount = 1;
        Regions.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Regions.dstSubresource.layerCount = 1;
        vkCmdBlitImage(*blit_command_buffer[i], *color_texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *back_buffer[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Regions, VK_FILTER_LINEAR);
        set_pipeline_barrier(*blit_command_buffer[i], *back_buffer[i], RESOURCE_USAGE::COPY_DEST, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);
        make_command_list_executable(*blit_command_buffer[i]);
    }
}

void sample::draw()
{
    submit_executable_command_list(*queue, *draw_command_buffer);
    wait_for_command_queue_idle(*dev, *queue);

    uint32_t current_backbuffer = get_next_backbuffer_id(*dev, *chain);
    submit_executable_command_list(*queue, *blit_command_buffer[current_backbuffer]);
    wait_for_command_queue_idle(*dev, *queue);
    present(*dev, *queue, *chain, current_backbuffer);
}
