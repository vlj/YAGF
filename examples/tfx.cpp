
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <Core/VAO.h>
#include <Windows.h>

#include <Core/Shaders.h>
#include <Core/FBO.h>
#include <Util/Misc.h>
#include <Util/Samplers.h>
#include <Util/Debug.h>
#include <Util/Text.h>

#include <fstream>

FrameBuffer *MainFBO;
// For clearing
FrameBuffer *PerPixelLinkedListHeadFBO;

GLuint DepthStencilTexture;
GLuint MainTexture;

GLuint PerPixelLinkedListHeadTexture;
GLuint PerPixelLinkedListSSBO;
GLuint PixelCountAtomic;

GLuint InitialPosSSBO;
GLuint PosSSBO;
GLuint PrevPosSSBO;
GLuint StrandTypeSSBO;
GLuint LengthSSBO;
GLuint TangentSSBO;
GLuint RotationSSBO;
GLuint LocalRefSSBO;
GLuint ThicknessSSBO;

GLuint BilinearSampler;


GLuint TFXVao;
GLuint TFXVbo;
GLuint TFXTriangleIdx;
GLuint ConstantBuffer;
GLuint ConstantSimBuffer;

struct PerPixelListBucket
{
  float depth;
  unsigned int TangentAndCoverage;
  unsigned int next;
};

class Transparent : public ShaderHelperSingleton<Transparent>, public TextureRead<Image2D>
{
public:
  Transparent()
  {
    std::ifstream invtx("..\\examples\\shaders\\Hair.vert", std::ios::in);

    const std::string &vtxshader = std::string((std::istreambuf_iterator<char>(invtx)), std::istreambuf_iterator<char>());

    std::ifstream infrag("..\\examples\\shaders\\Hair.frag", std::ios::in);

    const std::string &fragshader = std::string((std::istreambuf_iterator<char>(infrag)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vtxshader.c_str(),
      GL_FRAGMENT_SHADER, fragshader.c_str());
    AssignUniforms();
    AssignSamplerNames(Program, 0, "PerPixelLinkedListHead");
  }
};

class FragmentMerge : public ShaderHelperSingleton<FragmentMerge>, public TextureRead<Image2D>
{
public:
  FragmentMerge()
  {
    std::ifstream in("..\\examples\\shaders\\FSShading.frag", std::ios::in);

    const std::string &fragmerge = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, screenquadshader,
      GL_FRAGMENT_SHADER, fragmerge.c_str());
    AssignSamplerNames(Program, 0, "PerPixelLinkedListHead");
  }
};

class GlobalConstraintSimulation : public ShaderHelperSingleton<GlobalConstraintSimulation>
{
public:
  GlobalConstraintSimulation()
  {
    std::ifstream in("..\\examples\\shaders\\Simulation.comp", std::ios::in);

    const std::string &shader = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_COMPUTE_SHADER, shader.c_str());
  }
};

class LocalConstraintSimulation : public ShaderHelperSingleton<LocalConstraintSimulation>
{
public:
  LocalConstraintSimulation()
  {
    std::ifstream in("..\\examples\\shaders\\LocalConstraints.comp", std::ios::in);

    const std::string &shader = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_COMPUTE_SHADER, shader.c_str());
  }
};

class WindLengthTangentConstraint : public ShaderHelperSingleton<WindLengthTangentConstraint>
{
public:

  WindLengthTangentConstraint()
  {
    std::ifstream in("..\\examples\\shaders\\LengthWindTangentComputation.comp", std::ios::in);

    const std::string &shader = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_COMPUTE_SHADER, shader.c_str());
  }
};

static GLuint generateRTT(size_t width, size_t height, GLint internalFormat, GLint format, GLint type, unsigned mipmaplevel = 1)
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

// From TressFx SDK


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


struct SimulationConstants
{
  float ModelTransformForHead[16];
  float ModelRotateForHead[4];

  float Wind[4];
  float Wind1[4];
  float Wind2[4];
  float Wind3[4];

