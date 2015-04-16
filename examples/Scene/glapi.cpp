// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <glapi.h>
#include <D3DAPI/Texture.h>
#include <D3DAPI/Resource.h>

static GLenum getOpenGLFormatAndParametersFromColorFormat(irr::video::ECOLOR_FORMAT format,
  //  GLint& filtering,
  GLenum& colorformat,
  GLenum& type)
{
  // default
//  filtering = GL_LINEAR;
  colorformat = GL_RGBA;
  type = GL_UNSIGNED_BYTE;
  GLenum internalformat = GL_RGBA;

  switch (format)
  {
  case irr::video::ECF_A1R5G5B5:
    colorformat = GL_BGRA_EXT;
    type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
    internalformat = GL_RGBA;
    break;
  case irr::video::ECF_R5G6B5:
    colorformat = GL_RGB;
    type = GL_UNSIGNED_SHORT_5_6_5;
    internalformat = GL_RGB;
    break;
  case irr::video::ECF_R8G8B8:
    colorformat = GL_BGR;
    type = GL_UNSIGNED_BYTE;
    internalformat = GL_RGB;
    break;
  case irr::video::ECF_A8R8G8B8:
    colorformat = GL_BGRA_EXT;
    type = GL_UNSIGNED_INT_8_8_8_8_REV;
    internalformat = GL_RGBA;
    break;
    /*  case irr::video::ECF_DXT1:
        colorformat = GL_BGRA_EXT;
        type = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        internalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        break;
      case irr::video::ECF_DXT2:
      case irr::video::ECF_DXT3:
        colorformat = GL_BGRA_EXT;
        type = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        break;
      case irr::video::ECF_DXT4:
      case irr::video::ECF_DXT5:
        colorformat = GL_BGRA_EXT;
        type = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        internalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;*/
/*  case irr::video::ECF_D16:
    colorformat = GL_DEPTH_COMPONENT;
    type = GL_UNSIGNED_BYTE;
    internalformat = GL_DEPTH_COMPONENT16;
    break;
  case irr::video::ECF_D32:
    colorformat = GL_DEPTH_COMPONENT;
    type = GL_UNSIGNED_BYTE;
    internalformat = GL_DEPTH_COMPONENT32;
    break;
  case irr::video::ECF_D24S8:
    colorformat = GL_DEPTH_STENCIL_EXT;
    type = GL_UNSIGNED_INT_24_8_EXT;
    internalformat = GL_DEPTH_STENCIL_EXT;
    break;*/
  case irr::video::ECF_R8:
    colorformat = GL_RED;
    type = GL_UNSIGNED_BYTE;
    internalformat = GL_R8;
    break;
  case irr::video::ECF_R8G8:
    colorformat = GL_RG;
    type = GL_UNSIGNED_BYTE;
    internalformat = GL_RG8;
    break;
  case irr::video::ECF_R16:
    colorformat = GL_RED;
    type = GL_UNSIGNED_SHORT;
    internalformat = GL_R16;
    break;
  case irr::video::ECF_R16G16:
    colorformat = GL_RG;
    type = GL_UNSIGNED_SHORT;
    internalformat = GL_RG16;
    break;
  case irr::video::ECF_R16F:
    //      filtering = GL_NEAREST;
    colorformat = GL_RED;
    internalformat = GL_R16F;
    type = GL_FLOAT;
    break;
  case irr::video::ECF_G16R16F:
    //      filtering = GL_NEAREST;
    colorformat = GL_RG;
    type = GL_FLOAT;
    break;
    /*  case irr::video::ECF_A16B16G16R16F:
          filtering = GL_NEAREST;
          colorformat = GL_RGBA;
          internalformat = GL_RGBA16F_ARB;
            type = GL_FLOAT;
        break;*/
  case irr::video::ECF_R32F:
    //      filtering = GL_NEAREST;
    colorformat = GL_RED;
    internalformat = GL_R32F;
    type = GL_FLOAT;
    break;
  case irr::video::ECF_G32R32F:
    //      filtering = GL_NEAREST;
    colorformat = GL_RG;
    internalformat = GL_RG32F;
    type = GL_FLOAT;
    break;
  case irr::video::ECF_A32B32G32R32F:
    //      filtering = GL_NEAREST;
    colorformat = GL_RGBA;
    internalformat = GL_RGBA32F_ARB;
    type = GL_FLOAT;
    break;
  default:
    break;
  }
  return internalformat;
}


WrapperResource* GLAPI::createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4])
{
  WrapperResource* result = (WrapperResource *)malloc(sizeof(WrapperResource));
  glGenTextures(1, &result->GLValue);
  glBindTexture(GL_TEXTURE_2D, result->GLValue);
  //    if (CVS->isARBTextureStorageUsable())
  //      glTexStorage2D(GL_TEXTURE_2D, mipmaplevel, internalFormat, res.Width, res.Height);
  //    else
  GLenum type, colorFormat;
  GLenum internalFormat = getOpenGLFormatAndParametersFromColorFormat(Format, colorFormat, type);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, (GLsizei)Width, (GLsizei)Height, 0, colorFormat, type, 0);
  return result;
}

union WrapperRTTSet* GLAPI::createRTTSet(const std::vector<WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height)
{
  WrapperRTTSet *result = (WrapperRTTSet*)malloc(sizeof(WrapperRTTSet));
  std::vector<GLuint> unwrappedRTTs;
  for (WrapperResource *RTT : RTTs)
  {
    unwrappedRTTs.push_back(RTT->GLValue);
  }
  new(result) GLRTTSet(unwrappedRTTs, Width, Height);
  return result;
}

void GLAPI::writeResourcesTransitionBarrier(union WrapperCommandList* wrappedCmdList, const std::vector<std::tuple<WrapperResource *, enum RESOURCE_USAGE, enum RESOURCE_USAGE> > &barriers)
{
}

void GLAPI::clearRTTSet(union WrapperCommandList* wrappedCmdList, union WrapperRTTSet* RTTSet, float color[4])
{
  //  RTTSet->GLValue.Clear(wrappedCmdList->D3DValue.CommandList, color);
}

void GLAPI::setRTTSet(union WrapperCommandList* wrappedCmdList, union WrapperRTTSet*RTTSet)
{
  RTTSet->GLValue.Bind();
}

void GLAPI::setIndexVertexBuffersSet(union WrapperCommandList* wrappedCmdList, WrapperIndexVertexBuffersSet* wrappedVAO)
{
  glBindVertexArray(wrappedVAO->GLValue);
}

WrapperResource *GLAPI::createConstantsBuffer(size_t)
{
  return nullptr;
}

WrapperCommandList* GLAPI::createCommandList()
{
  return nullptr;
}

void GLAPI::closeCommandList(union WrapperCommandList *wrappedCmdList)
{}

void GLAPI::drawIndexedInstanced(union WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t indexOffset, size_t vertexOffset, size_t instanceOffset)
{
  glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_SHORT, (void *)(indexOffset * sizeof(unsigned short)), (GLsizei)vertexOffset);
}