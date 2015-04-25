#include <MeshSceneNode.h>
#include <MeshManager.h>
#include <RenderTargets.h>
#include <Scene.h>
#include <FullscreenPass.h>

#ifdef GLBUILD
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <GLAPI/Debug.h>
#include <API/glapi.h>
#endif

#ifdef DXBUILD
#include <D3DAPI/Context.h>
#include <D3DAPI/Misc.h>
#include <API/d3dapi.h>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")
#endif

RenderTargets *rtts;

Scene *scnmgr;
FullscreenPassManager *fspassmgr;
irr::scene::ISceneNode *xue;
WrapperResource *cubemap;
WrapperDescriptorHeap *skyboxTextureHeap;

#ifdef DXBUILD
GFXAPI *GlobalGFXAPI = new D3DAPI();
#endif

#ifdef GLBUILD
GFXAPI *GlobalGFXAPI = new GLAPI();
#endif

void init()
{
#ifdef GLBUILD
  DebugUtil::enableDebugOutput();
  glDepthFunc(GL_LEQUAL);
#endif
  std::vector<std::string> xueB3Dname = { "..\\examples\\assets\\xue.b3d" };

  scnmgr = new Scene();

  MeshManager::getInstance()->LoadMesh(xueB3Dname);
  xue = scnmgr->addMeshSceneNode(MeshManager::getInstance()->getMesh(xueB3Dname[0]), nullptr, irr::core::vector3df(0.f, 0.f, 2.f));

  rtts = new RenderTargets(1024, 1024);
  fspassmgr = new FullscreenPassManager(*rtts);

  cubemap = (WrapperResource*)malloc(sizeof(WrapperResource));
  const std::string &fixed = "..\\examples\\assets\\w_sky_1BC1.dds";
  std::ifstream DDSFile(fixed, std::ifstream::binary);
  irr::video::CImageLoaderDDS DDSPic(DDSFile);
#if DXBUILD
  ID3D12Resource *SkyboxTexture;
  Texture TexInRam(DDSPic.getLoadedImage());
  const IImage &Image = DDSPic.getLoadedImage();

  HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
    &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_MISC_NONE,
    &CD3D12_RESOURCE_DESC::Tex2D(getDXGIFormatFromColorFormat(Image.Format), (UINT)TexInRam.getWidth(), (UINT)TexInRam.getHeight(), 6, (UINT)TexInRam.getMipLevelsCount()),
    D3D12_RESOURCE_USAGE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&SkyboxTexture)
    );

  WrapperCommandList *uploadcmdlist = GlobalGFXAPI->createCommandList();
  GlobalGFXAPI->openCommandList(uploadcmdlist);

  TexInRam.CreateUploadCommandToResourceInDefaultHeap(uploadcmdlist->D3DValue.CommandList, SkyboxTexture);

  D3D12_SHADER_RESOURCE_VIEW_DESC resdesc = {};
  resdesc.Format = getDXGIFormatFromColorFormat(Image.Format);
  resdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
  resdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  resdesc.TextureCube.MipLevels = 10;

  cubemap->D3DValue.resource = SkyboxTexture;
  cubemap->D3DValue.description.TextureView.SRV = resdesc;

  GlobalGFXAPI->closeCommandList(uploadcmdlist);
  GlobalGFXAPI->submitToQueue(uploadcmdlist);
  HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
  WaitForSingleObject(handle, INFINITE);
  CloseHandle(handle);
  GlobalGFXAPI->releaseCommandList(uploadcmdlist);
#endif
  skyboxTextureHeap = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap(
  {
    std::make_tuple(cubemap, RESOURCE_VIEW::SHADER_RESOURCE, 0)
  });
}

void clean()
{
  TextureManager::getInstance()->kill();
  MeshManager::getInstance()->kill();
  delete rtts;
  delete scnmgr;
  delete fspassmgr;
#ifdef DXBUILD
  Context::getInstance()->kill();
#endif
}

static float timer = 0.;

void draw()
{
  xue->setRotation(irr::core::vector3df(0.f, timer / 360.f, 0.f));
  scnmgr->update();
  scnmgr->renderGBuffer(nullptr, *rtts);
  GlobalGFXAPI->openCommandList(fspassmgr->CommandList);
  fspassmgr->renderSunlight();
  fspassmgr->renderSky(skyboxTextureHeap);
  fspassmgr->renderTonemap();
  GlobalGFXAPI->closeCommandList(fspassmgr->CommandList);
  GlobalGFXAPI->submitToQueue(fspassmgr->CommandList);

#ifdef DXBUILD
  Context::getInstance()->Swap();
  HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
  WaitForSingleObject(handle, INFINITE);
  CloseHandle(handle);
#endif

  timer += 16.f;
}

#ifdef GLBUILD
int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  GLFWwindow* window = glfwCreateWindow(1024, 1024, "GLtest", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glewExperimental = GL_TRUE;
  glewInit();
  init();

  while (!glfwWindowShouldClose(window))
  {
    draw();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  clean();
  glfwTerminate();
  return 0;
}
#endif

#ifdef DXBUILD
int WINAPI WinMain(HINSTANCE hInstance,
HINSTANCE hPrevInstance,
LPSTR lpCmdLine,
int nCmdShow)
{
  Context::getInstance()->InitD3D(WindowUtil::Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow));
  init();
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

    draw();
  }

  clean();
  // return this part of the WM_QUIT message to Windows
  return (int)msg.wParam;
}
#endif