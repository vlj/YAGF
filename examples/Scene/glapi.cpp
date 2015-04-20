// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <glapi.h>
#include <GLAPI/Samplers.h>
#include <GLAPI/Misc.h>

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

union WrapperRTTSet* GLAPI::createRTTSet(const std::vector<WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height, WrapperResource *DepthStencil)
{
  WrapperRTTSet *result = (WrapperRTTSet*)malloc(sizeof(WrapperRTTSet));
  std::vector<GLuint> unwrappedRTTs;
  for (WrapperResource *RTT : RTTs)
  {
    unwrappedRTTs.push_back(RTT->GLValue);
  }
  if (DepthStencil)
    new(result) GLRTTSet(unwrappedRTTs, DepthStencil->GLValue, Width, Height);
  else
    new(result) GLRTTSet(unwrappedRTTs, Width, Height);
  return result;
}

union WrapperResource* GLAPI::createDepthStencilTexture(size_t Width, size_t Height)
{
  WrapperResource* result = (WrapperResource *)malloc(sizeof(WrapperResource));
  glGenTextures(1, &result->GLValue);
  glBindTexture(GL_TEXTURE_2D, result->GLValue);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, (GLsizei)Width, (GLsizei)Height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
  return result;
}

void GLAPI::writeResourcesTransitionBarrier(union WrapperCommandList* wrappedCmdList, const std::vector<std::tuple<WrapperResource *, enum class RESOURCE_USAGE, enum class RESOURCE_USAGE> > &barriers)
{
}

// Not to call after bind !
void GLAPI::clearRTTSet(union WrapperCommandList* wrappedCmdList, union WrapperRTTSet* RTTSet, float color[4])
{
  glClearColor(color[0], color[1], color[2], color[3]);
  RTTSet->GLValue.Bind();
  glClear(GL_COLOR_BUFFER_BIT);
}

void GLAPI::clearDepthStencilFromRTTSet(union WrapperCommandList* wrappedCmdList, union WrapperRTTSet*RTTSet, float Depth, unsigned stencil)
{
  RTTSet->GLValue.Bind();
  glClearDepth(Depth);
  glClear(GL_DEPTH_BUFFER_BIT);
}

void GLAPI::setRTTSet(union WrapperCommandList* wrappedCmdList, union WrapperRTTSet*RTTSet)
{
  RTTSet->GLValue.Bind();
}

union WrapperDescriptorHeap* GLAPI::createCBVSRVUAVDescriptorHeap(const std::vector<std::tuple<union WrapperResource *, enum class RESOURCE_VIEW, size_t> > &Resources)
{
  WrapperDescriptorHeap *result = (WrapperDescriptorHeap*)malloc(sizeof(WrapperDescriptorHeap));
  new (&result->GLValue) std::vector<std::tuple<GLuint, RESOURCE_VIEW, unsigned>>();

  for (const std::tuple<union WrapperResource *, enum class RESOURCE_VIEW, size_t> &Resource : Resources)
  {
    result->GLValue.push_back(std::make_tuple(std::get<0>(Resource)->GLValue, std::get<1>(Resource), std::get<2>(Resource)));
  }
  return result;
}

union WrapperDescriptorHeap* GLAPI::createSamplerHeap(const std::vector<size_t> &SamplersDesc)
{
  WrapperDescriptorHeap *result = (WrapperDescriptorHeap*)malloc(sizeof(WrapperDescriptorHeap));
  new (&result->GLValue) std::vector<std::tuple<GLuint, RESOURCE_VIEW, unsigned>>();
  for (size_t Unit : SamplersDesc)
  {
    GLuint Sampler = SamplerHelper::createTrilinearSampler();
    result->GLValue.push_back(std::make_tuple(Sampler, RESOURCE_VIEW::SAMPLER, Unit));
  }
  return result;
}

void GLAPI::setDescriptorHeap(union WrapperCommandList* wrappedCmdList, size_t slot, union WrapperDescriptorHeap *DescriptorHeap)
{
  for (std::tuple<GLuint, RESOURCE_VIEW, unsigned> Resource : DescriptorHeap->GLValue)
  {
    switch (std::get<1>(Resource))
    {
    case RESOURCE_VIEW::CONSTANTS_BUFFER:
      glBindBufferBase(GL_UNIFORM_BUFFER, std::get<2>(Resource), std::get<0>(Resource));
      break;
    case RESOURCE_VIEW::SHADER_RESOURCE:
      glActiveTexture(GL_TEXTURE0 + std::get<2>(Resource));
      glBindTexture(GL_TEXTURE_2D, std::get<0>(Resource));
      break;
    case RESOURCE_VIEW::SAMPLER:
      glBindSampler(std::get<2>(Resource), std::get<0>(Resource));
      break;
    }
  }
}

void GLAPI::setIndexVertexBuffersSet(union WrapperCommandList* wrappedCmdList, WrapperIndexVertexBuffersSet* wrappedVAO)
{
  glBindVertexArray(wrappedVAO->GLValue);
}

void GLAPI::setPipelineState(union WrapperCommandList* wrappedCmdList, union WrapperPipelineState* wrappedPipelineState)
{
  glUseProgram(wrappedPipelineState->GLValue.Program);
  wrappedPipelineState->GLValue.StateSetter();
}

WrapperResource *GLAPI::createConstantsBuffer(size_t sizeInByte)
{
  WrapperResource* result = (WrapperResource *)malloc(sizeof(WrapperResource));
  glGenBuffers(1, &result->GLValue);
  glBindBuffer(GL_UNIFORM_BUFFER, result->GLValue);
  glBufferData(GL_UNIFORM_BUFFER, sizeInByte, nullptr, GL_STATIC_DRAW);
  return result;
}

void *GLAPI::mapConstantsBuffer(union WrapperResource *wrappedConstantsBuffer)
{
  glBindBuffer(GL_UNIFORM_BUFFER, wrappedConstantsBuffer->GLValue);
  return glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
}

void GLAPI::unmapConstantsBuffers(union WrapperResource *wrappedConstantsBuffer)
{
  glUnmapBuffer(GL_UNIFORM_BUFFER);
}

WrapperCommandList* GLAPI::createCommandList()
{
  return nullptr;
}

void GLAPI::closeCommandList(union WrapperCommandList *wrappedCmdList)
{}

void GLAPI::openCommandList(union WrapperCommandList* wrappedCmdList)
{}

void GLAPI::drawIndexedInstanced(union WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t indexOffset, size_t vertexOffset, size_t instanceOffset)
{
  glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_SHORT, (void *)indexOffset, (GLsizei)vertexOffset);
}

void GLAPI::drawInstanced(union WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t vertexOffset, size_t instanceOffset)
{
  glDrawArrays(GL_TRIANGLES, (GLsizei)vertexOffset, (GLsizei)indexCount);
}

void GLAPI::submitToQueue(union WrapperCommandList *wrappedCmdList)
{

}

void GLAPI::fullscreenSetVertexBufferAndDraw(union WrapperCommandList *wrappedCmdList)
{
  glBindVertexArray(SharedObject::getInstance()->FullScreenQuadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}