  int NumLengthConstraintIterations;
  int bCollision;

  float GravityMagnitude;
  float timeStep;

  float Damping0;
  float StiffnessForLocalShapeMatching0;
  float StiffnessForGlobalShapeMatching0;
  float GlobalShapeMatchingEffectiveRange0;

  float Damping1;
  float StiffnessForLocalShapeMatching1;
  float StiffnessForGlobalShapeMatching1;
  float GlobalShapeMatchingEffectiveRange1;

  float Damping2;
  float StiffnessForLocalShapeMatching2;
  float StiffnessForGlobalShapeMatching2;
  float GlobalShapeMatchingEffectiveRange2;

  float Damping3;
  float StiffnessForLocalShapeMatching3;
  float StiffnessForGlobalShapeMatching3;
  float GlobalShapeMatchingEffectiveRange3;

  unsigned int NumOfStrandsPerThreadGroup;
  unsigned int NumFollowHairsPerOneGuideHair;

  int bWarp;
  int NumLocalShapeMatchingIterations;
};

struct Float4
{
  float X;
  float Y;
  float Z;
  float W;
};

struct StrandVertex
{
  irr::core::vector3df position;
  irr::core::vector3df tangent;
  Float4 texcoord;
};

class BSphere
{
public:
  irr::core::vector3df center;
  float radius;
  BSphere()
  {
    center = irr::core::vector3df(0, 0, 0);
    radius = 0;
  }

};

struct TressFXAsset
{
  int*						m_pHairStrandType;
  Float4*			m_pRefVectors;
  Float4*			m_pGlobalRotations;
  Float4*			m_pLocalRotations;
  Float4*			m_pVertices;
  Float4*			m_pTangents;
  Float4*			m_pFollowRootOffset;
  StrandVertex*				m_pTriangleVertices;
  float*						m_pThicknessCoeffs;
  std::vector<int>			m_LineIndices;
  std::vector<int>			m_Triangleindices;
  float*						m_pRestLengths;
  BSphere						m_bSphere;
  int							m_NumTotalHairStrands;
  int                         m_NumTotalHairVertices;
  int							m_MaxNumOfVerticesInStrand;
  int                         m_NumGuideHairStrands;
  int                         m_NumGuideHairVertices;
  int                         m_NumFollowHairsPerOneGuideHair;
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

TressFXAsset tfxassets;

void init()
{
  DebugUtil::enableDebugOutput();

  tfxassets = loadTress("..\\examples\\ruby.tfxb");
  // Need to create manually buffer size indexes are 32 bits and not 16bits
  glGenVertexArrays(1, &TFXVao);
  glBindVertexArray(TFXVao);
  glGenBuffers(1, &TFXVbo);
  glBindBuffer(GL_ARRAY_BUFFER, TFXVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(StrandVertex), 0, GL_STATIC_DRAW);

  glGenBuffers(1, &TFXTriangleIdx);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TFXTriangleIdx);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, tfxassets.m_Triangleindices.size() * sizeof(unsigned), tfxassets.m_Triangleindices.data(), GL_STATIC_DRAW);
  glBindVertexArray(0);

  DepthStencilTexture = generateRTT(1024, 1024, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
  MainTexture = generateRTT(1024, 1024, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
  glGenTextures(1, &PerPixelLinkedListHeadTexture);
  glBindTexture(GL_TEXTURE_2D, PerPixelLinkedListHeadTexture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, 1024, 1024);

  MainFBO = new FrameBuffer({ MainTexture }, DepthStencilTexture, 1024, 1024);
  PerPixelLinkedListHeadFBO = new FrameBuffer({ PerPixelLinkedListHeadTexture }, 1024, 1024);

  glGenBuffers(1, &PerPixelLinkedListSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, PerPixelLinkedListSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, 100000000 * sizeof(PerPixelListBucket), 0, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, PerPixelLinkedListSSBO);

  glGenBuffers(1, &InitialPosSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, InitialPosSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pVertices, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, InitialPosSSBO);

  glGenBuffers(1, &PosSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, PosSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pVertices, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, PosSSBO);

  glGenBuffers(1, &PrevPosSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, PrevPosSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pVertices, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, PrevPosSSBO);

  glGenBuffers(1, &StrandTypeSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, StrandTypeSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairStrands * sizeof(int), tfxassets.m_pHairStrandType, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, StrandTypeSSBO);

  glGenBuffers(1, &LengthSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, LengthSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * sizeof(float), tfxassets.m_pRestLengths, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, LengthSSBO);

  glGenBuffers(1, &TangentSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, TangentSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pTangents, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, TangentSSBO);

  glGenBuffers(1, &RotationSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, RotationSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pGlobalRotations, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, RotationSSBO);

  glGenBuffers(1, &LocalRefSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, LocalRefSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pRefVectors, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, LocalRefSSBO);

  glGenBuffers(1, &ThicknessSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ThicknessSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * sizeof(float), tfxassets.m_pThicknessCoeffs, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ThicknessSSBO);

  glGenBuffers(1, &PixelCountAtomic);
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
  glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned int), 0, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, PixelCountAtomic);

  glGenBuffers(1, &ConstantBuffer);
  glBindBuffer(GL_UNIFORM_BUFFER, ConstantBuffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(struct Constants), 0, GL_STATIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, ConstantBuffer);

  glGenBuffers(1, &ConstantSimBuffer);
  glBindBuffer(GL_UNIFORM_BUFFER, ConstantSimBuffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(struct SimulationConstants), 0, GL_STATIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, 1, ConstantSimBuffer);

  BilinearSampler = SamplerHelper::createBilinearSampler();

  glDepthFunc(GL_LEQUAL);
}

