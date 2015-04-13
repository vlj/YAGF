#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <GLAPI/VAO.h>

#include <GLAPI/Shaders.h>
#include <GLAPI/FBO.h>
#include <GLAPI/Misc.h>
#include <GLAPI/Samplers.h>
#include <GLAPI/Debug.h>
#include <GLAPI/Text.h>

#include <sstream>

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
  "layout(std140) uniform GAUSSIANBuffer {\n"
  "  vec2 pixel; \n"
  "  vec2 pad; \n"
  "  float sigma; \n"
  "};\n"
  "uniform layout(rgba16f) volatile restrict writeonly image2D dest; \n"
  "layout(local_size_x = 8, local_size_y = 8) in; \n"
  "shared vec4 local_src[8 + 2 * 8][8]; \n"
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

class GaussianBlurH : public ShaderHelperSingleton<GaussianBlurH>, public TextureRead<UniformBufferResource<0>, TextureResource<GL_TEXTURE_2D, 0>, ImageResource<1> >
{
public:
    GaussianBlurH()
    {
        Program = ProgramShaderLoading::LoadProgram(
            GL_COMPUTE_SHADER, bilateralH);

        AssignSamplerNames(Program, "GAUSSIANBuffer", "source", "dest");
    }
};

class FullScreenPassthrough : public ShaderHelperSingleton<FullScreenPassthrough>, public TextureRead<TextureResource<GL_TEXTURE_2D, 0> >
{
public:
    FullScreenPassthrough()
    {
        Program = ProgramShaderLoading::LoadProgram(
            GL_VERTEX_SHADER, screenquadshader,
            GL_FRAGMENT_SHADER, passthrougfs);
        AssignSamplerNames(Program, "tex");
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

struct GAUSSIANBuffer
{
    float pixel[4];
    float sigma;
};

GLuint cbuf;

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

    glGenBuffers(1, &cbuf);
}

void clean()
{
    glDeleteSamplers(1, &BilinearSampler);
    glDeleteTextures(1, &MainTexture);
    glDeleteTextures(1, &AuxTexture);
    glDeleteBuffers(1, &cbuf);
}

void draw()
{
    glUseProgram(GaussianBlurH::getInstance()->Program);
    struct GAUSSIANBuffer cbufferdata;
    cbufferdata.pixel[0] = 1.f / 1024.f;
    cbufferdata.pixel[1] = 1.f / 1024.f;
    cbufferdata.sigma = 1.f;

    glBindBuffer(GL_UNIFORM_BUFFER, cbuf);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GAUSSIANBuffer), &cbufferdata, GL_STATIC_DRAW);

    GLuint timer;
    glGenQueries(1, &timer);
    glBeginQuery(GL_TIME_ELAPSED, timer);
    for (unsigned i = 0; i < 100; i++)
    {
        GaussianBlurH::getInstance()->SetTextureUnits(cbuf, InitialTexture, BilinearSampler, MainTexture, GL_WRITE_ONLY, GL_RGBA16F);
        glDispatchCompute(1024 / 8, 1024 / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    glEndQuery(GL_TIME_ELAPSED);
    GLuint result;
    glGetQueryObjectuiv(timer, GL_QUERY_RESULT, &result);
    glDeleteQueries(1, &timer);

    std::stringstream strbuf;
    strbuf << "100 x SSAO: " << result / 1000000.f << "ms";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    FullScreenPassthrough::getInstance()->SetTextureUnits(MainTexture, BilinearSampler);
    DrawFullScreenEffect<FullScreenPassthrough>();

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    BasicTextRender<14>::getInstance()->drawText(strbuf.str().c_str(), 0, 20, 1024, 1024);
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

