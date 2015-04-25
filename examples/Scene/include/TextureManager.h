// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __TEXTURE_MANAGER_H__
#define __TEXTURE_MANAGER_H__

#include <Core/Singleton.h>
#include <list>
#include <string>
#include <Loaders/DDS.h>
#include <unordered_map>

#ifdef GLBUILD
#include <GLAPI/Texture.h>
#endif

#ifdef DXBUILD
#include <D3DAPI/Texture.h>
#endif
class TextureManager : public Singleton<TextureManager>
{
private:
  std::list<Texture> TextureStore;
  std::unordered_map<std::string, Texture*> textureSet;
public:
  TextureManager();
  ~TextureManager();

  void LoadTextures(const std::vector<std::string>& TexturesLocation);
  const Texture* getTexture(const std::string &name) const;
};

#endif