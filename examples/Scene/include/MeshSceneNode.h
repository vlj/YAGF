// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2015 Vincent Lejeune
// Contains code from the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef MESHSCENENODE_H
#define MESHSCENENODE_H

#ifdef GLBUILD
#include <GL/glew.h>
#include <GLAPI/VAO.h>
#include <GLAPI/Texture.h>
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
#ifdef DXBUILD
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptors;
#endif
      const Texture *textures[8];
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
#ifdef GLBUILD
      GLuint cbuffer;
#endif

#ifdef DXBUILD
      ConstantBuffer<ObjectData> cbuffer;
      FormattedVertexStorage<irr::video::S3DVertex2TCoords> *PackedVertexBuffer;
      std::vector<Microsoft::WRL::ComPtr<ID3D12Resource> > Tex;
#endif

      const ISkinnedMesh* Mesh;
      std::vector<irr::video::DrawData> DrawDatas;

      irr::video::DrawData allocateMeshBuffer(const irr::scene::SMeshBufferLightMap& mb, const IMeshSceneNode *Object)
      {
        irr::video::DrawData result = {};
        result.object = Object;
        result.IndexCount = mb.getIndexCount();

//        result.VAOType = mb->getVertexType();
        result.Stride = getVertexPitchFromType(irr::video::EVT_2TCOORDS);

/*        switch (mb->getPrimitiveType())
        {
        case scene::EPT_POINTS:
          result.PrimitiveType = GL_POINTS;
          break;
        case scene::EPT_TRIANGLE_STRIP:
          result.PrimitiveType = GL_TRIANGLE_STRIP;
          break;
        case scene::EPT_TRIANGLE_FAN:
          result.PrimitiveType = GL_TRIANGLE_FAN;
          break;
        case scene::EPT_LINES:
          result.PrimitiveType = GL_LINES;
          break;
        case scene::EPT_TRIANGLES:
          result.PrimitiveType = GL_TRIANGLES;
          break;
        case scene::EPT_POINT_SPRITES:
        case scene::EPT_LINE_LOOP:
        case scene::EPT_POLYGON:
        case scene::EPT_LINE_STRIP:
        case scene::EPT_QUAD_STRIP:
        case scene::EPT_QUADS:
          assert(0 && "Unsupported primitive type");
        }
//        for (unsigned i = 0; i < 8; i++)
//          result.textures[i] = mb->getMaterial().getTexture(i);
        result.TextureMatrix = 0;
        result.VAOType = mb->getVertexType();*/
        return result;
      }

    public:

      //! Constructor
      /** Use setMesh() to set the mesh to display.
      */
      IMeshSceneNode(ISceneNode* parent,
        const core::vector3df& position = core::vector3df(0, 0, 0),
        const core::vector3df& rotation = core::vector3df(0, 0, 0),
        const core::vector3df& scale = core::vector3df(1.f, 1.f, 1.f))
        : ISceneNode(parent, position, rotation, scale) {
#ifdef GLBUILD
        glGenBuffers(1, &cbuffer);
#endif
#ifdef DXBUILD
        PackedVertexBuffer = nullptr;
#endif
      }

      ~IMeshSceneNode()
      {
#ifdef GLBUILD
        glDeleteBuffers(1, &cbuffer);
#endif
#ifdef DXBUILD
        delete PackedVertexBuffer;
#endif
      }

      const std::vector<irr::video::DrawData> getDrawDatas() const
      {
        return DrawDatas;
      }

      //! Sets a new mesh to display
      /** \param mesh Mesh to display. */
      void setMesh(const ISkinnedMesh* mesh)
      {
        Mesh = mesh;

        const std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > &buffers = Mesh->getMeshBuffers();

#ifdef DXBUILD
        std::vector<irr::scene::SMeshBufferLightMap> reorg;
#endif

        for (const std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> &buffer: buffers)
        {
          const irr::scene::SMeshBufferLightMap* mb = &buffer.first;
          DrawDatas.push_back(allocateMeshBuffer(buffer.first, this));

#ifdef GLBUILD
          DrawDatas.back().textures[0] = TextureManager::getInstance()->getTexture(buffer.second.TextureNames[0]);
          std::pair<size_t, size_t> p = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords> >::getInstance()->getBase(mb);
          DrawDatas.back().vaoBaseVertex = p.first;
          DrawDatas.back().vaoOffset = p.second;
#endif

#ifdef DXBUILD
          reorg.push_back(buffer.first);

          const Texture* TextureInRam = TextureManager::getInstance()->getTexture(buffer.second.TextureNames[0]);

          Tex.emplace_back();

          HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
            &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_MISC_NONE,
            &CD3D12_RESOURCE_DESC::Tex2D(TextureInRam->getFormat(), (UINT)TextureInRam->getWidth(), (UINT)TextureInRam->getHeight(), 1, (UINT16)TextureInRam->getMipLevelsCount()),
            D3D12_RESOURCE_USAGE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&Tex.back())
            );

          D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
          heapdesc.Type = D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP;
          heapdesc.NumDescriptors = 2;
          heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
          hr = Context::getInstance()->dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&DrawDatas.back().descriptors));

          Context::getInstance()->dev->CreateConstantBufferView(&cbuffer.getDesc(), DrawDatas.back().descriptors->GetCPUDescriptorHandleForHeapStart());
          Context::getInstance()->dev->CreateShaderResourceView(Tex.back().Get(), &TextureInRam->getResourceViewDesc(), DrawDatas.back().descriptors->GetCPUDescriptorHandleForHeapStart().MakeOffsetted(Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP)));

          Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdalloc;
          Context::getInstance()->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));
          Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdlist;
          Context::getInstance()->dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), nullptr, IID_PPV_ARGS(&cmdlist));

          TextureInRam->CreateUploadCommandToResourceInDefaultHeap(cmdlist.Get(), Tex.back().Get());

          cmdlist->Close();
          Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());
          HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
          WaitForSingleObject(handle, INFINITE);
          CloseHandle(handle);
