
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <Core/VAO.h>


#include <Core/Shaders.h>
#include <Core/FBO.h>
#include <Util/Misc.h>
#include <Util/Samplers.h>
#include <Util/Debug.h>
#include <Util/Text.h>

#include <fstream>

float density = .4;

FrameBuffer *MainFBO;
// For clearing
FrameBuffer *PerPixelLinkedListHeadFBO;

GLuint DepthStencilTexture;
GLuint MainTexture;

GLuint HairShadowMapTexture;
GLuint HairShadowMapDepth;
FrameBuffer *HairSMFBO;

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
GLuint FollowRootSSBO;

GLuint TFXVao;
GLuint TFXTriangleIdx;
GLuint TFXVaoLine;
GLuint TFXLineIdx;
GLuint ConstantBuffer;
GLuint ConstantSimBuffer;

GLuint Sampler;

struct PerPixelListBucket
{
  float depth;
  unsigned int TangentAndCoverage;
  unsigned int next;
};

class HairShadow : public ShaderHelperSingleton<HairShadow>
{
public:
  HairShadow()
  {
    std::ifstream invtx("..\\examples\\shaders\\HairSM.vert", std::ios::in);

    const std::string &vtxshader = std::string((std::istreambuf_iterator<char>(invtx)), std::istreambuf_iterator<char>());

    std::ifstream infrag("..\\examples\\shaders\\HairSM.frag", std::ios::in);

    const std::string &fragshader = std::string((std::istreambuf_iterator<char>(infrag)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vtxshader.c_str(),
      GL_FRAGMENT_SHADER, fragshader.c_str());
    AssignUniforms();
  }
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

class FragmentMerge : public ShaderHelperSingleton<FragmentMerge>, public TextureRead<Texture2D, Image2D>
{
public:
  FragmentMerge()
  {
    std::ifstream in("..\\examples\\shaders\\FSShading.frag", std::ios::in);

    const std::string &fragmerge = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, screenquadshader,
      GL_FRAGMENT_SHADER, fragmerge.c_str());
    AssignSamplerNames(Program, 0, "HairShadowMap", 1, "PerPixelLinkedListHead");
  }
};

#define TO_STRING(x) #x
class Passthrough : public ShaderHelperSingleton<Passthrough>, public TextureRead<Texture2D>
{
public:
  Passthrough()
  {
    const char *shader = TO_STRING(
      #version 430\n

    uniform sampler2D tex;
    out vec4 FragColor;
    void main() {
      FragColor = texture(tex, gl_FragCoord.xy / 1024.);
    }
      );
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, screenquadshader,
      GL_FRAGMENT_SHADER, shader);
    AssignSamplerNames(Program, 0, "tex");
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

class PrepareFollowHairGuide : public ShaderHelperSingleton<PrepareFollowHairGuide>
{
public:

  PrepareFollowHairGuide()
  {
    std::ifstream in("..\\examples\\shaders\\PrepareFollowHair.comp", std::ios::in);

    const std::string &shader = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_COMPUTE_SHADER, shader.c_str());
  }
};

class UpdateFollowHairGuide : public ShaderHelperSingleton<UpdateFollowHairGuide>
{
public:

  UpdateFollowHairGuide()
  {
    std::ifstream in("..\\examples\\shaders\\UpdateFollowHair.comp", std::ios::in);

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
  int* m_pHairStrandType;
  Float4* m_pRefVectors;
  Float4* m_pGlobalRotations;
  Float4* m_pLocalRotations;
  Float4* m_pVertices;
  Float4* m_pTangents;
  Float4* m_pFollowRootOffset;
  StrandVertex* m_pTriangleVertices;
  float* m_pThicknessCoeffs;
  std::vector<int> m_LineIndices;
  std::vector<int> m_Triangleindices;
  float* m_pRestLengths;
  BSphere m_bSphere;
  int m_NumTotalHairStrands;
  int m_NumTotalHairVertices;
  int m_MaxNumOfVerticesInStrand;
  int m_NumGuideHairStrands;
  int m_NumGuideHairVertices;
  int m_NumFollowHairsPerOneGuideHair;
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
  glGenVertexArrays(1, &TFXVao);
  glBindVertexArray(TFXVao);

  glGenBuffers(1, &TFXTriangleIdx);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TFXTriangleIdx);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, tfxassets.m_Triangleindices.size() * sizeof(unsigned), tfxassets.m_Triangleindices.data(), GL_STATIC_DRAW);

