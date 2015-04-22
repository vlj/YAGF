// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __BASIC_VERTEX_LAYOUT_H__
#define __BASIC_VERTEX_LAYOUT_H__

#include <Core/BasicVertexLayout.h>
#include <GL/glew.h>

template<typename...Buffers>
struct VertexAttribBinder
{
  template<int N>
  static void bind(std::vector<GLuint> vbos);
};

template<>
struct VertexAttribBinder<>
{
  template<int N>
  static void bind(std::vector<GLuint> vbos)
  {}
};

template<typename... ExtraBuffers>
struct VertexAttribBinder<irr::video::S3DVertex, ExtraBuffers...>
{
  template<int N>
  static void bind(std::vector<GLuint> vbos)
  {
    glBindBuffer(GL_ARRAY_BUFFER, vbos[N]);
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertex), 0);
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertex), (GLvoid*)12);
    // Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(irr::video::S3DVertex), (GLvoid*)24);
    // Texcoord
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertex), (GLvoid*)28);
    VertexAttribBinder<ExtraBuffers...>::template bind<N + 1>(vbos);
  }
};

template<typename... ExtraBuffers>
struct VertexAttribBinder <irr::video::S3DVertex2TCoords, ExtraBuffers...>
{
  template<int N>
  static void bind(std::vector<GLuint> vbos)
  {
    glBindBuffer(GL_ARRAY_BUFFER, vbos[N]);
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertex2TCoords), 0);
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertex2TCoords), (GLvoid*)12);
    // Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(irr::video::S3DVertex2TCoords), (GLvoid*)24);
    // Texcoord
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertex2TCoords), (GLvoid*)28);
    // SecondTexcoord
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertex2TCoords), (GLvoid*)36);
    VertexAttribBinder<ExtraBuffers...>::template bind<N + 1>(vbos);
  }
};

template<typename... ExtraBuffers>
struct VertexAttribBinder < irr::video::S3DVertexTangents, ExtraBuffers...>
{
  template<int N>
  static void bind(std::vector<GLuint> vbos)
  {
    glBindBuffer(GL_ARRAY_BUFFER, vbos[N]);
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertexTangents), 0);
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertexTangents), (GLvoid*)12);
    // Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(irr::video::S3DVertexTangents), (GLvoid*)24);
    // Texcoord
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertexTangents), (GLvoid*)28);
    // Tangent
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertexTangents), (GLvoid*)36);
    // Bitangent
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertexTangents), (GLvoid*)48);
    VertexAttribBinder<ExtraBuffers...>::template bind<N + 1>(vbos);
  }
};

template<typename... ExtraBuffers>
struct VertexAttribBinder <irr::video::SkinnedVertexData, ExtraBuffers...>
{
  template <int N>
  static void bind(std::vector<GLuint> vbos)
  {
    glBindBuffer(GL_ARRAY_BUFFER, vbos[N]);
    glEnableVertexAttribArray(7);
    glVertexAttribIPointer(7, 1, GL_INT, 4 * 2 * sizeof(float), 0);
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, 4 * 2 * sizeof(float), (GLvoid*)(sizeof(int)));
    glEnableVertexAttribArray(9);
    glVertexAttribIPointer(9, 1, GL_INT, 4 * 2 * sizeof(float), (GLvoid*)(sizeof(float) + sizeof(int)));
    glEnableVertexAttribArray(10);
    glVertexAttribPointer(10, 1, GL_FLOAT, GL_FALSE, 4 * 2 * sizeof(float), (GLvoid*)(sizeof(float) + 2 * sizeof(int)));
    glEnableVertexAttribArray(11);
    glVertexAttribIPointer(11, 1, GL_INT, 4 * 2 * sizeof(float), (GLvoid*)(2 * sizeof(float) + 2 * sizeof(int)));
    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 1, GL_FLOAT, GL_FALSE, 4 * 2 * sizeof(float), (GLvoid*)(2 * sizeof(float) + 3 * sizeof(int)));
    glEnableVertexAttribArray(13);
    glVertexAttribIPointer(13, 1, GL_INT, 4 * 2 * sizeof(float), (GLvoid*)(3 * sizeof(float) + 3 * sizeof(int)));
    glEnableVertexAttribArray(14);
    glVertexAttribPointer(14, 1, GL_FLOAT, GL_FALSE, 4 * 2 * sizeof(float), (GLvoid*)(3 * sizeof(float) + 4 * sizeof(int)));
    VertexAttribBinder<ExtraBuffers...>::template bind<N + 1>(vbos);
  }
};

template<>
struct VertexAttribBinder <irr::video::ScreenQuadVertex>
{
  template <int N>
  static void bind(std::vector<GLuint> vbos)
  {
    static_assert(N == 0, "ScreenQuadVertex VAO should not have companion buffer");
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(2 * sizeof(float)));
  }
};

#endif