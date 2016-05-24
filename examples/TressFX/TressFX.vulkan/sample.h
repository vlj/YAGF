#pragma once
#define VULKAN
#include <Api/Vkapi.h>
#include <AMD_TressFX.h>

struct sample
{
    AMD::TressFX_Desc tressfx_helper;
    sample(HINSTANCE hinstance, HWND hwnd);
};