#endif


//          video::E_MATERIAL_TYPE type = mb->getMaterial().MaterialType;
//          f32 MaterialTypeParam = mb->getMaterial().MaterialTypeParam;
//          video::IMaterialRenderer* rnd = driver->getMaterialRenderer(type);

/*          GLMesh &mesh = GLmeshes[i];
          Material* material = material_manager->getMaterialFor(mb->getMaterial().getTexture(0), mb);
          if (rnd->isTransparent())
          {
            TransparentMaterial TranspMat = MaterialTypeToTransparentMaterial(type, MaterialTypeParam, material);
            if (!immediate_draw)
              TransparentMesh[TranspMat].push_back(&mesh);
            else
              additive = (TranspMat == TM_ADDITIVE);
          }
          else
          {
            assert(!isDisplacement);
            Material* material2 = NULL;
            if (mb->getMaterial().getTexture(1) != NULL)
              material2 = material_manager->getMaterialFor(mb->getMaterial().getTexture(1), mb);
            Material::ShaderType MatType = MaterialTypeToMeshMaterial(type, mb->getVertexType(), material, material2);
            if (!immediate_draw)
              MeshSolidMaterial[MatType].push_back(&mesh);
          }*/
        }
#ifdef DXBUILD
        PackedVertexBuffer = new FormattedVertexStorage<irr::video::S3DVertex2TCoords>(Context::getInstance()->cmdqueue.Get(), reorg);
        for (unsigned i = 0; i < DrawDatas.size(); i++)
        {
          irr::video::DrawData &drawdata = DrawDatas[i];
          drawdata.IndexCount = std::get<0>(PackedVertexBuffer->meshOffset[i]);
          drawdata.vaoOffset = std::get<2>(PackedVertexBuffer->meshOffset[i]);
          drawdata.vaoBaseVertex = std::get<1>(PackedVertexBuffer->meshOffset[i]);
        }
#endif
      }

      //! Get the currently defined mesh for display.
      /** \return Pointer to mesh which is displayed by this node. */
      virtual const ISkinnedMesh* getMesh(void)
      {
        return Mesh;
      }

      void render()
      {
        core::matrix4 invmodel;
        AbsoluteTransformation.getInverse(invmodel);
        ObjectData objdt;
        memcpy(objdt.Model, AbsoluteTransformation.pointer(), 16 * sizeof(float));
        memcpy(objdt.InverseModel, invmodel.pointer(), 16 * sizeof(float));

#ifdef GLBUILD
        glBindBuffer(GL_UNIFORM_BUFFER, cbuffer);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(ObjectData), &objdt, GL_STATIC_DRAW);
#endif
#ifdef DXBUILD
        memcpy(cbuffer.map(), &objdt, sizeof(ObjectData));
        cbuffer.unmap();
#endif
      }

#ifdef GLBUILD
      GLuint getConstantBuffer() const
      {
        return cbuffer;
      }
#endif

#ifdef DXBUILD
      const FormattedVertexStorage<irr::video::S3DVertex2TCoords> * getVAO() const
      {
        return PackedVertexBuffer;
      }
#endif
    };

  } // end namespace scene
} // end namespace irr


#endif
