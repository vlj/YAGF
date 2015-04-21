// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Scene.h>

#include <Shaders.h>
#include <RenderTargets.h>

#ifdef DXBUILD
#include <d3dapi.h>
#endif

struct ViewBuffer
{
  float ViewProj[16];
};

Scene::Scene()
  {
    cmdList = GlobalGFXAPI->createCommandList();
    cbuffer = GlobalGFXAPI->createConstantsBuffer(sizeof(ViewBuffer));
    cbufferDescriptorHeap = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap({ std::make_tuple(cbuffer, RESOURCE_VIEW::CONSTANTS_BUFFER, 0) });
    SamplersHeap = GlobalGFXAPI->createSamplerHeap({ {SAMPLER_TYPE::ANISOTROPIC, 0 } });
    object = createObjectShader();
  }

Scene::~Scene()
  {
    for (irr::scene::IMeshSceneNode* node : Meshes)
      delete node;
    GlobalGFXAPI->releasePSO(object);
    GlobalGFXAPI->releaseCommandList(cmdList);
    GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(cbufferDescriptorHeap);
    GlobalGFXAPI->releaseSamplerHeap(SamplersHeap);
  }

  void Scene:: update()
  {
    for (irr::scene::IMeshSceneNode* mesh : Meshes)
    {
      mesh->updateAbsolutePosition();
      mesh->render();
    }
  }

  irr::scene::IMeshSceneNode *Scene::addMeshSceneNode(const irr::scene::ISkinnedMesh *mesh,
    irr::scene::ISceneNode* parent,
    const irr::core::vector3df& position,
    const irr::core::vector3df& rotation,
    const irr::core::vector3df& scale)
  {
    irr::scene::IMeshSceneNode* node = new irr::scene::IMeshSceneNode(parent, position, rotation, scale);
    node->setMesh(mesh);
    Meshes.push_back(node);
    return node;
  }

  void Scene::renderGBuffer(const class Camera*, RenderTargets &rtts)
  {
    ViewBuffer cbufdata;
    irr::core::matrix4 View;
    View.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
    memcpy(&cbufdata, View.pointer(), 16 * sizeof(float));

    GlobalGFXAPI->openCommandList(cmdList);
    GlobalGFXAPI->setPipelineState(cmdList, object);
    float clearColor[] = { 0.f, 0.f, 0.f, 0.f };
    GlobalGFXAPI->clearRTTSet(cmdList, rtts.getRTTSet(RenderTargets::FBO_GBUFFER), clearColor);
    GlobalGFXAPI->clearDepthStencilFromRTTSet(cmdList, rtts.getRTTSet(RenderTargets::FBO_GBUFFER), 1., 0);
    void *mappedCBuffer = GlobalGFXAPI->mapConstantsBuffer(cbuffer);
    memcpy(mappedCBuffer, &cbufdata, sizeof(ViewBuffer));
    GlobalGFXAPI->unmapConstantsBuffers(cbuffer);


    GlobalGFXAPI->setRTTSet(cmdList, rtts.getRTTSet(RenderTargets::FBO_GBUFFER));
    GlobalGFXAPI->setDescriptorHeap(cmdList, 0, cbufferDescriptorHeap);
    GlobalGFXAPI->setDescriptorHeap(cmdList, 2, SamplersHeap);

    for (irr::scene::IMeshSceneNode* node : Meshes)
    {
      GlobalGFXAPI->setIndexVertexBuffersSet(cmdList, node->getVAO());

      for (irr::video::DrawData drawdata : node->getDrawDatas())
      {
        GlobalGFXAPI->setDescriptorHeap(cmdList, 1, drawdata.descriptors);
        GlobalGFXAPI->drawIndexedInstanced(cmdList, drawdata.IndexCount, 1, drawdata.vaoOffset, drawdata.vaoBaseVertex, 0);
      }
    }
    GlobalGFXAPI->closeCommandList(cmdList);
    GlobalGFXAPI->submitToQueue(cmdList);
  }