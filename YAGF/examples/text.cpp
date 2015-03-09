
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

GLuint Texture[256];

GLuint BilinearSampler;

FT_Library Ft;
FT_Face Face;

GLuint GlyphVAO;
GLuint GlyphVBO;

static const char *glyphvs =
"#version 330\n"
"uniform vec2 GlyphCenter;\n"
"uniform vec2 GlyphScaling;\n"
"layout(location = 0) in vec2 Position;\n"
"layout(location = 1) in vec2 Texcoord;\n"
"out vec2 uv;"
"void main()\n"
"{\n"
"   uv = Texcoord;\n"
"   gl_Position = vec4(GlyphCenter + GlyphScaling * Position, 0., 1.);\n"
"}\n";

static const char *passthrougfs =
"#version 330\n"
"uniform sampler2D tex;\n"
"in vec2 uv;"
"out vec4 FragColor;\n"
"void main() {\n"
"FragColor = vec4(1., 1., 1., texture(tex,uv).r);\n"
"}\n";

class GlyphRendering : public ShaderHelperSingleton<GlyphRendering, irr::core::vector2df, irr::core::vector2df>, public TextureRead<Texture2D>
{
public:
    GlyphRendering()
    {
        Program = ProgramShaderLoading::LoadProgram(
            GL_VERTEX_SHADER, glyphvs,
            GL_FRAGMENT_SHADER, passthrougfs);
        AssignUniforms("GlyphCenter", "GlyphScaling");

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

    glGenVertexArrays(1, &GlyphVAO);
    glBindVertexArray(GlyphVAO);
    glGenBuffers(1, &GlyphVBO);
    glBindBuffer(GL_ARRAY_BUFFER, GlyphVBO);
    const float quad_vertex[] = {
        -1., -1., 0., 0., // UpperLeft
        -1., 1., 0., 1., // LowerLeft
        1., -1., 1., 0., // UpperRight
        1., 1., 1., 1., // LowerRight
    };
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid *)(2 * sizeof(float)));
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), quad_vertex, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glGenTextures(128, Texture);
    BilinearSampler = SamplerHelper::createBilinearSampler();

    for (int i = 0; i < 128; i++)
    {
        if (FT_Load_Char(Face, (char)i, FT_LOAD_RENDER))
            printf("Could not load character %c\n", i);

        glBindTexture(GL_TEXTURE_2D, Texture[i]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Face->glyph->bitmap.width, Face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, Face->glyph->bitmap.buffer);
    }

}

void clean()
{
    glDeleteTextures(128, Texture);
}

void draw()
{
    const char *string = "test";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 1024, 1024);
    glClearColor(0., 0., 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    glBindVertexArray(GlyphVAO);
    glUseProgram(GlyphRendering::getInstance()->Program);
    GlyphRendering::getInstance()->SetTextureUnits(Texture['X'], BilinearSampler);
    GlyphRendering::getInstance()->setUniforms(irr::core::vector2df(0., 0.), irr::core::vector2df(.5, .5));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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

