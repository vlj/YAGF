// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef __TEXTURE_MANAGER_H__
#define __TEXTURE_MANAGER_H__

#include <Core/Singleton.h>
#include <vector>
#include <string>
#include <Loaders/DDS.h>

#include <GLAPI/Texture.h>

class TextureManager : public Singleton<TextureManager>
{
private:
  std::unordered_map<std::string, Texture> textureSet;
public:
  TextureManager() {}

  void LoadTextures(const std::vector<std::string>& TexturesLocation)
  {
    for (std::string TextureLoc : TexturesLocation)
    {
      const std::string &fixed = "..\\examples\\" + TextureLoc.substr(0, TextureLoc.find_last_of('.')) + ".DDS";
      std::ifstream DDSFile(fixed, std::ifstream::binary);
      irr::video::CImageLoaderDDS DDSPic(DDSFile);
      textureSet.emplace(TextureLoc, DDSPic.getLoadedImage());
    }
  }

  const Texture& getTexture(const std::string &name) const
  {
    std::unordered_map<std::string, Texture>::const_iterator It = textureSet.find(name);

    // Crash if texture is not in the cache
    if (It == textureSet.end())
      abort();
    return It->second;
  }
};

#endif