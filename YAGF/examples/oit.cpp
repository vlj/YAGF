
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

irr::scene::IMeshBuffer *buffer;
FrameBuffer *MainFBO;

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
"out vec3 color;\n"
"void main(void) {\n"
"  gl_Position = ViewProjectionMatrix * ModelMatrix * vec4(Position, 1.);\n"
"  color = vec3(1.);\n"
"}\n";

const char *fragshader =
"#version 430 core\n"
"layout (binding = 0) uniform atomic_uint PixelCount;\n"
"layout(r32ui) uniform volatile restrict uimage2D PerPixelLinkedListHead;\n"
"in vec3 color;\n"
"out vec4 FragColor;\n"
"struct PerPixelListBucket\n"
"{\n"
"    float depth;\n"
"    float red;\n"
"    float blue;\n"
"    float green;\n"
"    float alpha;\n"
"    uint next;\n"
"};\n"
"layout(std140, binding = 1) buffer PerPixelLinkedList\n"
"{\n"
"    PerPixelListBucket PPLL[1000000];"
"};"
"void main() {\n"
"  uint pixel_id = atomicCounterIncrement(PixelCount);"
"  int pxid = int(pixel_id);"
"  ivec2 iuv = ivec2(gl_FragCoord.xy);"
"  uint tmp = imageAtomicExchange(PerPixelLinkedListHead, iuv, pixel_id);"
"  PPLL[pxid].depth = gl_FragCoord.z;\n"
"  PPLL[pxid].red = 1.;\n"
"  PPLL[pxid].blue = 1.;\n"
"  PPLL[pxid].green = 1.;\n"
"  PPLL[pxid].alpha = 1.;\n"
"  PPLL[pxid].next = tmp;\n"
"  FragColor = vec4(vec3(tmp / 70000.), 1.);\n"
"}\n";

class Transparent : public ShaderHelperSingleton<Transparent, irr::core::matrix4, irr::core::matrix4>, public TextureRead<Image2D>
{
public:
    Transparent()
    {
        Program = ProgramShaderLoading::LoadProgram(
            GL_VERTEX_SHADER, vtxshader,
            GL_FRAGMENT_SHADER, fragshader);
        AssignUniforms("ModelMatrix", "ViewProjectionMatrix");
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

    MainFBO = new FrameBuffer({ MainTexture }, DepthStencilTexture, 1024, 1024);

    glGenBuffers(1, &PerPixelLinkedListSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, PerPixelLinkedListSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 1000000 * sizeof(PerPixelListBucket), 0, GL_STATIC_DRAW);

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
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    MainFBO->Bind();
    glClearColor(0., 0., 0., 1.);
    glClearDepth(1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    irr::core::matrix4 Model, View;
    View.buildProjectionMatrixPerspectiveFovLH(70. / 180. * 3.14, 1., 1., 100.);
    Model.setTranslation(irr::core::vector3df(0., 0., 8.));

    glUseProgram(Transparent::getInstance()->Program);
    glBindVertexArray(VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> >::getInstance()->getVAO());
    Transparent::getInstance()->SetTextureUnits(PerPixelLinkedListHeadTexture, GL_READ_WRITE, GL_R32UI);
    Transparent::getInstance()->setUniforms(Model, View);
    glDrawElementsBaseVertex(GL_TRIANGLES, buffer->getIndexCount(), GL_UNSIGNED_SHORT, 0, 0);

    Model.setTranslation(irr::core::vector3df(0., 0., 10.));
    Model.setScale(2.);
    Transparent::getInstance()->setUniforms(Model, View);
    glDrawElementsBaseVertex(GL_TRIANGLES, buffer->getIndexCount(), GL_UNSIGNED_SHORT, 0, 0);
    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);

//    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, PixelCountAtomic);
    int *tmp = (int*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
    printf("%d\n", *tmp);
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
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