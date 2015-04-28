// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#ifndef IBL_HPP
#define IBL_HPP

#include <Core/IImage.h>
#include <API/GfxApi.h>

struct Color
{
  float Red;
  float Green;
  float Blue;
};

struct SHCoefficients
{
  float Red[9];
  float Green[9];
  float Blue[9];
};

/** Generate the 9 first SH coefficients for each color channel
using the cubemap provided by CubemapFace.
*  \param textures sequence of 6 square textures.
*  \param row/columns count of textures.
*/
SHCoefficients computeSphericalHarmonics(WrapperResource *probe, size_t edge_size);

WrapperResource *generateSpecularCubemap(WrapperResource *probe);
IImage getDFGLUT(size_t DFG_LUT_size = 128);
#endif