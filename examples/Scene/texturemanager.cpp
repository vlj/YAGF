// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <TextureManager.h>
#include <Loaders/DDS.h>

#ifdef GLBUILD
#include <GLAPI/Texture.h>
#endif

#ifdef DXBUILD
#include <D3DAPI/Texture.h>
#endif

TextureManager::TextureManager()
{}

TextureManager::~TextureManager()
{}

void TextureManager::LoadTextures(const std::vector<std::string>& TexturesLocation)
{
  for (std::string TextureLoc : TexturesLocation)
  {
    const std::string &fixed = "..\\examples\\assets\\" + TextureLoc.substr(0, TextureLoc.find_last_of('.')) + ".DDS";
    std::ifstream DDSFile(fixed, std::ifstream::binary);
    irr::video::CImageLoaderDDS DDSPic(DDSFile);
    TextureStore.emplace_back(DDSPic.getLoadedImage());
    textureSet.emplace(TextureLoc, &TextureStore.back());
  }
}

const Texture* TextureManager::getTexture(const std::string &name) const
{
  std::unordered_map<std::string, Texture*>::const_iterator It = textureSet.find(name);
  if (It == textureSet.end())
    return nullptr;
  return It->second;
}