void clean()
{
  glDeleteSamplers(1, &BilinearSampler);
  glDeleteTextures(1, &MainTexture);
  glDeleteBuffers(1, &ConstantBuffer);
  glDeleteBuffers(1, &ConstantSimBuffer);
  glDeleteBuffers(1, &PixelCountAtomic);
  glDeleteBuffers(1, &StrandTypeSSBO);
  glDeleteBuffers(1, &PrevPosSSBO);
  glDeleteBuffers(1, &PosSSBO);
  glDeleteBuffers(1, &InitialPosSSBO);
  glDeleteBuffers(1, &PerPixelLinkedListSSBO);
}

void fillConstantBuffer(float time)
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

  pHairParams->color.x = ((g_vCustHairColors[RUBY_COLOR]>> 0) & 0xFF)/255.f;
  pHairParams->color.y = ((g_vCustHairColors[RUBY_COLOR]>> 8) & 0xFF)/255.f;
  pHairParams->color.z = ((g_vCustHairColors[RUBY_COLOR]>>16) & 0xFF)/255.f;
  pHairParams->Ka = 0.0f;
  pHairParams->Kd = 0.4f;
  pHairParams->Ks1 = 0.04f;
  pHairParams->Ex1 = 80.0f;
  pHairParams->Ks2 = 0.5f;
  pHairParams->Ex2 = 8.0f;
  pHairParams->alpha = 0.5f;
  pHairParams->alpha_sm = 0.004f;
  pHairParams->radius = 0.15f;
  pHairParams->density = 1.0f;
  pHairParams->iStrandCopies = 1;
  pHairParams->fiber_spacing = 0.3f;
  pHairParams->bUseCoverage = true;
  pHairParams->bThinTip = true;
  pHairParams->bTransparency = true;
  pHairParams->tech_shadow = SDSM_SHADOW_INDEX;
  pHairParams->bSimulation = true;
  pHairParams->iMaxFragments = MAX_FRAGMENTS;
  pHairParams->ambientLightColor = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
  pHairParams->pointLightColor = XMFLOAT4(1.f, 1.f, 1.f, 1.0f);
  */

  struct Constants cbuf;
  cbuf.g_MatBaseColor[0] = 98. / 255.;
  cbuf.g_MatBaseColor[1] = 14. / 255.;
  cbuf.g_MatBaseColor[2] = 4. / 255.;

  cbuf.g_MatKValue[0] = 0.; // Ka
  cbuf.g_MatKValue[1] = 0.4; // Kd
  cbuf.g_MatKValue[2] = 0.04; // Ks1
  cbuf.g_MatKValue[3] = 80.; // Ex1
  cbuf.g_fHairKs2 = .5; // Ks2
  cbuf.g_fHairEx2 = 8.0; // Ex2

  cbuf.g_FiberRadius = 0.15;
  cbuf.g_FiberAlpha = .5;
  cbuf.g_alphaThreshold = .00388;

  cbuf.g_AmbientLightColor[0] = .15;
  cbuf.g_AmbientLightColor[1] = .15;
  cbuf.g_AmbientLightColor[2] = .15;
  cbuf.g_AmbientLightColor[3] = 1.;

  cbuf.g_PointLightPos[0] = 421.25;
  cbuf.g_PointLightPos[1] = 306.79;
  cbuf.g_PointLightPos[2] = 343.;
  cbuf.g_PointLightPos[3] = 0.;

  cbuf.g_PointLightColor[0] = 1.;
  cbuf.g_PointLightColor[1] = 1.;
  cbuf.g_PointLightColor[2] = 1.;
  cbuf.g_PointLightColor[3] = 1.;

  irr::core::matrix4 View, InvView, tmp;
  tmp.setTranslation(irr::core::vector3df(0., 0., 200.));
  View.buildProjectionMatrixPerspectiveFovLH(70. / 180. * 3.14, 1., 1., 1000.);
  View *= tmp;
  View.getInverse(InvView);
  irr::core::matrix4 Model;

  memcpy(cbuf.g_mWorld, Model.pointer(), 16 * sizeof(float));
  memcpy(cbuf.g_mViewProj, View.pointer(), 16 * sizeof(float));
  memcpy(cbuf.g_mInvViewProj, InvView.pointer(), 16 * sizeof(float));
  cbuf.g_WinSize[0] = 1024.;
  cbuf.g_WinSize[1] = 1024.;
  cbuf.g_WinSize[2] = 1. / 1024.;
  cbuf.g_WinSize[3] = 1. / 1024.;

  glBindBuffer(GL_UNIFORM_BUFFER, ConstantBuffer);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(struct Constants), &cbuf);
}

