// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#ifndef IBL_HPP
#define IBL_HPP

#ifdef D3D12
#include <API/d3dapi.h>
#include <d3dx12.h>
#else
#include <API/vkapi.h>
#endif

#include <Core/IImage.h>

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


struct SH
{
	float bL00;
	float bL1m1;
	float bL10;
	float bL11;
	float bL2m2;
	float bL2m1;
	float bL20;
	float bL21;
	float bL22;

	float gL00;
	float gL1m1;
	float gL10;
	float gL11;
	float gL2m2;
	float gL2m1;
	float gL20;
	float gL21;
	float gL22;

	float rL00;
	float rL1m1;
	float rL10;
	float rL11;
	float rL2m2;
	float rL2m1;
	float rL20;
	float rL21;
	float rL22;
};

/** Generate the 9 first SH coefficients for each color channel
using the cubemap provided by CubemapFace.
*  \param textures sequence of 6 square textures.
*  \param row/columns count of textures.
*/
SHCoefficients computeSphericalHarmonics(image_t *probe, size_t edge_size);

/*WrapperResource *generateSpecularCubemap(WrapperResource *probe);
IImage getDFGLUT(size_t DFG_LUT_size = 128);*/
#endif