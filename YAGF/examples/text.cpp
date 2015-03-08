
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

#include <ft2build.h>

GLuint InitialTexture;
GLuint MainTexture;
GLuint AuxTexture;

GLuint BilinearSampler;

static const char *passthrougfs =
"#version 330\n"
"uniform sampler2D tex;\n"
"out vec4 FragColor;\n"
"void main() {\n"
"vec2 uv = gl_FragCoord.xy / 1024.;\n"
"FragColor = texture(tex,uv);\n"
"}\n";

class FullScreenPassthrough : public ShaderHelperSingleton<FullScreenPassthrough>, public TextureRead<Texture2D>
{
public:
    FullScreenPassthrough()
    {
        Program = ProgramShaderLoading::LoadProgram(
            GL_VERTEX_SHADER, screenquadshader,
            GL_FRAGMENT_SHADER, passthrougfs);
        AssignUniforms();

        AssignSamplerNames(Program, 0, "tex");
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

}

void clean()
{

}

void draw()
{

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

