#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <API/glapi.h>
#include <Maths/matrix4.h>

#include <GLAPI/Texture.h>

#include <Scene/Shaders.h>
#include <GLAPI/Misc.h>
#include <GLAPI/Debug.h>

#include <Loaders/DDS.h>

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

void init()
{
  GlobalGFXAPI = new GLAPI();
  DebugUtil::enableDebugOutput();

  cubemap = (WrapperResource*)malloc(sizeof(WrapperResource));


  const std::string &fixed = "..\\examples\\assets\\hdrsky.dds";
  std::ifstream DDSFile(fixed, std::ifstream::binary);
  irr::video::CImageLoaderDDS DDSPic(DDSFile);

  Texture *Tex = new Texture(DDSPic.getLoadedImage());
  cubemap->GLValue.Resource = Tex->Id;
  cubemap->GLValue.Type = GL_TEXTURE_CUBE_MAP;

  CommandList = GlobalGFXAPI->createCommandList();

  cbuf = GlobalGFXAPI->createConstantsBuffer(sizeof(Matrixes));
  Inputs = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap({ std::make_tuple(cbuf, RESOURCE_VIEW::CONSTANTS_BUFFER, 0), std::make_tuple(cubemap, RESOURCE_VIEW::SHADER_RESOURCE, 0) });
  Samplers = GlobalGFXAPI->createSamplerHeap({ {SAMPLER_TYPE::BILINEAR, 0} });

  SkyboxPSO = createSkyboxShader();
  ScreenQuad = GlobalGFXAPI->createFullscreenTri();

  glDepthFunc(GL_LEQUAL);
}

void clean()
{
  GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(Inputs);
  GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(Samplers);
  GlobalGFXAPI->releaseConstantsBuffers(cbuf);
  GlobalGFXAPI->releaseCommandList(CommandList);
  GlobalGFXAPI->releasePSO(SkyboxPSO);
  GlobalGFXAPI->releaseIndexVertexBuffersSet(ScreenQuad);
}

static float time = 0.;

void draw()
{
  Matrixes cbufdata;
  irr::core::matrix4 View, invView, Proj, invProj;
  View.buildCameraLookAtMatrixLH(irr::core::vector3df(cos(3.14 * time / 10000.), 0., sin(3.14 * time / 10000.)), irr::core::vector3df(0, 0, 0.), irr::core::vector3df(0, 1., 0.));
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
  GlobalGFXAPI->closeCommandList(CommandList);
  GlobalGFXAPI->submitToQueue(CommandList);

//  glClearColor(0.f, 0.f, 0.f, 1.f);
//  glClearDepth(1.);
//  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  time += 1.f;
}

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

