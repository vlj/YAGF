#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <GLAPI/VAO.h>
#include <GLAPI/GLS3DVertex.h>

#include <GLAPI/Shaders.h>
#include <GLAPI/GLRTTSet.h>
#include <GLAPI/Misc.h>
#include <GLAPI/Samplers.h>
#include <GLAPI/Debug.h>
#include <GLAPI/Text.h>

irr::scene::IMeshBuffer<irr::video::S3DVertex> *buffer;
GLRTTSet *MainFBO;
// For clearing
GLRTTSet *PerPixelLinkedListHeadFBO;

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
"layout(std140) uniform OITBuffer\n"
"{\n"
"  mat4 ModelMatrix;\n"
"  mat4 ViewProjectionMatrix;\n"
"  vec4 color;\n"
"};\n"
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
"layout(std140) uniform OITBuffer\n"
"{\n"
"  mat4 ModelMatrix;\n"
"  mat4 ViewProjectionMatrix;\n"
"  vec4 color;\n"
"};\n"
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

class Transparent : public ShaderHelperSingleton<Transparent>, public TextureRead<UniformBufferResource<0>, ImageResource<0> >
{
public:
    Transparent()
    {
        Program = ProgramShaderLoading::LoadProgram(
            GL_VERTEX_SHADER, vtxshader,
            GL_FRAGMENT_SHADER, fragshader);
        AssignSamplerNames(Program, "OITBuffer", "PerPixelLinkedListHead");
    }
};

class FragmentMerge : public ShaderHelperSingleton<FragmentMerge>, public TextureRead<ImageResource<0> >
{
public:
    FragmentMerge()
    {
        Program = ProgramShaderLoading::LoadProgram(
            GL_VERTEX_SHADER, screenquadshader,
            GL_FRAGMENT_SHADER, fragmerge);
        AssignSamplerNames(Program, "PerPixelLinkedListHead");
    }
};

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

struct OITBuffer
{
    float Model[16];
    float ViewProj[16];
    float color[4];
};

GLuint cbuf;

void init()
{
    DebugUtil::enableDebugOutput();
    buffer = GeometryCreator::createCubeMeshBuffer(
        irr::core::vector3df(1., 1., 1.));
    auto tmp = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> >::getInstance()->getBase(buffer);

    DepthStencilTexture = generateRTT(1024, 1024, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
    MainTexture = generateRTT(1024, 1024, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
    glGenTextures(1, &PerPixelLinkedListHeadTexture);
    glBindTexture(GL_TEXTURE_2D, PerPixelLinkedListHeadTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, 1024, 1024);

    MainFBO = new GLRTTSet({ MainTexture }, DepthStencilTexture, 1024, 1024);
    PerPixelLinkedListHeadFBO = new GLRTTSet({ PerPixelLinkedListHeadTexture }, 1024, 1024);

    glGenBuffers(1, &PerPixelLinkedListSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, PerPixelLinkedListSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 1000000 * sizeof(PerPixelListBucket), 0, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, PerPixelLinkedListSSBO);

    glGenBuffers(1, &PixelCountAtomic);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned int), 0, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &cbuf);

    BilinearSampler = SamplerHelper::createBilinearSampler();

    glDepthFunc(GL_LEQUAL);
}

void clean()
{
    glDeleteSamplers(1, &BilinearSampler);
    glDeleteTextures(1, &MainTexture);
    glDeleteBuffers(1, &cbuf);
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
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    MainFBO->Bind();
    glClearDepth(1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    irr::core::matrix4 Model, View;
    View.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
    Model.setTranslation(irr::core::vector3df(0.f, 0.f, 8.f));

    struct OITBuffer cbufdata;

    memcpy(cbufdata.ViewProj, View.pointer(), 16 * sizeof(float));


    glUseProgram(Transparent::getInstance()->Program);
    glBindVertexArray(VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> >::getInstance()->getVAO());

    memcpy(cbufdata.Model, Model.pointer(), 16 * sizeof(float));
    cbufdata.color[0] = 0.f;
    cbufdata.color[1] = 1.f;
    cbufdata.color[2] = 1.f;
    cbufdata.color[3] = .5f;
    glBindBuffer(GL_UNIFORM_BUFFER, cbuf);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(struct OITBuffer), &cbufdata, GL_STATIC_DRAW);
    Transparent::getInstance()->SetTextureUnits(cbuf, PerPixelLinkedListHeadTexture, GL_READ_WRITE, GL_R32UI);
    glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)buffer->getIndexCount(), GL_UNSIGNED_SHORT, 0, 0);

    Model.setTranslation(irr::core::vector3df(0.f, 0.f, 10.f));
    Model.setScale(2.);
    memcpy(cbufdata.Model, Model.pointer(), 16 * sizeof(float));
    cbufdata.color[0] = 1.f;
    cbufdata.color[1] = 1.f;
    cbufdata.color[2] = 0.f;
    cbufdata.color[3] = .5f;
    glBindBuffer(GL_UNIFORM_BUFFER, cbuf);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(struct OITBuffer), &cbufdata, GL_STATIC_DRAW);
    Transparent::getInstance()->SetTextureUnits(cbuf, PerPixelLinkedListHeadTexture, GL_READ_WRITE, GL_R32UI);
    glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)buffer->getIndexCount(), GL_UNSIGNED_SHORT, 0, 0);
    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    FragmentMerge::getInstance()->SetTextureUnits(PerPixelLinkedListHeadTexture, GL_READ_ONLY, GL_R32UI);
    DrawFullScreenEffect<FragmentMerge>();

//    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
    int *tmp = (int*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
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