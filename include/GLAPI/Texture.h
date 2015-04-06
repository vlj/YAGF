// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <GL/glew.h>
#include <Core/IImage.h>

class Texture
{
private:
    size_t Width, Height;
public:
    GLuint Id;

    Texture(const IImage& image) : Width(image.getWidth()), Height(image.getHeight())
    {
        glGenTextures(1, &Id);
        glBindTexture(GL_TEXTURE_2D, Id);

        if (!irr::video::isCompressed(image.getFormat()))
        {
            for (unsigned i = 0; i < image.Mips.size(); i++)
            {
                IImage::MipMapLevel miplevel = image.Mips[i];
                glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA8, (GLsizei)miplevel.Width, (GLsizei)miplevel.Height, 0, GL_BGRA, GL_UNSIGNED_BYTE, image.getPointer());
            }
        }
        else
        {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            for (unsigned i = 0; i < image.Mips.size(); i++)
            {
                IImage::MipMapLevel miplevel = image.Mips[i];
                glCompressedTexImage2D(GL_TEXTURE_2D, i, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, miplevel.Width, miplevel.Height, 0, miplevel.Size, ((char*)image.getPointer()) + miplevel.Offset);
            }
        }
    }

    ~Texture()
    {
        glDeleteTextures(1, &Id);
    }
};

#endif