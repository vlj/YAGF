#include <API/d3dapi.h>
#include <Scene/Shaders.h>

#include <D3DAPI/D3DRTTSet.h>
#include <D3DAPI/VAO.h>
#include <D3DAPI/Texture.h>
#include <Loaders/B3D.h>
#include <Loaders/DDS.h>
#include <tuple>


#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

using namespace Microsoft::WRL; // For ComPtr

WrapperCommandList *cmdlist;
WrapperDescriptorHeap *ReadResourceHeaps;
ComPtr<ID3D12DescriptorHeap> TexResourceHeaps;
std::vector<ComPtr<ID3D12Resource> > Textures;
WrapperDescriptorHeap *Sampler;
WrapperResource *DepthBuffer;

FormattedVertexStorage<irr::video::S3DVertex2TCoords, irr::video::SkinnedVertexData> *vao;

struct Matrixes
{
  float Model[16];
  float ViewProj[16];
};

struct JointTransform
{
  float Model[16 * 48];
};

WrapperResource *cbuffer;
WrapperResource *jointbuffer;
WrapperPipelineState *objectpso;
D3DRTTSet *fbo[2];

irr::scene::CB3DMeshFileLoader *loader;
std::unordered_map<std::string, D3D12_GPU_DESCRIPTOR_HANDLE> textureSet;

GFXAPI *GlobalGFXAPI;

