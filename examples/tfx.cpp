
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

irr::scene::IMeshBuffer<irr::video::S3DVertex> *buffer;
FrameBuffer *MainFBO;
// For clearing
FrameBuffer *PerPixelLinkedListHeadFBO;

GLuint DepthStencilTexture;
GLuint MainTexture;

GLuint PerPixelLinkedListHeadTexture;
GLuint PerPixelLinkedListSSBO;
GLuint PixelCountAtomic;

GLuint BilinearSampler;

struct PerPixelListBucket
{
  float depth;
  float red;
  float blue;
  float green;
  float alpha;
  unsigned int next;
};

const char *vtxshader =
"#version 430\n"
"uniform mat4 ModelMatrix;\n"
"uniform mat4 ViewProjectionMatrix;\n"
"layout(location = 0) in vec3 Position;\n"
"out float depth;"
"void main(void) {\n"
"  gl_Position = ViewProjectionMatrix * ModelMatrix * vec4(Position, 1.);\n"
"  depth = gl_Position.z;"
"}\n";

const char *fragshader =
"#version 430 core\n"
"layout (binding = 0) uniform atomic_uint PixelCount;\n"
"layout(r32ui) uniform volatile restrict uimage2D PerPixelLinkedListHead;\n"
"uniform vec4 color;\n"
"in float depth;"
"out vec4 FragColor;\n"
"struct PerPixelListBucket\n"
"{\n"
"    float depth;\n"
"    vec4 color;\n"
"    uint next;\n"
"};\n"
"layout(std430, binding = 1) buffer PerPixelLinkedList\n"
"{\n"
"    PerPixelListBucket PPLL[1000000];"
"};"
"void main() {\n"
"  uint pixel_id = atomicCounterIncrement(PixelCount);\n"
"  int pxid = int(pixel_id);\n"
"  ivec2 iuv = ivec2(gl_FragCoord.xy);\n"
"  uint tmp = imageAtomicExchange(PerPixelLinkedListHead, iuv, pixel_id);\n"
"  PPLL[pxid].depth = depth;\n"
"  PPLL[pxid].color = color;\n"
"  PPLL[pxid].next = tmp;\n"
"  FragColor = vec4(0.);\n"
"}\n";

const char *fragmerge =
"#version 430 core\n"
"layout(r32ui) uniform volatile restrict uimage2D PerPixelLinkedListHead;\n"
"out vec4 FragColor;\n"
"struct PerPixelListBucket\n"
"{\n"
"    float depth;\n"
"    vec4 color;\n"
"    uint next;\n"
"};\n"
"layout(std430, binding = 1) buffer PerPixelLinkedList\n"
"{\n"
"    PerPixelListBucket PPLL[1000000];\n"
"};\n"
"void BubbleSort(uint ListBucketHead) {\n"
"  bool isSorted = false;\n"
"  while (!isSorted) {\n"
"    isSorted = true;\n"
"    uint ListBucketId = ListBucketHead;\n"
"    uint NextListBucketId = PPLL[ListBucketId].next;\n"
"    while (NextListBucketId != 0) {\n"
"    if (PPLL[ListBucketId].depth < PPLL[NextListBucketId].depth) {\n"
"        isSorted = false;\n"
"        float tmp = PPLL[ListBucketId].depth;\n"
"        PPLL[ListBucketId].depth = PPLL[NextListBucketId].depth;\n"
"        PPLL[NextListBucketId].depth = tmp;\n"
"        vec4 ctmp = PPLL[ListBucketId].color;\n"
"        PPLL[ListBucketId].color = PPLL[NextListBucketId].color;\n"
"        PPLL[NextListBucketId].color = ctmp;\n"
"      }\n"
"      ListBucketId = NextListBucketId;\n"
"      NextListBucketId = PPLL[NextListBucketId].next;\n"
"    }\n"
"  }\n"
"}\n"
"\n"
"void main() {\n"
"  ivec2 iuv = ivec2(gl_FragCoord.xy);"
"  uint ListBucketHead = imageLoad(PerPixelLinkedListHead, iuv).x;\n"
"  if (ListBucketHead == 0) discard;\n"
"  BubbleSort(ListBucketHead);\n"
"  uint ListBucketId = ListBucketHead;\n"
"  vec4 result = vec4(0., 0., 0., 1.);"
"  while (ListBucketId != 0) {\n"
"    result.xyz += PPLL[ListBucketId].color.xyz;\n"
"    result *= PPLL[ListBucketId].color.a;\n"
"    ListBucketId = PPLL[ListBucketId].next;\n"
"  }\n"
"  FragColor = result;\n"
"}\n";

