// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef VAOMANAGER_HPP
#define VAOMANAGER_HPP

#include <GL/glew.h>
#include <Core/IMeshBuffer.h>
#include <Core/Singleton.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <tuple>

#include <GLAPI/GLS3DVertex.h>

enum InstanceType
{
  InstanceTypeDualTex,
  InstanceTypeThreeTex,
  InstanceTypeShadow,
  InstanceTypeRSM,
  InstanceTypeGlow,
  InstanceTypeCount,
};

#ifdef WIN32
#pragma pack(push, 1)
#endif
struct InstanceDataShadow
{
  struct
  {
    float X;
    float Y;
    float Z;
  } Origin;
  struct
  {
    float X;
    float Y;
    float Z;
  } Orientation;
  struct
  {
    float X;
    float Y;
    float Z;
  } Scale;
  uint64_t Texture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct InstanceDataRSM
{
  struct
  {
    float X;
    float Y;
    float Z;
  } Origin;
  struct
  {
    float X;
    float Y;
    float Z;
  } Orientation;
  struct
  {
    float X;
    float Y;
    float Z;
  } Scale;
  uint64_t Texture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct InstanceDataDualTex
{
  struct
  {
    float X;
    float Y;
    float Z;
  } Origin;
  struct
  {
    float X;
    float Y;
    float Z;
  } Orientation;
  struct
  {
    float X;
    float Y;
    float Z;
  } Scale;
  uint64_t Texture;
  uint64_t SecondTexture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct InstanceDataThreeTex
{
  struct
  {
    float X;
    float Y;
    float Z;
  } Origin;
  struct
  {
    float X;
    float Y;
    float Z;
  } Orientation;
  struct
  {
    float X;
    float Y;
    float Z;
  } Scale;
  uint64_t Texture;
  uint64_t SecondTexture;
  uint64_t ThirdTexture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct GlowInstanceData
{
  struct
  {
    float X;
    float Y;
    float Z;
  } Origin;
  struct
  {
    float X;
    float Y;
    float Z;
  } Orientation;
  struct
  {
    float X;
    float Y;
    float Z;
  } Scale;
  unsigned Color;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct SkinnedVertexData
{
  int index0;
  float weight0;
  int index1;
  float weight1;
  int index2;
  float weight2;
  int index3;
  float weight3;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

#ifdef WIN32
#pragma pack(pop)
#endif

template<typename... Args>
struct VertexFiller;

template<>
struct VertexFiller<>
{
  static void Fill(std::vector<GLuint > &, size_t vtx_count)
  {}
};

template<typename T, typename... Args>
struct VertexFiller<T, Args...>
{
  static void Fill(std::vector<GLuint> &vertexbuffers, size_t vtx_count, const std::vector<std::vector<T> > &Data, const std::vector<std::vector<Args> > &... Otherdata)
  {
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vtx_count * sizeof(T), nullptr, GL_STATIC_DRAW);
    size_t offset = 0;
    for (const std::vector<T> &appendedData : Data)
    {
      glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(T) * appendedData.size(), appendedData.data());
      offset += sizeof(T) * appendedData.size();
    }

    vertexbuffers.push_back(vbo);
    VertexFiller<Args...>::Fill(vertexbuffers, vtx_count, Otherdata...);
  }
};

struct GLVertexStorage
{
private:
  GLuint indexBuffer;
  std::vector<GLuint> vertexBuffers;
public:
  std::vector<std::tuple<size_t, size_t, size_t> > meshOffset;
  GLuint vao;

  template <typename S3DVertexFormat, typename ...AppendedData>
  GLVertexStorage(const std::vector<S3DVertexFormat >& meshes)
  {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    vertexBuffers.resize(1);

    glGenBuffers(1, &vertexBuffers[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[0]);
    glBufferData(GL_ARRAY_BUFFER, meshes.size() * sizeof(S3DVertexFormat), meshes.data(), GL_STATIC_DRAW);

    VertexAttribBinder<S3DVertexFormat>::bind<0>(vertexBuffers);
  }

  template <typename S3DVertexFormat, typename ...AppendedData>
  GLVertexStorage(const std::vector<irr::scene::IMeshBuffer<S3DVertexFormat> > &meshes, const std::vector<std::vector<AppendedData> > &... otherData)
  {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    size_t vertexCount = 0, indexCount = 0;
    for (const irr::scene::IMeshBuffer<S3DVertexFormat> &mesh : meshes)
    {
      vertexCount += mesh.getVertexCount();
      indexCount += mesh.getIndexCount();
    }

    vertexBuffers.resize(1);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned short), nullptr, GL_STATIC_DRAW);
    glGenBuffers(1, &vertexBuffers[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[0]);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(S3DVertexFormat), nullptr, GL_STATIC_DRAW);

    size_t currentIndex = 0, currentVertex = 0;
    for (const irr::scene::IMeshBuffer<S3DVertexFormat> &mesh : meshes)
    {
      glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, currentIndex * sizeof(unsigned short), mesh.getIndexCount() * sizeof(unsigned short), mesh.getIndices());
      glBufferSubData(GL_ARRAY_BUFFER, currentVertex * sizeof(S3DVertexFormat), mesh.getVertexCount() * sizeof(S3DVertexFormat), mesh.getVertices());
      meshOffset.emplace_back(mesh.getIndexCount(), currentVertex, currentIndex * sizeof(unsigned short));
      currentIndex += mesh.getIndexCount();
      currentVertex += mesh.getVertexCount();
    }
    VertexFiller<AppendedData...>::Fill(vertexBuffers, vertexCount, otherData...);

    VertexAttribBinder<S3DVertexFormat, AppendedData...>::bind<0>(vertexBuffers);
  }
};

/*template<typename Data>
class InstanceBuffer : public Singleton<InstanceBuffer<Data> >, public ArrayBuffer <Data, GL_ARRAY_BUFFER>
{
public:
InstanceBuffer() : ArrayBuffer<Data, GL_ARRAY_BUFFER>()
{
ArrayBuffer<Data, GL_ARRAY_BUFFER>::resizeBufferIfNecessary(10000);
}
};*/

template<typename VT>
struct InstanceAttribBinder
{
public:
  static void bind();
};

static void SetVertexAttrib_impl(GLsizei s)
{
  glEnableVertexAttribArray(7);
  glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, s, 0);
  glVertexAttribDivisorARB(7, 1);
  glEnableVertexAttribArray(8);
  glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, s, (GLvoid*)(3 * sizeof(float)));
  glVertexAttribDivisorARB(8, 1);
  glEnableVertexAttribArray(9);
  glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, s, (GLvoid*)(6 * sizeof(float)));
  glVertexAttribDivisorARB(9, 1);
}



