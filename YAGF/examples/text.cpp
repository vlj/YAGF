
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

extern "C" {
#include <ft2build.h>
#include FT_FREETYPE_H
}

GLuint TextureA;

GLuint BilinearSampler;

FT_Library Ft;
FT_Face Face;

static const char *passthrougfs =
"#version 330\n"
"uniform sampler2D tex;\n"
"out vec4 FragColor;\n"
"void main() {\n"
"vec2 uv = gl_FragCoord.xy / vec2(1024., -1024);\n"
"FragColor = vec4(1., 1., 1., texture(tex,uv).r);\n"
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

void init()
{
    DebugUtil::enableDebugOutput();
    if (FT_Init_FreeType(&Ft))
        printf("Can't init freetype\n");
    if (FT_New_Face(Ft, "C:\\Windows\\Fonts\\arial.ttf", 0, &Face))
        printf("Can't init Arial\n");
    FT_Set_Pixel_Sizes(Face, 0, 48);

    if (FT_Load_Char(Face, 'B', FT_LOAD_RENDER)) {
        printf("Could not load character 'X'\n");
    }

    glGenTextures(1, &TextureA);
    glBindTexture(GL_TEXTURE_2D, TextureA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Face->glyph->bitmap.width, Face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, Face->glyph->bitmap.buffer);
    glBindTexture(GL_TEXTURE_2D, 0);

    BilinearSampler = SamplerHelper::createBilinearSampler();
}

void clean()
{
    glDeleteTextures(1, &TextureA);
}

void draw()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 1024, 1024);
    glClearColor(0., 0., 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    FullScreenPassthrough::getInstance()->SetTextureUnits(TextureA, BilinearSampler);
    DrawFullScreenEffect<FullScreenPassthrough>();
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
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

