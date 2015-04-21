#include <GL/glew.h>

#include "common.hpp"

#include <GLAPI/Shaders.h>
#include <GLAPI/Misc.h>
#include <GLAPI/GLRTTSet.h>
#include <GLAPI/Samplers.h>
#include <GLAPI/Text.h>

#include <fstream>
#include <sstream>


struct PerPixelListBucket
{
  float depth;
  unsigned int TangentAndCoverage;
  unsigned int next;
};

struct Constants
{
  float g_mWorld[16];
  float g_mViewProj[16];
  float g_mInvViewProj[16];
  float g_mViewProjLight[16];

  float g_vEye[3];
  float g_fvFov;

  float g_AmbientLightColor[4];
  float g_PointLightColor[4];
  float g_PointLightPos[4];
  float g_MatBaseColor[4];
  float g_MatKValue[4]; // Ka, Kd, Ks, Ex

  float g_FiberAlpha;
  float g_HairShadowAlpha;
  float g_bExpandPixels;
  float g_FiberRadius;

  float g_WinSize[4]; // screen size

  float g_FiberSpacing; // average spacing between fibers
  float g_bThinTip;
  float g_fNearLight;
  float g_fFarLight;

  int g_iTechSM;
  int g_bUseCoverage;
  int g_iStrandCopies; // strand copies that the transparency shader will produce
  int g_iMaxFragments;

  float g_alphaThreshold;
  float g_fHairKs2; // for second highlight
  float g_fHairEx2; // for second highlight

  float g_mInvViewProjViewport[16];
};

class HairShadow : public ShaderHelperSingleton<HairShadow>, public TextureRead<UniformBufferResource<0>>
{
public:
  HairShadow()
  {
    std::ifstream invtx("../examples/TressFX/shaders/HairSM.vert", std::ios::in);

    const std::string &vtxshader = std::string((std::istreambuf_iterator<char>(invtx)), std::istreambuf_iterator<char>());

    std::ifstream infrag("../examples/TressFX/shaders/HairSM.frag", std::ios::in);

    const std::string &fragshader = std::string((std::istreambuf_iterator<char>(infrag)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vtxshader.c_str(),
      GL_FRAGMENT_SHADER, fragshader.c_str());
    AssignSamplerNames(Program, "Constants");
  }
};

class Transparent : public ShaderHelperSingleton<Transparent>, public TextureRead<UniformBufferResource<0>, ImageResource<1> >
{
public:
  Transparent()
  {
    std::ifstream invtx("../examples/TressFX/shaders/Hair.vert", std::ios::in);

    const std::string &vtxshader = std::string((std::istreambuf_iterator<char>(invtx)), std::istreambuf_iterator<char>());

    std::ifstream infrag("../examples/TressFX/shaders/Hair.frag", std::ios::in);

    const std::string &fragshader = std::string((std::istreambuf_iterator<char>(infrag)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vtxshader.c_str(),
      GL_FRAGMENT_SHADER, fragshader.c_str());
    AssignSamplerNames(Program, "Constants", "PerPixelLinkedListHead");
  }
};

class FragmentMerge : public ShaderHelperSingleton<FragmentMerge>, public TextureRead<UniformBufferResource<0>, TextureResource<GL_TEXTURE_2D, 0>, ImageResource<1> >
{
public:
  FragmentMerge()
  {
    std::ifstream in("../examples/TressFX/shaders/FSShading.frag", std::ios::in);

    const std::string &fragmerge = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, screenquadshader,
      GL_FRAGMENT_SHADER, fragmerge.c_str());
    AssignSamplerNames(Program, "Constants", "HairShadowMap", "PerPixelLinkedListHead");
  }
};

class Passthrough : public ShaderHelperSingleton<Passthrough>, public TextureRead<TextureResource<GL_TEXTURE_2D, 0> >
{
public:
  Passthrough()
  {
    const char *shader = TO_STRING(
      \#version 430\n

      uniform sampler2D tex;
    out vec4 FragColor;
    void main() {
      FragColor = texture(tex, gl_FragCoord.xy / 1024.);
    }
    );
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, screenquadshader,
      GL_FRAGMENT_SHADER, shader);
    AssignSamplerNames(Program, "tex");
  }
};

