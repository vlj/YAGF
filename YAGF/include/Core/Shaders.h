// Copyright (C) 2015 Vincent Lejeune

#ifndef SHADERS_UTIL_HPP
#define SHADERS_UTIL_HPP

#include <Core/Singleton.h>
#include <vector>
#include <Maths/matrix4.h>
#include <Core/SColor.h>
#include <Maths/vector3d.h>
//#include "gl_headers.hpp"
#include <functional>
#include <fstream>


/**
\page shaders_overview Shaders Overview

 \section shader_declaration Shader declaration
 You need to create a class for each shader in shaders.cpp
 This class should inherit from the template ShaderHelper<>.
 The template first parameter is the shader class being declared and the following ones are the types
 of every uniform (except samplers) required by the shaders.

 The template inheritance will provide the shader with a setUniforms() variadic function which calls
 the glUniform*() that pass uniforms value to the shader according to the type given as parameter
 to the template.

 The class constructor is used to
 \li \ref shader_declaration_compile
 \li \ref shader_declaration_uniform_names
 \li \ref shader_declaration_bind_texture_unit

 Of course it's possible to use the constructor to declare others things if needed.

 \subsection shader_declaration_compile Compile the shader

 The LoadProgram() function is provided to ease shader compilation and link.
 It takes a flat sequence of SHADER_TYPE, filename pairs that will be linked together.
 This way you can add any shader stage you want (geometry, domain/hull shader)

 It is highly recommended to use explicit attribute location for a program input.
 However as not all hardware support this extension, default location are provided for
 input whose name is either Position (location 0) or Normal (location 1) or
 Texcoord (location 3) or Color (location 2) or SecondTexcoord (location 4).
 You can use these predefined name and location in your vao for shader
 that needs GL pre 3.3 support.

 \subsection shader_declaration_uniform_names Declare uniforms

 Use the AssignUniforms() function to pass name of the uniforms in the program.
 The order of name declaration is the same as the argument passed to setUniforms function.

 \subsection shader_declaration_bind_texture_unit Bind texture unit and name

 Texture are optional but if you have one, you must give them determined texture unit (up to 32).
 You can do this using the AssignTextureUnit function that takes pair of texture unit and sampler name
 as argument.

 \section shader_usage

 Shader's class are singleton that can be retrieved using ShaderClassName::getInstance() which automatically
 creates an instance the first time it is called.

 As the program id of a shader instance is public it can be used to bind a program :
 \code
 glUseProgram(MyShaderClass::getInstance()->Program);
 \endcode

 To set uniforms use the automatically generated function setUniforms:

 \code
 MyShaderClass::getInstance()->setUniforms(Args...)
 \endcode

 A Vertex Array must be bound (VAO creation process is currently left to the reader) :

 \code
 glBindVertexAttrib(vao);
 \endcode

 To actually perform the rendering you also need to call a glDraw* function (left to the reader as well) :

 \code
 glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT);
 \endcode

*/

class ProgramShaderLoading
{
  static GLuint LoadShader(const char * Code, GLenum type)
  {
      GLuint Id = glCreateShader(type);
      GLint Result = GL_FALSE;
      int InfoLogLength;

      int length = (int)strlen(Code);
      glShaderSource(Id, 1, &Code, &length);
      glCompileShader(Id);
  
      glGetShaderiv(Id, GL_COMPILE_STATUS, &Result);
      if (Result == GL_FALSE)
      {
          glGetShaderiv(Id, GL_INFO_LOG_LENGTH, &InfoLogLength);
          if (InfoLogLength<0)
              InfoLogLength = 1024;
          char *ErrorMessage = new char[InfoLogLength];
          ErrorMessage[0] = 0;
          glGetShaderInfoLog(Id, InfoLogLength, NULL, ErrorMessage);
          printf("%s\n", ErrorMessage);
          delete[] ErrorMessage;
      }
      glGetError();
      return Id;
  }