void Init(HWND hWnd)
{
  Context::getInstance()->InitD3D(hWnd);
  ID3D12Device *dev = Context::getInstance()->dev.Get();
  GlobalGFXAPI = new D3DAPI();

  cmdlist = GlobalGFXAPI->createCommandList();
  cbuffer = GlobalGFXAPI->createConstantsBuffer(sizeof(Matrixes));
  jointbuffer = GlobalGFXAPI->createConstantsBuffer(sizeof(JointTransform));
  ReadResourceHeaps = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap(
  {
    std::make_tuple(cbuffer, RESOURCE_VIEW::CONSTANTS_BUFFER, 0),
    std::make_tuple(jointbuffer, RESOURCE_VIEW::CONSTANTS_BUFFER, 0),
  });

  Sampler = GlobalGFXAPI->createSamplerHeap({ { SAMPLER_TYPE::TRILINEAR, 0 } });
  DepthBuffer = GlobalGFXAPI->createDepthStencilTexture(1024, 1024);

  fbo[0] = new D3DRTTSet({ Context::getInstance()->getBackBuffer(0) }, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, 1024, 1024, DepthBuffer->D3DValue.resource, &DepthBuffer->D3DValue.description.DSV);
  fbo[1] = new D3DRTTSet({ Context::getInstance()->getBackBuffer(1) }, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, 1024, 1024, DepthBuffer->D3DValue.resource, &DepthBuffer->D3DValue.description.DSV);

  irr::io::CReadFile reader("..\\examples\\assets\\xue.b3d");
  loader = new irr::scene::CB3DMeshFileLoader(&reader);
  std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > buffers = loader->AnimatedMesh.getMeshBuffers();
  std::vector<irr::scene::SMeshBufferLightMap> reorg;

  for (auto buf : buffers)
    reorg.push_back(buf.first);

  // Format Weight

  std::vector<std::vector<irr::video::SkinnedVertexData> > weightsList;
  for (auto weightbuffer : loader->AnimatedMesh.WeightBuffers)
  {
    std::vector<irr::video::SkinnedVertexData> weights;
    for (unsigned j = 0; j < weightbuffer.size(); j += 4)
    {
      irr::video::SkinnedVertexData tmp = {
        weightbuffer[j].Index, weightbuffer[j].Weight,
        weightbuffer[j + 1].Index, weightbuffer[j + 1].Weight,
        weightbuffer[j + 2].Index, weightbuffer[j + 2].Weight,
        weightbuffer[j + 3].Index, weightbuffer[j + 3].Weight,
      };
      weights.push_back(tmp);
    }
    weightsList.push_back(weights);
  }

  vao = new FormattedVertexStorage<irr::video::S3DVertex2TCoords, irr::video::SkinnedVertexData>(Context::getInstance()->cmdqueue.Get(), reorg, weightsList);

  // Upload to gpudata

  // Texture
  std::vector<std::tuple<std::string, ID3D12Resource*, D3D12_SHADER_RESOURCE_VIEW_DESC> > textureNamePairs;
  for (auto Tex : loader->Textures)
  {
    Textures.push_back(ComPtr<ID3D12Resource>());
    const std::string &fixed = "..\\examples\\assets\\" + Tex.TextureName.substr(0, Tex.TextureName.find_last_of('.')) + ".DDS";
    std::ifstream DDSFile(fixed, std::ifstream::binary);
    irr::video::CImageLoaderDDS DDSPic(DDSFile);

    Texture TextureInRam(DDSPic.getLoadedImage());

    HRESULT hr = dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Tex2D(getDXGIFormatFromColorFormat(DDSPic.getLoadedImage().Format), (UINT)TextureInRam.getWidth(), (UINT)TextureInRam.getHeight(), 1, (UINT16)DDSPic.getLoadedImage().MipMapData.size()),
      D3D12_RESOURCE_USAGE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&Textures.back())
      );

    WrapperCommandList *uploadcmdlist = GlobalGFXAPI->createCommandList();
    GlobalGFXAPI->openCommandList(uploadcmdlist);
    TextureInRam.CreateUploadCommandToResourceInDefaultHeap(uploadcmdlist->D3DValue.CommandList, Textures.back().Get());
    textureNamePairs.push_back(std::make_tuple(Tex.TextureName, Textures.back().Get(), TextureInRam.getResourceViewDesc()));

    GlobalGFXAPI->closeCommandList(uploadcmdlist);
    GlobalGFXAPI->submitToQueue(uploadcmdlist);
    HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
    GlobalGFXAPI->releaseCommandList(uploadcmdlist);

    // Create Texture Heap
    {
      TexResourceHeaps = createDescriptorHeap(dev, (UINT)textureNamePairs.size(), D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP, true);
      for (unsigned i = 0; i < textureNamePairs.size(); i++)
      {
        std::tuple<std::string, ID3D12Resource*, D3D12_SHADER_RESOURCE_VIEW_DESC> texturedata = textureNamePairs[i];
        dev->CreateShaderResourceView(std::get<1>(texturedata), &std::get<2>(texturedata), TexResourceHeaps->GetCPUDescriptorHandleForHeapStart().MakeOffsetted(i * dev->GetDescriptorHandleIncrementSize(D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP)));
        textureSet[std::get<0>(texturedata)] = TexResourceHeaps->GetGPUDescriptorHandleForHeapStart().MakeOffsetted(i * dev->GetDescriptorHandleIncrementSize(D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP));
      }
    }
  }
  objectpso = createSkinnedObjectShader();
}


static float timer = 0.;

