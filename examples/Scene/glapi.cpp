// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <API/glapi.h>
#include <GLAPI/Samplers.h>
#include <GLAPI/Misc.h>
#include <Core/BasicVertexLayout.h>

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
  case irr::video::ECF_R16G16B16A16F:
    colorformat = GL_RGBA;
    internalformat = GL_RGBA16F;
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


struct WrapperResource* GLAPI::createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4])
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

struct WrapperRTTSet* GLAPI::createRTTSet(const std::vector<struct WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height, struct WrapperResource *DepthStencil)
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

void GLAPI::releaseRTTSet(WrapperRTTSet * RTTSet)
{
  RTTSet->GLValue.~GLRTTSet();
  free(RTTSet);
}

void GLAPI::releasePSO(WrapperPipelineState * pso)
{
  glDeleteProgram(pso->GLValue.Program);
  free(pso);
}

struct WrapperResource* GLAPI::createDepthStencilTexture(size_t Width, size_t Height)
{
  WrapperResource* result = (WrapperResource *)malloc(sizeof(WrapperResource));
  glGenTextures(1, &result->GLValue);
  glBindTexture(GL_TEXTURE_2D, result->GLValue);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, (GLsizei)Width, (GLsizei)Height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
  return result;
}

void GLAPI::releaseRTTOrDepthStencilTexture(WrapperResource * res)
{
  glDeleteTextures(1, &res->GLValue);
  free(res);
}

void GLAPI::writeResourcesTransitionBarrier(struct WrapperCommandList* wrappedCmdList, const std::vector<std::tuple<struct WrapperResource *, enum class RESOURCE_USAGE, enum class RESOURCE_USAGE> > &barriers)
{
}

// Not to call after bind !
void GLAPI::clearRTTSet(struct WrapperCommandList* wrappedCmdList, struct WrapperRTTSet* RTTSet, float color[4])
{
  glClearColor(color[0], color[1], color[2], color[3]);
  RTTSet->GLValue.Bind();
  glClear(GL_COLOR_BUFFER_BIT);
}

void GLAPI::clearDepthStencilFromRTTSet(struct WrapperCommandList* wrappedCmdList, struct WrapperRTTSet*RTTSet, float Depth, unsigned stencil)
{
  RTTSet->GLValue.Bind();
  glClearDepth(Depth);
  glClear(GL_DEPTH_BUFFER_BIT);
}

void GLAPI::setRTTSet(struct WrapperCommandList* wrappedCmdList, struct WrapperRTTSet*RTTSet)
{
  RTTSet->GLValue.Bind();
}

void GLAPI::setBackbufferAsRTTSet(struct WrapperCommandList* wrappedCmdList, size_t width, size_t height)
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

void GLAPI::setBackbufferAsPresent(struct WrapperCommandList* wrappedCmdList)
{}

struct WrapperDescriptorHeap* GLAPI::createCBVSRVUAVDescriptorHeap(const std::vector<std::tuple<struct WrapperResource *, enum class RESOURCE_VIEW, size_t> > &Resources)
{
  WrapperDescriptorHeap *result = (WrapperDescriptorHeap*)malloc(sizeof(WrapperDescriptorHeap));
  new (&result->GLValue) std::vector<std::tuple<GLuint, RESOURCE_VIEW, unsigned>>();

  for (const std::tuple<struct WrapperResource *, enum class RESOURCE_VIEW, size_t> &Resource : Resources)
  {
    result->GLValue.push_back(std::make_tuple(std::get<0>(Resource)->GLValue, std::get<1>(Resource), std::get<2>(Resource)));
  }
  return result;
}

void GLAPI::releaseCBVSRVUAVDescriptorHeap(WrapperDescriptorHeap * Heap)
{
  Heap->GLValue.~vector();
  free(Heap);
}

struct WrapperDescriptorHeap* GLAPI::createSamplerHeap(const std::vector<std::pair<enum class SAMPLER_TYPE, size_t>> &SamplersDesc)
{
  WrapperDescriptorHeap *result = (WrapperDescriptorHeap*)malloc(sizeof(WrapperDescriptorHeap));
  new (&result->GLValue) std::vector<std::tuple<GLuint, RESOURCE_VIEW, unsigned>>();
  for (const std::pair<enum class SAMPLER_TYPE, size_t> &Sampler : SamplersDesc)
  {
    GLuint SamplerObject;
    switch (Sampler.first)
    {
    case SAMPLER_TYPE::ANISOTROPIC:
      SamplerObject = SamplerHelper::createTrilinearAnisotropicSampler(16);
      break;
    case SAMPLER_TYPE::TRILINEAR:
      SamplerObject = SamplerHelper::createTrilinearSampler();
      break;
    case SAMPLER_TYPE::BILINEAR:
      SamplerObject = SamplerHelper::createBilinearSampler();
      break;
    case SAMPLER_TYPE::NEAREST:
      SamplerObject = SamplerHelper::createNearestSampler();
      break;
    }
    result->GLValue.push_back(std::make_tuple(SamplerObject, RESOURCE_VIEW::SAMPLER, Sampler.second));
  }
  return result;
}