  template<typename ... Types>
  static void loadAndAttach(GLint ProgramID)
  {
      return;
  }

  template<typename ... Types>
  static void loadAndAttach(GLint ProgramID, GLint ShaderType, const char *filepath, Types ... args)
  {
      GLint ShaderID = LoadShader(filepath, ShaderType);
      glAttachShader(ProgramID, ShaderID);
      glDeleteShader(ShaderID);
      loadAndAttach(ProgramID, args...);
  }

  static std::string LoadHeader()
  {
      std::string result;
//      std::ifstream Stream(file_manager->getAsset("shaders/header.txt").c_str(), std::ios::in);
  
/*      if (Stream.is_open())
      {
          std::string Line = "";
          while (getline(Stream, Line))
              result += "\n" + Line;
          Stream.close();
      }
  */
      return result;
  }

  template<typename ...Types>
  static void printFileList()
  {
      return;
  }

  template<typename ...Types>
  static void printFileList(GLint ShaderType, const char *filepath, Types ... args)
  {
      printf("%s\n", filepath);
      printFileList(args...);
  }


/*  GLuint LoadTFBProgram(const char * vertex_file_path, const char **varyings, unsigned varyingscount)
  {
      GLuint Program = glCreateProgram();
      loadAndAttach(Program, GL_VERTEX_SHADER, vertex_file_path);
      if (CVS->getGLSLVersion() < 330)
      {
          glBindAttribLocation(Program, 0, "particle_position");
          glBindAttribLocation(Program, 1, "lifetime");
          glBindAttribLocation(Program, 2, "particle_velocity");
          glBindAttribLocation(Program, 3, "size");
          glBindAttribLocation(Program, 4, "particle_position_initial");
          glBindAttribLocation(Program, 5, "lifetime_initial");
          glBindAttribLocation(Program, 6, "particle_velocity_initial");
          glBindAttribLocation(Program, 7, "size_initial");
      }
      glTransformFeedbackVaryings(Program, varyingscount, varyings, GL_INTERLEAVED_ATTRIBS);
      glLinkProgram(Program);
  
      GLint Result = GL_FALSE;
      int InfoLogLength;
      glGetProgramiv(Program, GL_LINK_STATUS, &Result);
      if (Result == GL_FALSE)
      {
          glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &InfoLogLength);
          char *ErrorMessage = new char[InfoLogLength];
          glGetProgramInfoLog(Program, InfoLogLength, NULL, ErrorMessage);
          Log::error("GLWrap", ErrorMessage);
          delete[] ErrorMessage;
      }
      glGetError();
      return Program;
  }*/
public:
  GLuint LoadTFBProgram(const char * vertex_file_path, const char **varyings, unsigned varyingscount);
  const char *getShaderCode(const char *file)
  {
    printf("Loading shader : %s\n", file);
    char versionString[20];
    sprintf(versionString, "#version %d\n", 330);
    std::string Code = versionString;
/*      if (CVS->isAMDVertexShaderLayerUsable())
        Code += "#extension GL_AMD_vertex_shader_layer : enable\n";
    if (CVS->isAZDOEnabled())
    {
        Code += "#extension GL_ARB_bindless_texture : enable\n";
        Code += "#define Use_Bindless_Texture\n";
    }*/
    std::ifstream Stream(file, std::ios::in);
    Code += "//" + std::string(file) + "\n";
/*    if (!CVS->isARBUniformBufferObjectUsable())
        Code += "#define UBO_DISABLED\n";
    if (CVS->isAMDVertexShaderLayerUsable())
        Code += "#define VSLayer\n";
    if (CVS->needsRGBBindlessWorkaround())
        Code += "#define SRGBBindlessFix\n";*/
    Code += LoadHeader();
    if (Stream.is_open())
    {
        std::string Line = "";
        while (getline(Stream, Line))
            Code += "\n" + Line;
        Stream.close();
    }
    return Code.c_str();
  }

