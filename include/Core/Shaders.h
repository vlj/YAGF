// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

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
#include <string>


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
      if (InfoLogLength < 0)
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
    glUniform4f(uniforms[N], col.r, col.g, col.b, col.a);
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

extern std::vector<void(*)()> CleanTable;

template<typename T, typename... Args>
class ShaderHelperSingleton : public Singleton < T >
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
    CleanTable.push_back(this->kill);
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

template<GLenum TextureType, unsigned TextureUnit>
struct TextureResource
{
  template <typename BindStruct, typename ...Args>
  static void ChainedBind(GLuint texid, GLuint samplerid, const Args&...args)
  {
    glActiveTexture(GL_TEXTURE0 + TextureUnit);
    glBindTexture(TextureType, texid);
    glBindSampler(TextureUnit, samplerid);
    BindStruct::exec(args...);
  }

  template <typename AssignStruct, typename...Args>
  static void ChainedAssign(GLuint Program, std::vector<GLuint> &TextureLocation, const char* name, const Args&...args)
  {
    GLuint location = glGetUniformLocation(Program, name);
    TextureLocation.push_back(location);
    glUniform1i(location, TextureUnit);
    AssignStruct::exec(Program, TextureLocation, args...);
  }
};

template<unsigned TextureUnit>
struct ImageResource
{
  template <typename BindStruct, typename ...Args>
  static void ChainedBind(GLuint texid, GLenum access, GLenum format, const Args&...args)
  {
    glActiveTexture(GL_TEXTURE0 + TextureUnit);
    glBindImageTexture(TextureUnit, texid, 0, GL_FALSE, 0, access, format);
    glBindSampler(TextureUnit, 0);
    BindStruct::exec(args...);
  }

  template <typename AssignStruct, typename...Args>
  static void ChainedAssign(GLuint Program, std::vector<GLuint> &TextureLocation, const char* name, const Args&...args)
  {
    GLuint location = glGetUniformLocation(Program, name);
    TextureLocation.push_back(location);
    glUniform1i(location, TextureUnit);
    AssignStruct::exec(Program, TextureLocation, args...);
  }
};

template<typename...tp>
struct BindSamplerAndTexture;

template<>
struct BindSamplerAndTexture < >
{
  static void exec()
  {}
};

template<typename T, typename...tp>
struct BindSamplerAndTexture <T, tp...>
{
  template <typename ...Args>
  static void exec(const Args&... args)
  {
    typedef BindSamplerAndTexture<tp...> Next;
    T::template ChainedBind<Next>(args...);
  }
};



template<typename...tp>
struct TextureAssign;

template<>
struct TextureAssign < >
{
public:
  static void exec(GLuint, std::vector<GLuint> &)
  {
  }
};

template<typename T, typename...tp>
struct TextureAssign <T, tp... >
{
public:
  template<typename...Args>
  static void exec(GLuint Program, std::vector<GLuint> &TextureLocation, const Args&...args)
  {
    typedef typename TextureAssign <tp... > AssignerStruct;
    T::template ChainedAssign<AssignerStruct>(Program, TextureLocation, args...);
  }
};


template<typename...tp>
class TextureRead
{
protected:
  std::vector<GLuint> TextureLocation;

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

  template<typename...Args>
  void AssignSamplerNames(GLuint Program, Args...args)
  {
    glUseProgram(Program);
    TextureAssign<tp...>::template exec(Program, TextureLocation, args...);
    glUseProgram(0);
  }

public:
  template<typename... TexIds>
  void SetTextureUnits(TexIds... args)
  {
    BindSamplerAndTexture<tp...>::template exec(args...);
  }

  template<typename... HandlesId>
  void SetTextureHandles(HandlesId... ids)
  {
    SetTextureHandles_impl<0>(ids...);
  }
};

#define TO_STRING(x) #x

#endif
