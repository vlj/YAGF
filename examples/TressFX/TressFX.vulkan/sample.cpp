#include "sample.h"
#include "TFXFileIO.h"

static DirectX::XMVECTOR         g_lightEyePt = DirectX::XMVectorSet(-421.25043f, 306.7890949f, -343.22232f, 1.0f);

sample::sample(HINSTANCE hinstance, HWND hwnd)
{
    auto dev_swapchain_queue = create_device_swapchain_and_graphic_presentable_queue(hinstance, hwnd);
    dev = std::move(std::get<0>(dev_swapchain_queue));
    queue = std::move(std::get<2>(dev_swapchain_queue));
    chain = std::move(std::get<1>(dev_swapchain_queue));
    back_buffer = get_image_view_from_swap_chain(*dev, *chain);
    tressfx_helper.pvkDevice = *dev;
    tressfx_helper.texture_memory_index = dev->default_memory_index;

    tressfx_helper.lightPosition = g_lightEyePt;
    tressfx_helper.bShortCutOn = false;
    tressfx_helper.hairParams.bAntialias = false;
    tressfx_helper.hairParams.strandCopies = 1;
    tressfx_helper.backBufferHeight = 1024;
    tressfx_helper.backBufferWidth = 1024;

    depth_texture = create_image(*dev, irr::video::D24U8, 1024, 1024, 1, 1, usage_depth_stencil | usage_transfer_src, nullptr);
    depth_texture_view = create_image_view(*dev, *depth_texture, irr::video::D24U8, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D, irr::video::E_ASPECT::EA_DEPTH);

    TressFX_Initialize(tressfx_helper, *depth_texture_view);

    TFXProjectFile tfxproject;
    bool tmp = tfxproject.Read(L"..\\..\\..\\TressFX\\amd_tressfx_viewer\\media\\testhair1\\TestHair1.tfxproj");

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
    VkClearDepthStencilValue clear_values{};
    clear_values.depth = 1.f;
    vkCmdClearDepthStencilImage(*upload_command_buffer, *depth_texture, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &clear_values, 1, &structures::image_subresource_range(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT));
    set_pipeline_barrier(*upload_command_buffer, *back_buffer[0], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);
    set_pipeline_barrier(*upload_command_buffer, *back_buffer[1], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);

    VkClearColorValue color_clear{};
    float ctmp[4] = { 1., 1., 1., 1. };
    memcpy(color_clear.float32, ctmp, 4 * sizeof(float));
    vkCmdClearColorImage(*upload_command_buffer, *back_buffer[0], VK_IMAGE_LAYOUT_GENERAL, &color_clear, 1, &structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT));
    vkCmdClearColorImage(*upload_command_buffer, *back_buffer[1], VK_IMAGE_LAYOUT_GENERAL, &color_clear, 1, &structures::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT));

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
        Regions.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        Regions.srcSubresource.layerCount = 1;
        Regions.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Regions.dstSubresource.layerCount = 1;
        vkCmdBlitImage(*blit_command_buffer[i], *depth_texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *back_buffer[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Regions, VK_FILTER_LINEAR);
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
