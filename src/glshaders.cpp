// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Scene/Shaders.h>

#include <GL/glew.h>
#include <GLAPI/Shaders.h>
#include <GLAPI/Misc.h>
#include <fstream>
#include <API/GfxApi.h>
#include <API/glapi.h>

class ObjectShader : public ShaderHelperSingleton<ObjectShader>, public TextureRead<UniformBufferResource<0>, UniformBufferResource<1>, TextureResource<GL_TEXTURE_2D, 0> >
{
public:
  ObjectShader()
  {
    std::ifstream vsin("../examples/shaders/object.vert", std::ios::in);
    const std::string &vs = std::string((std::istreambuf_iterator<char>(vsin)), std::istreambuf_iterator<char>());

    std::ifstream fsin("../examples/shaders/object_gbuffer.frag", std::ios::in);
    const std::string &fs = std::string((std::istreambuf_iterator<char>(fsin)), std::istreambuf_iterator<char>());

    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vs.c_str(),
      GL_FRAGMENT_SHADER, fs.c_str());

    AssignSamplerNames(Program, "ViewMatrices", "ObjectData", "tex");
  }
};

static void ObjectStateSetter()
{
  glEnable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
}

struct WrapperPipelineState *createObjectShader()
{
  WrapperPipelineState *result = (WrapperPipelineState*)malloc(sizeof(WrapperPipelineState));
  result->GLValue.Program = ObjectShader::getInstance()->Program;
  result->GLValue.StateSetter = ObjectStateSetter;
  return result;
}

class Sunlight : public ShaderHelperSingleton<Sunlight>, TextureRead<UniformBufferResource<0>, UniformBufferResource<1>, TextureResource<GL_TEXTURE_2D, 0>, TextureResource<GL_TEXTURE_2D, 1>, TextureResource<GL_TEXTURE_2D, 2>>
{
public:
  Sunlight()
  {
    std::ifstream vsin("../examples/shaders/screenquad.vert", std::ios::in);
    const std::string &vs = std::string((std::istreambuf_iterator<char>(vsin)), std::istreambuf_iterator<char>());

    std::ifstream fsin("../examples/shaders/sunlight.frag", std::ios::in);
    const std::string &fs = std::string((std::istreambuf_iterator<char>(fsin)), std::istreambuf_iterator<char>());
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vs.c_str(),
      GL_FRAGMENT_SHADER, fs.c_str());

    AssignSamplerNames(Program, "VIEWDATA", "LIGHTDATA", "ntex", "ctex", "dtex");
  }
};

static void sunlightStateSetter()
{
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glBlendEquation(GL_FUNC_ADD);
}

struct WrapperPipelineState *createSunlightShader()
{
  WrapperPipelineState *result = (WrapperPipelineState*)malloc(sizeof(WrapperPipelineState));
  result->GLValue.Program = Sunlight::getInstance()->Program;
  result->GLValue.StateSetter = sunlightStateSetter;
  return result;
}

class Tonemap : public ShaderHelperSingleton<Tonemap>, TextureRead<TextureResource<GL_TEXTURE_2D, 0>>
{
public:
  Tonemap()
  {
    std::ifstream vsin("../examples/shaders/screenquad.vert", std::ios::in);
    const std::string &vs = std::string((std::istreambuf_iterator<char>(vsin)), std::istreambuf_iterator<char>());

    std::ifstream fsin("../examples/shaders/tonemap.frag", std::ios::in);
    const std::string &fs = std::string((std::istreambuf_iterator<char>(fsin)), std::istreambuf_iterator<char>());
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vs.c_str(),
      GL_FRAGMENT_SHADER, fs.c_str());

    AssignSamplerNames(Program, "tex");
  }
};

static void tonemapStateSetter()
{
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glDisable(GL_BLEND);
}

struct WrapperPipelineState *createTonemapShader()
{
  WrapperPipelineState *result = (WrapperPipelineState*)malloc(sizeof(WrapperPipelineState));
  result->GLValue.Program = Tonemap::getInstance()->Program;
  result->GLValue.StateSetter = tonemapStateSetter;
  return result;
}


