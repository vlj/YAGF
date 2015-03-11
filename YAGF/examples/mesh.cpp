
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

std::vector<std::tuple<unsigned, unsigned, unsigned> > CountBaseIndexVTX;
FrameBuffer *MainFBO;
FrameBuffer *LinearDepthFBO;

GLuint TrilinearSampler;

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
    irr::io::CReadFile *reader = new irr::io::CReadFile("..\\examples\\anchor.b3d");
    irr::scene::CB3DMeshFileLoader *loader = new irr::scene::CB3DMeshFileLoader();
    std::vector<irr::scene::SMeshBufferLightMap> buffers = loader->createMesh(reader);

    for (auto tmp : buffers)
    {
        std::pair<unsigned, unsigned> BaseIndexVtx = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords> >::getInstance()->getBase(&tmp);
        CountBaseIndexVTX.push_back(std::make_tuple(tmp.getIndexCount(), BaseIndexVtx.first, BaseIndexVtx.second));
    }

    TrilinearSampler = SamplerHelper::createBilinearSampler();

    glDepthFunc(GL_LEQUAL);
    delete loader;
}

void clean()
{
    glDeleteSamplers(1, &TrilinearSampler);
}

void draw()
{
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 1024, 1024);

    glClearColor(0., 0., 0., 1.);
    glClearDepth(1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    irr::core::matrix4 Model, View;
    View.buildProjectionMatrixPerspectiveFovLH(70. / 180. * 3.14, 1., 1., 100.);
    Model.setTranslation(irr::core::vector3df(0., 0., 8.));
    Model.setRotationDegrees(irr::core::vector3df(270., 0., 0.));

    glUseProgram(ObjectShader::getInstance()->Program);
    glBindVertexArray(VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords> >::getInstance()->getVAO());
    ObjectShader::getInstance()->setUniforms(Model, View);
    for (auto tmp : CountBaseIndexVTX)
        glDrawElementsBaseVertex(GL_TRIANGLES, std::get<0>(tmp), GL_UNSIGNED_SHORT, (void *)std::get<1>(tmp), std::get<2>(tmp));
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

