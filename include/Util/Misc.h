// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#ifndef MISC_H
#define MISC_H

#include <Core/Singleton.h>
#include <GL/glew.h>

class SharedObject : public Singleton < SharedObject >
{
public:
  GLuint tri_vbo;
  GLuint FullScreenQuadVAO;

  SharedObject()
  {
    const float tri_vertex[] = {
      -1., -1.,
      -1., 3.,
      3., -1.,
    };
    glGenBuffers(1, &tri_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, tri_vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), tri_vertex, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &FullScreenQuadVAO);
    glBindVertexArray(FullScreenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tri_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
  }

  ~SharedObject()
  {
    glDeleteVertexArrays(1, &FullScreenQuadVAO);
    glDeleteBuffers(1, &tri_vbo);
  }
};

extern const char *screenquadshader;

template<typename T, typename... Args>
static void DrawFullScreenEffect(const Args &...args)
{
  glUseProgram(T::getInstance()->Program);
  glBindVertexArray(SharedObject::getInstance()->FullScreenQuadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

#endif
