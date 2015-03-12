
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

FrameBuffer *MainFBO;
// For clearing
FrameBuffer *PerPixelLinkedListHeadFBO;

GLuint DepthStencilTexture;
GLuint MainTexture;

GLuint PerPixelLinkedListHeadTexture;
GLuint PerPixelLinkedListSSBO;
GLuint PixelCountAtomic;

GLuint BilinearSampler;


GLuint TFXVao;
GLuint TFXVbo;
GLuint TFXTriangleIdx;
GLuint ConstantBuffer;

struct PerPixelListBucket
{
  float depth;
  unsigned int TangentAndCoverage;
  unsigned int next;
};

class Transparent : public ShaderHelperSingleton<Transparent, irr::core::matrix4, irr::core::matrix4, irr::video::SColorf>, public TextureRead<Image2D>
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
    AssignUniforms("ModelMatrix", "ViewProjectionMatrix", "color");
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
  glBufferData(GL_ARRAY_BUFFER, tfxassets.m_NumTotalHairVertices * sizeof(StrandVertex), tfxassets.m_pTriangleVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StrandVertex), 0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(StrandVertex), (GLvoid *) (3 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(StrandVertex), (GLvoid *)(6 * sizeof(float)));

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
  glBufferData(GL_SHADER_STORAGE_BUFFER, 10000000 * sizeof(PerPixelListBucket), 0, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, PerPixelLinkedListSSBO);

  glGenBuffers(1, &PixelCountAtomic);
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
  glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned int), 0, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, PixelCountAtomic);

  glGenBuffers(1, &ConstantBuffer);
  glBindBuffer(GL_UNIFORM_BUFFER, ConstantBuffer);
  glBufferData(GL_UNIFORM_BUFFER, 0, 0, GL_STATIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, ConstantBuffer);

  BilinearSampler = SamplerHelper::createBilinearSampler();

  glDepthFunc(GL_LEQUAL);
}

void clean()
{
  glDeleteSamplers(1, &BilinearSampler);
  glDeleteTextures(1, &MainTexture);
}

void draw(float time)
{
  // Reset PixelCount
  int pxcnt = 1;
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
  glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(unsigned int), &pxcnt);
//  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  PerPixelLinkedListHeadFBO->Bind();
  glClearColor(0., 0., 0., 1.);
  glClear(GL_COLOR_BUFFER_BIT);
  MainFBO->Bind();
  glClearDepth(1.);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  irr::core::matrix4 Model, View;
  View.buildProjectionMatrixPerspectiveFovLH(70. / 180. * 3.14, 1., 1., 1000.);
  Model.setTranslation(irr::core::vector3df(0., 0., 200.));
  Model.setInverseRotationDegrees(irr::core::vector3df(0., time / 360., 0.));

  glUseProgram(Transparent::getInstance()->Program);
  glBindVertexArray(TFXVao);
  Transparent::getInstance()->SetTextureUnits(PerPixelLinkedListHeadTexture, GL_READ_WRITE, GL_R32UI);
  Transparent::getInstance()->setUniforms(Model, View, irr::video::SColorf(0., 1., 1., .5));
  glDrawElementsBaseVertex(GL_TRIANGLES, .05 * tfxassets.m_Triangleindices.size(), GL_UNSIGNED_INT, 0, 0);
  glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  glViewport(0, 0, 1024, 1024);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  FragmentMerge::getInstance()->SetTextureUnits(PerPixelLinkedListHeadTexture, GL_READ_ONLY, GL_R32UI);
  DrawFullScreenEffect<FragmentMerge>();
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
    draw(tmp);
    glfwSwapBuffers(window);
    glfwPollEvents();
    tmp += 300.;
  }
  clean();
  glfwTerminate();
  return 0;
}