void GLAPI::releaseSamplerHeap(WrapperDescriptorHeap * Heap)
{
  for (std::tuple<GLuint, RESOURCE_VIEW, size_t> Resource : Heap->GLValue)
    glDeleteSamplers(1, &std::get<0>(Resource));
  Heap->GLValue.~vector();
  free(Heap);
}

void GLAPI::setDescriptorHeap(struct WrapperCommandList* wrappedCmdList, size_t slot, struct WrapperDescriptorHeap *DescriptorHeap)
{
  for (std::tuple<GLuint, RESOURCE_VIEW, size_t> Resource : DescriptorHeap->GLValue)
  {
    switch (std::get<1>(Resource))
    {
    case RESOURCE_VIEW::CONSTANTS_BUFFER:
      glBindBufferBase(GL_UNIFORM_BUFFER, (GLuint)std::get<2>(Resource), std::get<0>(Resource));
      break;
    case RESOURCE_VIEW::SHADER_RESOURCE:
      glActiveTexture((GLenum)(GL_TEXTURE0 + std::get<2>(Resource)));
      glBindTexture(GL_TEXTURE_CUBE_MAP, std::get<0>(Resource));
      break;
    case RESOURCE_VIEW::SAMPLER:
      glBindSampler((GLuint)std::get<2>(Resource), std::get<0>(Resource));
      break;
    }
  }
}

void GLAPI::setIndexVertexBuffersSet(struct WrapperCommandList* wrappedCmdList, struct WrapperIndexVertexBuffersSet* wrappedVAO)
{
  glBindVertexArray(wrappedVAO->GLValue.vao);
}

void GLAPI::setPipelineState(struct WrapperCommandList* wrappedCmdList, struct WrapperPipelineState* wrappedPipelineState)
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

void GLAPI::releaseConstantsBuffers(WrapperResource * cbuf)
{
  glDeleteBuffers(1, &cbuf->GLValue);
  free(cbuf);
}

void *GLAPI::mapConstantsBuffer(struct WrapperResource *wrappedConstantsBuffer)
{
  glBindBuffer(GL_UNIFORM_BUFFER, wrappedConstantsBuffer->GLValue);
  return glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
}

void GLAPI::unmapConstantsBuffers(struct WrapperResource *wrappedConstantsBuffer)
{
  glUnmapBuffer(GL_UNIFORM_BUFFER);
}

WrapperCommandList* GLAPI::createCommandList()
{
  return nullptr;
}

void GLAPI::releaseCommandList(WrapperCommandList * wrappedCmdList)
{
}

void GLAPI::closeCommandList(struct WrapperCommandList *wrappedCmdList)
{}

void GLAPI::openCommandList(struct WrapperCommandList* wrappedCmdList)
{}

void GLAPI::drawIndexedInstanced(struct WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t indexOffset, size_t vertexOffset, size_t instanceOffset)
{
  glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_SHORT, (void *)indexOffset, (GLsizei)vertexOffset);
}

void GLAPI::drawInstanced(struct WrapperCommandList *wrappedCmdList, size_t indexCount, size_t instanceCount, size_t vertexOffset, size_t instanceOffset)
{
  glDrawArrays(GL_TRIANGLES, (GLsizei)vertexOffset, (GLsizei)indexCount);
}

void GLAPI::submitToQueue(struct WrapperCommandList *wrappedCmdList)
{

}

struct WrapperIndexVertexBuffersSet* GLAPI::createFullscreenTri()
{
  std::vector<irr::video::ScreenQuadVertex> TriVertices =
  {
    irr::video::ScreenQuadVertex({ irr::core::vector2df(-1., -1.), irr::core::vector2df(0., 0.) }),
    irr::video::ScreenQuadVertex({ irr::core::vector2df(-1., 3.), irr::core::vector2df(0., 2.) }),
    irr::video::ScreenQuadVertex({ irr::core::vector2df(3., -1.), irr::core::vector2df(2., 0.) })
  };

  WrapperIndexVertexBuffersSet *result = (WrapperIndexVertexBuffersSet*)malloc(sizeof(WrapperIndexVertexBuffersSet));
  new (&result->GLValue) GLVertexStorage(TriVertices);
  return result;
}
void GLAPI::releaseIndexVertexBuffersSet(struct WrapperIndexVertexBuffersSet *res)
{
  res->GLValue.~GLVertexStorage();
  free(res);
}