void Draw()
{
  GlobalGFXAPI->openCommandList(cmdlist);
  Matrixes *cbufdata = (Matrixes*)GlobalGFXAPI->mapConstantsBuffer(cbuffer);
  irr::core::matrix4 Model, View;
  View.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
  Model.setTranslation(irr::core::vector3df(0.f, 0.f, 2.f));
  Model.setRotationDegrees(irr::core::vector3df(0.f, timer / 360.f, 0.f));

  memcpy(cbufdata->Model, Model.pointer(), 16 * sizeof(float));
  memcpy(cbufdata->ViewProj, View.pointer(), 16 * sizeof(float));
  GlobalGFXAPI->unmapConstantsBuffers(cbuffer);

  timer += 16.f;

  double intpart;
  float frame = (float)modf(timer / 10000., &intpart);
  frame *= 300.f;
  loader->AnimatedMesh.animateMesh(frame, 1.f);
  loader->AnimatedMesh.skinMesh(1.f);

  memcpy(GlobalGFXAPI->mapConstantsBuffer(jointbuffer), loader->AnimatedMesh.JointMatrixes.data(), loader->AnimatedMesh.JointMatrixes.size() * 16 * sizeof(float));
  GlobalGFXAPI->unmapConstantsBuffers(jointbuffer);

  cmdlist->D3DValue.CommandList->ResourceBarrier(1, &setResourceTransitionBarrier(Context::getInstance()->getCurrentBackBuffer(), D3D12_RESOURCE_USAGE_PRESENT, D3D12_RESOURCE_USAGE_RENDER_TARGET));

  float clearColor[] = { .25f, .25f, 0.35f, 1.0f };
  fbo[Context::getInstance()->getCurrentBackBufferIndex()]->Clear(cmdlist->D3DValue.CommandList, clearColor);
  fbo[Context::getInstance()->getCurrentBackBufferIndex()]->ClearDepthStencil(cmdlist->D3DValue.CommandList, 1.f, 0);

  fbo[Context::getInstance()->getCurrentBackBufferIndex()]->Bind(cmdlist->D3DValue.CommandList);
  GlobalGFXAPI->setPipelineState(cmdlist, objectpso);

  GlobalGFXAPI->setDescriptorHeap(cmdlist, 0, ReadResourceHeaps);
  GlobalGFXAPI->setDescriptorHeap(cmdlist, 2, Sampler);

  cmdlist->D3DValue.CommandList->SetIndexBuffer(&vao->getIndexBufferView());
  cmdlist->D3DValue.CommandList->SetVertexBuffers(0, &vao->getVertexBufferView()[0], 1);
  cmdlist->D3DValue.CommandList->SetVertexBuffers(1, &vao->getVertexBufferView()[1], 1);

  std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > buffers = loader->AnimatedMesh.getMeshBuffers();
  for (unsigned i = 0; i < buffers.size(); i++)
  {
    cmdlist->D3DValue.CommandList->SetGraphicsRootDescriptorTable(1, textureSet[buffers[i].second.TextureNames[0]]);
    GlobalGFXAPI->drawIndexedInstanced(cmdlist, std::get<0>(vao->meshOffset[i]), 1, std::get<2>(vao->meshOffset[i]), std::get<1>(vao->meshOffset[i]), 0);
  }

  cmdlist->D3DValue.CommandList->ResourceBarrier(1, &setResourceTransitionBarrier(Context::getInstance()->getCurrentBackBuffer(), D3D12_RESOURCE_USAGE_RENDER_TARGET, D3D12_RESOURCE_USAGE_PRESENT));
  GlobalGFXAPI->closeCommandList(cmdlist);
  GlobalGFXAPI->submitToQueue(cmdlist);
  HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
  WaitForSingleObject(handle, INFINITE);
  CloseHandle(handle);
  Context::getInstance()->Swap();
}

void Clean()
{
  Context::getInstance()->kill();
  GlobalGFXAPI->releaseCommandList(cmdlist);
  GlobalGFXAPI->releaseConstantsBuffers(cbuffer);
  GlobalGFXAPI->releaseConstantsBuffers(jointbuffer);
  GlobalGFXAPI->releasePSO(objectpso);
  GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(ReadResourceHeaps);
  GlobalGFXAPI->releaseSamplerHeap(Sampler);
  GlobalGFXAPI->releaseRTTOrDepthStencilTexture(DepthBuffer);
  delete vao;
  delete fbo[0];
  delete fbo[1];
}

int WINAPI WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{

  Init(WindowUtil::Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow));
  // this struct holds Windows event messages
  MSG msg = {};

  // Loop from https://msdn.microsoft.com/en-us/library/windows/apps/dn166880.aspx
  while (WM_QUIT != msg.message)
  {
    bool bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);

    if (bGotMsg)
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    Draw();
  }

  Clean();
  // return this part of the WM_QUIT message to Windows
  return (int)msg.wParam;
}