  template<typename ... Types>
  static GLint LoadProgram(Types ... args)
  {
      GLint ProgramID = glCreateProgram();
      loadAndAttach(ProgramID, args...);
      glLinkProgram(ProgramID);
  
      GLint Result = GL_FALSE;
      int InfoLogLength;
      glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
      if (Result == GL_FALSE) {
          printf("Error when linking these shaders :\n");
          printFileList(args...);
          glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
          char *ErrorMessage = new char[InfoLogLength];
          glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, ErrorMessage);
          printf("%s\n", ErrorMessage);
          delete[] ErrorMessage;
      }
  
      GLenum glErr = glGetError();
      if (glErr != GL_NO_ERROR)
      {
  //        Log::warn("IrrDriver", "GLWrap : OpenGL error %i\n", glErr);
      }
  
      return ProgramID;
  }
};


struct UniformHelper
{
    template<unsigned N = 0>
    static void setUniformsHelper(const std::vector<GLuint> &uniforms)
    {
    }

    template<unsigned N = 0, typename... Args>
    static void setUniformsHelper(const std::vector<GLuint> &uniforms, const irr::core::matrix4 &mat, const Args &... arg)
    {
        glUniformMatrix4fv(uniforms[N], 1, GL_FALSE, mat.pointer());
        setUniformsHelper<N + 1>(uniforms, arg...);
    }

    template<unsigned N = 0, typename... Args>
    static void setUniformsHelper(const std::vector<GLuint> &uniforms, const irr::video::SColorf &col, const Args &... arg)
    {
        glUniform3f(uniforms[N], col.r, col.g, col.b);
        setUniformsHelper<N + 1>(uniforms, arg...);
    }

    template<unsigned N = 0, typename... Args>
    static void setUniformsHelper(const std::vector<GLuint> &uniforms, const irr::video::SColor &col, const Args &... arg)
    {
        glUniform4i(uniforms[N], col.getRed(), col.getGreen(), col.getBlue(), col.getAlpha());
        setUniformsHelper<N + 1>(uniforms, arg...);
    }

    template<unsigned N = 0, typename... Args>
    static void setUniformsHelper(const std::vector<GLuint> &uniforms, const irr::core::vector3df &v, const Args &... arg)
    {
        glUniform3f(uniforms[N], v.X, v.Y, v.Z);
        setUniformsHelper<N + 1>(uniforms, arg...);
    }


    template<unsigned N = 0, typename... Args>
    static void setUniformsHelper(const std::vector<GLuint> &uniforms, const irr::core::vector2df &v, const Args &... arg)
    {
        glUniform2f(uniforms[N], v.X, v.Y);
        setUniformsHelper<N + 1>(uniforms, arg...);
    }

/*    template<unsigned N = 0, typename... Args>
    static void setUniformsHelper(const std::vector<GLuint> &uniforms, const irr::core::dimension2df &v, const Args &... arg)
    {
        glUniform2f(uniforms[N], v.Width, v.Height);
        setUniformsHelper<N + 1>(uniforms, arg...);
    }*/

    template<unsigned N = 0, typename... Args>
    static void setUniformsHelper(const std::vector<GLuint> &uniforms, float f, const Args &... arg)
    {
        glUniform1f(uniforms[N], f);
        setUniformsHelper<N + 1>(uniforms, arg...);
    }

    template<unsigned N = 0, typename... Args>
    static void setUniformsHelper(const std::vector<GLuint> &uniforms, int f, const Args &... arg)
    {
        glUniform1i(uniforms[N], f);
        setUniformsHelper<N + 1>(uniforms, arg...);
    }

    template<unsigned N = 0, typename... Args>
    static void setUniformsHelper(const std::vector<GLuint> &uniforms, const std::vector<float> &v, const Args &... arg)
    {
        glUniform1fv(uniforms[N], (int)v.size(), v.data());
        setUniformsHelper<N + 1>(uniforms, arg...);
    }