  glGenVertexArrays(1, &TFXVaoLine);
  glBindVertexArray(TFXVaoLine);

  glGenBuffers(1, &TFXLineIdx);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TFXLineIdx);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, tfxassets.m_LineIndices.size() * sizeof(unsigned), tfxassets.m_LineIndices.data(), GL_STATIC_DRAW);
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
  glGenTextures(1, &HairShadowMapDepth);
  glBindTexture(GL_TEXTURE_2D, HairShadowMapDepth);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 640, 640);

  HairSMFBO = new FrameBuffer({ HairShadowMapTexture }, HairShadowMapDepth, 640, 640);

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

  glGenBuffers(1, &FollowRootSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, FollowRootSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairStrands * sizeof(float), tfxassets.m_pFollowRootOffset, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, FollowRootSSBO);

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


  Sampler = SamplerHelper::createNearestSampler();

  glDepthFunc(GL_LEQUAL);
}

void clean()
{
  glDeleteTextures(1, &MainTexture);
  glDeleteTextures(1, &DepthStencilTexture);
  glDeleteBuffers(1, &ConstantBuffer);
  glDeleteBuffers(1, &ConstantSimBuffer);
  glDeleteBuffers(1, &PixelCountAtomic);
  glDeleteTextures(1, &HairShadowMapDepth);
  glDeleteTextures(1, &HairShadowMapTexture);


  glDeleteBuffers(1, &StrandTypeSSBO);
  glDeleteBuffers(1, &PrevPosSSBO);
  glDeleteBuffers(1, &PosSSBO);
  glDeleteBuffers(1, &InitialPosSSBO);
  glDeleteBuffers(1, &PerPixelLinkedListSSBO);
  glDeleteBuffers(1, &LengthSSBO);
  glDeleteBuffers(1, &TangentSSBO);
  glDeleteBuffers(1, &RotationSSBO);
  glDeleteBuffers(1, &LocalRefSSBO);
  glDeleteBuffers(1, &ThicknessSSBO);
  glDeleteBuffers(1, &FollowRootSSBO);

  glDeleteVertexArrays(1, &TFXVao);
  glDeleteVertexArrays(1, &TFXVaoLine);
  glDeleteBuffers(1, &TFXLineIdx);
  glDeleteBuffers(1, &TFXTriangleIdx);
  glDeleteTextures(1, &PerPixelLinkedListHeadTexture);
  glDeleteSamplers(1, &Sampler);

  delete PerPixelLinkedListHeadFBO;
  delete MainFBO;
  delete HairSMFBO;

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

  struct Constants cbuf = {};
  cbuf.g_MatBaseColor[0] = 98. / 255.;
  cbuf.g_MatBaseColor[1] = 14. / 255.;
  cbuf.g_MatBaseColor[2] = 4. / 255.;

  cbuf.g_MatKValue[0] = 0.; // Ka
  cbuf.g_MatKValue[1] = 0.4; // Kd
  cbuf.g_MatKValue[2] = 0.04; // Ks1
  cbuf.g_MatKValue[3] = 80.; // Ex1
  cbuf.g_fHairKs2 = .5; // Ks2
  cbuf.g_fHairEx2 = 8.0; // Ex2

  cbuf.g_FiberRadius = 0.3;
  cbuf.g_FiberAlpha = .5;
  cbuf.g_FiberSpacing = .3;
  cbuf.g_alphaThreshold = .00388;
  cbuf.g_HairShadowAlpha = .004000;

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

  irr::core::matrix4 View, InvView, tmp, LightMatrix;
  tmp.buildCameraLookAtMatrixRH(irr::core::vector3df(0., 0., 200.), irr::core::vector3df(0., 0., 0.), irr::core::vector3df(0., 1., 0.));
  View.buildProjectionMatrixPerspectiveFovRH(70. / 180. * 3.14, 1., 1., 1000.);
  View *= tmp;
  View.getInverse(InvView);

  cbuf.g_vEye[0] = 0.;
  cbuf.g_vEye[1] = 0.;
  cbuf.g_vEye[2] = 200.;

  irr::core::matrix4 Model;

  LightMatrix.buildProjectionMatrixPerspectiveFovRH(0.6, 1., 532, 769);

//  LightMatrix.print();
  tmp.buildCameraLookAtMatrixRH(irr::core::vector3df(cbuf.g_PointLightPos[0], cbuf.g_PointLightPos[1] = 306.79, cbuf.g_PointLightPos[2] = 343.),
    irr::core::vector3df(0., 0., 0), irr::core::vector3df(0., 1., 0.));
  LightMatrix *= tmp;
  memcpy(cbuf.g_mWorld, Model.pointer(), 16 * sizeof(float));
  memcpy(cbuf.g_mViewProj, View.pointer(), 16 * sizeof(float));
  memcpy(cbuf.g_mInvViewProj, InvView.pointer(), 16 * sizeof(float));
  memcpy(cbuf.g_mViewProjLight, LightMatrix.pointer(), 16 * sizeof(float));

/*  float World[16] = {
    1., 0., 0., -26.,
    0., 1., 0., 36.,
    0., 0., 1., -58.,
    0., 0., 0., 1.
  };
  float ViewProj[16] = {
    -1.441579, -0.000000, 1.095600, 0.000000,
    -0.138931, 2.403270, -0.182805, -96.130806,
    -0.603548, -0.095297, -0.794142, 309.860260,
    -0.602341, -0.095106, -0.792553, 319.240540
  };

  float invViewProj[16] = {
    -0.439708, -0.023837, -18.962017, 18.397676,
    0.000000, 0.412336, -6.986005, 6.904898,
    0.334179, -0.031364, -24.950022, 24.207466,
    0.000000, 0.000000, -0.099800, 0.100000
  };
  float LightMat[16] = {
    -2.397597, 0.000000, 2.647745, 101.223297,
    -0.983831, 3.316237, -0.890883, -297.174957,
    -2.245398, -1.212366, -2.033262, 277.551025,
    -0.688184, -0.371573, -0.623167, 617.776917,
  };

  float InvViewport[16] = {
    -0.000859, 0.000062, -18.962017, 18.813549,
    0.000000, -0.001074, -6.986005, 7.317234,
    0.000653, 0.000082, -24.950022, 23.841923,
    0.000000, 0.000000, -0.099800, 0.100000
  };
  memcpy(cbuf.g_mInvViewProjViewport, InvViewport, 16 * sizeof(float));

  memcpy(cbuf.g_mViewProjLight, LightMat, 16 * sizeof(float));

//  tmp.print();

  memcpy(cbuf.g_mWorld, World, 16 * sizeof(float));
  memcpy(cbuf.g_mViewProj, ViewProj, 16 * sizeof(float));
  memcpy(cbuf.g_mInvViewProj, invViewProj, 16 * sizeof(float));*/
  cbuf.g_WinSize[0] = 1024.;
  cbuf.g_WinSize[1] = 1024.;
  cbuf.g_WinSize[2] = 1. / 1024.;
  cbuf.g_WinSize[3] = 1. / 1024.;

  cbuf.g_fNearLight = 532.711365;
  cbuf.g_fFarLight = 768.133728;


  glBindBuffer(GL_UNIFORM_BUFFER, ConstantBuffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(struct Constants), &cbuf, GL_STATIC_DRAW);
}

