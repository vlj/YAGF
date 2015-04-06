// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <GL/glew.h>

class SamplerHelper
{
public:
  static GLuint createNearestSampler()
  {
    GLuint id;
    glGenSamplers(1, &id);
    glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
    return id;
  }

  static GLuint createBilinearSampler()
  {
    GLuint id;
    glGenSamplers(1, &id);
    glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
    return id;
  }

  static GLuint createTrilinearSampler()
  {
    GLuint id;
    glGenSamplers(1, &id);
    glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
    return id;
  }

  static GLuint createTrilinearAnisotropicSampler(float anisolevel)
  {
    GLuint id;
    glGenSamplers(1, &id);
    glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisolevel);
    return id;
  }
};