template<>
struct InstanceAttribBinder < InstanceDataShadow >
{
public:
  static void SetVertexAttrib()
  {
    SetVertexAttrib_impl(sizeof(InstanceDataShadow));
    glEnableVertexAttribArray(10);
    glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataShadow), (GLvoid*)(9 * sizeof(float)));
    glVertexAttribDivisorARB(10, 1);
  }
};

template<>
struct InstanceAttribBinder < InstanceDataRSM >
{
public:
  static void SetVertexAttrib()
  {
    SetVertexAttrib_impl(sizeof(InstanceDataRSM));
    glEnableVertexAttribArray(10);
    glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataRSM), (GLvoid*)(9 * sizeof(float)));
    glVertexAttribDivisorARB(10, 1);
  }
};

template<>
struct InstanceAttribBinder < InstanceDataDualTex >
{
public:
  static void SetVertexAttrib()
  {
    SetVertexAttrib_impl(sizeof(InstanceDataDualTex));
    glEnableVertexAttribArray(10);
    glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataDualTex), (GLvoid*)(9 * sizeof(float)));
    glVertexAttribDivisorARB(10, 1);
    glEnableVertexAttribArray(11);
    glVertexAttribIPointer(11, 2, GL_UNSIGNED_INT, sizeof(InstanceDataDualTex), (GLvoid*)(9 * sizeof(float) + 2 * sizeof(unsigned)));
    glVertexAttribDivisorARB(11, 1);
  }
};

template<>
struct InstanceAttribBinder < InstanceDataThreeTex >
{
public:
  static void SetVertexAttrib()
  {
    SetVertexAttrib_impl(sizeof(InstanceDataThreeTex));
    glEnableVertexAttribArray(10);
    glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataThreeTex), (GLvoid*)(9 * sizeof(float)));
    glVertexAttribDivisorARB(10, 1);
    glEnableVertexAttribArray(11);
    glVertexAttribIPointer(11, 2, GL_UNSIGNED_INT, sizeof(InstanceDataThreeTex), (GLvoid*)(9 * sizeof(float) + 2 * sizeof(unsigned)));
    glVertexAttribDivisorARB(11, 1);
    glEnableVertexAttribArray(13);
    glVertexAttribIPointer(13, 2, GL_UNSIGNED_INT, sizeof(InstanceDataThreeTex), (GLvoid*)(9 * sizeof(float) + 4 * sizeof(unsigned)));
    glVertexAttribDivisorARB(13, 1);
  }
};

template<>
struct InstanceAttribBinder < GlowInstanceData >
{
public:
  static void SetVertexAttrib()
  {
    SetVertexAttrib_impl(sizeof(GlowInstanceData));
    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GlowInstanceData), (GLvoid*)(9 * sizeof(float)));
    glVertexAttribDivisorARB(12, 1);
  }
};

#endif
