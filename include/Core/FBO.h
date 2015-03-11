#ifndef FBO_HPP
#define FBO_HPP

#include <vector>
#include <assert.h>

class FrameBuffer
{
private:
    GLuint fbo, fbolayer;
    std::vector<GLuint> RenderTargets;
    GLuint DepthTexture;
    size_t width, height;
public:
    FrameBuffer() {}
    
    FrameBuffer(const std::vector <GLuint> &RTTs, size_t w, size_t h)
      : fbolayer(0), RenderTargets(RTTs), DepthTexture(0),
      width(w), height(h)
    {
      glGenFramebuffers(1, &fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      if (false)
      {
          for (unsigned i = 0; i < RTTs.size(); i++)
              glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, RTTs[i], 0);
      }
      else
      {
          for (unsigned i = 0; i < RTTs.size(); i++)
              glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, RTTs[i], 0);
      }
      GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      assert(result == GL_FRAMEBUFFER_COMPLETE_EXT);
    }

    FrameBuffer(const std::vector <GLuint> &RTTs, GLuint DS, size_t w, size_t h)
      : fbolayer(0), RenderTargets(RTTs), DepthTexture(DS), width(w),
      height(h)
    {
      glGenFramebuffers(1, &fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      if (false)
      {
          for (unsigned i = 0; i < RTTs.size(); i++)
              glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, RTTs[i], 0);
          glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, DS, 0);
      }
      else
      {
          for (unsigned i = 0; i < RTTs.size(); i++)
              glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, RTTs[i], 0);
          glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, DS, 0);
      }
      GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      assert(result == GL_FRAMEBUFFER_COMPLETE_EXT);
      if (false)
          glGenFramebuffers(1, &fbolayer);
    }

    ~FrameBuffer()
    {
      glDeleteFramebuffers(1, &fbo);
      if (fbolayer)
          glDeleteFramebuffers(1, &fbolayer);
    }

    void Bind()
    {
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glViewport(0, 0, (int)width, (int)height);
      GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
      glDrawBuffers((int)RenderTargets.size(), bufs);
    }

    void BindLayer(unsigned i)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, fbolayer);
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, RenderTargets[0], 0, i);
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, DepthTexture, 0, i);
      glViewport(0, 0, (int)width, (int)height);
      GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
      glDrawBuffers((int)RenderTargets.size(), bufs);
    }

    const std::vector<GLuint> &getRTT() const
    {
      return RenderTargets;
    }

    GLuint &getDepthTexture()
    {
      assert(DepthTexture);
      return DepthTexture;
    }

    size_t getWidth() const
    {
      return width;
    }

    size_t getHeight() const
    {
      return height;
    }

    static void Blit(const FrameBuffer &Src, FrameBuffer &Dst, GLbitfield mask = GL_COLOR_BUFFER_BIT, GLenum filter = GL_NEAREST)
    {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, Src.fbo);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Dst.fbo);
      glBlitFramebuffer(0, 0, (int)Src.width, (int)Src.height, 0, 0,
          (int)Dst.width, (int)Dst.height, mask, filter);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void BlitToDefault(size_t x0, size_t y0, size_t x1, size_t y1)
    {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glBlitFramebuffer(0, 0, (int)width, (int)height, (int)x0, (int)y0, (int)x1, (int)y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
};

#endif
