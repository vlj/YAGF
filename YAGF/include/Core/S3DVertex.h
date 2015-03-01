// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __S_3D_VERTEX_H_INCLUDED__
#define __S_3D_VERTEX_H_INCLUDED__

#include <Maths/vector3d.h>
#include <Maths/vector2d.h>
#include <Core/SColor.h>

namespace irr
{
namespace video
{

//! Enumeration for all vertex types there are.
enum E_VERTEX_TYPE
{
	//! Standard vertex type used by the Irrlicht engine, video::S3DVertex.
	EVT_STANDARD = 0,

	//! Vertex with two texture coordinates, video::S3DVertex2TCoords.
	/** Usually used for geometry with lightmaps or other special materials. */
	EVT_2TCOORDS,

	//! Vertex with a tangent and binormal vector, video::S3DVertexTangents.
	/** Usually used for tangent space normal mapping. */
	EVT_TANGENTS
};

//! Array holding the built in vertex type names
const char* const sBuiltInVertexTypeNames[] =
{
	"standard",
	"2tcoords",
	"tangents",
	0
};

//! standard vertex used by the Irrlicht engine.
struct S3DVertex
{
	//! default constructor
	S3DVertex() {}

	//! constructor
	S3DVertex(float x, float y, float z, float nx, float ny, float nz, SColor c, float tu, float tv)
		: Pos(x,y,z), Normal(nx,ny,nz), Color(c), TCoords(tu,tv) {}

	//! constructor
	S3DVertex(const core::vector3df& pos, const core::vector3df& normal,
		SColor color, const core::vector2d<float>& tcoords)
		: Pos(pos), Normal(normal), Color(color), TCoords(tcoords) {}

	//! Position
	core::vector3df Pos;

	//! Normal vector
	core::vector3df Normal;

	//! Color
	SColor Color;

	//! Texture coordinates
	core::vector2d<float> TCoords;

	bool operator==(const S3DVertex& other) const
	{
		return ((Pos == other.Pos) && (Normal == other.Normal) &&
			(Color == other.Color) && (TCoords == other.TCoords));
	}

	bool operator!=(const S3DVertex& other) const
	{
		return ((Pos != other.Pos) || (Normal != other.Normal) ||
			(Color != other.Color) || (TCoords != other.TCoords));
	}

	bool operator<(const S3DVertex& other) const
	{
		return ((Pos < other.Pos) ||
				((Pos == other.Pos) && (Normal < other.Normal)) ||
				((Pos == other.Pos) && (Normal == other.Normal) && (Color < other.Color)) ||
				((Pos == other.Pos) && (Normal == other.Normal) && (Color == other.Color) && (TCoords < other.TCoords)));
	}

	E_VERTEX_TYPE getType() const
	{
		return EVT_STANDARD;
	}

	S3DVertex getInterpolated(const S3DVertex& other, float d)
	{
//		d = core::clamp(d, 0.0f, 1.0f);
		return S3DVertex(Pos.getInterpolated(other.Pos, d),
				Normal.getInterpolated(other.Normal, d),
				Color.getInterpolated(other.Color, d),
				TCoords.getInterpolated(other.TCoords, d));
	}
};


//! Vertex with two texture coordinates.
/** Usually used for geometry with lightmaps
or other special materials.
*/
struct S3DVertex2TCoords : public S3DVertex
{
	//! default constructor
	S3DVertex2TCoords() : S3DVertex() {}

	//! constructor with two different texture coords, but no normal
	S3DVertex2TCoords(float x, float y, float z, SColor c, float tu, float tv, float tu2, float tv2)
		: S3DVertex(x,y,z, 0.0f, 0.0f, 0.0f, c, tu,tv), TCoords2(tu2,tv2) {}

	//! constructor with two different texture coords, but no normal
	S3DVertex2TCoords(const core::vector3df& pos, SColor color,
		const core::vector2d<float>& tcoords, const core::vector2d<float>& tcoords2)
		: S3DVertex(pos, core::vector3df(), color, tcoords), TCoords2(tcoords2) {}

	//! constructor with all values
	S3DVertex2TCoords(const core::vector3df& pos, const core::vector3df& normal, const SColor& color,
		const core::vector2d<float>& tcoords, const core::vector2d<float>& tcoords2)
		: S3DVertex(pos, normal, color, tcoords), TCoords2(tcoords2) {}