class Transparent : public ShaderHelperSingleton<Transparent, irr::core::matrix4, irr::core::matrix4, irr::video::SColorf>, public TextureRead<Image2D>
{
public:
  Transparent()
  {
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vtxshader,
      GL_FRAGMENT_SHADER, fragshader);
    AssignUniforms("ModelMatrix", "ViewProjectionMatrix", "color");
    AssignSamplerNames(Program, 0, "PerPixelLinkedListHead");
  }
};

class FragmentMerge : public ShaderHelperSingleton<FragmentMerge>, public TextureRead<Image2D>
{
public:
  FragmentMerge()
  {
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, screenquadshader,
      GL_FRAGMENT_SHADER, fragmerge);
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



void loadTress(const char* filename)
{
    std::ifstream inFile(filename, std::ios::binary);

    if (!inFile.is_open())
      return;

    TressFXAsset m_HairAsset;

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
    return;
}

void init()
{
  DebugUtil::enableDebugOutput();
  buffer = GeometryCreator::createCubeMeshBuffer(
    irr::core::vector3df(1., 1., 1.));
  auto tmp = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> >::getInstance()->getBase(buffer);

  loadTress("..\\examples\\ruby.tfxb");

  DepthStencilTexture = generateRTT(1024, 1024, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
  MainTexture = generateRTT(1024, 1024, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
  glGenTextures(1, &PerPixelLinkedListHeadTexture);
  glBindTexture(GL_TEXTURE_2D, PerPixelLinkedListHeadTexture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, 1024, 1024);

  MainFBO = new FrameBuffer({ MainTexture }, DepthStencilTexture, 1024, 1024);
  PerPixelLinkedListHeadFBO = new FrameBuffer({ PerPixelLinkedListHeadTexture }, 1024, 1024);

  glGenBuffers(1, &PerPixelLinkedListSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, PerPixelLinkedListSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, 1000000 * sizeof(PerPixelListBucket), 0, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, PerPixelLinkedListSSBO);

  glGenBuffers(1, &PixelCountAtomic);
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
  glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned int), 0, GL_DYNAMIC_DRAW);

  BilinearSampler = SamplerHelper::createBilinearSampler();

  glDepthFunc(GL_LEQUAL);
}

void clean()
{
  glDeleteSamplers(1, &BilinearSampler);
  glDeleteTextures(1, &MainTexture);
}

void draw()
{
  // Reset PixelCount
  int pxcnt = 1;
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
  glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(unsigned int), &pxcnt);
  glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, PixelCountAtomic);
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

  //    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  irr::core::matrix4 Model, View;
  View.buildProjectionMatrixPerspectiveFovLH(70. / 180. * 3.14, 1., 1., 100.);
  Model.setTranslation(irr::core::vector3df(0., 0., 8.));

  glUseProgram(Transparent::getInstance()->Program);
  glBindVertexArray(VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> >::getInstance()->getVAO());
  Transparent::getInstance()->SetTextureUnits(PerPixelLinkedListHeadTexture, GL_READ_WRITE, GL_R32UI);
  Transparent::getInstance()->setUniforms(Model, View, irr::video::SColorf(0., 1., 1., .5));
  glDrawElementsBaseVertex(GL_TRIANGLES, buffer->getIndexCount(), GL_UNSIGNED_SHORT, 0, 0);

  Model.setTranslation(irr::core::vector3df(0., 0., 10.));
  Model.setScale(2.);
  Transparent::getInstance()->setUniforms(Model, View, irr::video::SColorf(1., 1., 0., .5));
  glDrawElementsBaseVertex(GL_TRIANGLES, buffer->getIndexCount(), GL_UNSIGNED_SHORT, 0, 0);
  glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  FragmentMerge::getInstance()->SetTextureUnits(PerPixelLinkedListHeadTexture, GL_READ_ONLY, GL_R32UI);
  DrawFullScreenEffect<FragmentMerge>();

  //    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
  int *tmp = (int*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
  //    printf("%d\n", *tmp);
  glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
}

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  //    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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