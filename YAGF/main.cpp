#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <Core/VAO.h>

#include <Core/Shaders.h>
#include <Core/FBO.h>
#include <Util/Misc.h>

irr::scene::IMeshBuffer *buffer;
FrameBuffer *MainFBO;
FrameBuffer *LinearDepthFBO;
FrameBuffer *SSAOFBO;

GLuint DepthStencilTexture;
GLuint MainTexture;
GLuint LinearTexture;
GLuint SSAOTexture;

GLuint NearestSampler;

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

const char *screenquadshader = "#version 330\n"
    "layout(location = 0) in vec2 Position;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(Position, 0., 1.);\n"
    "}\n";

const char *lineardepthshader = "#version 330\n"
    "uniform sampler2D tex;\n"
    "uniform float zn;\n"
    "uniform float zf;\n"
    "out float Depth;\n"
    "void main()\n"
    "{\n"
    "   vec2 uv = gl_FragCoord.xy / vec2(640, 480);\n"
    "   float d = texture(tex, uv).x;\n"
    "   float c0 = zn * zf, c1 = zn - zf, c2 = zf;\n"
    "   Depth = c0 / (d * c1 + c2);\n"
    "   if (Depth > 1.) discard;\n"
    "}\n";

const char *ssaoshader = "// From paper http://graphics.cs.williams.edu/papers/AlchemyHPG11/"
    "// and improvements here http://graphics.cs.williams.edu/papers/SAOHPG12/ \n"
    "#version 330\n"
    "uniform sampler2D dtex;"
    "uniform mat4 ProjectionMatrix;"
    "uniform float radius;\n"
    "uniform float k = 1.5;\n"
    "uniform float sigma = 1.;\n"
    "out float AO;\n"
    "const float tau = 7.;\n"
    "const float beta = 0.002;\n"
    "const float epsilon = .00001;\n"
    "#define SAMPLES 16\n"
    "const float invSamples = 1. / SAMPLES;\n"
    "vec2 screen = vec2(640, 480);\n"
    "vec3 getXcYcZc(int x, int y, float zC)\n"
    "{\n"
        "// We use perspective symetric projection matrix hence P(0,2) = P(1, 2) = 0 \n"
        "float xC= (2 * (float(x)) / screen.x - 1.) * zC / ProjectionMatrix[0][0];\n"
        "float yC= (2 * (float(y)) / screen.y - 1.) * zC / ProjectionMatrix[1][1];\n"
        "return vec3(xC, yC, zC);\n"
    "}\n"
    "void main(void)\n"
    "{\n"
        "vec2 uv = gl_FragCoord.xy / screen;\n"
        "float lineardepth = textureLod(dtex, uv, 0.).x;\n"
        "int x = int(gl_FragCoord.x), y = int(gl_FragCoord.y);\n"
        "vec3 FragPos = getXcYcZc(x, y, lineardepth);\n"
        "// get the normal of current fragment\n"
        "vec3 ddx = dFdx(FragPos);\n"
        "vec3 ddy = dFdy(FragPos);\n"
        "vec3 norm = normalize(cross(ddy, ddx));\n"
        "float r = radius / FragPos.z;\n"
        "float phi = 3. * (x ^ y) + x * y;\n"
        "float bl = 0.0;\n"
        "float m = log2(r) + 6 + log2(invSamples);\n"
        "float theta = 2. * 3.14 * tau * .5 * invSamples + phi;\n"
        "vec2 rotations = vec2(cos(theta), sin(theta)) * screen;\n"
        "vec2 offset = vec2(cos(invSamples), sin(invSamples));\n"
        "for(int i = 0; i < SAMPLES; ++i) {\n"
            "float alpha = (i + .5) * invSamples;\n"
            "rotations = vec2(rotations.x * offset.x - rotations.y * offset.y, rotations.x * offset.y + rotations.y * offset.x);\n"
            "float h = r * alpha;\n"
            "vec2 localoffset = h * rotations;\n"
            "m = m + .5;\n"
            "ivec2 ioccluder_uv = ivec2(x, y) + ivec2(localoffset);\n"
            "if (ioccluder_uv.x < 0 || ioccluder_uv.x > screen.x || ioccluder_uv.y < 0 || ioccluder_uv.y > screen.y) continue;\n"
            "float LinearoccluderFragmentDepth = textureLod(dtex, vec2(ioccluder_uv) / screen, max(0, 0.)).x;\n"
            "vec3 OccluderPos = getXcYcZc(ioccluder_uv.x, ioccluder_uv.y, LinearoccluderFragmentDepth);\n"
            "vec3 vi = OccluderPos - FragPos;\n"
            "bl += max(0, dot(vi, norm) - FragPos.z * beta) / (dot(vi, vi) + epsilon);\n"
        "}\n"
        "AO = max(pow(1.0 - min(2. * sigma * bl * invSamples, 0.99), k), 0.);\n"
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

class LinearizeDepthShader : public ShaderHelperSingleton<LinearizeDepthShader, float, float>, public TextureRead<Texture2D>
{
public:
    LinearizeDepthShader()
    {
      Program = ProgramShaderLoading::LoadProgram(
          GL_VERTEX_SHADER, screenquadshader,
          GL_FRAGMENT_SHADER, lineardepthshader);
      AssignUniforms("zn", "zf");

      AssignSamplerNames(Program, 0, "tex");
    }
};

class SSAOShader : public ShaderHelperSingleton<SSAOShader, irr::core::matrix4>, public TextureRead<Texture2D>
{
public:
    SSAOShader()
    {
      Program = ProgramShaderLoading::LoadProgram(
          GL_VERTEX_SHADER, screenquadshader,
          GL_FRAGMENT_SHADER, ssaoshader);

      AssignSamplerNames(Program, 0, "dtex");
      AssignUniforms("ProjectionMatrix");
    }
};

void init()
{
  buffer = GeometryCreator::createCubeMeshBuffer(
        irr::core::vector3df(1., 1., 1.));
  auto tmp = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> >::getInstance()->getBase(buffer);
  glViewport(0, 0, 640, 480);

  DepthStencilTexture = generateRTT(640, 480, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
  MainTexture = generateRTT(640, 480, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
  LinearTexture = generateRTT(640, 480, GL_R32F, GL_RED, GL_FLOAT);
  SSAOTexture = generateRTT(640, 480, GL_R16F, GL_RED, GL_FLOAT);

  MainFBO = new FrameBuffer({ MainTexture }, DepthStencilTexture, 640, 480);
  LinearDepthFBO = new FrameBuffer({ LinearTexture }, 640, 480);

  glGenSamplers(1, &NearestSampler);
  glSamplerParameteri(NearestSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glSamplerParameteri(NearestSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glSamplerParameteri(NearestSampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glSamplerParameteri(NearestSampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
//  glSamplerParameterf(NearestSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);

  glDepthFunc(GL_LEQUAL);
}

void clean()
{
  delete buffer;
}

void draw()
{
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  MainFBO->Bind();
  glClearColor(0., 0., 0., 1.);
  glClearDepth(1.);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  irr::core::matrix4 Model, View;
  View.buildProjectionMatrixPerspectiveFovLH(70. / 180. * 3.14, 640. / 480., 1., 100.);
  Model.setTranslation(irr::core::vector3df(0., 0., 8.));

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

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  SSAOShader::getInstance()->SetTextureUnits(LinearTexture, NearestSampler);
  DrawFullScreenEffect<SSAOShader>(View);
}

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window = glfwCreateWindow(640, 480, "GLtest", NULL, NULL);
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