class Skybox : public ShaderHelperSingleton<Skybox>, TextureRead<UniformBufferResource<0>, TextureResource<GL_TEXTURE_2D, 0>>
{
public:
  Skybox()
  {
    std::ifstream vsin("../examples/shaders/skybox.vert", std::ios::in);
    const std::string &vs = std::string((std::istreambuf_iterator<char>(vsin)), std::istreambuf_iterator<char>());

    std::ifstream fsin("../examples/shaders/skybox.frag", std::ios::in);
    const std::string &fs = std::string((std::istreambuf_iterator<char>(fsin)), std::istreambuf_iterator<char>());
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vs.c_str(),
      GL_FRAGMENT_SHADER, fs.c_str());

    AssignSamplerNames(Program, "Matrixes", "skytexture");
  }
};

static void skyboxStateSetter()
{
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glDisable(GL_BLEND);
}

struct WrapperPipelineState *createSkyboxShader()
{
  WrapperPipelineState *result = (WrapperPipelineState*)malloc(sizeof(WrapperPipelineState));
  result->GLValue.Program = Skybox::getInstance()->Program;
  result->GLValue.StateSetter = skyboxStateSetter;
  return result;
}

class ImportanceSamplingForSpecularCubemap : public ShaderHelperSingleton< class ImportanceSamplingForSpecularCubemap>, TextureRead<UniformBufferResource<0>, TextureResource<GL_TEXTURE_2D, 0>, TextureResource<GL_TEXTURE_2D, 1>>
{
public:
  ImportanceSamplingForSpecularCubemap()
  {
    std::ifstream vsin("../examples/shaders/screenquad.vert", std::ios::in);
    const std::string &vs = std::string((std::istreambuf_iterator<char>(vsin)), std::istreambuf_iterator<char>());

    std::ifstream fsin("../examples/shaders/importance_sampling_specular.frag", std::ios::in);
    const std::string &fs = std::string((std::istreambuf_iterator<char>(fsin)), std::istreambuf_iterator<char>());
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vs.c_str(),
      GL_FRAGMENT_SHADER, fs.c_str());

    AssignSamplerNames(Program, "Matrix", "tex", "samples");
  }
};

static void ImportanceSamplingForSpecularCubemapStateSetter()
{
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
}


struct WrapperPipelineState *ImportanceSamplingForSpecularCubemap()
{
  WrapperPipelineState *result = (WrapperPipelineState*)malloc(sizeof(WrapperPipelineState));
  result->GLValue.Program = ImportanceSamplingForSpecularCubemap::getInstance()->Program;
  result->GLValue.StateSetter = ImportanceSamplingForSpecularCubemapStateSetter;
  return result;
}

class IBLShader : public ShaderHelperSingleton< class IBLShader>, TextureRead < UniformBufferResource<0>, UniformBufferResource<1>, TextureResource<GL_TEXTURE_2D, 0>, TextureResource<GL_TEXTURE_2D, 1>, TextureResource<GL_TEXTURE_2D, 2>, TextureResource<GL_TEXTURE_2D, 3>, TextureResource<GL_TEXTURE_2D, 4> >
{
public:
  IBLShader()
  {
    std::ifstream vsin("../examples/shaders/screenquad.vert", std::ios::in);
    const std::string &vs = std::string((std::istreambuf_iterator<char>(vsin)), std::istreambuf_iterator<char>());

    std::ifstream fsin("../examples/shaders/ibl.frag", std::ios::in);
    const std::string &fs = std::string((std::istreambuf_iterator<char>(fsin)), std::istreambuf_iterator<char>());
    Program = ProgramShaderLoading::LoadProgram(
      GL_VERTEX_SHADER, vs.c_str(),
      GL_FRAGMENT_SHADER, fs.c_str());

    AssignSamplerNames(Program, "VIEWDATA", "IBLDATA", "ntex", "ctex", "dtex", "probe", "dfg");
  }
};

static void IBLShaderStateSetter()
{
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
}


struct WrapperPipelineState *createIBLShader()
{
  WrapperPipelineState *result = (WrapperPipelineState*)malloc(sizeof(WrapperPipelineState));
  result->GLValue.Program = IBLShader::getInstance()->Program;
  result->GLValue.StateSetter = IBLShaderStateSetter;
  return result;
}