	//! constructor with all values
	S3DVertex2TCoords(float x, float y, float z, float nx, float ny, float nz, SColor c, float tu, float tv, float tu2, float tv2)
		: S3DVertex(x,y,z, nx,ny,nz, c, tu,tv), TCoords2(tu2,tv2) {}

	//! constructor with the same texture coords and normal
	S3DVertex2TCoords(float x, float y, float z, float nx, float ny, float nz, SColor c, float tu, float tv)
		: S3DVertex(x,y,z, nx,ny,nz, c, tu,tv), TCoords2(tu,tv) {}

	//! constructor with the same texture coords and normal
	S3DVertex2TCoords(const core::vector3df& pos, const core::vector3df& normal,
		SColor color, const core::vector2d<float>& tcoords)
		: S3DVertex(pos, normal, color, tcoords), TCoords2(tcoords) {}

	//! constructor from S3DVertex
	S3DVertex2TCoords(S3DVertex& o) : S3DVertex(o) {}

	//! Second set of texture coordinates
	core::vector2d<float> TCoords2;

	//! Equality operator
	bool operator==(const S3DVertex2TCoords& other) const
	{
		return ((static_cast<S3DVertex>(*this)==other) &&
			(TCoords2 == other.TCoords2));
	}

	//! Inequality operator
	bool operator!=(const S3DVertex2TCoords& other) const
	{
		return ((static_cast<S3DVertex>(*this)!=other) ||
			(TCoords2 != other.TCoords2));
	}

	bool operator<(const S3DVertex2TCoords& other) const
	{
		return ((static_cast<S3DVertex>(*this) < other) ||
				((static_cast<S3DVertex>(*this) == other) && (TCoords2 < other.TCoords2)));
	}

	E_VERTEX_TYPE getType() const
	{
		return EVT_2TCOORDS;
	}

	S3DVertex2TCoords getInterpolated(const S3DVertex2TCoords& other, float d)
	{
//		d = clamp(d, 0.0f, 1.0f);
		return S3DVertex2TCoords(Pos.getInterpolated(other.Pos, d),
				Normal.getInterpolated(other.Normal, d),
				Color.getInterpolated(other.Color, d),
				TCoords.getInterpolated(other.TCoords, d),
				TCoords2.getInterpolated(other.TCoords2, d));
	}
};


//! Vertex with a tangent and binormal vector.
/** Usually used for tangent space normal mapping. */
struct S3DVertexTangents : public S3DVertex
{
	//! default constructor
	S3DVertexTangents() : S3DVertex() { }

	//! constructor
	S3DVertexTangents(float x, float y, float z, float nx=0.0f, float ny=0.0f, float nz=0.0f,
			SColor c = 0xFFFFFFFF, float tu=0.0f, float tv=0.0f,
			float tanx=0.0f, float tany=0.0f, float tanz=0.0f,
			float bx=0.0f, float by=0.0f, float bz=0.0f)
		: S3DVertex(x,y,z, nx,ny,nz, c, tu,tv), Tangent(tanx,tany,tanz), Binormal(bx,by,bz) { }

	//! constructor
	S3DVertexTangents(const core::vector3df& pos, SColor c,
		const core::vector2df& tcoords)
		: S3DVertex(pos, core::vector3df(), c, tcoords) { }

	//! constructor
	S3DVertexTangents(const core::vector3df& pos,
		const core::vector3df& normal, SColor c,
		const core::vector2df& tcoords,
		const core::vector3df& tangent=core::vector3df(),
		const core::vector3df& binormal=core::vector3df())
		: S3DVertex(pos, normal, c, tcoords), Tangent(tangent), Binormal(binormal) { }

	//! Tangent vector along the x-axis of the texture
	core::vector3df Tangent;

	//! Binormal vector (tangent x normal)
	core::vector3df Binormal;

	bool operator==(const S3DVertexTangents& other) const
	{
		return ((static_cast<S3DVertex>(*this)==other) &&
			(Tangent == other.Tangent) &&
			(Binormal == other.Binormal));
	}

	bool operator!=(const S3DVertexTangents& other) const
	{
		return ((static_cast<S3DVertex>(*this)!=other) ||
			(Tangent != other.Tangent) ||
			(Binormal != other.Binormal));
	}

