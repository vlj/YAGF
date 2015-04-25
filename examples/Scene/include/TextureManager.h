// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __TEXTURE_MANAGER_H__
#define __TEXTURE_MANAGER_H__

#include <Core/Singleton.h>
#include <list>
#include <string>
#include <unordered_map>
#include <API/GfxApi.h>

class TextureManager : public Singleton<TextureManager>
{
private:
  std::unordered_map<std::string, WrapperResource *> textureSet;
public:
  TextureManager();
  ~TextureManager();

  void LoadTextures(const std::vector<std::string>& TexturesLocation);
  WrapperResource* getTexture(const std::string &name) const;
};

#endif