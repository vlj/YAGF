
#include <Util/GeometryCreator.h>
#include <API/d3dapi.h>
#include <Maths/matrix4.h>
#include <wrl/client.h>

#include <D3DAPI/Texture.h>

#include <Scene/Shaders.h>

#include <Loaders/DDS.h>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

struct Matrixes
{
  float InvView[16];
  float InvProj[16];
};

WrapperResource *cbuf;
WrapperResource *cubemap;
WrapperDescriptorHeap* Samplers;
WrapperDescriptorHeap *Inputs;
WrapperCommandList *CommandList;
WrapperPipelineState *SkyboxPSO;
WrapperIndexVertexBuffersSet *ScreenQuad;

GFXAPI *GlobalGFXAPI;

Microsoft::WRL::ComPtr<ID3D12Resource> SkyboxTexture;

void Init(HWND hWnd)
{
  Context::getInstance()->InitD3D(hWnd);
  GlobalGFXAPI = new D3DAPI();

  cubemap = (WrapperResource*)malloc(sizeof(WrapperResource));


  const std::string &fixed = "..\\examples\\assets\\w_sky_1.dds";
  std::ifstream DDSFile(fixed, std::ifstream::binary);
  irr::video::CImageLoaderDDS DDSPic(DDSFile);

  Microsoft::WRL::ComPtr<ID3D12Resource> TexInRam;
  const IImage &Image = DDSPic.getLoadedImage();

  size_t Width = Image.Layers[0][0].Width, Height = Image.Layers[0][0].Height;

  HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Buffer((UINT)Width * Height * 4 * 6),
    D3D12_RESOURCE_USAGE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&TexInRam)
    );

  hr = Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Tex2D(getDXGIFormatFromColorFormat(Image.Format), (UINT)Width, (UINT)Height, 6, 1),
    D3D12_RESOURCE_USAGE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&SkyboxTexture)
    );

  char *tmp;
  TexInRam->Map(0, nullptr, (void**)&tmp);
  for (unsigned face = 0; face < 6; face++)
  {
    size_t stride = Width * Height * 4;
    memcpy(&tmp[face * stride], Image.Layers[face][0].Data, stride);
  }
  TexInRam->Unmap(0, nullptr);


  WrapperCommandList *uploadcmdlist = GlobalGFXAPI->createCommandList();
  GlobalGFXAPI->openCommandList(uploadcmdlist);

  for (unsigned face = 0; face < 6; face++)
  {
//    MipLevelData miplevel = Mips[i];
    D3D12_RESOURCE_BARRIER_DESC barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = SkyboxTexture.Get();
    barrier.Transition.Subresource = face;
    barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_GENERIC_READ;
    barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_COPY_DEST;
    uploadcmdlist->D3DValue.CommandList->ResourceBarrier(1, &barrier);

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.Type = D3D12_SUBRESOURCE_VIEW_SELECT_SUBRESOURCE;
    dst.pResource = SkyboxTexture.Get();
    dst.Subresource = face;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.Type = D3D12_SUBRESOURCE_VIEW_PLACED_PITCHED_SUBRESOURCE;
    src.pResource = TexInRam.Get();
    src.PlacedTexture.Offset = Width * Height * 4 * face;
    src.PlacedTexture.Placement.Format = getDXGIFormatFromColorFormat(Image.Format);
    src.PlacedTexture.Placement.Width = (UINT)Width;
    src.PlacedTexture.Placement.Height = (UINT)Height;
    src.PlacedTexture.Placement.Depth = 1;
    src.PlacedTexture.Placement.RowPitch = (UINT)Width * 4;
    uploadcmdlist->D3DValue.CommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr, D3D12_COPY_NONE);

    barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_GENERIC_READ;
    uploadcmdlist->D3DValue.CommandList->ResourceBarrier(1, &barrier);
  }

  D3D12_SHADER_RESOURCE_VIEW_DESC resdesc = {};
  resdesc.Format = getDXGIFormatFromColorFormat(Image.Format);
  resdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
  resdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  resdesc.TextureCube.MipLevels = 1;

  cubemap->D3DValue.resource = SkyboxTexture.Get();
  cubemap->D3DValue.description.SRV = resdesc;

  GlobalGFXAPI->closeCommandList(uploadcmdlist);
  GlobalGFXAPI->submitToQueue(uploadcmdlist);
  HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
  WaitForSingleObject(handle, INFINITE);
  CloseHandle(handle);
  GlobalGFXAPI->releaseCommandList(uploadcmdlist);


  CommandList = GlobalGFXAPI->createCommandList();

  cbuf = GlobalGFXAPI->createConstantsBuffer(sizeof(Matrixes));
  Inputs = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap({ std::make_tuple(cbuf, RESOURCE_VIEW::CONSTANTS_BUFFER, 0), std::make_tuple(cubemap, RESOURCE_VIEW::SHADER_RESOURCE, 0) });
  Samplers = GlobalGFXAPI->createSamplerHeap({ { SAMPLER_TYPE::BILINEAR, 0 } });

  SkyboxPSO = createSkyboxShader();
  ScreenQuad = GlobalGFXAPI->createFullscreenTri();

}

void Clean()
{
  GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(Inputs);
  GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(Samplers);
  GlobalGFXAPI->releaseConstantsBuffers(cbuf);
  GlobalGFXAPI->releaseCommandList(CommandList);
  GlobalGFXAPI->releasePSO(SkyboxPSO);
  GlobalGFXAPI->releaseIndexVertexBuffersSet(ScreenQuad);
}

static float timer = 0.;

void Draw()
{
  Matrixes cbufdata;
  irr::core::matrix4 View, invView, Proj, invProj;
  View.buildCameraLookAtMatrixLH(irr::core::vector3df(cos(3.14 * timer / 10000.), 0., sin(3.14 * timer / 10000.)), irr::core::vector3df(0, 0, 0.), irr::core::vector3df(0, 1., 0.));
  View.getInverse(invView);
  Proj.buildProjectionMatrixPerspectiveFovLH(110.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
  Proj.getInverse(invProj);
  memcpy(cbufdata.InvView, invView.pointer(), 16 * sizeof(float));
  memcpy(cbufdata.InvProj, invProj.pointer(), 16 * sizeof(float));

  void * tmp = GlobalGFXAPI->mapConstantsBuffer(cbuf);
  memcpy(tmp, &cbufdata, sizeof(Matrixes));
  GlobalGFXAPI->unmapConstantsBuffers(cbuf);

  GlobalGFXAPI->openCommandList(CommandList);
  GlobalGFXAPI->setIndexVertexBuffersSet(CommandList, ScreenQuad);
  GlobalGFXAPI->setPipelineState(CommandList, SkyboxPSO);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 0, Inputs);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 1, Samplers);
  GlobalGFXAPI->setBackbufferAsRTTSet(CommandList, 1024, 1024);
  GlobalGFXAPI->drawInstanced(CommandList, 3, 1, 0, 0);
  GlobalGFXAPI->setBackbufferAsPresent(CommandList);
  GlobalGFXAPI->closeCommandList(CommandList);
  Context::getInstance()->Swap();
  GlobalGFXAPI->submitToQueue(CommandList);

  HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
  WaitForSingleObject(handle, INFINITE);
  CloseHandle(handle);

  timer += 1.f;
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