TressFXAsset loadTress(const char* filename)
{
  std::ifstream inFile(filename, std::ios::binary);

  TressFXAsset m_HairAsset;

  if (!inFile.is_open())
  {
    printf("Coulndt load %s\n", filename);
    return m_HairAsset;
  }

  inFile.read((char *)&m_HairAsset.m_NumTotalHairVertices, sizeof(int));
  inFile.read((char *)&m_HairAsset.m_NumTotalHairStrands, sizeof(int));
  inFile.read((char *)&m_HairAsset.m_MaxNumOfVerticesInStrand, sizeof(int));
  inFile.read((char *)&m_HairAsset.m_NumGuideHairVertices, sizeof(int));
  inFile.read((char *)&m_HairAsset.m_NumGuideHairStrands, sizeof(int));
  inFile.read((char *)&m_HairAsset.m_NumFollowHairsPerOneGuideHair, sizeof(int));

  m_HairAsset.m_pHairStrandType = new int[m_HairAsset.m_NumTotalHairStrands];
  inFile.read((char *)m_HairAsset.m_pHairStrandType, m_HairAsset.m_NumTotalHairStrands * sizeof(int));

  m_HairAsset.m_pRefVectors = new Float4[m_HairAsset.m_NumTotalHairVertices];
  inFile.read((char *)m_HairAsset.m_pRefVectors, m_HairAsset.m_NumTotalHairVertices * sizeof(Float4));

  m_HairAsset.m_pGlobalRotations = new Float4[m_HairAsset.m_NumTotalHairVertices];
  inFile.read((char *)m_HairAsset.m_pGlobalRotations, m_HairAsset.m_NumTotalHairVertices * sizeof(Float4));

  m_HairAsset.m_pLocalRotations = new Float4[m_HairAsset.m_NumTotalHairVertices];
  inFile.read((char *)m_HairAsset.m_pLocalRotations, m_HairAsset.m_NumTotalHairVertices * sizeof(Float4));

  m_HairAsset.m_pVertices = new Float4[m_HairAsset.m_NumTotalHairVertices];
  inFile.read((char *)m_HairAsset.m_pVertices, m_HairAsset.m_NumTotalHairVertices * sizeof(Float4));

  m_HairAsset.m_pTangents = new Float4[m_HairAsset.m_NumTotalHairVertices];
  inFile.read((char *)m_HairAsset.m_pTangents, m_HairAsset.m_NumTotalHairVertices * sizeof(Float4));

  m_HairAsset.m_pTriangleVertices = new StrandVertex[m_HairAsset.m_NumTotalHairVertices];
  inFile.read((char *)m_HairAsset.m_pTriangleVertices, m_HairAsset.m_NumTotalHairVertices * sizeof(StrandVertex));

  m_HairAsset.m_pThicknessCoeffs = new float[m_HairAsset.m_NumTotalHairVertices];
  inFile.read((char *)m_HairAsset.m_pThicknessCoeffs, m_HairAsset.m_NumTotalHairVertices * sizeof(float));

  m_HairAsset.m_pFollowRootOffset = new Float4[m_HairAsset.m_NumTotalHairStrands];
  inFile.read((char *)m_HairAsset.m_pFollowRootOffset, m_HairAsset.m_NumTotalHairStrands * sizeof(Float4));

  m_HairAsset.m_pRestLengths = new float[m_HairAsset.m_NumTotalHairVertices];
  inFile.read((char *)m_HairAsset.m_pRestLengths, m_HairAsset.m_NumTotalHairVertices * sizeof(float));

  inFile.read((char *)&m_HairAsset.m_bSphere, sizeof(BSphere));

  m_HairAsset.m_Triangleindices.clear();
  unsigned size;
  inFile >> size;
  int *pIndices = new int[size];
  inFile.read((char *)pIndices, size * sizeof(int));
  for (unsigned i = 0; i < size; i++)
    m_HairAsset.m_Triangleindices.push_back(pIndices[i]);
  delete pIndices;

  m_HairAsset.m_LineIndices.clear();
  inFile >> size;
  pIndices = new int[size];
  inFile.read((char *)pIndices, size * sizeof(int));
  for (unsigned i = 0; i < size; i++)
    m_HairAsset.m_LineIndices.push_back(pIndices[i]);
  delete pIndices;

  inFile.close();
  return m_HairAsset;
}

void cleanTFXAssetContent(const TressFXAsset &tfxassets)
{
  delete[] tfxassets.m_pHairStrandType;
  delete[] tfxassets.m_pRefVectors;
  delete[] tfxassets.m_pGlobalRotations;
  delete[] tfxassets.m_pLocalRotations;
  delete[] tfxassets.m_pVertices;
  delete[] tfxassets.m_pTangents;
  delete[] tfxassets.m_pTriangleVertices;
  delete[] tfxassets.m_pThicknessCoeffs;
  delete[] tfxassets.m_pFollowRootOffset;
  delete[] tfxassets.m_pRestLengths;
}


