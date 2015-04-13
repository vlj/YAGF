// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __RENDER_TARGETS_H__
#define __RENDER_TARGETS_H__

#ifdef GLBUILD
#include <GLAPI/FBO.h>
#endif

#ifdef DXBUILD
#include <d3d12.h>
#include <wrl/client.h>
#endif

class RenderTargets
{
public:
  enum RTT
  {
    GBUFFER_BASE_COLOR,
    GBUFFER_NORMAL_AND_DEPTH,
    GBUFFER_EMIT_VALUE,
    COLORS,
    LINEAR_DEPTH,
    RTT_COUNT,
  };
#ifdef GLBUILD
  enum FBOType
  {
    FBO_GBUFFER,
    FBO_COLORS,
    FBO_LINEAR_DEPTH,
    FBO_COUNT,
  };
#endif
private:
  size_t Width, Height;
#ifdef GLBUILD
  GLuint DepthStencilTexture;
  GLuint RenderTargetTextures[RTT_COUNT];
  std::vector<FrameBuffer >FrameBuffers;

  static GLuint generateRTT(size_t w, size_t h, GLint internalFormat, GLint format, GLint type, unsigned mipmaplevel = 1)
  {
    GLuint result;
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);
//    if (CVS->isARBTextureStorageUsable())
//      glTexStorage2D(GL_TEXTURE_2D, mipmaplevel, internalFormat, res.Width, res.Height);
//    else
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, (GLsizei)w, (GLsizei)h, 0, format, type, 0);
    return result;
}
#endif
#ifdef DXBUILD
  Microsoft::WRL::ComPtr<ID3D12Resource> DepthBuffer;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DepthDescriptorHeap;
#endif
public:
  RenderTargets(size_t w, size_t h): Width(w), Height(h)
  {
#ifdef GLBUILD
    glGenTextures(1, &DepthStencilTexture);
    DepthStencilTexture = generateRTT(w, h, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);

    glGenTextures(RTT_COUNT, RenderTargetTextures);
    RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH] = generateRTT(w, h , GL_RGBA16F, GL_RGBA, GL_FLOAT);
    RenderTargetTextures[GBUFFER_BASE_COLOR] = generateRTT(w, h, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    RenderTargetTextures[COLORS] = generateRTT(w, h, GL_RGBA16F, GL_BGRA, GL_FLOAT);

    FrameBuffers.push_back(FrameBuffer({ RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH], RenderTargetTextures[GBUFFER_BASE_COLOR]}, DepthStencilTexture, w, h));
    FrameBuffers.push_back(FrameBuffer({ RenderTargetTextures[COLORS], RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH] }, DepthStencilTexture, w, h));
    FrameBuffers.push_back(FrameBuffer({ RenderTargetTextures[LINEAR_DEPTH], RenderTargetTextures[GBUFFER_NORMAL_AND_DEPTH] }, DepthStencilTexture, w, h));
#endif
#ifdef DXBUILD
    HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, Width, Height, 1, 0, 1, 0, D3D12_RESOURCE_MISC_ALLOW_DEPTH_STENCIL),
      D3D12_RESOURCE_USAGE_DEPTH,
      &CD3D12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1., 0),
      IID_PPV_ARGS(&DepthBuffer));

    D3D12_DESCRIPTOR_HEAP_DESC Depthdesc = {};
    Depthdesc.Type = D3D12_DSV_DESCRIPTOR_HEAP;
    Depthdesc.NumDescriptors = 1;
    Depthdesc.Flags = D3D12_DESCRIPTOR_HEAP_NONE;
    hr = Context::getInstance()->dev->CreateDescriptorHeap(&Depthdesc, IID_PPV_ARGS(&DepthDescriptorHeap));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    Context::getInstance()->dev->CreateDepthStencilView(DepthBuffer.Get(), &dsv, DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
#endif
  }

#ifdef DXBUILD
  ID3D12DescriptorHeap* getDepthDescriptorHeap() const
  {
    return DepthDescriptorHeap.Get();
  }
#endif

  ~RenderTargets()
  {
#ifdef GLBUILD
    glDeleteTextures(1, &DepthStencilTexture);
    glDeleteTextures(RTT_COUNT, RenderTargetTextures);
#endif
  }

#ifdef GLBUILD
  FrameBuffer& getFrameBuffer(enum FBOType tp)
  {
    return FrameBuffers[tp];
  }
#endif


};

#endif