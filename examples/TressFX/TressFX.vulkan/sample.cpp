#include "sample.h"
#include <Api/Vkapi.h>
#include "TFXFileIO.h"

static DirectX::XMVECTOR         g_lightEyePt = DirectX::XMVectorSet(-421.25043f, 306.7890949f, -343.22232f, 1.0f);

sample::sample(HINSTANCE hinstance, HWND hwnd)
{
    auto dev_swapchain_queue = create_device_swapchain_and_graphic_presentable_queue(hinstance, hwnd);
    std::unique_ptr<device_t> dev = std::move(std::get<0>(dev_swapchain_queue));
    tressfx_helper.pvkDevice = *dev;
    tressfx_helper.texture_memory_index = dev->default_memory_index;

    tressfx_helper.lightPosition = g_lightEyePt;

    TressFX_Initialize(tressfx_helper);

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
    std::unique_ptr<command_list_storage_t> command_storage = create_command_storage(*dev);
    std::unique_ptr<command_list_t> upload_command_buffer = create_command_list(*dev, *command_storage);
    start_command_list_recording(*upload_command_buffer, *command_storage);
    std::unique_ptr<buffer_t> upload_buffer = create_buffer(*dev, 1024 * 1024 * 128, irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE, usage_buffer_transfer_src);
    TressFX_CreateProcessedAsset(tressfx_helper, nullptr, nullptr, nullptr, 0, *upload_command_buffer, *upload_buffer, *upload_buffer->baking_memory);

    TressFX_Begin(tressfx_helper);
    TressFX_End(tressfx_helper);
}
