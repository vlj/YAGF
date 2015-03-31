// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2015 Vincent Lejeune
// Contains code from the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __I_MESH_BUFFER_H_INCLUDED__
#define __I_MESH_BUFFER_H_INCLUDED__

#include <cstdint>
#include <vector>

namespace irr
{
  namespace scene
  {
    //! Struct for holding a mesh with a single material.
    /** A part of an IMesh which has the same material on each face of that
    group. Logical groups of an IMesh need not be put into separate mesh
    buffers, but can be. Separately animated parts of the mesh must be put
    into separate mesh buffers.
    Some mesh buffer implementations have limitations on the number of
    vertices the buffer can hold. In that case, logical grouping can help.
    Moreover, the number of vertices should be optimized for the GPU upload,
    which often depends on the type of gfx card. Typial figures are
    1000-10000 vertices per buffer.
    SMeshBuffer is a simple implementation of a MeshBuffer, which supports
    up to 65535 vertices.

    Since meshbuffers are used for drawing, and hence will be exposed
    to the driver, chances are high that they are grab()'ed from somewhere.
    It's therefore required to dynamically allocate meshbuffers which are
    passed to a video driver and only drop the buffer once it's not used in
    the current code block anymore.
    */
    template <typename T>
    class IMeshBuffer// : public virtual IReferenceCounted
    {
    public:
      IMeshBuffer()// : Primitive(GL_TRIANGLES)
      {

      }

      //! Get access to vertex data. The data is an array of vertices.
      /** Which vertex type is used can be determined by getVertexType().
      \return Pointer to array of vertices. */
      virtual const void* getVertices() const
      {
        return Vertices.data();
      }

      //! Get access to vertex data. The data is an array of vertices.
      /** Which vertex type is used can be determined by getVertexType().
      \return Pointer to array of vertices. */
      virtual void* getVertices()
      {
        return Vertices.data();
      }

      //! Get amount of vertices in meshbuffer.
      /** \return Number of vertices in this buffer. */
      virtual size_t getVertexCount() const
      {
        return Vertices.size();
      }

      //! Get access to Indices.
      /** \return Pointer to indices array. */
      virtual const uint16_t* getIndices() const
      {
        return Indices.data();
      }

      //! Get access to Indices.
      /** \return Pointer to indices array. */
      virtual uint16_t* getIndices()
      {
        return Indices.data();
      }

      //! Get amount of indices in this meshbuffer.
      /** \return Number of indices in this buffer. */
      virtual size_t getIndexCount() const
      {
        return Indices.size();
      }

      //! Get the axis aligned bounding box of this meshbuffer.
      /** \return Axis aligned bounding box of this buffer. */
      //		virtual const core::aabbox3df& getBoundingBox() const = 0;

      //! Set axis aligned bounding box
      /** \param box User defined axis aligned bounding box to use
      for this buffer. */
      //		virtual void setBoundingBox(const core::aabbox3df& box) = 0;

      //! Recalculates the bounding box. Should be called if the mesh changed.
      //		virtual void recalculateBoundingBox() = 0;

      //! returns position of vertex i
      //		virtual const core::vector3df& getPosition(size_t i) const = 0;

      //! returns position of vertex i
      //		virtual core::vector3df& getPosition(size_t i) = 0;

      //! returns normal of vertex i
      //		virtual const core::vector3df& getNormal(size_t i) const = 0;

      //! returns normal of vertex i
      //		virtual core::vector3df& getNormal(size_t i) = 0;

      //! returns texture coord of vertex i
      //		virtual const core::vector2df& getTCoords(size_t i) const = 0;

      //! returns texture coord of vertex i
      //		virtual core::vector2df& getTCoords(size_t i) = 0;

      //! Returns the primitive type of this buffer
/*      virtual GLenum getPrimitiveType() const
      {
        return Primitive;
      }*/

      //! Append the vertices and indices to the current buffer
      /** Only works for compatible types, i.e. either the same type
      or the main buffer is of standard type. Otherwise, behavior is
      undefined.
      \param vertices Pointer to a vertex array.
      \param numVertices Number of vertices in the array.
      \param indices Pointer to index array.
      \param numIndices Number of indices in array. */
      virtual void append(const void* const vertices, size_t numVertices, const uint16_t* const indices, size_t numIndices)
      {
        if (vertices == getVertices())
          return;

        const size_t vertexCount = getVertexCount();
        size_t i;

        Vertices.resize(vertexCount + numVertices);
        for (i = 0; i < numVertices; ++i)
        {
          Vertices.push_back(reinterpret_cast<const T*>(vertices)[i]);
        }

        Indices.resize(getIndexCount() + numIndices);
        for (i = 0; i < numIndices; ++i)
        {
          Indices.push_back((unsigned short)(indices[i] + vertexCount));
        }
      }

      //! Append the meshbuffer to the current buffer
      /** Only works for compatible types, i.e. either the same type
      or the main buffer is of standard type. Otherwise, behavior is
      undefined.
      \param other Meshbuffer to be appended to this one.
      */
      void append(const IMeshBuffer* const other)
      {
        /*
        if (this==other)
        return;

        const u32 vertexCount = getVertexCount();
        u32 i;

        Vertices.reallocate(vertexCount+other->getVertexCount());
        for (i=0; i<other->getVertexCount(); ++i)
        {
        Vertices.push_back(reinterpret_cast<const T*>(other->getVertices())[i]);
        }

        Indices.reallocate(getIndexCount()+other->getIndexCount());
        for (i=0; i<other->getIndexCount(); ++i)
        {
        Indices.push_back(other->getIndices()[i]+vertexCount);
        }
        BoundingBox.addInternalBox(other->getBoundingBox());
        */

      }

      //! Vertices of this buffer
      std::vector<T> Vertices;
      //! Indices into the vertices of this buffer.
      std::vector<uint16_t> Indices;

      //! What kind of primitives does this buffer contain? Default triangles
//      GLenum Primitive;
    };

  } // end namespace scene
} // end namespace irr

#endif


