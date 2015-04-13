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
#endif

#ifdef DXBUILD
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
#ifdef DXBUILD
  // Should be tied to view rather than scene
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbufferDescriptorHeap;
  ConstantBuffer<ViewBuffer> cbuffer;

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdalloc;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdlist;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Sampler;
#endif
#ifdef GLBUILD
  GLuint cbuf;
  GLuint TrilinearSampler;
#endif
public:
  Scene()
  {
#ifdef GLBUILD
    glGenBuffers(1, &cbuf);
    TrilinearSampler = SamplerHelper::createTrilinearSampler();
#endif
#ifdef DXBUILD
    D3D12_DESCRIPTOR_HEAP_DESC cbuffheapdesc = {};
    cbuffheapdesc.Type = D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP;
    cbuffheapdesc.NumDescriptors = 1;
    cbuffheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
    HRESULT hr = Context::getInstance()->dev->CreateDescriptorHeap(&cbuffheapdesc, IID_PPV_ARGS(&cbufferDescriptorHeap));
    Context::getInstance()->dev->CreateConstantBufferView(&cbuffer.getDesc(), cbufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    hr = Context::getInstance()->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));
    hr = Context::getInstance()->dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), nullptr, IID_PPV_ARGS(&cmdlist));

    D3D12_DESCRIPTOR_HEAP_DESC sampler_heap = {};
    sampler_heap.Type = D3D12_SAMPLER_DESCRIPTOR_HEAP;
    sampler_heap.NumDescriptors = 1;
    sampler_heap.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
    hr = Context::getInstance()->dev->CreateDescriptorHeap(&sampler_heap, IID_PPV_ARGS(&Sampler));

    Context::getInstance()->dev->CreateSampler(&Samplers::getTrilinearSamplerDesc(), Sampler->GetCPUDescriptorHandleForHeapStart());
#endif
  }

  ~Scene()
  {
    for (irr::scene::IMeshSceneNode* node : Meshes)
      delete node;
#ifdef GLBUILD
    glDeleteSamplers(1, &TrilinearSampler);
    glDeleteBuffers(1, &cbuf);
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
#ifdef GLBUILD
    glEnable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    rtts.getFrameBuffer(RenderTargets::FBO_GBUFFER).Bind();

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClearDepth(1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindBuffer(GL_UNIFORM_BUFFER, cbuf);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ViewBuffer), &cbufdata, GL_STATIC_DRAW);

    glUseProgram(ObjectShader::getInstance()->Program);
    glBindVertexArray(VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords> >::getInstance()->getVAO());
    for (const irr::scene::IMeshSceneNode* node : Meshes)
    {
      for (const irr::video::DrawData &drawdata : node->getDrawDatas())
      {
        ObjectShader::getInstance()->SetTextureUnits(node->getConstantBuffer(), cbuf, drawdata.textures[0]->Id, TrilinearSampler);
        glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)drawdata.IndexCount, GL_UNSIGNED_SHORT, (void *)drawdata.vaoOffset, (GLsizei)drawdata.vaoBaseVertex);
      }
    }
#endif

#ifdef DXBUILD

    D3D12_RECT rect = {};
    rect.left = 0;
    rect.top = 0;
    rect.bottom = 1024;
    rect.right = 1024;

    D3D12_VIEWPORT view = {};
    view.Height = 1024;
    view.Width = 1024;
    view.TopLeftX = 0;
    view.TopLeftY = 0;
    view.MinDepth = 0;
    view.MaxDepth = 1.;

    memcpy(cbuffer.map(), &cbufdata, sizeof(ViewBuffer));
    cbuffer.unmap();

    cmdlist->RSSetViewports(1, &view);
    cmdlist->RSSetScissorRects(1, &rect);

    ID3D12DescriptorHeap *descriptorlst[] =
    {
      cbufferDescriptorHeap.Get()
    };
    cmdlist->SetDescriptorHeaps(descriptorlst, 1);

    cmdlist->ResourceBarrier(1, &setResourceTransitionBarrier(Context::getInstance()->getCurrentBackBuffer(), D3D12_RESOURCE_USAGE_PRESENT, D3D12_RESOURCE_USAGE_RENDER_TARGET));

    float clearColor[] = { .25f, .25f, 0.35f, 1.0f };
    cmdlist->ClearRenderTargetView(Context::getInstance()->getCurrentBackBufferDescriptor(), clearColor, 0, 0);
    cmdlist->ClearDepthStencilView(rtts.getDepthDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_DEPTH, 1., 0, nullptr, 0);

    cmdlist->SetPipelineState(Object::getInstance()->pso.Get());
    cmdlist->SetGraphicsRootSignature(RS::getInstance()->pRootSignature.Get());
    float c[] = { 1., 1., 1., 1. };
    cmdlist->SetGraphicsRootDescriptorTable(0, cbufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    cmdlist->SetRenderTargets(&Context::getInstance()->getCurrentBackBufferDescriptor(), true, 1, &rtts.getDepthDescriptorHeap()->GetCPUDescriptorHandleForHeapStart());
    cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (irr::scene::IMeshSceneNode* node : Meshes)
    {
      cmdlist->SetIndexBuffer(&node->getVAO()->getIndexBufferView());
      cmdlist->SetVertexBuffers(0, node->getVAO()->getVertexBufferView().data(), 1);

      for (irr::video::DrawData drawdata : node->getDrawDatas())
      {
        cmdlist->SetGraphicsRootDescriptorTable(1, drawdata.descriptors->GetGPUDescriptorHandleForHeapStart());
        cmdlist->SetGraphicsRootDescriptorTable(2, drawdata.descriptors->GetGPUDescriptorHandleForHeapStart().MakeOffsetted(Context::getInstance()->dev->GetDescriptorHandleIncrementSize(D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP)));
        cmdlist->SetGraphicsRootDescriptorTable(3, Sampler->GetGPUDescriptorHandleForHeapStart());
        cmdlist->DrawIndexedInstanced((UINT)drawdata.IndexCount, 1, (UINT)drawdata.vaoOffset, (UINT)drawdata.vaoBaseVertex, 0);
      }
    }

    cmdlist->ResourceBarrier(1, &setResourceTransitionBarrier(Context::getInstance()->getCurrentBackBuffer(), D3D12_RESOURCE_USAGE_RENDER_TARGET, D3D12_RESOURCE_USAGE_PRESENT));
    HRESULT hr = cmdlist->Close();
    Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());
    HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
    cmdalloc->Reset();
    cmdlist->Reset(cmdalloc.Get(), nullptr);
    Context::getInstance()->Swap();
#endif

  }
};

#endif