#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <Core/VAO.h>
#include <GLAPI/S3DVertex.h>
#include <GLAPI/Texture.h>

#include <Core/Shaders.h>
#include <Core/FBO.h>
#include <Util/Misc.h>
#include <Util/Samplers.h>
#include <Util/Text.h>
#include <Util/Debug.h>

#include <Loaders/B3D.h>
#include <Loaders/DDS.h>

std::vector<std::tuple<size_t, size_t, size_t> > CountBaseIndexVTX;
FrameBuffer *MainFBO;
FrameBuffer *LinearDepthFBO;

GLuint TrilinearSampler;

const char *vtxshader =
"#version 330\n"
"layout(std140) uniform Matrices\n"
"{\n"
"mat4 ModelMatrix;\n"
"mat4 ViewProjectionMatrix;\n"
"};\n"
"layout(location = 0) in vec3 Position;\n"
"layout(location = 3) in vec2 Texcoord;\n"
"out vec2 uv;\n"
"out vec3 color;\n"
"void main(void) {\n"
"  gl_Position = ViewProjectionMatrix * ModelMatrix * vec4(Position, 1.);\n"
"  uv = Texcoord;\n"
"}\n";

const char *fragshader =
"#version 330\n"
"uniform sampler2D tex;\n"
"in vec2 uv;\n"
"out vec4 FragColor;\n"
"void main() {\n"
"  FragColor = texture(tex, uv);\n"
"}\n";

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

class ObjectShader : public ShaderHelperSingleton<ObjectShader>, public TextureRead<UniformBufferResource<0>, TextureResource<GL_TEXTURE_2D, 0> >
{
public:
    ObjectShader()
    {
        Program = ProgramShaderLoading::LoadProgram(
            GL_VERTEX_SHADER, vtxshader,
            GL_FRAGMENT_SHADER, fragshader);

        AssignSamplerNames(Program, "Matrices", "tex");
    }
};

Texture *texture;

struct Matrixes
{
    float Model[16];
    float ViewProj[16];
};

GLuint cbuf;

void init()
{
  DebugUtil::enableDebugOutput();
  irr::io::CReadFile reader("..\\examples\\anchor.b3d");
  irr::scene::CB3DMeshFileLoader loader(&reader);
  std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > buffers = loader.AnimatedMesh;

  for (auto tmp : buffers)
  {
    const irr::scene::SMeshBufferLightMap &tmpbuf = tmp.first;
    std::pair<size_t, size_t> BaseIndexVtx = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords> >::getInstance()->getBase(&tmpbuf);
    CountBaseIndexVTX.push_back(std::make_tuple(tmpbuf.getIndexCount(), BaseIndexVtx.first, BaseIndexVtx.second));
  }

  std::ifstream DDSFile("..\\examples\\anchorBC5.DDS", std::ifstream::binary);
  irr::video::CImageLoaderDDS DDSPic(DDSFile);

  texture = new Texture(DDSPic.getLoadedImage());

  glGenBuffers(1, &cbuf);

  TrilinearSampler = SamplerHelper::createTrilinearSampler();

  glDepthFunc(GL_LEQUAL);
}

void clean()
{
    glDeleteSamplers(1, &TrilinearSampler);
    delete(texture);
    glDeleteBuffers(1, &cbuf);
}

void draw()
{
    glEnable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 1024, 1024);

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClearDepth(1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Matrixes cbufdata;
    irr::core::matrix4 Model, View;
    View.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f, 100.f);
    Model.setTranslation(irr::core::vector3df(0.f, 0.f, 8.f));
    Model.setRotationDegrees(irr::core::vector3df(270.f, 0.f, 0.f));
    memcpy(cbufdata.Model, Model.pointer(), 16 * sizeof(float));
    memcpy(cbufdata.ViewProj, View.pointer(), 16 * sizeof(float));

    glBindBuffer(GL_UNIFORM_BUFFER, cbuf);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(struct Matrixes), &cbufdata, GL_STATIC_DRAW);

    glUseProgram(ObjectShader::getInstance()->Program);
    glBindVertexArray(VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords> >::getInstance()->getVAO());
    ObjectShader::getInstance()->SetTextureUnits(cbuf, texture->Id, TrilinearSampler);
    for (auto tmp : CountBaseIndexVTX)
      glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)std::get<0>(tmp), GL_UNSIGNED_SHORT, (void *)std::get<1>(tmp), (GLsizei)std::get<2>(tmp));
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