GLsync syncobj;

void simulate(float time)
{
  struct SimulationConstants cbuf = {};

  irr::core::matrix4 Model;
  if (time < 30.)
    cbuf.bWarp = 1;
  else
    cbuf.bWarp = 0;


  float p;

  float dir = modf(time / (360. * 360.), &p);
  bool clockwise = ((int)p) % 2;
  time = 360 * 360 * dir * (clockwise ? 1 : -1);

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
  cbuf.NumFollowHairsPerOneGuideHair = tfxassets.m_NumFollowHairsPerOneGuideHair;

  cbuf.GravityMagnitude = 10.;
  cbuf.NumLengthConstraintIterations = 2;
  cbuf.NumLocalShapeMatchingIterations = 1;

  memset(cbuf.Wind, 0, 4 * sizeof(float));
  memset(cbuf.Wind1, 0, 4 * sizeof(float));
  memset(cbuf.Wind2, 0, 4 * sizeof(float));
  memset(cbuf.Wind3, 0, 4 * sizeof(float));

  if (syncobj)
  {
    GLenum status = glClientWaitSync(syncobj, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
    while (status == GL_TIMEOUT_EXPIRED)
    {
      status = glClientWaitSync(syncobj, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
    }
    glDeleteSync(syncobj);
  }

  glBindBuffer(GL_UNIFORM_BUFFER, ConstantSimBuffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(struct SimulationConstants), &cbuf, GL_STATIC_DRAW);

  int numOfGroupsForCS_VertexLevel = (int)((density + .05) * (tfxassets.m_NumGuideHairVertices / 64));

  // Prepare follow hair guide
  glUseProgram(PrepareFollowHairGuide::getInstance()->Program);
  glDispatchCompute(numOfGroupsForCS_VertexLevel, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

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

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glUseProgram(UpdateFollowHairGuide::getInstance()->Program);
  glDispatchCompute(numOfGroupsForCS_VertexLevel, 1, 1);
}

void draw(float time)
{
  fillConstantBuffer(time);
  // Reset PixelCount
  int pxcnt = 1;
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
  glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(unsigned int), &pxcnt);

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
  HairShadow::getInstance()->setUniforms();
  glDrawElementsBaseVertex(GL_LINES, density * tfxassets.m_Triangleindices.size(), GL_UNSIGNED_INT, 0, 0);

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
  char t[50];
  {
    GLuint timer;
    glGenQueries(1, &timer);
    glBeginQuery(GL_TIME_ELAPSED, timer);
    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBindVertexArray(TFXVao);
    glUseProgram(Transparent::getInstance()->Program);
    Transparent::getInstance()->SetTextureUnits(PerPixelLinkedListHeadTexture, GL_READ_WRITE, GL_R32UI);
    Transparent::getInstance()->setUniforms();
    glDrawElementsBaseVertex(GL_TRIANGLES, density * tfxassets.m_Triangleindices.size(), GL_UNSIGNED_INT, 0, 0);

    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glEndQuery(GL_TIME_ELAPSED);
    GLuint result;
    glGetQueryObjectuiv(timer, GL_QUERY_RESULT, &result);

    sprintf(t, "First pass: %f ms", result / 1000000.);
  }

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_EQUAL, 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  glEnable(GL_BLEND);
  glEnable(GL_FRAMEBUFFER_SRGB);
  glClearColor(0.25f, 0.25f, 0.35f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glBlendFunc(GL_ONE, GL_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);


  FragmentMerge::getInstance()->SetTextureUnits(HairShadowMapDepth, Sampler, PerPixelLinkedListHeadTexture, GL_READ_ONLY, GL_R32UI);
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

  BasicTextRender<14>::getInstance()->drawText(t, 0, 20, 1024, 1024);
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
    syncobj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glfwSwapBuffers(window);
    glfwPollEvents();
    tmp += 300.;
  }
  clean();
  glfwTerminate();
  return 0;
}
