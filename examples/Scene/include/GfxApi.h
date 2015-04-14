// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#ifndef __GFX_API_H__
#define __GFX_API_H__

#include <Core/SColor.h>
#include <memory>
#include <vector>

class WrapperRTT
{
public:
  virtual void nothing() = 0;
};

class WrapperRTTSet
{
public:
  virtual void nothing() = 0;
};

class GFXAPI
{
public:
  virtual std::shared_ptr<WrapperRTT> createRTT(irr::video::ECOLOR_FORMAT, size_t Width, size_t Height, float fastColor[4]) = 0;
  virtual std::shared_ptr<WrapperRTTSet> createRTTSet(std::vector<WrapperRTT*> RTTs, size_t Width, size_t Height) = 0;
};

//Global
extern GFXAPI* GlobalGFXAPI;

#endif