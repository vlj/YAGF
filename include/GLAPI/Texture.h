// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <GL/glew.h>
#include <Core/IImage.h>

inline GLenum getInternalFormatFromColorFormat(irr::video::ECOLOR_FORMAT fmt)
{
  switch (fmt)
  {
  case irr::video::ECF_BC1_UNORM:
    return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
  case irr::video::ECF_BC1_UNORM_SRGB:
    return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
  case irr::video::ECF_BC2_UNORM:
    return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
  case irr::video::ECF_BC2_UNORM_SRGB:
    return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
  case irr::video::ECF_BC3_UNORM:
    return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
  case irr::video::ECF_BC3_UNORM_SRGB:
    return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
  case irr::video::ECF_BC4_UNORM:
    return GL_COMPRESSED_RED_RGTC1;
  case irr::video::ECF_BC4_SNORM:
    return GL_COMPRESSED_SIGNED_RED_RGTC1;
  case irr::video::ECF_BC5_UNORM:
    return GL_COMPRESSED_RG_RGTC2;
  case irr::video::ECF_BC5_SNORM:
    return GL_COMPRESSED_SIGNED_RG_RGTC2;
  default:
    return -1;
  }
}

class Texture
{
private:
    size_t Width, Height;
public:
    GLuint Id;
    Texture() {}

    Texture(const IImage& image)
    {
      if (image.Type == TextureType::TEXTURE2D)
      {
        Width = image.Layers[0][0].Width;
        Height = image.Layers[0][0].Height;
        glGenTextures(1, &Id);
        glBindTexture(GL_TEXTURE_2D, Id);

        if (!irr::video::isCompressed(image.Format))
        {
          for (unsigned i = 0; i < image.Layers[0].size(); i++)
          {
            struct PackedMipMapLevel miplevel = image.Layers[0][i];
            glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA8, (GLsizei)miplevel.Width, (GLsizei)miplevel.Height, 0, GL_BGRA, GL_UNSIGNED_BYTE, miplevel.Data);
          }
        }
        else
        {
          glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
          for (unsigned i = 0; i < image.Layers[0].size(); i++)
          {
            struct PackedMipMapLevel miplevel = image.Layers[0][i];
            glCompressedTexImage2D(GL_TEXTURE_2D, i, getInternalFormatFromColorFormat(image.Format), (GLsizei)miplevel.Width, (GLsizei)miplevel.Height, 0, (GLsizei)miplevel.DataSize, miplevel.Data);
          }
        }
      }
    }

    ~Texture()
    {
      if (Id)
        glDeleteTextures(1, &Id);
    }
};

#endif