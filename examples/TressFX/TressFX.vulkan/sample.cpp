#include "sample.h"
#include "TFXFileIO.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <VKAPI\pipeline_layout_helpers.h>

const auto& blit_vert = std::vector<uint32_t>
#include <generatedShaders\blit_vert.h>
;

const auto& blit_frag = std::vector<uint32_t>
#include <generatedShaders\blit_frag.h>
;

namespace
{
    DirectX::XMVECTOR         g_lightEyePt = DirectX::XMVectorSet(-421.25043f, 306.7890949f, -343.22232f, 1.0f);
    DirectX::XMVECTOR         g_defaultEyePt = DirectX::XMVectorSet(-190.0f, 70.0f, -250.0f, 1.0f);
    DirectX::XMVECTOR         g_defaultLookAt = DirectX::XMVectorSet(0, 40, 0, 1.0f);

    // (inv)modelmatrix, jointmatrix
    const auto blit_descriptor = descriptor_set({
        range_of_descriptors(RESOURCE_VIEW::SHADER_RESOURCE, 0, 1),
        range_of_descriptors(RESOURCE_VIEW::SAMPLER, 1, 1)},
        shader_stage::fragment_shader);

    std::unique_ptr<render_pass_t> get_render_pass(device_t &dev)
    {
        return dev.create_blit_pass();
    }

    auto get_blit_pso(device_t &dev, pipeline_layout_t& layout, render_pass_t& rp)
    {
		graphic_pipeline_state_description pso_desc = graphic_pipeline_state_description::get()
			.set_vertex_shader(blit_vert)
			.set_fragment_shader(blit_frag)
			.set_vertex_attributes(std::vector<pipeline_vertex_attributes>{
				pipeline_vertex_attributes{ 0, irr::video::ECF_R32G32F, 0, 4 * sizeof(float), 0 },
				pipeline_vertex_attributes{ 1, irr::video::ECF_R32G32B32F, 0, 4 * sizeof(float), 2 * sizeof(float) }
			})
			.set_color_outputs(std::vector<color_output>{ {false} });

		return dev.create_graphic_pso(pso_desc, rp, layout, 0);
    }

}