static GLRTTSet *MainFBO;
static GLRTTSet *PerPixelLinkedListHeadFBO; // For clearing

static GLuint DepthStencilTexture;
static GLuint MainTexture;

static GLuint HairShadowMapTexture;
static GLuint HairShadowMapDepth;
static GLRTTSet *HairSMFBO;

static GLuint PerPixelLinkedListHeadTexture;
static GLuint PerPixelLinkedListSSBO;
static GLuint PixelCountAtomic;

static GLuint TFXVao;
static GLuint TFXTriangleIdx;
static GLuint TFXVaoLine;
static GLuint TFXLineIdx;
static GLuint ConstantBuffer;

static GLuint Sampler;

// Shared
GLuint PosSSBO;
GLuint TangentSSBO;
GLuint ThicknessSSBO;

static GLsizei lineIndicesCount, triangleIndicesCount;

static GLuint generateRTT(GLsizei width, GLsizei height, GLint internalFormat, GLint format, GLint type, unsigned mipmaplevel = 1)
{
  GLuint result;
  glGenTextures(1, &result);
  glBindTexture(GL_TEXTURE_2D, result);
  /*    if (CVS->isARBTextureStorageUsable())
  glTexStorage2D(GL_TEXTURE_2D, mipmaplevel, internalFormat, Width, Height);
  else*/
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, 0);
  return result;
}

void initCommon(const TressFXAsset& tfxassets)
{
  glGenVertexArrays(1, &TFXVao);
  glBindVertexArray(TFXVao);

  glGenBuffers(1, &TFXTriangleIdx);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TFXTriangleIdx);
  triangleIndicesCount = (GLsizei)tfxassets.m_Triangleindices.size();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangleIndicesCount * sizeof(unsigned), tfxassets.m_Triangleindices.data(), GL_STATIC_DRAW);

  glGenVertexArrays(1, &TFXVaoLine);
  glBindVertexArray(TFXVaoLine);

  glGenBuffers(1, &TFXLineIdx);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TFXLineIdx);
  lineIndicesCount = (GLsizei)tfxassets.m_LineIndices.size();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIndicesCount * sizeof(unsigned), tfxassets.m_LineIndices.data(), GL_STATIC_DRAW);
  glBindVertexArray(0);

  DepthStencilTexture = generateRTT(1024, 1024, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
  glGenTextures(1, &MainTexture);
  glBindTexture(GL_TEXTURE_2D, MainTexture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_SRGB8_ALPHA8, 1024, 1024);
  glGenTextures(1, &PerPixelLinkedListHeadTexture);
  glBindTexture(GL_TEXTURE_2D, PerPixelLinkedListHeadTexture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, 1024, 1024);

  glGenTextures(1, &HairShadowMapTexture);
  glBindTexture(GL_TEXTURE_2D, HairShadowMapTexture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, 640, 640);
  HairShadowMapDepth = generateRTT(640, 640, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);

  HairSMFBO = new GLRTTSet({ HairShadowMapTexture }, HairShadowMapDepth, 640, 640);

  MainFBO = new GLRTTSet({ MainTexture }, DepthStencilTexture, 1024, 1024);
  PerPixelLinkedListHeadFBO = new GLRTTSet({ PerPixelLinkedListHeadTexture }, 1024, 1024);

  glGenBuffers(1, &PerPixelLinkedListSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, PerPixelLinkedListSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, 100000000 * sizeof(PerPixelListBucket), 0, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, PerPixelLinkedListSSBO);

  glGenBuffers(1, &PosSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, PosSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pVertices, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, PosSSBO);

  glGenBuffers(1, &TangentSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, TangentSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pTangents, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, TangentSSBO);

  glGenBuffers(1, &ThicknessSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ThicknessSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * sizeof(float), tfxassets.m_pThicknessCoeffs, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ThicknessSSBO);

  glGenBuffers(1, &PixelCountAtomic);
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
  glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned int), 0, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, PixelCountAtomic);

  glGenBuffers(1, &ConstantBuffer);

  Sampler = SamplerHelper::createNearestSampler();

  glDepthFunc(GL_LEQUAL);
}

