// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2015 Vincent Lejeune
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_MESH_BUFFER_H_INCLUDED__
#define __I_MESH_BUFFER_H_INCLUDED__

#include <cstdint>
#include <Core/S3DVertex.h>

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
	class IMeshBuffer// : public virtual IReferenceCounted
	{
	public:
		//! Get type of vertex data which is stored in this meshbuffer.
		/** \return Vertex type of this buffer. */
		virtual video::E_VERTEX_TYPE getVertexType() const = 0;

		//! Get access to vertex data. The data is an array of vertices.
		/** Which vertex type is used can be determined by getVertexType().
		\return Pointer to array of vertices. */
		virtual const void* getVertices() const = 0;

		//! Get access to vertex data. The data is an array of vertices.
		/** Which vertex type is used can be determined by getVertexType().
		\return Pointer to array of vertices. */
		virtual void* getVertices() = 0;

		//! Get amount of vertices in meshbuffer.
		/** \return Number of vertices in this buffer. */
		virtual size_t getVertexCount() const = 0;

		//! Get access to Indices.
		/** \return Pointer to indices array. */
		virtual const uint16_t* getIndices() const = 0;

		//! Get access to Indices.
		/** \return Pointer to indices array. */
		virtual uint16_t* getIndices() = 0;

		//! Get amount of indices in this meshbuffer.
		/** \return Number of indices in this buffer. */
		virtual size_t getIndexCount() const = 0;

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
		virtual const core::vector3df& getPosition(size_t i) const = 0;

		//! returns position of vertex i
		virtual core::vector3df& getPosition(size_t i) = 0;

		//! returns normal of vertex i
		virtual const core::vector3df& getNormal(size_t i) const = 0;

		//! returns normal of vertex i
		virtual core::vector3df& getNormal(size_t i) = 0;

		//! returns texture coord of vertex i
		virtual const core::vector2df& getTCoords(size_t i) const = 0;

		//! returns texture coord of vertex i
		virtual core::vector2df& getTCoords(size_t i) = 0;

		//! Returns the primitive type of this buffer
		virtual GLenum getPrimitiveType() const = 0;

		//! Append the vertices and indices to the current buffer
		/** Only works for compatible vertex types.
		\param vertices Pointer to a vertex array.
		\param numVertices Number of vertices in the array.
		\param indices Pointer to index array.
		\param numIndices Number of indices in array. */
		virtual void append(const void* const vertices, size_t numVertices, const uint16_t* const indices, size_t numIndices) = 0;

		//! Append the meshbuffer to the current buffer
		/** Only works for compatible vertex types
		\param other Buffer to append to this one. */
		virtual void append(const IMeshBuffer* const other) = 0;
	};

} // end namespace scene
} // end namespace irr

#endif


