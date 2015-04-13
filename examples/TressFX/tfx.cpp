#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <GLAPI/Shaders.h>
#include <GLAPI/Debug.h>

#include <fstream>
#include "common.hpp"


float density = .4f;


GLuint InitialPosSSBO;
GLuint PrevPosSSBO;
GLuint StrandTypeSSBO;
GLuint LengthSSBO;
GLuint RotationSSBO;
GLuint LocalRefSSBO;
GLuint FollowRootSSBO;


GLuint ConstantSimBuffer;


class GlobalConstraintSimulation : public ShaderHelperSingleton<GlobalConstraintSimulation>, public TextureRead<UniformBufferResource<0> >
{
public:
  GlobalConstraintSimulation()
  {
    std::ifstream in("../examples/TressFX/shaders/GlobalConstraints.comp", std::ios::in);

    const std::string &shader = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_COMPUTE_SHADER, shader.c_str());
  }
};

class LocalConstraintSimulation : public ShaderHelperSingleton<LocalConstraintSimulation>, public TextureRead<UniformBufferResource<0> >
{
public:
  LocalConstraintSimulation()
  {
    std::ifstream in("../examples/TressFX/shaders/LocalConstraints.comp", std::ios::in);

    const std::string &shader = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_COMPUTE_SHADER, shader.c_str());
  }
};

class WindLengthTangentConstraint : public ShaderHelperSingleton<WindLengthTangentConstraint>, public TextureRead<UniformBufferResource<0> >
{
public:

  WindLengthTangentConstraint()
  {
    std::ifstream in("../examples/TressFX/shaders/LengthWindTangentComputation.comp", std::ios::in);

    const std::string &shader = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_COMPUTE_SHADER, shader.c_str());
  }
};

class PrepareFollowHairGuide : public ShaderHelperSingleton<PrepareFollowHairGuide>, public TextureRead<UniformBufferResource<0> >
{
public:

  PrepareFollowHairGuide()
  {
    std::ifstream in("../examples/TressFX/shaders/PrepareFollowHair.comp", std::ios::in);

    const std::string &shader = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_COMPUTE_SHADER, shader.c_str());
  }
};

class UpdateFollowHairGuide : public ShaderHelperSingleton<UpdateFollowHairGuide>, public TextureRead<UniformBufferResource<0> >
{
public:

  UpdateFollowHairGuide()
  {
    std::ifstream in("../examples/TressFX/shaders/UpdateFollowHair.comp", std::ios::in);

    const std::string &shader = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_COMPUTE_SHADER, shader.c_str());
  }
};




TressFXAsset tfxassets;

void init()
{
  DebugUtil::enableDebugOutput();

  tfxassets = loadTress("../examples/TressFX/ruby.tfxb");
  initCommon(tfxassets);

  glGenBuffers(1, &InitialPosSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, InitialPosSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pVertices, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, InitialPosSSBO);

  glGenBuffers(1, &PrevPosSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, PrevPosSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pVertices, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, PrevPosSSBO);

  glGenBuffers(1, &StrandTypeSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, StrandTypeSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairStrands * sizeof(int), tfxassets.m_pHairStrandType, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, StrandTypeSSBO);

  glGenBuffers(1, &LengthSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, LengthSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * sizeof(float), tfxassets.m_pRestLengths, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, LengthSSBO);

  glGenBuffers(1, &RotationSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, RotationSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pGlobalRotations, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, RotationSSBO);

  glGenBuffers(1, &LocalRefSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, LocalRefSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pRefVectors, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, LocalRefSSBO);

  glGenBuffers(1, &FollowRootSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, FollowRootSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairStrands * sizeof(float), tfxassets.m_pFollowRootOffset, GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, FollowRootSSBO);

  glGenBuffers(1, &ConstantSimBuffer);
}

void clean()
{
  cleanCommon();
  glDeleteBuffers(1, &ConstantSimBuffer);
  glDeleteBuffers(1, &StrandTypeSSBO);
  glDeleteBuffers(1, &PrevPosSSBO);
  glDeleteBuffers(1, &InitialPosSSBO);
  glDeleteBuffers(1, &LengthSSBO);
  glDeleteBuffers(1, &RotationSSBO);
  glDeleteBuffers(1, &LocalRefSSBO);
  glDeleteBuffers(1, &FollowRootSSBO);

  cleanTFXAssetContent(tfxassets);
}

