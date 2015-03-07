
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <Core/VAO.h>

#include <Core/Shaders.h>
#include <Core/FBO.h>
#include <Util/Misc.h>
#include <Util/Samplers.h>

GLuint InitialTexture;
GLuint MainTexture;
GLuint AuxTexture;

GLuint BilinearSampler;

static const char *bilateralH =
  "#version 430\n"
  "// From http://http.developer.nvidia.com/GPUGems3/gpugems3_ch40.html \n"
  "uniform sampler2D source;\n"
  "uniform sampler2D depth; \n"
  "uniform vec2 pixel; \n"
  "uniform layout(r16f) volatile restrict writeonly image2D dest; \n"
  "uniform float sigma = 5.; \n"
  "layout(local_size_x = 8, local_size_y = 8) in; \n"
  "shared float local_src[8 + 2 * 8][8]; \n"
  "shared float local_depth[8 + 2 * 8][8]; \n"
  "void main()\n"
"  {\n"
"  int x = int(gl_LocalInvocationID.x), y = int(gl_LocalInvocationID.y); \n"
"  ivec2 iuv = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y); \n"
"  vec2 guv = gl_GlobalInvocationID.xy + .5; \n"
"  vec2 uv_m = (guv - vec2(8, 0)) * pixel; \n"
"  vec2 uv = guv * pixel; \n"
"  vec2 uv_p = (guv + vec2(8, 0)) * pixel; \n"
"  local_src[x][y] = texture(source, uv_m).x; \n"
"  local_depth[x][y] = texture(depth, uv_m).x; \n"
"  local_src[x + 8][y] = texture(source, uv).x; \n"
"  local_depth[x + 8][y] = texture(depth, uv).x; \n"
"  local_src[x + 16][y] = texture(source, uv_p).x; \n"
"  local_depth[x + 16][y] = texture(depth, uv_p).x; \n"
"  barrier(); \n"
"  float g0, g1, g2; \n"
"  g0 = 1.0 / (sqrt(2.0 * 3.14) * sigma); \n"
"  g1 = exp(-0.5 / (sigma * sigma)); \n"
"  g2 = g1 * g1; \n"
"  float sum = local_src[x + 8][y] * g0; \n"
"  float pixel_depth = local_depth[x + 8][y]; \n"
"  g0 *= g1; \n"
"  g1 *= g2; \n"
"  float tmp_weight, total_weight = g0; \n"
"  for (int j = 1; j < 8; j++) { \n"
"    tmp_weight = max(0.0, 1.0 - .001 * abs(local_depth[8 + x - j][y] - pixel_depth)); \n"
"    total_weight += g0 * tmp_weight; \n"
"    sum += local_src[8 + x - j][y] * g0 * tmp_weight; \n"
"    tmp_weight = max(0.0, 1.0 - .001 * abs(local_depth[8 + x + j][y] - pixel_depth)); \n"
"    total_weight += g0 * tmp_weight; \n"
"    sum += local_src[8 + x + j][y] * g0 * tmp_weight; \n"
"    g0 *= g1; \n"
"    g1 *= g2; \n"
"  }\n"
"  imageStore(dest, iuv, vec4(sum / total_weight)); \n"
"}";

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);
    delete texture;

    MainTexture = generateRTT(1024, 1024, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    AuxTexture = generateRTT(1024, 1024, GL_RGBA32F, GL_RGBA, GL_FLOAT);

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
    /*
    glUseProgram(ObjectShader::getInstance()->Program);
    glBindVertexArray(VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> >::getInstance()->getVAO());
    ObjectShader::getInstance()->setUniforms(Model, View);
    glDrawElementsBaseVertex(GL_TRIANGLES, buffer->getIndexCount(), GL_UNSIGNED_SHORT, 0, 0);

    Model.setTranslation(irr::core::vector3df(0., 0., 10.));
    Model.setScale(2.);
    ObjectShader::getInstance()->setUniforms(Model, View);
    glDrawElementsBaseVertex(GL_TRIANGLES, buffer->getIndexCount(), GL_UNSIGNED_SHORT, 0, 0);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    LinearDepthFBO->Bind();
    LinearizeDepthShader::getInstance()->SetTextureUnits(DepthStencilTexture, NearestSampler);
    DrawFullScreenEffect<LinearizeDepthShader>(1., 100.);

    // Generate mipmap
    glBindTexture(GL_TEXTURE_2D, LinearTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint timer;
    glGenQueries(1, &timer);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    {
        glBeginQuery(GL_TIME_ELAPSED, timer);
        for (unsigned i = 0; i < 100; i++)
        {
            SSAOShader::getInstance()->SetTextureUnits(LinearTexture, TrilinearSampler);
            DrawFullScreenEffect<SSAOShader>(View);
        }
        glEndQuery(GL_TIME_ELAPSED);
    }
    GLuint result;
    glGetQueryObjectuiv(timer, GL_QUERY_RESULT, &result);
    printf("time elapsed : %d ms\n", result / 1000);
    */
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
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

