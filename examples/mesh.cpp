#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <GLAPI/VAO.h>
#include <GLAPI/S3DVertex.h>
#include <GLAPI/Texture.h>

#include <GLAPI/Shaders.h>
#include <GLAPI/FBO.h>
#include <GLAPI/Misc.h>
#include <GLAPI/Samplers.h>
#include <GLAPI/Text.h>
#include <GLAPI/Debug.h>

#include <Loaders/B3D.h>
#include <Loaders/DDS.h>


std::vector<std::tuple<size_t, size_t, size_t> > CountBaseIndexVTX;
FrameBuffer *MainFBO;
FrameBuffer *LinearDepthFBO;

GLuint TrilinearSampler;

const char *vtxshader = TO_STRING(
\#version 330 \n

layout(std140) uniform Matrices
{
  mat4 ModelMatrix;
  mat4 ViewProjectionMatrix;
};

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec2 Texcoord;
layout(location = 4) in vec2 SecondTexcoord;
layout(location = 5) in vec3 Tangent;
layout(location = 6) in vec3 Bitangent;

layout(location = 7) in int index0;
layout(location = 8) in float weight0;
layout(location = 9) in int index1;
layout(location = 10) in float weight1;
layout(location = 11) in int index2;
layout(location = 12) in float weight2;
layout(location = 13) in int index3;
layout(location = 14) in float weight3;

out vec3 nor;
out vec3 tangent;
out vec3 bitangent;
out vec2 uv;
out vec2 uv_bis;
out vec4 color;

layout(std140) uniform Joints
{
  uniform mat4 JointTransform[48];
};

void main()
{
  vec4 IdlePosition = vec4(Position, 1.);
  vec4 SkinnedPosition = vec4(0.);

  vec4 SingleBoneInfluencedPosition;
  if (index0 >= 0)
  {
    SingleBoneInfluencedPosition = JointTransform[index0] * IdlePosition;
    SingleBoneInfluencedPosition /= SingleBoneInfluencedPosition.w;
  }
  else
  {
    SingleBoneInfluencedPosition = IdlePosition;
  }
  SkinnedPosition += weight0 * SingleBoneInfluencedPosition;

  if (index1 >= 0)
  {
    SingleBoneInfluencedPosition = JointTransform[index1] * IdlePosition;
    SingleBoneInfluencedPosition /= SingleBoneInfluencedPosition.w;
  }
  else
  {
    SingleBoneInfluencedPosition = IdlePosition;
  }
  SkinnedPosition += weight1 * SingleBoneInfluencedPosition;

  if (index2 >= 0)
  {
    SingleBoneInfluencedPosition = JointTransform[index2] * IdlePosition;
    SingleBoneInfluencedPosition /= SingleBoneInfluencedPosition.w;
  }
  else
  {
    SingleBoneInfluencedPosition = IdlePosition;
  }
  SkinnedPosition += weight2 * SingleBoneInfluencedPosition;

  if (index3 >= 0)
  {
    SingleBoneInfluencedPosition = JointTransform[index3] * IdlePosition;
    SingleBoneInfluencedPosition /= SingleBoneInfluencedPosition.w;
  }
  else
  {
    SingleBoneInfluencedPosition = IdlePosition;
  }
  SkinnedPosition += weight3 * SingleBoneInfluencedPosition;

  color = Color.zyxw;
  mat4 ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
//  mat4 TransposeInverseModelView = transpose(InverseModelMatrix * InverseViewMatrix);
  gl_Position = ModelViewProjectionMatrix * vec4(SkinnedPosition.xyz, 1.);
//  nor = (TransposeInverseModelView * vec4(Normal, 0.)).xyz;
//  tangent = (TransposeInverseModelView * vec4(Tangent, 0.)).xyz;
//  bitangent = (TransposeInverseModelView * vec4(Bitangent, 0.)).xyz;
  uv = vec4(Texcoord, 1., 1.).xy;
//  uv_bis = SecondTexcoord;
}
);

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

class ObjectShader : public ShaderHelperSingleton<ObjectShader>, public TextureRead<UniformBufferResource<0>, UniformBufferResource<1>, TextureResource<GL_TEXTURE_2D, 0> >
{
public:
    ObjectShader()
    {
        Program = ProgramShaderLoading::LoadProgram(
            GL_VERTEX_SHADER, vtxshader,
            GL_FRAGMENT_SHADER, fragshader);

        AssignSamplerNames(Program, "Matrices", "Joints", "tex");
    }
};