void simulate(float time)
{
  struct SimulationConstants cbuf = {};

  irr::core::matrix4 Model;
  if (time < 30.)
    cbuf.bWarp = 1;
  else
    cbuf.bWarp = 0;


  double p;

  float dir = (float)modf(time / (360.f * 360.f), &p);
  int clockwise = ((int)p) % 2;
  time = 360.f * 360.f * dir * ((clockwise > 0) ? 1.f : -1.f);

  Model.setRotationDegrees(irr::core::vector3df(0.f, time / 360.f, 0.f));

  memcpy(cbuf.ModelTransformForHead, Model.pointer(), 16 * sizeof(float));
  cbuf.ModelRotateForHead[0] = 0.f;
  cbuf.ModelRotateForHead[1] = sinf((time / 720.f) * (3.14f / 180.f));
  cbuf.ModelRotateForHead[2] = 0.f;
  cbuf.ModelRotateForHead[3] = cosf((time / 720.f) * (3.14f / 180.f));

  cbuf.timeStep = .016f;

  cbuf.Damping0 = 0.125f;
  cbuf.StiffnessForGlobalShapeMatching0 = 0.8f;
  cbuf.GlobalShapeMatchingEffectiveRange0 = 0.3f;
  cbuf.StiffnessForLocalShapeMatching0 = 0.2f;
  cbuf.Damping1 = 0.125f;
  cbuf.StiffnessForGlobalShapeMatching1 = 0.25f;
  cbuf.GlobalShapeMatchingEffectiveRange1 = 0.4f;
  cbuf.StiffnessForLocalShapeMatching1 = 0.75f;
  cbuf.Damping2 = 0.0199f;
  cbuf.StiffnessForGlobalShapeMatching2 = 0.f;
  cbuf.GlobalShapeMatchingEffectiveRange2 = 0.f;
  cbuf.StiffnessForLocalShapeMatching2 = 0.699f;
  cbuf.Damping3 = 0.1f;
  cbuf.StiffnessForGlobalShapeMatching3 = 0.2f;
  cbuf.GlobalShapeMatchingEffectiveRange3 = 0.3f;
  cbuf.StiffnessForLocalShapeMatching3 = 1.f;

  cbuf.NumOfStrandsPerThreadGroup = 4;
  cbuf.NumFollowHairsPerOneGuideHair = tfxassets.m_NumFollowHairsPerOneGuideHair;

  cbuf.GravityMagnitude = 10.f;
  cbuf.NumLengthConstraintIterations = 2;
  cbuf.NumLocalShapeMatchingIterations = 1;

  memset(cbuf.Wind, 0, 4 * sizeof(float));
  memset(cbuf.Wind1, 0, 4 * sizeof(float));
  memset(cbuf.Wind2, 0, 4 * sizeof(float));
  memset(cbuf.Wind3, 0, 4 * sizeof(float));

  if (syncobj)
    glDeleteSync(syncobj);

  glBindBuffer(GL_UNIFORM_BUFFER, ConstantSimBuffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(struct SimulationConstants), &cbuf, GL_STATIC_DRAW);

  int numOfGroupsForCS_VertexLevel = (int)((density + .05f) * (tfxassets.m_NumGuideHairVertices / 64));

  // Prepare follow hair guide
  glUseProgram(PrepareFollowHairGuide::getInstance()->Program);
  PrepareFollowHairGuide::getInstance()->SetTextureUnits(ConstantSimBuffer);
  glDispatchCompute(numOfGroupsForCS_VertexLevel, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // Global Constraints
  glUseProgram(GlobalConstraintSimulation::getInstance()->Program);
  GlobalConstraintSimulation::getInstance()->SetTextureUnits(ConstantSimBuffer);
  glDispatchCompute(numOfGroupsForCS_VertexLevel, 1, 1);

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // Local Constraints
  glUseProgram(LocalConstraintSimulation::getInstance()->Program);
  LocalConstraintSimulation::getInstance()->SetTextureUnits(ConstantSimBuffer);
  glDispatchCompute(numOfGroupsForCS_VertexLevel, 1, 1);

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // Wind Lenght Tangent
  glUseProgram(WindLengthTangentConstraint::getInstance()->Program);
  WindLengthTangentConstraint::getInstance()->SetTextureUnits(ConstantSimBuffer);
  glDispatchCompute(numOfGroupsForCS_VertexLevel, 1, 1);

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glUseProgram(UpdateFollowHairGuide::getInstance()->Program);
  UpdateFollowHairGuide::getInstance()->SetTextureUnits(ConstantSimBuffer);
  glDispatchCompute(numOfGroupsForCS_VertexLevel, 1, 1);
}

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//      glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window = glfwCreateWindow(1024, 1024, "GLtest", NULL, NULL);
  glfwMakeContextCurrent(window);

  glewExperimental = GL_TRUE;
  glewInit();
  init();
  float tmp = 0.;
  while (!glfwWindowShouldClose(window))
  {
    simulate(tmp);
    draw(density);
    glfwSwapBuffers(window);
    glfwPollEvents();
    tmp += 300.;
  }
  clean();
  glfwTerminate();
  return 0;
}
