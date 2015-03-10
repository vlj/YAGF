
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

static const char *bilateralH =
"#version 430\n"
"// From http://http.developer.nvidia.com/GPUGems3/gpugems3_ch40.html \n"
"uniform sampler2D source;\n"
"uniform vec2 pixel; \n"
"uniform layout(rgba16f) volatile restrict writeonly image2D dest; \n"
"uniform float sigma = 5.; \n"
"layout(local_size_x = 8, local_size_y = 4) in; \n"
"shared vec4 local_src[8 + 2 * 8][4]; \n"
"void main()\n"
"  {\n"
"  int x = int(gl_LocalInvocationID.x), y = int(gl_LocalInvocationID.y); \n"
"  ivec2 iuv = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y); \n"
"  vec2 guv = gl_GlobalInvocationID.xy + .5; \n"
"  vec2 uv_m = (guv - vec2(8, 0)) * pixel; \n"
"  vec2 uv = guv * pixel; \n"
"  vec2 uv_p = (guv + vec2(8, 0)) * pixel; \n"
"  local_src[x][y] = texture(source, uv_m); \n"
"  local_src[x + 8][y] = texture(source, uv); \n"
"  local_src[x + 16][y] = texture(source, uv_p); \n"
"  barrier(); \n"
"  float g0, g1, g2; \n"
"  g0 = 1.0 / (sqrt(2.0 * 3.14) * sigma); \n"
"  g1 = exp(-0.5 / (sigma * sigma)); \n"
"  g2 = g1 * g1; \n"
"  vec4 sum = local_src[x + 8][y] * g0; \n"
"  g0 *= g1; \n"
"  g1 *= g2; \n"
"  float total_weight = g0; \n"
"  for (int j = 1; j < 8; j++) { \n"
"    total_weight += g0; \n"
"    sum += local_src[8 + x - j][y] * g0; \n"
"    total_weight += g0; \n"
"    sum += local_src[8 + x + j][y] * g0; \n"
"    g0 *= g1; \n"
"    g1 *= g2; \n"
"  }\n"
"  imageStore(dest, iuv, sum / total_weight); \n"
"}";

class GaussianBlurH : public ShaderHelperSingleton<GaussianBlurH, irr::core::vector2df, float>, public TextureRead<Texture2D, Image2D>
{
public:
    GaussianBlurH()
    {
        Program = ProgramShaderLoading::LoadProgram(
            GL_COMPUTE_SHADER, bilateralH);
        AssignUniforms("pixel", "sigma");

        AssignSamplerNames(Program, 0, "source", 1, "dest");
    }
};

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
    glViewport(0, 0, 1024, 1024);
    int *texture = new int[1024 * 1024];
    for (int i = 0; i < 1024 * 1024; i++)
    {
        if (i % 2)
            texture[i] = 0;
        else
            texture[i] = -1;
    }

    glGenTextures(1, &InitialTexture);
    glBindTexture(GL_TEXTURE_2D, InitialTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);
    delete texture;

    MainTexture = generateRTT(1024, 1024, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    AuxTexture = generateRTT(1024, 1024, GL_RGBA16F, GL_RGBA, GL_FLOAT);

    BilinearSampler = SamplerHelper::createBilinearSampler();
}

void clean()
{
    glDeleteSamplers(1, &BilinearSampler);
    glDeleteTextures(1, &MainTexture);
    glDeleteTextures(1, &AuxTexture);
}

void draw()
{
    glUseProgram(GaussianBlurH::getInstance()->Program);
    GaussianBlurH::getInstance()->setUniforms(irr::core::vector2df(1. / 1024., 1. / 1024.), 1.);

    GLuint timer;
    glGenQueries(1, &timer);
    glBeginQuery(GL_TIME_ELAPSED, timer);
    for (unsigned i = 0; i < 100; i++)
    {
        GaussianBlurH::getInstance()->SetTextureUnits(InitialTexture, BilinearSampler, MainTexture, GL_WRITE_ONLY, GL_RGBA16F);
        glDispatchCompute(1024 / 8, 1024 / 4, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    glEndQuery(GL_TIME_ELAPSED);
    GLuint result;
    glGetQueryObjectuiv(timer, GL_QUERY_RESULT, &result);

    char time[50];
    sprintf(time, "100 x SSAO: %f ms\0", result / 1000000.);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    FullScreenPassthrough::getInstance()->SetTextureUnits(MainTexture, BilinearSampler);
    DrawFullScreenEffect<FullScreenPassthrough>();

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    BasicTextRender<14>::getInstance()->drawText(time, 0, 20, 1024, 1024);
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