GLsync syncobj;

void simulate(float time)
{
  struct SimulationConstants cbuf;

  irr::core::matrix4 Model;
  if (time < 30.)
    cbuf.bWarp = 1;
  else
    cbuf.bWarp = 0;

  if (time > 180 * 360)
    time = 180 * 360;

  Model.setRotationDegrees(irr::core::vector3df(0., time / 360., 0.));

  memcpy(cbuf.ModelTransformForHead, Model.pointer(), 16 * sizeof(float));
  cbuf.ModelRotateForHead[0] = 0.;
  cbuf.ModelRotateForHead[1] = sin((time / 720) * (3.14 / 180.));
  cbuf.ModelRotateForHead[2] = 0.;
  cbuf.ModelRotateForHead[3] = cos((3.14 / 180.) * time / 720);

  cbuf.timeStep = .016;

  cbuf.Damping0 = 0.125;
  cbuf.StiffnessForGlobalShapeMatching0 = 0.8;
  cbuf.GlobalShapeMatchingEffectiveRange0 = 0.3;
  cbuf.StiffnessForLocalShapeMatching0 = 0.2;
  cbuf.Damping1 = 0.125;
  cbuf.StiffnessForGlobalShapeMatching1 = 0.25;
  cbuf.GlobalShapeMatchingEffectiveRange1 = 0.4;
  cbuf.StiffnessForLocalShapeMatching1 = 0.75;
  cbuf.Damping2 = 0.0199;
  cbuf.StiffnessForGlobalShapeMatching2 = 0.;
  cbuf.GlobalShapeMatchingEffectiveRange2 = 0.;
  cbuf.StiffnessForLocalShapeMatching2 = 0.699;
  cbuf.Damping3 = 0.1;
  cbuf.StiffnessForGlobalShapeMatching3 = 0.2;
  cbuf.GlobalShapeMatchingEffectiveRange3 = 0.3;
  cbuf.StiffnessForLocalShapeMatching3 = 1.;

  cbuf.NumOfStrandsPerThreadGroup = 4;

  cbuf.GravityMagnitude = 10.;
  cbuf.NumLengthConstraintIterations = 2;
  cbuf.NumLocalShapeMatchingIterations = 1;

  memset(cbuf.Wind, 0, 4 * sizeof(float));
  memset(cbuf.Wind1, 0, 4 * sizeof(float));
  memset(cbuf.Wind2, 0, 4 * sizeof(float));
  memset(cbuf.Wind3, 0, 4 * sizeof(float));

  glBindBuffer(GL_UNIFORM_BUFFER, ConstantSimBuffer);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(struct SimulationConstants), &cbuf);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  int numOfGroupsForCS_VertexLevel = (int)(0.5 * (tfxassets.m_Triangleindices.size() / 64));

  // Global Constraints
  glUseProgram(GlobalConstraintSimulation::getInstance()->Program);
  glDispatchCompute(numOfGroupsForCS_VertexLevel, 1, 1);

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // Local Constraints
  glUseProgram(LocalConstraintSimulation::getInstance()->Program);
  glDispatchCompute(numOfGroupsForCS_VertexLevel, 1, 1);

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // Wind Lenght Tangent
  glUseProgram(WindLengthTangentConstraint::getInstance()->Program);
  glDispatchCompute(numOfGroupsForCS_VertexLevel, 1, 1);
}

