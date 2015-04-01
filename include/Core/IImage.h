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

  irr::video::ECOLOR_FORMAT Format;
  void *ptr;
  bool isSrgb;
public:
  IImage(irr::video::ECOLOR_FORMAT fmt, size_t w, size_t h, bool issrgb) : Width(w), Height(h), Format(fmt), isSrgb(issrgb) {
    // Alloc enough size for RGBA8
    ptr = malloc(w * h * 4 * sizeof(char));
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
    switch (Format)
    {
    case irr::video::ECF_A8R8G8B8:
      return 4;
    case irr::video::ECF_R8G8B8:
      return 3;
    default:
      assert(0 && "unimplemented");
      return -1;
    }
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