void cleanCommon()
{
  glDeleteTextures(1, &MainTexture);
  glDeleteTextures(1, &DepthStencilTexture);
  glDeleteBuffers(1, &ConstantBuffer);

  glDeleteBuffers(1, &PixelCountAtomic);
  glDeleteTextures(1, &HairShadowMapDepth);
  glDeleteTextures(1, &HairShadowMapTexture);

  glDeleteBuffers(1, &PosSSBO);
  glDeleteBuffers(1, &TangentSSBO);
  glDeleteBuffers(1, &ThicknessSSBO);

  glDeleteBuffers(1, &PerPixelLinkedListSSBO);
  glDeleteVertexArrays(1, &TFXVao);
  glDeleteVertexArrays(1, &TFXVaoLine);
  glDeleteBuffers(1, &TFXLineIdx);
  glDeleteBuffers(1, &TFXTriangleIdx);
  glDeleteTextures(1, &PerPixelLinkedListHeadTexture);
  glDeleteSamplers(1, &Sampler);
  glDeleteSync(syncobj);

  delete PerPixelLinkedListHeadFBO;
  delete MainFBO;
  delete HairSMFBO;
}

static void fillConstantBuffer()
{
  /*
  // From Ruby tfx demo
  static COLORREF g_vCustHairColors[16] =
  {
  RGB( 112,  84,  48 ),	//0 brown
  RGB( 98,  14,  4 ),		//1 natural red
  RGB(  59,  48,  36 ),	//2 darkest brown
  RGB(  78,  67,  63 ),	//3 Med dark brown
  RGB(  80,  68,  68 ),	//4 chestnut brown
  RGB( 106,  78,  66 ),	//5 lt chestnut brown
  RGB(  85,  72,  56 ),	//6 dark golden brown
  RGB( 167, 133, 106 ),	//7 light golden brown
  RGB( 184, 151, 120 ),	//8 dark honey blonde
  RGB( 220, 208, 186 ),	//9 bleached blonde
  RGB( 222, 188, 153 ),	//10 light ash blonde
  RGB( 151, 121,  97 ),	//11 med ash brown
  RGB( 230, 206, 168 ),	//12 lightest blonde
  RGB( 229, 200, 168 ),	//13 pale golden blonde
  RGB( 165, 107,  70 ),	//14 strawberry blonde
  RGB( 41, 28, 22 ),		//15 Brownish fur color
  };
  */

  struct Constants cbuf = {};
  cbuf.g_MatBaseColor[0] = 98.f / 255.f;
  cbuf.g_MatBaseColor[1] = 14.f / 255.f;
  cbuf.g_MatBaseColor[2] = 4.f / 255.f;

  cbuf.g_MatKValue[0] = 0.f; // Ka
  cbuf.g_MatKValue[1] = 0.4f; // Kd
  cbuf.g_MatKValue[2] = 0.04f; // Ks1
  cbuf.g_MatKValue[3] = 80.f; // Ex1
  cbuf.g_fHairKs2 = .5f; // Ks2
  cbuf.g_fHairEx2 = 8.0f; // Ex2

  cbuf.g_FiberRadius = 0.3f;
  cbuf.g_FiberAlpha = .5f;
  cbuf.g_FiberSpacing = .3f;
  cbuf.g_alphaThreshold = .00388f;
  cbuf.g_HairShadowAlpha = .004000f;

  cbuf.g_AmbientLightColor[0] = .15f;
  cbuf.g_AmbientLightColor[1] = .15f;
  cbuf.g_AmbientLightColor[2] = .15f;
  cbuf.g_AmbientLightColor[3] = 1.f;

  cbuf.g_PointLightPos[0] = 421.25f;
  cbuf.g_PointLightPos[1] = 306.79f;
  cbuf.g_PointLightPos[2] = 343.f;
  cbuf.g_PointLightPos[3] = 0.f;

  cbuf.g_PointLightColor[0] = 1.f;
  cbuf.g_PointLightColor[1] = 1.f;
  cbuf.g_PointLightColor[2] = 1.f;
  cbuf.g_PointLightColor[3] = 1.f;

  irr::core::matrix4 View, InvView, tmp, LightMatrix;
  tmp.buildCameraLookAtMatrixRH(irr::core::vector3df(0.f, 0.f, 200.f), irr::core::vector3df(0.f, 0.f, 0.f), irr::core::vector3df(0.f, 1.f, 0.f));
  View.buildProjectionMatrixPerspectiveFovRH(70.f / 180.f * 3.14f, 1.f, 1.f, 1000.f);
  View *= tmp;
  View.getInverse(InvView);

  cbuf.g_vEye[0] = 0.f;
  cbuf.g_vEye[1] = 0.f;
  cbuf.g_vEye[2] = 200.f;

  irr::core::matrix4 Model;

  LightMatrix.buildProjectionMatrixPerspectiveFovRH(0.6f, 1.f, 532.f, 769.f);

  tmp.buildCameraLookAtMatrixRH(irr::core::vector3df(cbuf.g_PointLightPos[0], cbuf.g_PointLightPos[1], cbuf.g_PointLightPos[2]),
    irr::core::vector3df(0.f, 0.f, 0.f), irr::core::vector3df(0.f, 1.f, 0.f));
  LightMatrix *= tmp;
  memcpy(cbuf.g_mWorld, Model.pointer(), 16 * sizeof(float));
  memcpy(cbuf.g_mViewProj, View.pointer(), 16 * sizeof(float));
  memcpy(cbuf.g_mInvViewProj, InvView.pointer(), 16 * sizeof(float));
  memcpy(cbuf.g_mViewProjLight, LightMatrix.pointer(), 16 * sizeof(float));

  cbuf.g_WinSize[0] = 1024.f;
  cbuf.g_WinSize[1] = 1024.f;
  cbuf.g_WinSize[2] = 1.f / 1024.f;
  cbuf.g_WinSize[3] = 1.f / 1024.f;

  cbuf.g_fNearLight = 532.711365f;
  cbuf.g_fFarLight = 768.133728f;

  glBindBuffer(GL_UNIFORM_BUFFER, ConstantBuffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(struct Constants), &cbuf, GL_STATIC_DRAW);
}

