
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <Core/VAO.h>

#include <Core/Shaders.h>
#include <Core/FBO.h>
#include <Util/Misc.h>
#include <Util/Samplers.h>
#include <Util/Text.h>

#include <Loaders/B3D.h>

irr::scene::IMeshBuffer *buffer;
FrameBuffer *MainFBO;
FrameBuffer *LinearDepthFBO;

GLuint DepthStencilTexture;
GLuint MainTexture;
GLuint LinearTexture;

GLuint NearestSampler, TrilinearSampler;

const char *vtxshader =
"#version 330\n"
"uniform mat4 ModelMatrix;\n"
"uniform mat4 ViewProjectionMatrix;\n"
"layout(location = 0) in vec3 Position;\n"
"out vec3 color;\n"
"void main(void) {\n"
"  gl_Position = ViewProjectionMatrix * ModelMatrix * vec4(Position, 1.);\n"
"  color = vec3(1.);\n"
"}\n";

const char *fragshader =
"#version 330\n"
"in vec3 color;\n"
"out vec4 FragColor;\n"
"void main() {\n"
"  FragColor = vec4(color, 1.);\n"
"}\n";

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

class ObjectShader : public ShaderHelperSingleton<ObjectShader, irr::core::matrix4, irr::core::matrix4>, public TextureRead<>
{
public:
    ObjectShader()
    {
        Program = ProgramShaderLoading::LoadProgram(
            GL_VERTEX_SHADER, vtxshader,
            GL_FRAGMENT_SHADER, fragshader);
        AssignUniforms("ModelMatrix", "ViewProjectionMatrix");
    }
};

void init()
{
    buffer = GeometryCreator::createCubeMeshBuffer(
        irr::core::vector3df(1., 1., 1.));
    auto tmp = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> >::getInstance()->getBase(buffer);

    DepthStencilTexture = generateRTT(1024, 1024, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
    MainTexture = generateRTT(1024, 1024, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
    LinearTexture = generateRTT(1024, 1024, GL_R32F, GL_RED, GL_FLOAT);

    NearestSampler = SamplerHelper::createNearestSampler();
    TrilinearSampler = SamplerHelper::createBilinearSampler();

    glDepthFunc(GL_LEQUAL);
}

void clean()
{
    delete buffer;
    delete MainFBO;
    delete LinearDepthFBO;
    glDeleteSamplers(1, &NearestSampler);
    glDeleteTextures(1, &DepthStencilTexture);
    glDeleteTextures(1, &MainTexture);
    glDeleteTextures(1, &LinearTexture);
}

void draw()
{
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindBuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 1024, 1024);

    glClearColor(0., 0., 0., 1.);
    glClearDepth(1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    irr::core::matrix4 Model, View;
    View.buildProjectionMatrixPerspectiveFovLH(70. / 180. * 3.14, 1., 1., 100.);
    Model.setTranslation(irr::core::vector3df(0., 0., 8.));

    glUseProgram(ObjectShader::getInstance()->Program);
    glBindVertexArray(VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> >::getInstance()->getVAO());
    ObjectShader::getInstance()->setUniforms(Model, View);
    glDrawElementsBaseVertex(GL_TRIANGLES, buffer->getIndexCount(), GL_UNSIGNED_SHORT, 0, 0);

    Model.setTranslation(irr::core::vector3df(0., 0., 10.));
    Model.setScale(2.);
    ObjectShader::getInstance()->setUniforms(Model, View);
    glDrawElementsBaseVertex(GL_TRIANGLES, buffer->getIndexCount(), GL_UNSIGNED_SHORT, 0, 0);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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

