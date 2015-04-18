#include <MeshSceneNode.h>
#include <MeshManager.h>
#include <RenderTargets.h>
#include <Scene.h>
#include <FullscreenPass.h>

#ifdef GLBUILD
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <GLAPI/Debug.h>
#include <glapi.h>
#endif

#ifdef DXBUILD
#include <D3DAPI/Context.h>
#include <D3DAPI/RootSignature.h>

#include <D3DAPI/Misc.h>
#include <D3DAPI/VAO.h>
#include <D3DAPI/D3DS3DVertex.h>
#include <Loaders/B3D.h>
#include <Loaders/DDS.h>
#include <tuple>
#include <D3DAPI/PSO.h>
#include <D3DAPI/Resource.h>
#include <D3DAPI/ConstantBuffer.h>
#include <d3dapi.h>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")
#endif

RenderTargets *rtts;

Scene *scnmgr;
irr::scene::ISceneNode *xue;

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


}

void clean()
{
  TextureManager::getInstance()->kill();
  MeshManager::getInstance()->kill();
  delete rtts;
  delete scnmgr;
#ifdef DXBUILD
  Object::getInstance()->kill();
  RS::getInstance()->kill();
  Context::getInstance()->kill();
#endif
}

static float timer = 0.;

void draw()
{
  xue->setRotation(irr::core::vector3df(0.f, timer / 360.f, 0.f));
  scnmgr->update();
  scnmgr->renderGBuffer(nullptr, *rtts);
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