    template<unsigned N = 0, typename... Args>
    static void setUniformsHelper(const std::vector<GLuint> &uniforms, const std::vector<irr::core::matrix4> &v, Args... arg)
    {
        std::vector<float> tmp;
        for (unsigned mat = 0; mat < v.size(); mat++)
        {
            for (unsigned i = 0; i < 4; i++)
                for (unsigned j = 0; j < 4; j++)
                    tmp.push_back(v[mat].pointer()[4 * i + j]);
        }
        glUniformMatrix4fv(uniforms[N], (int)v.size(), GL_FALSE, tmp.data());
        setUniformsHelper<N + 1>(uniforms, arg...);
    }

};

void bypassUBO(GLuint Program);

extern std::vector<void(*)()> CleanTable;

template<typename T, typename... Args>
class ShaderHelperSingleton : public Singleton<T>
{
protected:
    std::vector<GLuint> uniforms;

    void AssignUniforms_impl()
    {
        GLuint uniform_ViewProjectionMatrixesUBO = glGetUniformBlockIndex(Program, "MatrixesData");
        if (uniform_ViewProjectionMatrixesUBO != GL_INVALID_INDEX)
            glUniformBlockBinding(Program, uniform_ViewProjectionMatrixesUBO, 0);
        GLuint uniform_LightingUBO = glGetUniformBlockIndex(Program, "LightingData");
        if (uniform_LightingUBO != GL_INVALID_INDEX)
            glUniformBlockBinding(Program, uniform_LightingUBO, 1);
    }

    template<typename... U>
    void AssignUniforms_impl(const char* name, U... rest)
    {
        uniforms.push_back(glGetUniformLocation(Program, name));
        AssignUniforms_impl(rest...);
    }

    template<typename... U>
    void AssignUniforms(U... rest)
    {
        static_assert(sizeof...(rest) == sizeof...(Args), "Count of Uniform's name mismatch");
        AssignUniforms_impl(rest...);
    }

public:
    GLuint Program;

    ShaderHelperSingleton()
    {
//        CleanTable.push_back(this->kill);
    }

    ~ShaderHelperSingleton()
    {
        glDeleteProgram(Program);
    }

    void setUniforms(const Args & ... args) const
    {
        UniformHelper::setUniformsHelper(uniforms, args...);
    }
};

enum SamplerType {
    Trilinear_Anisotropic_Filtered,
    Semi_trilinear,
    Bilinear_Filtered,
    Bilinear_Clamped_Filtered,
    Neared_Clamped_Filtered,
    Nearest_Filtered,
    Shadow_Sampler,
    Volume_Linear_Filtered,
    Trilinear_cubemap,
    Trilinear_Clamped_Array2D,
};

void setTextureSampler(GLenum, GLuint, GLuint, GLuint);

template<SamplerType...tp>
struct CreateSamplers;

template<SamplerType...tp>
struct BindTexture;

template<>
struct CreateSamplers<>
{
    static void exec(std::vector<unsigned> &, std::vector<GLenum> &e)
    {}
};

template<>
struct BindTexture<>
{
    template<int N>
    static void exec(const std::vector<unsigned> &TU)
    {}
};

template<SamplerType...tp>
struct CreateSamplers<Nearest_Filtered, tp...>
{
    static void exec(std::vector<unsigned> &v, std::vector<GLenum> &e)
    {
        unsigned id;
#ifdef GL_VERSION_3_3
        glGenSamplers(1, &id);
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
#endif
        v.push_back(id);
        e.push_back(GL_TEXTURE_2D);
        CreateSamplers<tp...>::exec(v, e);
    }
};

