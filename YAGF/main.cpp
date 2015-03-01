#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Core/CMeshBuffer.h>

irr::scene::IMeshBuffer *buffer;

void init()
{
  buffer = new irr::scene::SMeshBuffer();
}

void draw()
{
  
}

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window = glfwCreateWindow(640, 480, "GLtest", NULL, NULL);
  glfwMakeContextCurrent(window);

  glewInit();
  init;

  while (!glfwWindowShouldClose(window))
  {
    glClearColor(1., 1., 0., 1.);
    glClear(GL_COLOR_BUFFER_BIT);
    draw();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}

