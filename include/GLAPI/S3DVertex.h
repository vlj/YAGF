// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __BASIC_VERTEX_LAYOUT_H__
#define __BASIC_VERTEX_LAYOUT_H__

#include <Core/BasicVertexLayout.h>

template<>
struct VertexAttribBinder < uint16_t >
{
public:
static void bind()
{}
};

template<>
struct VertexAttribBinder < irr::video::S3DVertex >
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
struct VertexAttribBinder < irr::video::S3DVertex2TCoords >
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
struct VertexAttribBinder < irr::video::S3DVertexTangents >
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