template<SamplerType...tp>
struct BindTexture<Nearest_Filtered, tp...>
{
    template<int N, typename...Args>
    static void exec(const std::vector<unsigned> &TU, GLuint TexId, Args... args)
    {
        glActiveTexture(GL_TEXTURE0 + TU[N]);
        glBindTexture(GL_TEXTURE_2D, TexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
        BindTexture<tp...>::template exec<N + 1>(TU, args...);
    }
};

template<SamplerType...tp>
struct CreateSamplers<Neared_Clamped_Filtered, tp...>
{
    static void exec(std::vector<unsigned> &v, std::vector<GLenum> &e)
    {
        unsigned id;
#ifdef GL_VERSION_3_3
        glGenSamplers(1, &id);
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
#endif
        v.push_back(id);
        e.push_back(GL_TEXTURE_2D);
        CreateSamplers<tp...>::exec(v, e);
    }
};

template<SamplerType...tp>
struct BindTexture<Neared_Clamped_Filtered, tp...>
{
    template<int N, typename...Args>
    static void exec(const std::vector<unsigned> &TU, GLuint TexId, Args... args)
    {
        glActiveTexture(GL_TEXTURE0 + TU[N]);
        glBindTexture(GL_TEXTURE_2D, TexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
        BindTexture<tp...>::template exec<N + 1>(TU, args...);
    }
};

template<SamplerType...tp>
struct CreateSamplers<Bilinear_Filtered, tp...>
{
    static void exec(std::vector<unsigned> &v, std::vector<GLenum> &e)
    {
        unsigned id;
#ifdef GL_VERSION_3_3
        glGenSamplers(1, &id);
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
#endif
        v.push_back(id);
        e.push_back(GL_TEXTURE_2D);
        CreateSamplers<tp...>::exec(v, e);
    }
};

template<SamplerType...tp>
struct BindTexture<Bilinear_Filtered, tp...>
{
    template<int N, typename...Args>
    static void exec(const std::vector<unsigned> &TU, GLuint TexId, Args... args)
    {
        glActiveTexture(GL_TEXTURE0 + TU[N]);
        glBindTexture(GL_TEXTURE_2D, TexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
        BindTexture<tp...>::template exec<N + 1>(TU, args...);
    }
};

template<SamplerType...tp>
struct CreateSamplers<Bilinear_Clamped_Filtered, tp...>
{
    static void exec(std::vector<unsigned> &v, std::vector<GLenum> &e)
    {
        unsigned id;
#ifdef GL_VERSION_3_3
        glGenSamplers(1, &id);
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
#endif
        v.push_back(id);
        e.push_back(GL_TEXTURE_2D);
        CreateSamplers<tp...>::exec(v, e);
    }
};

template<SamplerType...tp>
struct BindTexture<Bilinear_Clamped_Filtered, tp...>
{
    template<int N, typename...Args>
    static void exec(const std::vector<unsigned> &TU, GLuint TexId, Args... args)
    {
        glActiveTexture(GL_TEXTURE0 + TU[N]);
        glBindTexture(GL_TEXTURE_2D, TexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
        BindTexture<tp...>::template exec<N + 1>(TU, args...);
    }
};

template<SamplerType...tp>
struct CreateSamplers<Semi_trilinear, tp...>
{
    static void exec(std::vector<unsigned> &v, std::vector<GLenum> &e)
    {
        unsigned id;
#ifdef GL_VERSION_3_3
        glGenSamplers(1, &id);
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
#endif
        v.push_back(id);
        e.push_back(GL_TEXTURE_2D);
        CreateSamplers<tp...>::exec(v, e);
    }
};

template<SamplerType...tp>
struct BindTexture<Semi_trilinear, tp...>
{
    template<int N, typename...Args>
    static void exec(const std::vector<unsigned> &TU, GLuint TexId, Args... args)
    {
        glActiveTexture(GL_TEXTURE0 + TU[N]);
        glBindTexture(GL_TEXTURE_2D, TexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
        BindTexture<tp...>::template exec<N + 1>(TU, args...);
    }
};

template<SamplerType...tp>
struct CreateSamplers<Trilinear_Anisotropic_Filtered, tp...>
{
    static void exec(std::vector<unsigned> &v, std::vector<GLenum> &e)
    {
        unsigned id;
#ifdef GL_VERSION_3_3
        glGenSamplers(1, &id);
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);