	bool operator<(const S3DVertexTangents& other) const
	{
		return ((static_cast<S3DVertex>(*this) < other) ||
				((static_cast<S3DVertex>(*this) == other) && (Tangent < other.Tangent)) ||
				((static_cast<S3DVertex>(*this) == other) && (Tangent == other.Tangent) && (Binormal < other.Binormal)));
	}

	E_VERTEX_TYPE getType() const
	{
		return EVT_TANGENTS;
	}

	S3DVertexTangents getInterpolated(const S3DVertexTangents& other, float d)
	{
//		d = clamp(d, 0.0f, 1.0f);
		return S3DVertexTangents(Pos.getInterpolated(other.Pos, d),
				Normal.getInterpolated(other.Normal, d),
				Color.getInterpolated(other.Color, d),
				TCoords.getInterpolated(other.TCoords, d),
				Tangent.getInterpolated(other.Tangent, d),
				Binormal.getInterpolated(other.Binormal, d));
	}
};



inline unsigned getVertexPitchFromType(E_VERTEX_TYPE vertexType)
{
	switch (vertexType)
	{
	case video::EVT_2TCOORDS:
		return sizeof(video::S3DVertex2TCoords);
	case video::EVT_TANGENTS:
		return sizeof(video::S3DVertexTangents);
	default:
		return sizeof(video::S3DVertex);
	}
}


} // end namespace video
} // end namespace irr

/*#include <unordered_map>

namespace std {
	template <>
	struct hash<irr::video::S3DVertex>
	{
		std::size_t hashVector3d(const irr::core::vector3df &v) const
		{
			return hash<float>()(v.X) ^ hash<float>()(v.Y) ^ hash<float>()(v.Z);
		}

		std::size_t hashVector2d(const irr::core::vector2df &v) const
		{
			return hash<float>()(v.X) ^ hash<float>()(v.Y);
		}

		std::size_t operator()(const irr::video::S3DVertex& v) const
		{
			return hashVector3d(v.Pos) ^ hashVector3d(v.Normal) ^ hashVector2d(v.TCoords);
		}
	};

	template <>
	struct hash<irr::video::S3DVertex2TCoords>
	{
		std::size_t hashVector3d(const irr::core::vector3df &v) const
		{
			return hash<float>()(v.X) ^ hash<float>()(v.Y) ^ hash<float>()(v.Z);
		}

		std::size_t hashVector2d(const irr::core::vector2df &v) const
		{
			return hash<float>()(v.X) ^ hash<float>()(v.Y);
		}

		std::size_t operator()(const irr::video::S3DVertex2TCoords& v) const
		{
			return hashVector3d(v.Pos) ^ hashVector3d(v.Normal) ^ hashVector2d(v.TCoords) ^ hashVector2d(v.TCoords2);
		}
	};

	template <>
	struct hash<irr::video::S3DVertexTangents>
	{
		std::size_t hashVector3d(const irr::core::vector3df &v) const
		{
			return hash<float>()(v.X) ^ hash<float>()(v.Y) ^ hash<float>()(v.Z);
		}

		std::size_t hashVector2d(const irr::core::vector2df &v) const
		{
			return hash<float>()(v.X) ^ hash<float>()(v.Y);
		}

		std::size_t operator()(const irr::video::S3DVertexTangents& v) const
		{
			return hashVector3d(v.Pos) ^ hashVector3d(v.Normal) ^ hashVector2d(v.TCoords) ^ hashVector3d(v.Tangent) ^ hashVector3d(v.Binormal);
		}
	};
}*/

template<typename VT>
struct VertexAttribBinder
{
public:
    static void bind();
};


template<>
struct VertexAttribBinder <uint16_t>
{
public:
    static void bind()
    {}
};

template<>
struct VertexAttribBinder<irr::video::S3DVertex>
{
public:
    static void bind()
    {
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
    }
};

template<>
struct VertexAttribBinder<irr::video::S3DVertex2TCoords>
{
public:
    static void bind()
    {
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
    }
};

template<>
struct VertexAttribBinder<irr::video::S3DVertexTangents>
{
public:
    static void bind()
    {
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
    }
};

#endif

