// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __SCENE_H__
#define __SCENE_H__
#include <list>
#include <ISceneNode.h>
#include <MeshSceneNode.h>

#include <Material.h>
#include <RenderTargets.h>

#ifdef GLBUILD
#include <GLAPI/Samplers.h>
#include <GLAPI/VAO.h>
#endif

#ifdef DXBUILD
#include <d3dapi.h>
#include <D3DAPI/Sampler.h>
#endif

struct ViewBuffer
{
  float ViewProj[16];
};

class Scene
{
private:
  std::list<irr::scene::IMeshSceneNode*> Meshes;
  WrapperCommandList* cmdList;
  WrapperResource *cbuffer;
#ifdef DXBUILD
  // Should be tied to view rather than scene
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbufferDescriptorHeap;

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Sampler;
#endif
#ifdef GLBUILD
  GLuint TrilinearSampler;
#endif
public:
  Scene()
  {
    cmdList = GlobalGFXAPI->createCommandList();
    cbuffer = GlobalGFXAPI->createConstantsBuffer(sizeof(ViewBuffer));
#ifdef GLBUILD
    TrilinearSampler = SamplerHelper::createTrilinearSampler();
#endif
#ifdef DXBUILD
    cbufferDescriptorHeap = createDescriptorHeap(Context::getInstance()->dev.Get(), 1, D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP, true);
    Context::getInstance()->dev->CreateConstantBufferView(&cbuffer->D3DValue.description, cbufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    Sampler = createDescriptorHeap(Context::getInstance()->dev.Get(), 1, D3D12_SAMPLER_DESCRIPTOR_HEAP, true);

    Context::getInstance()->dev->CreateSampler(&Samplers::getTrilinearSamplerDesc(), Sampler->GetCPUDescriptorHandleForHeapStart());
#endif
  }

  ~Scene()
  {
    for (irr::scene::IMeshSceneNode* node : Meshes)
      delete node;
#ifdef GLBUILD
    glDeleteSamplers(1, &TrilinearSampler);
#endif
  }

  void update()
  {
    for (irr::scene::IMeshSceneNode* mesh : Meshes)
    {
      mesh->updateAbsolutePosition();
      mesh->render();
    }
  }

  irr::scene::IMeshSceneNode *addMeshSceneNode(const irr::scene::ISkinnedMesh *mesh, irr::scene::ISceneNode* parent,
    const irr::core::vector3df& position =irr:: core::vector3df(0, 0, 0),
    const irr::core::vector3df& rotation = irr::core::vector3df(0, 0, 0),
    const irr::core::vector3df& scale = irr::core::vector3df(1.f, 1.f, 1.f))
  {
    irr::scene::IMeshSceneNode* node = new irr::scene::IMeshSceneNode(parent, position, rotation, scale);
    node->setMesh(mesh);
    Meshes.push_back(node);
    return node;
  }

  void renderGBuffer(const class Camera*, RenderTargets &rtts)
  {
    ViewBuffer cbufdata;
    irr::core::matrix4 View;
    View.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
    memcpy(&cbufdata, View.pointer(), 16 * sizeof(float));

    float clearColor[] = { 0.f, 0.f, 0.f, 0.f };
    GlobalGFXAPI->clearRTTSet(cmdList, rtts.getRTTSet(RenderTargets::FBO_GBUFFER), clearColor);
    void *mappedCBuffer = GlobalGFXAPI->mapConstantsBuffer(cbuffer);
    memcpy(mappedCBuffer, &cbufdata, sizeof(ViewBuffer));
    GlobalGFXAPI->unmapConstantsBuffers(cbuffer);

    WrapperPipelineState *object = createObjectShader();
#ifdef GLBUILD
    glClearDepth(1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
#ifdef DXBUILD
    cmdList->D3DValue.CommandList->ClearDepthStencilView(rtts.getDepthDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_DEPTH, 1., 0, nullptr, 0);
#endif
    GlobalGFXAPI->setRTTSet(cmdList, rtts.getRTTSet(RenderTargets::FBO_GBUFFER));
#ifdef GLBUILD
    const GLuint& tmp = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords> >::getInstance()->getVAO();
    WrapperIndexVertexBuffersSet *vao = (WrapperIndexVertexBuffersSet*) &tmp;
#endif
    GlobalGFXAPI->setPipelineState(cmdList, object);
#ifdef DXBUILD

    ID3D12DescriptorHeap *descriptorlst[] =
    {
      cbufferDescriptorHeap.Get()
    };
    ID3D12GraphicsCommandList *cmdlist = cmdList->D3DValue.CommandList;
    cmdlist->SetDescriptorHeaps(descriptorlst, 1);
    cmdlist->SetGraphicsRootDescriptorTable(0, cbufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#endif

    for (irr::scene::IMeshSceneNode* node : Meshes)
    {
#ifdef DXBUILD
      WrapperIndexVertexBuffersSet *vao = (WrapperIndexVertexBuffersSet *)(node->getVAO());
#endif
      GlobalGFXAPI->setIndexVertexBuffersSet(cmdList, vao);

      for (irr::video::DrawData drawdata : node->getDrawDatas())
      {
#ifdef GLBUILD
        ObjectShader::getInstance()->SetTextureUnits(node->getConstantBuffer()->GLValue, cbuffer->GLValue, drawdata.textures[0]->Id, TrilinearSampler);
#endif
#ifdef DXBUILD
        cmdlist->SetGraphicsRootDescriptorTable(1, drawdata.descriptors->GetGPUDescriptorHandleForHeapStart());
        cmdlist->SetGraphicsRootDescriptorTable(2, drawdata.descriptors->GetGPUDescriptorHandleForHeapStart().MakeOffsetted(Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP)));
        cmdlist->SetGraphicsRootDescriptorTable(3, Sampler->GetGPUDescriptorHandleForHeapStart());
#endif
        GlobalGFXAPI->drawIndexedInstanced(cmdList, drawdata.IndexCount, 1, drawdata.vaoOffset, drawdata.vaoBaseVertex, 0);
      }
    }

#ifdef GLBUILD
    rtts.getRTTSet(RenderTargets::FBO_GBUFFER)->GLValue.BlitToDefault(0, 0, 1024, 1024);
#endif
#ifdef DXBUILD
    ID3D12Resource *gbuffer_base_color = rtts.getRTT(RenderTargets::GBUFFER_BASE_COLOR)->D3DValue.resource;
    GlobalGFXAPI->writeResourcesTransitionBarrier(cmdList, { std::make_tuple(rtts.getRTT(RenderTargets::GBUFFER_BASE_COLOR), GFXAPI::RENDER_TARGET, GFXAPI::COPY_SRC) });
    cmdlist->ResourceBarrier(1, &setResourceTransitionBarrier(Context::getInstance()->getCurrentBackBuffer(), D3D12_RESOURCE_USAGE_PRESENT, D3D12_RESOURCE_USAGE_COPY_DEST));
    D3D12_TEXTURE_COPY_LOCATION src = {}, dst = {};
    src.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
    src.pResource = gbuffer_base_color;
    dst.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
    dst.pResource = Context::getInstance()->getCurrentBackBuffer();
    D3D12_BOX box = {0, 0, 0, 1008, 985, 1};
    cmdlist->CopyTextureRegion(&dst, 0, 0, 0, &src, &box, D3D12_COPY_NONE);
    GlobalGFXAPI->writeResourcesTransitionBarrier(cmdList, { std::make_tuple(rtts.getRTT(RenderTargets::GBUFFER_BASE_COLOR), GFXAPI::COPY_SRC, GFXAPI::RENDER_TARGET) });
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
};

#endif