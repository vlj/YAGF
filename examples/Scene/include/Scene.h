// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __SCENE_H__
#define __SCENE_H__
#include <list>
#include <ISceneNode.h>
#include <MeshSceneNode.h>
#include <RenderTargets.h>

#ifdef GLBUILD
#include <GL/glew.h>
#endif

#ifdef DXBUILD
#include <d3dapi.h>
#endif

class Scene
{
private:
  std::list<irr::scene::IMeshSceneNode*> Meshes;
  WrapperCommandList* cmdList;
  WrapperResource *cbuffer;
  // Should be tied to view rather than scene
  WrapperDescriptorHeap *cbufferDescriptorHeap;
#ifdef DXBUILD
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Sampler;
#endif
#ifdef GLBUILD
  GLuint TrilinearSampler;
#endif
public:
  Scene();
  ~Scene();

  void update();

  irr::scene::IMeshSceneNode *addMeshSceneNode(const irr::scene::ISkinnedMesh *mesh, irr::scene::ISceneNode* parent,
    const irr::core::vector3df& position = irr::core::vector3df(0, 0, 0),
    const irr::core::vector3df& rotation = irr::core::vector3df(0, 0, 0),
    const irr::core::vector3df& scale = irr::core::vector3df(1.f, 1.f, 1.f));

  void renderGBuffer(const class Camera*, RenderTargets &rtts);
};

#endif