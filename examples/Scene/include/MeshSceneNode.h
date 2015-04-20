// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2015 Vincent Lejeune
// Contains code from the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef MESHSCENENODE_H
#define MESHSCENENODE_H

#include <GfxApi.h>

#ifdef GLBUILD
#include <GL/glew.h>
#include <GLAPI/VAO.h>
#include <GLAPI/Texture.h>
#include <GLAPI/GLS3DVertex.h>
#endif

#ifdef DXBUILD
#include <D3DAPI/Texture.h>
#include <D3DAPI/VAO.h>
#include <D3DAPI/ConstantBuffer.h>
#endif

#include <Core/SColor.h>
#include <Maths/matrix4.h>
#include <Core/ISkinnedMesh.h>
#include <ISceneNode.h>

#include <Core/BasicVertexLayout.h>
#include <TextureManager.h>


namespace irr
{
  namespace scene
  {
    class IMeshSceneNode;
  }
}

namespace irr
{
  namespace video
  {
    struct DrawData {
#ifdef GLBUILD
      GLuint vao;
      GLuint vertex_buffer;
      GLuint index_buffer;
      GLenum PrimitiveType;
#endif
      struct WrapperDescriptorHeap *descriptors;
      size_t IndexCount;
      size_t Stride;
      core::matrix4 TextureMatrix;
      size_t vaoBaseVertex;
      size_t vaoOffset;
//      video::E_VERTEX_TYPE VAOType;
      uint64_t TextureHandles[6];
      const irr::scene::IMeshSceneNode *object;
    };
  }

  namespace scene
  {
    struct ObjectData
    {
      float Model[16];
      float InverseModel[16];
    };
    //! A scene node displaying a static mesh
    class IMeshSceneNode : public ISceneNode
    {
    private:
      const ISkinnedMesh* Mesh;
      std::vector<irr::video::DrawData> DrawDatas;
      WrapperResource *cbuffer;
      WrapperIndexVertexBuffersSet *PackedVertexBuffer;
#ifdef DXBUILD
      std::vector<Microsoft::WRL::ComPtr<ID3D12Resource> > Tex;
#endif

    public:

      //! Constructor
      /** Use setMesh() to set the mesh to display.
      */
      IMeshSceneNode(ISceneNode* parent,
        const core::vector3df& position = core::vector3df(0, 0, 0),
        const core::vector3df& rotation = core::vector3df(0, 0, 0),
        const core::vector3df& scale = core::vector3df(1.f, 1.f, 1.f));

      ~IMeshSceneNode();

      const std::vector<irr::video::DrawData> getDrawDatas() const
      {
        return DrawDatas;
      }

      //! Sets a new mesh to display
      /** \param mesh Mesh to display. */
      void setMesh(const ISkinnedMesh* mesh);

      //! Get the currently defined mesh for display.
      /** \return Pointer to mesh which is displayed by this node. */
      virtual const ISkinnedMesh* getMesh(void)
      {
        return Mesh;
      }

      void render();


      WrapperResource *getConstantBuffer() const
      {
        return cbuffer;
      }

      WrapperIndexVertexBuffersSet * getVAO()
      {
        return PackedVertexBuffer;
      }
    };

  } // end namespace scene
} // end namespace irr


#endif
