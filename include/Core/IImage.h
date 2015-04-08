// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __IIMAGE_H__
#define __IIMAGE_H__

#include <Core/SColor.h>
#include <stdlib.h>
#include <cassert>
#include <vector>

struct PackedMipMapLevel
{
    size_t Width;
    size_t Height;
    void* Data;
    size_t DataSize;
};

struct IImage {
  irr::video::ECOLOR_FORMAT Format;
  std::vector<PackedMipMapLevel> MipMapData;
};

#endif