struct Matrixes
{
    float Model[16];
    float ViewProj[16];
};

GLuint cbuf;
GLuint jointsbuf;

irr::scene::CB3DMeshFileLoader *loader;

std::unordered_map<std::string, Texture> textureSet;

void init()
{
  DebugUtil::enableDebugOutput();
  irr::io::CReadFile reader("..\\examples\\assets\\xue.b3d");
  loader = new irr::scene::CB3DMeshFileLoader(&reader);
  const std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > &buffers = loader->AnimatedMesh.getMeshBuffers();

  for (unsigned i = 0; i < buffers.size(); i++)
  {
    const irr::scene::SMeshBufferLightMap &tmpbuf = buffers[i].first;
    const std::vector<irr::scene::ISkinnedMesh::WeightInfluence> &weigts = loader->AnimatedMesh.WeightBuffers[i];
    std::pair<size_t, size_t> BaseVtxIndex = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords, irr::video::SkinnedVertexData> >::getInstance()->getBase(&tmpbuf, weigts.data());
    CountBaseIndexVTX.push_back(std::make_tuple(tmpbuf.getIndexCount(), BaseVtxIndex.second, BaseVtxIndex.first));
  }

  for (unsigned i = 0; i < loader->Textures.size(); i++)
  {
    const std::string &fixed = "..\\examples\\assets\\" + loader->Textures[i].TextureName.substr(0, loader->Textures[i].TextureName.find_last_of('.')) + ".DDS";
    std::ifstream DDSFile(fixed, std::ifstream::binary);
    irr::video::CImageLoaderDDS DDSPic(DDSFile);
    textureSet.emplace(loader->Textures[i].TextureName, DDSPic.getLoadedImage());
  }

  glGenBuffers(1, &cbuf);
  glGenBuffers(1, &jointsbuf);

  TrilinearSampler = SamplerHelper::createTrilinearSampler();

  glDepthFunc(GL_LEQUAL);
}

void clean()
{
    glDeleteSamplers(1, &TrilinearSampler);
    glDeleteBuffers(1, &cbuf);
    delete loader;
}

static float time = 0.;

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
    Model.setTranslation(irr::core::vector3df(0.f, 0.f, 2.f));
    Model.setRotationDegrees(irr::core::vector3df(0.f, time / 360.f, 0.f));
    memcpy(cbufdata.Model, Model.pointer(), 16 * sizeof(float));
    memcpy(cbufdata.ViewProj, View.pointer(), 16 * sizeof(float));

    time += 1.f;

    glBindBuffer(GL_UNIFORM_BUFFER, cbuf);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(struct Matrixes), &cbufdata, GL_STATIC_DRAW);

    double intpart;
    float frame = (float)modf(time / 10000., &intpart);
    frame *= 300.f;
    loader->AnimatedMesh.animateMesh(frame, 1.f);
    loader->AnimatedMesh.skinMesh(1.f);

    glBindBuffer(GL_UNIFORM_BUFFER, jointsbuf);
    glBufferData(GL_UNIFORM_BUFFER, loader->AnimatedMesh.JointMatrixes.size() * 16 * sizeof(float), loader->AnimatedMesh.JointMatrixes.data(), GL_STATIC_DRAW);

    glUseProgram(ObjectShader::getInstance()->Program);
    glBindVertexArray(VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords, irr::video::SkinnedVertexData> >::getInstance()->getVAO());
    for (unsigned i = 0; i < CountBaseIndexVTX.size(); i++)
    {
      auto tmp = CountBaseIndexVTX[i];
      const std::vector<std::pair<irr::scene::SMeshBufferLightMap, irr::video::SMaterial> > &buffers = loader->AnimatedMesh.getMeshBuffers();
      ObjectShader::getInstance()->SetTextureUnits(cbuf, jointsbuf, textureSet[buffers[i].second.TextureNames[0]].Id, TrilinearSampler);
      glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)std::get<0>(tmp), GL_UNSIGNED_SHORT, (void *)std::get<1>(tmp), (GLsizei)std::get<2>(tmp));
    }
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

