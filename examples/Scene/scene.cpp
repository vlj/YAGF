// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Scene.h>

#include <Material.h>
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
    SamplersHeap = GlobalGFXAPI->createSamplerHeap({ 0 });
  }

Scene::~Scene()
  {
    for (irr::scene::IMeshSceneNode* node : Meshes)
      delete node;
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

    float clearColor[] = { 0.f, 0.f, 0.f, 0.f };
    GlobalGFXAPI->clearRTTSet(cmdList, rtts.getRTTSet(RenderTargets::FBO_GBUFFER), clearColor);
    GlobalGFXAPI->clearDepthStencilFromRTTSet(cmdList, rtts.getRTTSet(RenderTargets::FBO_GBUFFER), 1., 0);
    void *mappedCBuffer = GlobalGFXAPI->mapConstantsBuffer(cbuffer);
    memcpy(mappedCBuffer, &cbufdata, sizeof(ViewBuffer));
    GlobalGFXAPI->unmapConstantsBuffers(cbuffer);

    WrapperPipelineState *object = createObjectShader();
    GlobalGFXAPI->setRTTSet(cmdList, rtts.getRTTSet(RenderTargets::FBO_GBUFFER));
    GlobalGFXAPI->setPipelineState(cmdList, object);
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

#ifdef GLBUILD
    rtts.getRTTSet(RenderTargets::FBO_GBUFFER)->GLValue.BlitToDefault(0, 0, 1024, 1024);
#endif
#ifdef DXBUILD
    ID3D12GraphicsCommandList *cmdlist = cmdList->D3DValue.CommandList;
    ID3D12Resource *gbuffer_base_color = rtts.getRTT(RenderTargets::GBUFFER_BASE_COLOR)->D3DValue.resource;
    GlobalGFXAPI->writeResourcesTransitionBarrier(cmdList, { std::make_tuple(rtts.getRTT(RenderTargets::GBUFFER_BASE_COLOR), RESOURCE_USAGE::RENDER_TARGET, RESOURCE_USAGE::COPY_SRC) });
    cmdlist->ResourceBarrier(1, &setResourceTransitionBarrier(Context::getInstance()->getCurrentBackBuffer(), D3D12_RESOURCE_USAGE_PRESENT, D3D12_RESOURCE_USAGE_COPY_DEST));
    D3D12_TEXTURE_COPY_LOCATION src = {}, dst = {};
    src.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
    src.pResource = gbuffer_base_color;
    dst.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
    dst.pResource = Context::getInstance()->getCurrentBackBuffer();
    D3D12_BOX box = { 0, 0, 0, 1008, 985, 1 };
    cmdlist->CopyTextureRegion(&dst, 0, 0, 0, &src, &box, D3D12_COPY_NONE);
    GlobalGFXAPI->writeResourcesTransitionBarrier(cmdList, { std::make_tuple(rtts.getRTT(RenderTargets::GBUFFER_BASE_COLOR), RESOURCE_USAGE::COPY_SRC, RESOURCE_USAGE::RENDER_TARGET) });
    cmdlist->ResourceBarrier(1, &setResourceTransitionBarrier(Context::getInstance()->getCurrentBackBuffer(), D3D12_RESOURCE_USAGE_COPY_DEST, D3D12_RESOURCE_USAGE_PRESENT));
    GlobalGFXAPI->closeCommandList(cmdList);
    Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)&cmdlist);
    HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
    cmdList->D3DValue.CommandAllocator->Reset();
    cmdlist->Reset(cmdList->D3DValue.CommandAllocator, nullptr);
    Context::getInstance()->Swap();
#endif
  }