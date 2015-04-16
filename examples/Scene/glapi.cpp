// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <glapi.h>
#include <D3DAPI/Texture.h>
#include <D3DAPI/Resource.h>

WrapperResource* GLAPI::createRTT(irr::video::ECOLOR_FORMAT Format, size_t Width, size_t Height, float fastColor[4])
{
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  //    if (CVS->isARBTextureStorageUsable())
  //      glTexStorage2D(GL_TEXTURE_2D, mipmaplevel, internalFormat, res.Width, res.Height);
  //    else
//  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, (GLsizei)Width, (GLsizei)Height, 0, format, type, 0);
  WrapperResource* result = (WrapperResource *)malloc(sizeof(WrapperResource));
  return result;
}

union WrapperRTTSet* GLAPI::createRTTSet(const std::vector<WrapperResource*> &RTTs, const std::vector<irr::video::ECOLOR_FORMAT> &formats, size_t Width, size_t Height)
{
  return nullptr;
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
  glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_SHORT, (void *) (indexOffset * sizeof(unsigned short)), (GLsizei)vertexOffset);
}