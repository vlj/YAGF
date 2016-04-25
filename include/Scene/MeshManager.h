// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __MESH_MANAGER_H__
#define __MESH_MANAGER_H__

#include <Core/Singleton.h>
#include <list>
#include <string>
#include <Core/ISkinnedMesh.h>
#include <Loaders/B3D.h>

#include <unordered_map>
#include <TextureManager.h>

class MeshManager : public Singleton<MeshManager>
{
private:
  std::unordered_map <std::string, irr::scene::ISkinnedMesh *> meshSet;
  std::list<irr::scene::CB3DMeshFileLoader> B3DStore;
public:
  MeshManager() {}
  ~MeshManager() {}

  void LoadMesh(const std::vector<std::string> &meshNames)
  {
    for (const std::string meshName : meshNames)
    {
      irr::io::CReadFile reader(meshName);
      B3DStore.emplace_back(&reader);
      meshSet[meshName] = &B3DStore.back().AnimatedMesh;
      std::vector<std::string> TextureNames;
      for (auto tmp : B3DStore.back().Textures)
        TextureNames.push_back(tmp.TextureName);
      TextureManager::getInstance()->LoadTextures(TextureNames);
    }
  }

  const irr::scene::ISkinnedMesh *getMesh(const std::string &name) const
  {
    std::unordered_map<std::string, irr::scene::ISkinnedMesh *>::const_iterator It = meshSet.find(name);
    if (It == meshSet.end())
      return nullptr;
    return It->second;
  }
};

#endif