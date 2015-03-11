// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __T_MESH_BUFFER_H_INCLUDED__
#define __T_MESH_BUFFER_H_INCLUDED__

#include <Core/IMeshBuffer.h>
#include <vector>

namespace irr
{
namespace scene
{
	//! Template implementation of the IMeshBuffer interface
	template <class T>
	class CMeshBuffer : public IMeshBuffer
	{
	public:
		//! Default constructor for empty meshbuffer
		CMeshBuffer(): Primitive(GL_TRIANGLES)
		{ }

		//! Get pointer to vertices
		/** \return Pointer to vertices. */
		virtual const void* getVertices() const override
		{
			return Vertices.data();
		}


		//! Get pointer to vertices
		/** \return Pointer to vertices. */
		virtual void* getVertices() override
		{
			return Vertices.data();
		}


		//! Get number of vertices
		/** \return Number of vertices. */
		virtual size_t getVertexCount() const override
		{
			return Vertices.size();
		}

		//! Get pointer to indices
		/** \return Pointer to indices. */
		virtual const uint16_t* getIndices() const override
		{
			return Indices.data();
		}


		//! Get pointer to indices
		/** \return Pointer to indices. */
		virtual uint16_t* getIndices() override
		{
			return Indices.data();
		}


		//! Get number of indices
		/** \return Number of indices. */
		virtual size_t getIndexCount() const override
		{
			return Indices.size();
		}

		//! Get type of vertex data stored in this buffer.
		/** \return Type of vertex data. */
/*		virtual video::E_VERTEX_TYPE getVertexType() const override
		{
			return T().getType();
		}

		//! returns position of vertex i
		virtual const core::vector3df& getPosition(size_t i) const override
		{
			return Vertices[i].Pos;
		}

		//! returns position of vertex i
		virtual core::vector3df& getPosition(size_t i) override
		{
			return Vertices[i].Pos;
		}

		//! returns normal of vertex i
		virtual const core::vector3df& getNormal(size_t i) const override
		{
			return Vertices[i].Normal;
		}

		//! returns normal of vertex i
		virtual core::vector3df& getNormal(size_t i) override
		{
			return Vertices[i].Normal;
		}

		//! returns texture coord of vertex i
		virtual const core::vector2df& getTCoords(size_t i) const override
		{
			return Vertices[i].TCoords;
		}

		//! returns texture coord of vertex i
		virtual core::vector2df& getTCoords(size_t i) override
		{
			return Vertices[i].TCoords;
		}*/

		virtual GLenum getPrimitiveType() const override
		{
			return Primitive;
		}

		//! Append the vertices and indices to the current buffer
		/** Only works for compatible types, i.e. either the same type
		or the main buffer is of standard type. Otherwise, behavior is
		undefined.
		*/
		virtual void append(const void* const vertices, size_t numVertices, const uint16_t* const indices, size_t numIndices) override
		{
			if (vertices == getVertices())
				return;

			const size_t vertexCount = getVertexCount();
			size_t i;

			Vertices.resize(vertexCount+numVertices);
			for (i=0; i<numVertices; ++i)
			{
				Vertices.push_back(reinterpret_cast<const T*>(vertices)[i]);
			}

			Indices.resize(getIndexCount() + numIndices);
			for (i=0; i<numIndices; ++i)
			{
				Indices.push_back(indices[i]+vertexCount);
			}
		}


		//! Append the meshbuffer to the current buffer
		/** Only works for compatible types, i.e. either the same type
		or the main buffer is of standard type. Otherwise, behavior is
		undefined.
		\param other Meshbuffer to be appended to this one.
		*/
		virtual void append(const IMeshBuffer* const other)
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
		GLenum Primitive;
	};

} // end namespace scene
} // end namespace irr

#endif


