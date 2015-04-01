// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2015 Vincent Lejeune
// Contains code from the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in License.txt

#include <Core/BasicVertexLayout.h>
#include <Maths/vector3d.h>


class GeometryCreator
{
public:
  static irr::scene::SMeshBuffer*
    createCubeMeshBuffer(const irr::core::vector3df& size)
  {
    irr::scene::SMeshBuffer* buffer = new irr::scene::SMeshBuffer();

    // Create indices
    const uint16_t u[36] = { 0, 2, 1, 0, 3, 2, 1, 5, 4, 1, 2, 5, 4, 6, 7, 4, 5, 6,
      7, 3, 0, 7, 6, 3, 9, 5, 2, 9, 8, 5, 0, 11, 10, 0, 10, 7 };

    buffer->Indices.resize(36);

    for (size_t i = 0; i < 36; ++i)
      buffer->Indices[i] = u[i];


    // Create vertices
    irr::video::SColor clr(255, 255, 255, 255);

    buffer->Vertices.push_back(irr::video::S3DVertex(0, 0, 0, -1, -1, -1, clr, 0, 1));
    buffer->Vertices.push_back(irr::video::S3DVertex(1, 0, 0, 1, -1, -1, clr, 1, 1));
    buffer->Vertices.push_back(irr::video::S3DVertex(1, 1, 0, 1, 1, -1, clr, 1, 0));
    buffer->Vertices.push_back(irr::video::S3DVertex(0, 1, 0, -1, 1, -1, clr, 0, 0));
    buffer->Vertices.push_back(irr::video::S3DVertex(1, 0, 1, 1, -1, 1, clr, 0, 1));
    buffer->Vertices.push_back(irr::video::S3DVertex(1, 1, 1, 1, 1, 1, clr, 0, 0));
    buffer->Vertices.push_back(irr::video::S3DVertex(0, 1, 1, -1, 1, 1, clr, 1, 0));
    buffer->Vertices.push_back(irr::video::S3DVertex(0, 0, 1, -1, -1, 1, clr, 1, 1));
    buffer->Vertices.push_back(irr::video::S3DVertex(0, 1, 1, -1, 1, 1, clr, 0, 1));
    buffer->Vertices.push_back(irr::video::S3DVertex(0, 1, 0, -1, 1, -1, clr, 1, 1));
    buffer->Vertices.push_back(irr::video::S3DVertex(1, 0, 1, 1, -1, 1, clr, 1, 0));
    buffer->Vertices.push_back(irr::video::S3DVertex(1, 0, 0, 1, -1, -1, clr, 0, 0));

    for (size_t i = 0; i < 12; ++i)
    {
      buffer->Vertices[i].Pos -= irr::core::vector3df(0.5f, 0.5f, 0.5f);
      buffer->Vertices[i].Pos *= size;
    }

    return buffer;
  }
};
