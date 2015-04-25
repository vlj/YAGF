// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#include <TextureManager.h>
#include <Loaders/DDS.h>

#ifdef GLBUILD
#include <API/glapi.h>
#include <GLAPI/Texture.h>
#endif

#ifdef DXBUILD
#include <API/d3dapi.h>
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
#ifdef GLBUILD
    WrapperResource *res = (WrapperResource*)malloc(sizeof(WrapperResource));
    // TODO : clean it
    Texture *tmptexture = new Texture(DDSPic.getLoadedImage());
    res->GLValue.Resource = tmptexture->Id;
    res->GLValue.Type = GL_TEXTURE_2D;
#endif

#ifdef DXBUILD
    Texture TextureInRam(DDSPic.getLoadedImage());
    WrapperResource *res = (WrapperResource*)malloc(sizeof(WrapperResource));

    HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Tex2D(TextureInRam.getFormat(), (UINT)TextureInRam.getWidth(), (UINT)TextureInRam.getHeight(), 1, (UINT16)TextureInRam.getMipLevelsCount()),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&res->D3DValue.resource)
      );

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdalloc;
    Context::getInstance()->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdlist;
    Context::getInstance()->dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), nullptr, IID_PPV_ARGS(&cmdlist));

    TextureInRam.CreateUploadCommandToResourceInDefaultHeap(cmdlist.Get(), res->D3DValue.resource);

    cmdlist->Close();
    Context::getInstance()->cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());
    HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
    res->D3DValue.description.TextureView.SRV = TextureInRam.getResourceViewDesc();

    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
#endif
    textureSet.emplace(TextureLoc, res);
  }
}

WrapperResource* TextureManager::getTexture(const std::string &name) const
{
  std::unordered_map<std::string, WrapperResource*>::const_iterator It = textureSet.find(name);
  if (It == textureSet.end())
    return nullptr;
  return It->second;
}