void draw(float time)
{
  if (syncobj)
  {
    auto status = glClientWaitSync(syncobj, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000);
    glDeleteSync(syncobj);
  }
  fillConstantBuffer(time);
  // Reset PixelCount
  int pxcnt = 1;
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
  glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(unsigned int), &pxcnt);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  PerPixelLinkedListHeadFBO->Bind();
  glClearColor(0., 0., 0., 1.);
  glClear(GL_COLOR_BUFFER_BIT);
  MainFBO->Bind();
  glClearDepth(1.);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);


  glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  glUseProgram(Transparent::getInstance()->Program);
  glBindVertexArray(TFXVao);
  Transparent::getInstance()->SetTextureUnits(PerPixelLinkedListHeadTexture, GL_READ_WRITE, GL_R32UI);
  Transparent::getInstance()->setUniforms();
  glDrawElementsBaseVertex(GL_TRIANGLES, .5 * tfxassets.m_Triangleindices.size(), GL_UNSIGNED_INT, 0, 0);
  glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glEnable(GL_FRAMEBUFFER_SRGB);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  glViewport(0, 0, 1024, 1024);
//  glClearColor(.1, .1, .1, 1.);
  glClear(GL_COLOR_BUFFER_BIT);

  FragmentMerge::getInstance()->SetTextureUnits(PerPixelLinkedListHeadTexture, GL_READ_ONLY, GL_R32UI);
  DrawFullScreenEffect<FragmentMerge>();


  syncobj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//      glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window = glfwCreateWindow(1024, 1024, "GLtest", NULL, NULL);
  glfwMakeContextCurrent(window);

  glewExperimental = GL_TRUE;
  glewInit();
  init();
  float tmp = 0.;
  while (!glfwWindowShouldClose(window))
  {
    simulate(tmp);
    draw(tmp);
    glfwSwapBuffers(window);
    glfwPollEvents();
    tmp += 300.;
  }
  clean();
  glfwTerminate();
  return 0;
}