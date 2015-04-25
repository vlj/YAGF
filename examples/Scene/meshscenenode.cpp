// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <MeshSceneNode.h>

#ifdef GLBUILD
#include <API/glapi.h>
#endif

#ifdef DXBUILD
#include <API/d3dapi.h>
#endif

static irr::video::DrawData allocateMeshBuffer(
  const irr::scene::SMeshBufferLightMap& mb,
  const irr::scene::IMeshSceneNode *Object)
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

namespace irr
{
  namespace scene
  {
    //! Constructor
    /** Use setMesh() to set the mesh to display.
    */
    IMeshSceneNode::IMeshSceneNode(ISceneNode* parent,
      const core::vector3df& position,
      const core::vector3df& rotation,
      const core::vector3df& scale)
      : ISceneNode(parent, position, rotation, scale)
    {
      cbuffer = GlobalGFXAPI->createConstantsBuffer(sizeof(ObjectData));
    }

    IMeshSceneNode::~IMeshSceneNode()
    {
      GlobalGFXAPI->releaseIndexVertexBuffersSet(PackedVertexBuffer);
      GlobalGFXAPI->releaseConstantsBuffers(cbuffer);
      for (irr::video::DrawData &drawdata : DrawDatas)
        GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(drawdata.descriptors);
    }

    void IMeshSceneNode::setMesh(const irr::scene::ISkinnedMesh* mesh)
    {
      Mesh = mesh;

      const std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > &buffers = Mesh->getMeshBuffers();

      std::vector<irr::scene::SMeshBufferLightMap> reorg;

      for (const std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> &buffer : buffers)
      {
        const irr::scene::SMeshBufferLightMap* mb = &buffer.first;
        DrawDatas.push_back(allocateMeshBuffer(buffer.first, this));
        WrapperResource *WrapperTexture = (WrapperResource*)malloc(sizeof(WrapperResource));
        reorg.push_back(buffer.first);
#ifdef GLBUILD
        WrapperTexture->GLValue.Resource = TextureManager::getInstance()->getTexture(buffer.second.TextureNames[0])->Id;
        WrapperTexture->GLValue.Type = GL_TEXTURE_2D;
#endif

#ifdef DXBUILD
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


        WrapperTexture->D3DValue.resource = Tex.back().Get();
        WrapperTexture->D3DValue.description.TextureView.SRV = TextureInRam->getResourceViewDesc();

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
        DrawDatas.back().descriptors = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap(
        {
          std::make_tuple(cbuffer, RESOURCE_VIEW::CONSTANTS_BUFFER, 1),
          std::make_tuple(WrapperTexture, RESOURCE_VIEW::SHADER_RESOURCE, 0)
        }
        );

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
      PackedVertexBuffer = (WrapperIndexVertexBuffersSet*)malloc(sizeof(WrapperIndexVertexBuffersSet));
#ifdef GLBUILD
      new (&PackedVertexBuffer->GLValue) GLVertexStorage(reorg);
      for (unsigned i = 0; i < DrawDatas.size(); i++)
      {
        irr::video::DrawData &drawdata = DrawDatas[i];
        drawdata.IndexCount = std::get<0>(PackedVertexBuffer->GLValue.meshOffset[i]);
        drawdata.vaoOffset = std::get<2>(PackedVertexBuffer->GLValue.meshOffset[i]);
        drawdata.vaoBaseVertex = std::get<1>(PackedVertexBuffer->GLValue.meshOffset[i]);
      }
#endif
#ifdef DXBUILD
      new (&PackedVertexBuffer->D3DValue) FormattedVertexStorage(Context::getInstance()->cmdqueue.Get(), reorg);
      for (unsigned i = 0; i < DrawDatas.size(); i++)
      {
        irr::video::DrawData &drawdata = DrawDatas[i];
        drawdata.IndexCount = std::get<0>(PackedVertexBuffer->D3DValue.meshOffset[i]);
        drawdata.vaoOffset = std::get<2>(PackedVertexBuffer->D3DValue.meshOffset[i]);
        drawdata.vaoBaseVertex = std::get<1>(PackedVertexBuffer->D3DValue.meshOffset[i]);
      }
#endif
    }

    void IMeshSceneNode::render()
    {
      core::matrix4 invmodel;
      AbsoluteTransformation.getInverse(invmodel);
      ObjectData objdt;
      memcpy(objdt.Model, AbsoluteTransformation.pointer(), 16 * sizeof(float));
      memcpy(objdt.InverseModel, invmodel.pointer(), 16 * sizeof(float));
      void *ptr = GlobalGFXAPI->mapConstantsBuffer(cbuffer);
      memcpy(ptr, &objdt, sizeof(ObjectData));
      GlobalGFXAPI->unmapConstantsBuffers(cbuffer);
    }
  }
}