        /*int aniso = UserConfigParams::m_anisotropic;
        if (aniso == 0) aniso = 1;
        glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)aniso);*/
#endif
        v.push_back(id);
        e.push_back(GL_TEXTURE_2D);
        CreateSamplers<tp...>::exec(v, e);
    }
};

template<SamplerType...tp>
struct BindTexture<Trilinear_Anisotropic_Filtered, tp...>
{
    template<int N, typename...Args>
    static void exec(const std::vector<unsigned> &TU, GLuint TexId, Args... args)
    {
        glActiveTexture(GL_TEXTURE0 + TU[N]);
        glBindTexture(GL_TEXTURE_2D, TexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        /*int aniso = UserConfigParams::m_anisotropic;
        if (aniso == 0) aniso = 1;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)aniso);
        BindTexture<tp...>::template exec<N + 1>(TU, args...);*/
    }
};

template<SamplerType...tp>
struct CreateSamplers<Trilinear_cubemap, tp...>
{
    static void exec(std::vector<unsigned> &v, std::vector<GLenum> &e)
    {
        unsigned id;
#ifdef GL_VERSION_3_3
        glGenSamplers(1, &id);
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);

        /*int aniso = UserConfigParams::m_anisotropic;
        if (aniso == 0) aniso = 1;
        glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)aniso);*/
#endif
        v.push_back(id);
        e.push_back(GL_TEXTURE_CUBE_MAP);
        CreateSamplers<tp...>::exec(v, e);
    }
};

template<SamplerType...tp>
struct BindTexture<Trilinear_cubemap, tp...>
{
    template<int N, typename...Args>
    static void exec(const std::vector<unsigned> &TU, GLuint TexId, Args... args)
    {
        glActiveTexture(GL_TEXTURE0 + TU[N]);
        glBindTexture(GL_TEXTURE_CUBE_MAP, TexId);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);