sample::sample(GLFWwindow *window)
{
    auto dev_swapchain_queue = create_device_swapchain_and_graphic_presentable_queue(window);
    dev = std::move(std::get<0>(dev_swapchain_queue));
    queue = std::move(std::get<2>(dev_swapchain_queue));
    chain = std::move(std::get<1>(dev_swapchain_queue));
    back_buffer = chain->get_image_view_from_swap_chain();

    blit_render_pass = get_render_pass(*dev);

    blit_layout_set = dev->get_object_descriptor_set(blit_descriptor);
    blit_layout = dev->create_pipeline_layout(std::vector<const descriptor_set_layout*>{blit_layout_set.get()});
    blit_pso = get_blit_pso(*dev, *blit_layout, *blit_render_pass);

    big_triangle = dev->create_buffer(4 * 3 * sizeof(float), irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_vertex);
    float fullscreen_tri[]
    {
        -1., -3., 0., 2.,
        3., 1., 2., 0.,
        -1., 1., 0., 0.
    };

    memcpy(big_triangle->map_buffer(), fullscreen_tri, 4 * 3 * sizeof(float));
	big_triangle->unmap_buffer();

    descriptor_pool =
        dev->create_descriptor_storage(1, {{RESOURCE_VIEW::SHADER_RESOURCE, 1},
                                            {RESOURCE_VIEW::SAMPLER, 1}});
    descriptor = descriptor_pool->allocate_descriptor_set_from_cbv_srv_uav_heap(0, { blit_layout_set.get() }, 2);

	std::transform(back_buffer.begin(), back_buffer.end(), std::back_inserter(back_buffer_view),
		[&](const auto& img) {return dev->create_image_view(*img, std::get<5>(dev_swapchain_queue), 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D); });

    fbo[0] = dev->create_frame_buffer(std::vector<const image_view_t*>{ back_buffer_view[0].get() }, 900, 900, blit_render_pass.get());
    fbo[1] = dev->create_frame_buffer(std::vector<const image_view_t*>{ back_buffer_view[1].get() }, 900, 900, blit_render_pass.get());

    tressfx_helper.pvkDevice = dynamic_cast<vk_device_t&>(*dev).object;
    tressfx_helper.memoryProperties = dynamic_cast<vk_device_t&>(*dev).mem_properties;

    DirectX::XMFLOAT4 lightpos{ 121.386368f, 420.605896f, -426.585876f, 1.00000000f };


	const auto& View =
		glm::transpose(
			glm::perspectiveFovRH(70.f / 180.f * 3.14f, 900.f, 900.f, 100.f, 600.f) *
			glm::lookAtRH(glm::vec3(-190.0f, 70.0f, -250.0f), glm::vec3(0.f, 40.f, 0.f), glm::vec3(0.f, -1.f, 0.f))
		);
	const auto& InvView = glm::inverse(View);

/*    tressfx_helper.g_vEye[0] = 0.f;
    tressfx_helper.g_vEye[1] = 0.f;
    tressfx_helper.g_vEye[2] = 200.f;*/

    tressfx_helper.lightPosition = g_lightEyePt;
    tressfx_helper.eyePoint = g_defaultEyePt;
    tressfx_helper.mViewProj = DirectX::XMMATRIX(glm::value_ptr(View));
    tressfx_helper.mInvViewProj = DirectX::XMMATRIX(glm::value_ptr(InvView));
    tressfx_helper.bShortCutOn = true;
    tressfx_helper.hairParams.bAntialias = true;
    tressfx_helper.hairParams.strandCopies = 1;
    tressfx_helper.backBufferHeight = 1024;
    tressfx_helper.backBufferWidth = 1024;
    tressfx_helper.hairParams.density = .5;
    tressfx_helper.hairParams.thickness = 0.3f;
    tressfx_helper.hairParams.duplicateStrandSpacing = 0.300000012f;
    tressfx_helper.maxConstantBuffers = 1;


    tressfx_helper.hairParams.color = DirectX::XMFLOAT3(98.f / 255.f, 14.f / 255.f, 4.f / 255.f);

    tressfx_helper.hairParams.Ka = 0.f;
    tressfx_helper.hairParams.Kd = 0.4f;
    tressfx_helper.hairParams.Ks1 = 0.04f;
    tressfx_helper.hairParams.Ex1 = 80.f;
    tressfx_helper.hairParams.Ks2 = .5f;
    tressfx_helper.hairParams.Ex2 = 8.0f;

    tressfx_helper.hairParams.alpha = .5f;
    tressfx_helper.hairParams.alphaThreshold = .00388f;
    tressfx_helper.hairParams.shadowMapAlpha = .004000f;

    tressfx_helper.hairParams.ambientLightColor = DirectX::XMFLOAT4(.15f, .15f, .15f, 1.f);

/*    cbuf.g_PointLightPos[0] = 421.25f;
    cbuf.g_PointLightPos[1] = 306.79f;
    cbuf.g_PointLightPos[2] = 343.f;
    cbuf.g_PointLightPos[3] = 0.f;*/

    tressfx_helper.hairParams.pointLightColor = DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	tressfx_helper.depthStencilFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
	tressfx_helper.colorFormat = VK_FORMAT_R8G8B8A8_SRGB;


    depth_texture = dev->create_image(irr::video::D32U8, 1024, 1024, 1, 1, usage_depth_stencil | usage_sampled | usage_transfer_dst, nullptr);
    depth_texture_view = dev->create_image_view(*depth_texture, irr::video::D32U8, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D, irr::video::E_ASPECT::EA_DEPTH);
    color_texture = dev->create_image( irr::video::ECF_R8G8B8A8_UNORM_SRGB, 1024, 1024, 1, 1, usage_render_target | usage_sampled | usage_transfer_dst, nullptr);
    color_texture_view = dev->create_image_view(*color_texture, irr::video::ECF_R8G8B8A8_UNORM_SRGB, 0, 1, 0, 1, irr::video::E_TEXTURE_TYPE::ETT_2D, irr::video::E_ASPECT::EA_COLOR);

    dev->set_image_view(*descriptor, 0, 0, *color_texture_view);
    bilinear_sampler = dev->create_sampler(SAMPLER_TYPE::BILINEAR_CLAMPED);
    dev->set_sampler(*descriptor, 1, 1, *bilinear_sampler);

    command_storage = dev->create_command_storage();
    std::unique_ptr<command_list_t> upload_command_buffer = command_storage->create_command_list();
	upload_command_buffer->start_command_list_recording(*command_storage);
    std::unique_ptr<buffer_t> upload_buffer = dev->create_buffer(1024 * 1024 * 128, irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_buffer_transfer_src);

    size_t offset = 0;
    TressFX_Initialize(tressfx_helper,
		dynamic_cast<vk_image_view_t&>(*depth_texture_view).object,
		dynamic_cast<vk_image_view_t&>(*color_texture_view).object,
		dynamic_cast<vk_command_list_t&>(*upload_command_buffer).object,
		dynamic_cast<vk_buffer_t&>(*upload_buffer).memory,
		dynamic_cast<vk_buffer_t&>(*upload_buffer).object, offset);
	upload_command_buffer->make_command_list_executable();
	queue->submit_executable_command_list(*upload_command_buffer, nullptr);
	queue->wait_for_command_queue_idle();
	upload_command_buffer->start_command_list_recording(*command_storage);

    TFXProjectFile tfxproject;
    bool success = tfxproject.Read(L"..\\..\\..\\deps\\TressFX\\amd_tressfx_viewer\\media\\ponytail\\ponytail.tfxproj");
	if (!success)
		throw;

    AMD::TressFX_HairBlob hairBlob;

    const int numHairSectionsTotal = tfxproject.CountTFXFiles();
    std::ifstream tfxFile;
    int numHairSectionsCounter = 0;
    tressfx_helper.simulationParams.bGuideFollowSimulation = false;
    tressfx_helper.simulationParams.gravityMagnitude = 9.82f;
    tressfx_helper.simulationParams.numLengthConstraintIterations = tfxproject.lengthConstraintIterations;
    tressfx_helper.simulationParams.numLocalShapeMatchingIterations = tfxproject.localShapeMatchingIterations;
    tressfx_helper.simulationParams.windMag = tfxproject.windMag;
    tressfx_helper.simulationParams.windDir = DirectX::XMFLOAT3(1., 0., 0.);
    tressfx_helper.targetFrameRate = 1.f / 60.f;

	const auto& Model = glm::rotate(3.14f, glm::vec3(0.f, 1.f, 0.f));

    tressfx_helper.modelTransformForHead = DirectX::XMMATRIX(glm::value_ptr(Model));

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

        AMD::TressFX_ShapeParams shapeParam;
        shapeParam.damping = tfxproject.mTFXSimFile[i].damping;
        shapeParam.globalShapeMatchingEffectiveRange = tfxproject.mTFXSimFile[i].globalShapeMatchingEffectiveRange;
        shapeParam.stiffnessForGlobalShapeMatching = tfxproject.mTFXSimFile[i].stiffnessForGlobalShapeMatching;
        shapeParam.stiffnessForLocalShapeMatching = tfxproject.mTFXSimFile[i].stiffnessForLocalShapeMatching;
        tressfx_helper.simulationParams.perSectionShapeParams[numHairSectionsCounter++] = shapeParam;
    }

    tressfx_helper.simulationParams.numHairSections = numHairSectionsCounter;

    TressFX_CreateProcessedAsset(tressfx_helper, nullptr, nullptr, nullptr,
		dynamic_cast<vk_command_list_t&>(*upload_command_buffer).object,
		dynamic_cast<vk_buffer_t&>(*upload_buffer).object,
		dynamic_cast<vk_buffer_t&>(*upload_buffer).memory);

	upload_command_buffer->set_pipeline_barrier(*depth_texture, RESOURCE_USAGE::undefined, RESOURCE_USAGE::DEPTH_WRITE, 0, irr::video::E_ASPECT::EA_DEPTH_STENCIL);
	upload_command_buffer->set_pipeline_barrier(*color_texture, RESOURCE_USAGE::undefined, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);


	upload_command_buffer->set_pipeline_barrier(*back_buffer[0], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);
	upload_command_buffer->set_pipeline_barrier(*back_buffer[1], RESOURCE_USAGE::undefined, RESOURCE_USAGE::PRESENT, 0, irr::video::E_ASPECT::EA_COLOR);

	upload_command_buffer->make_command_list_executable();
	queue->submit_executable_command_list(*upload_command_buffer, nullptr);
	queue->wait_for_command_queue_idle();

    draw_command_buffer = command_storage->create_command_list();
	draw_command_buffer->start_command_list_recording(*command_storage);

	draw_command_buffer->set_pipeline_barrier(*depth_texture, RESOURCE_USAGE::DEPTH_WRITE, RESOURCE_USAGE::COPY_DEST, 0, irr::video::E_ASPECT::EA_DEPTH_STENCIL);
	draw_command_buffer->set_pipeline_barrier(*color_texture, RESOURCE_USAGE::READ_GENERIC, RESOURCE_USAGE::COPY_DEST, 0, irr::video::E_ASPECT::EA_COLOR);
	draw_command_buffer->clear_depth_stencil(*depth_texture, 1.f);
	draw_command_buffer->clear_color(*color_texture, std::array<float, 4>{ .5f, .5f, .5f, 1.f });

	draw_command_buffer->set_pipeline_barrier(*depth_texture, RESOURCE_USAGE::COPY_DEST, RESOURCE_USAGE::DEPTH_WRITE, 0, irr::video::E_ASPECT::EA_DEPTH_STENCIL);
	draw_command_buffer->set_pipeline_barrier(*color_texture, RESOURCE_USAGE::COPY_DEST, RESOURCE_USAGE::RENDER_TARGET, 0, irr::video::E_ASPECT::EA_COLOR);
    TressFX_Begin(tressfx_helper, 0);
    TressFX_Simulate(tressfx_helper, dynamic_cast<vk_command_list_t&>(*draw_command_buffer).object, 0.16f, 0);
    TressFX_RenderShadowMap(tressfx_helper, dynamic_cast<vk_command_list_t&>(*draw_command_buffer).object, 0);
	draw_command_buffer->set_scissor(0, 1024, 0, 1024);
	draw_command_buffer->set_viewport(0, 1024., 0., 1024, 0., 1.);
    TressFX_Render(tressfx_helper, dynamic_cast<vk_command_list_t&>(*draw_command_buffer).object, 0);
    TressFX_End(tressfx_helper);
	draw_command_buffer->set_pipeline_barrier(*color_texture, RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::READ_GENERIC, 0, irr::video::E_ASPECT::EA_COLOR);
	draw_command_buffer->make_command_list_executable();

    for (int i = 0; i < 2; i++)
    {
        blit_command_buffer[i] = command_storage->create_command_list();
		blit_command_buffer[i]->start_command_list_recording(*command_storage);
		blit_command_buffer[i]->begin_renderpass(*blit_render_pass, *fbo[i], {}, 900, 900);

		blit_command_buffer[i]->set_scissor(0, 900, 0, 900);
		blit_command_buffer[i]->set_viewport(0, 900, 0., 900, 0., 1.);

		blit_command_buffer[i]->set_graphic_pipeline(*blit_pso);
		blit_command_buffer[i]->bind_graphic_descriptor(0, *descriptor, *blit_layout);
		blit_command_buffer[i]->bind_vertex_buffers(0, { { *big_triangle, 0ull, 4u * static_cast<uint32_t>(sizeof(float)), 4u * 3u * static_cast<uint32_t>(sizeof(float)) } });
		blit_command_buffer[i]->draw_non_indexed(3, 1, 0, 0);

		blit_command_buffer[i]->end_renderpass();
		blit_command_buffer[i]->make_command_list_executable();
    }

	present_semaphore = dev->create_semaphore();
}

sample::~sample()
{
    TressFX_Release(tressfx_helper);
}

void sample::draw()
{
	queue->submit_executable_command_list(*draw_command_buffer, nullptr);

    uint32_t current_backbuffer = chain->get_next_backbuffer_id(*present_semaphore);
	queue->submit_executable_command_list(*blit_command_buffer[current_backbuffer], present_semaphore.get());
	queue->wait_for_command_queue_idle();
	chain->present(*queue, current_backbuffer);
}
