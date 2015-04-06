// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __IIMAGE_H__
#define __IIMAGE_H__

#include <Core/SColor.h>
#include <stdlib.h>
#include <cassert>

class IImage {
protected:
  size_t Width;
  size_t Height;
  size_t Pitch;

  irr::video::ECOLOR_FORMAT Format;
  void *ptr;
  bool isSrgb;
public:
    size_t ptrsz;
  IImage(irr::video::ECOLOR_FORMAT fmt, size_t w, size_t h, size_t p, bool issrgb) : Width(w), Height(h), Format(fmt), Pitch(p), isSrgb(issrgb) {
    // Alloc enough size for RGBA8
    ptr = malloc(p * h * 4 * sizeof(char));
  }

  void *getPointer()
  {
    return ptr;
  }

  const void* getPointer() const
  {
    return ptr;
  }

  size_t getPitch() const
  {
    return Pitch;
  }

  size_t getWidth() const
  {
    return Width;
  }

  size_t getHeight() const
  {
    return Height;
  }

  ~IImage()
  {
    free(ptr);
  }
};

#endif