        /*int aniso = UserConfigParams::m_anisotropic;
        if (aniso == 0) aniso = 1;
        glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)aniso);
        BindTexture<tp...>::template exec<N + 1>(TU, args...);*/
    }
};

template<SamplerType...tp>
struct CreateSamplers<Volume_Linear_Filtered, tp...>
{
    static void exec(std::vector<unsigned> &v, std::vector<GLenum> &e)
    {
        unsigned id;
#ifdef GL_VERSION_3_3
        glGenSamplers(1, &id);
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
#endif
        v.push_back(id);
        e.push_back(GL_TEXTURE_3D);
        CreateSamplers<tp...>::exec(v, e);
    }
};

template<SamplerType...tp>
struct BindTexture<Volume_Linear_Filtered, tp...>
{
    template<int N, typename...Args>
    static void exec(const std::vector<unsigned> &TU, GLuint TexId, Args... args)
    {
        glActiveTexture(GL_TEXTURE0 + TU[N]);
        glBindTexture(GL_TEXTURE_3D, TexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
        BindTexture<tp...>::template exec<N + 1>(TU, args...);
    }
};

template<SamplerType...tp>
struct CreateSamplers<Shadow_Sampler, tp...>
{
    static void exec(std::vector<unsigned> &v, std::vector<GLenum> &e)
    {
        unsigned id;
#ifdef GL_VERSION_3_3
        glGenSamplers(1, &id);
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameterf(id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameterf(id, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
#endif
        v.push_back(id);
        e.push_back(GL_TEXTURE_2D_ARRAY);
        CreateSamplers<tp...>::exec(v, e);
    }
};

template<SamplerType...tp>
struct BindTexture<Shadow_Sampler, tp...>
{
    template <int N, typename...Args>
    static void exec(const std::vector<unsigned> &TU, GLuint TexId, Args... args)
    {
        glActiveTexture(GL_TEXTURE0 + TU[N]);
        glBindTexture(GL_TEXTURE_2D_ARRAY, TexId);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        BindTexture<tp...>::template exec<N + 1>(TU, args...);
    }
};

template<SamplerType...tp>
struct CreateSamplers<Trilinear_Clamped_Array2D, tp...>
{
    static void exec(std::vector<unsigned> &v, std::vector<GLenum> &e)
    {
        unsigned id;
#ifdef GL_VERSION_3_3
        glGenSamplers(1, &id);
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        /*int aniso = UserConfigParams::m_anisotropic;
        if (aniso == 0) aniso = 1;
        glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)aniso);*/
#endif
        v.push_back(id);
        e.push_back(GL_TEXTURE_2D_ARRAY);
        CreateSamplers<tp...>::exec(v, e);
    }
};

template<SamplerType...tp>
struct BindTexture<Trilinear_Clamped_Array2D, tp...>
{
    template <int N, typename...Args>
    static void exec(const std::vector<unsigned> &TU, GLuint TexId, Args... args)
    {
        glActiveTexture(GL_TEXTURE0 + TU[N]);
        glBindTexture(GL_TEXTURE_2D_ARRAY, TexId);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        /*int aniso = UserConfigParams::m_anisotropic;
        if (aniso == 0) aniso = 1;
        glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)aniso);
        BindTexture<tp...>::template exec<N + 1>(TU, args...);*/
    }
};


template<SamplerType...tp>
class TextureRead
{
private:
    template<unsigned N, typename...Args>
    void AssignTextureNames_impl(GLuint)
    {
        static_assert(N == sizeof...(tp), "Wrong number of texture name");
    }

    template<unsigned N, typename...Args>
    void AssignTextureNames_impl(GLuint Program, GLuint TexUnit, const char *name, Args...args)
    {
        GLuint location = glGetUniformLocation(Program, name);
        TextureLocation.push_back(location);
        glUniform1i(location, TexUnit);
        TextureUnits.push_back(TexUnit);
        AssignTextureNames_impl<N + 1>(Program, args...);
    }

protected:
    std::vector<GLuint> TextureUnits;
    std::vector<GLenum> TextureType;
    std::vector<GLenum> TextureLocation;
    template<typename...Args>
    void AssignSamplerNames(GLuint Program, Args...args)
    {
        CreateSamplers<tp...>::exec(SamplersId, TextureType);

        glUseProgram(Program);
        AssignTextureNames_impl<0>(Program, args...);
        glUseProgram(0);
    }

    template<int N>
    void SetTextureUnits_impl()
    {
        static_assert(N == sizeof...(tp), "Not enough texture set");
    }

    template<int N, typename... TexIds>
    void SetTextureUnits_impl(GLuint texid, TexIds... args)
    {
        setTextureSampler(TextureType[N], TextureUnits[N], texid, SamplersId[N]);
        SetTextureUnits_impl<N + 1>(args...);
    }


    template<int N>
    void SetTextureHandles_impl()
    {
        static_assert(N == sizeof...(tp), "Not enough handle set");
    }

    template<int N, typename... HandlesId>
    void SetTextureHandles_impl(uint64_t handle, HandlesId... args)
    {
        if (handle)
            glUniformHandleui64ARB(TextureLocation[N], handle);
        SetTextureHandles_impl<N + 1>(args...);
    }

public:
    std::vector<GLuint> SamplersId;

    template<typename... TexIds>
    void SetTextureUnits(TexIds... args)
    {
        SetTextureUnits_impl<0>(args...);
    }

    ~TextureRead()
    {
        for (unsigned i = 0; i < SamplersId.size(); i++)
            glDeleteSamplers(1, &SamplersId[i]);
    }

    template<typename... HandlesId>
    void SetTextureHandles(HandlesId... ids)
    {
        SetTextureHandles_impl<0>(ids...);
    }
};
#endif
