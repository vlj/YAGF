// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#ifndef IBL_HPP
#define IBL_HPP

struct Color
{
  float Red;
  float Green;
  float Blue;
};

/** Generate the 9 first SH coefficients for each color channel
using the cubemap provided by CubemapFace.
*  \param textures sequence of 6 square textures.
*  \param row/columns count of textures.
*/
void SphericalHarmonics(Color *CubemapFace[6], size_t edge_size, float *blueSHCoeff, float *greenSHCoeff, float *redSHCoeff);

size_t generateSpecularCubemap(size_t probe);
size_t generateSpecularDFGLUT();
#endif