// Copyright (C) 2015 Vincent Lejeune
#include <GL/glew.h>
#include <cstdio>

class DebugUtil
{
public:
  static void
#ifdef WIN32

#endif
    debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* msg, const void *userparam)
  {
    switch (source)
    {
    case GL_DEBUG_SOURCE_API_ARB:
      printf("OpenGL debug callback - API\n");
      break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
      printf("OpenGL debug callback - WINDOW_SYSTEM\n");
      break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
      printf("OpenGL debug callback - SHADER_COMPILER\n");
      break;
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
      printf("OpenGL debug callback - THIRD_PARTY\n");
      break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB:
      printf("OpenGL debug callback - APPLICATION\n");
      break;
    case GL_DEBUG_SOURCE_OTHER_ARB:
      printf("OpenGL debug callback - OTHER\n");
      break;
    }

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR_ARB:
      printf("    Error type : ERROR\n");
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
      printf("    Error type : DEPRECATED_BEHAVIOR\n");
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
      printf("    Error type : UNDEFINED_BEHAVIOR\n");
      break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
      printf("    Error type : PORTABILITY\n");
      break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
      printf("    Error type : PERFORMANCE\n");
      break;
    case GL_DEBUG_TYPE_OTHER_ARB:
      printf("    Error type : OTHER\n");
      break;
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH_ARB:
      printf("    Severity : HIGH\n");
      break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB:
      printf("    Severity : MEDIUM\n");
      break;
    case GL_DEBUG_SEVERITY_LOW_ARB:
      printf("    Severity : LOW\n");
      break;
    }

    if (msg)
      printf("%s\n", msg);
  }

  static void enableDebugOutput()
  {
    if (glDebugMessageCallbackARB)
      glDebugMessageCallbackARB((GLDEBUGPROCARB)debugCallback, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  }

};