GLsync syncobj;

void draw(float density)
{
  fillConstantBuffer();
  // Reset PixelCount
  int pxcnt = 1;
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
  glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned int), &pxcnt, GL_STATIC_DRAW);

  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glClearColor(0., 0., 0., 1.);
  glClearDepth(1.);
  glClearStencil(0);

  // Draw shadow map
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  HairSMFBO->Bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBindVertexArray(TFXVaoLine);
  glUseProgram(HairShadow::getInstance()->Program);
  HairShadow::getInstance()->SetTextureUnits(ConstantBuffer);
  glDrawElementsBaseVertex(GL_LINES, (GLsizei)(density * triangleIndicesCount), GL_UNSIGNED_INT, 0, 0);

  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  PerPixelLinkedListHeadFBO->Bind();

  glClear(GL_COLOR_BUFFER_BIT);
  MainFBO->Bind();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  std::string t;
  {
    GLuint timer;
    glGenQueries(1, &timer);
    glBeginQuery(GL_TIME_ELAPSED, timer);
    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBindVertexArray(TFXVao);
    glUseProgram(Transparent::getInstance()->Program);
    Transparent::getInstance()->SetTextureUnits(ConstantBuffer, PerPixelLinkedListHeadTexture, GL_READ_WRITE, GL_R32UI);
    glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei) (density * lineIndicesCount), GL_UNSIGNED_INT, 0, 0);

    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glEndQuery(GL_TIME_ELAPSED);
    GLuint result;
    glGetQueryObjectuiv(timer, GL_QUERY_RESULT, &result);

    std::stringstream strstream;
    strstream << "First pass: " << result / 1000000. << "ms";
    t = strstream.str();
    glDeleteQueries(1, &timer);
  }

  syncobj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_EQUAL, 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  glEnable(GL_BLEND);
  glEnable(GL_FRAMEBUFFER_SRGB);
  glClearColor(0.25f, 0.25f, 0.35f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glBlendFunc(GL_ONE, GL_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);


  FragmentMerge::getInstance()->SetTextureUnits(ConstantBuffer, HairShadowMapDepth, Sampler, PerPixelLinkedListHeadTexture, GL_READ_ONLY, GL_R32UI);
  DrawFullScreenEffect<FragmentMerge>();

  glDisable(GL_STENCIL_TEST);
  glDisable(GL_BLEND);


  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, 1024, 1024);
  Passthrough::getInstance()->SetTextureUnits(MainTexture, Sampler);
  DrawFullScreenEffect<Passthrough>();
  glDisable(GL_FRAMEBUFFER_SRGB);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);

  BasicTextRender<14>::getInstance()->drawText(t.c_str(), 0, 20, 1024, 1024);
}
