// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <GL/glew.h>
#include <Core/IImage.h>

inline void getInternalFormatFromColorFormat(irr::video::ECOLOR_FORMAT fmt, GLenum &internalFormat, GLenum &format, GLenum &type)
{
  switch (fmt)
  {
  case irr::video::ECF_R8G8B8A8_UNORM:
    internalFormat = GL_RGBA8;
    format = GL_RGBA;
    type = GL_UNSIGNED_BYTE;
    return;
  case irr::video::ECF_R16G16B16A16F:
    internalFormat = GL_RGBA16F;
    format = GL_RGBA;
    type = GL_HALF_FLOAT;
    return;
  case irr::video::ECF_R32G32B32A32F:
    internalFormat = GL_RGBA32F;
    format = GL_RGBA;
    type = GL_FLOAT;
    return;
  case irr::video::ECF_BC1_UNORM:
    internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    return;
  case irr::video::ECF_BC1_UNORM_SRGB:
    internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
    return;
  case irr::video::ECF_BC2_UNORM:
    internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    return;
  case irr::video::ECF_BC2_UNORM_SRGB:
    internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
    return;
  case irr::video::ECF_BC3_UNORM:
    internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    return;
  case irr::video::ECF_BC3_UNORM_SRGB:
    internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
    return;
  case irr::video::ECF_BC4_UNORM:
    internalFormat = GL_COMPRESSED_RED_RGTC1;
    return;
  case irr::video::ECF_BC4_SNORM:
    internalFormat = GL_COMPRESSED_SIGNED_RED_RGTC1;
    return;
  case irr::video::ECF_BC5_UNORM:
    internalFormat = GL_COMPRESSED_RG_RGTC2;
    return;
  case irr::video::ECF_BC5_SNORM:
    internalFormat = GL_COMPRESSED_SIGNED_RG_RGTC2;
    return;
  default:
    abort();
    return;
  }
}

class GLTexture
{
private:
  size_t Width, Height;
public:
  GLuint Id;
  GLTexture() {}

  GLTexture(const IImage& image)
  {
    Width = image.Layers[0][0].Width;
    Height = image.Layers[0][0].Height;
    glGenTextures(1, &Id);
    GLenum internalFormat, format, type;
    getInternalFormatFromColorFormat(image.Format, internalFormat, format, type);
    switch (image.Type)
    {
    case TextureType::TEXTURE2D:
      glBindTexture(GL_TEXTURE_2D, Id);

      if (!irr::video::isCompressed(image.Format))
      {
        for (unsigned i = 0; i < image.Layers[0].size(); i++)
        {
          struct PackedMipMapLevel miplevel = image.Layers[0][i];
          glTexImage2D(GL_TEXTURE_2D, i, internalFormat, (GLsizei)miplevel.Width, (GLsizei)miplevel.Height, 0, format, type, miplevel.Data);
        }
      }
      else
      {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        for (unsigned i = 0; i < image.Layers[0].size(); i++)
        {
          struct PackedMipMapLevel miplevel = image.Layers[0][i];
          glCompressedTexImage2D(GL_TEXTURE_2D, i, internalFormat, (GLsizei)miplevel.Width, (GLsizei)miplevel.Height, 0, (GLsizei)miplevel.DataSize, miplevel.Data);
        }
      }
      break;
    case TextureType::CUBEMAP:
      glBindTexture(GL_TEXTURE_CUBE_MAP, Id);
      for (unsigned face = 0; face < 6; face++)
      {
        if (!irr::video::isCompressed(image.Format))
        {
          for (unsigned i = 0; i < image.Layers[face].size(); i++)
          {
            struct PackedMipMapLevel miplevel = image.Layers[face][i];
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, i, internalFormat, (GLsizei)miplevel.Width, (GLsizei)miplevel.Height, 0, format, type, miplevel.Data);
          }
        }
        else
        {
          glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
          for (unsigned i = 0; i < image.Layers[face].size(); i++)
          {
            struct PackedMipMapLevel miplevel = image.Layers[face][i];
            glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, i, internalFormat, (GLsizei)miplevel.Width, (GLsizei)miplevel.Height, 0, (GLsizei)miplevel.DataSize, miplevel.Data);
          }
        }
      }
      break;
    }
    }

    ~GLTexture()
    {
      if (Id)
        glDeleteTextures(1, &Id);
    }
  };

#endif