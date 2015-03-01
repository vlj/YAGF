#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <Core/VAO.h>

irr::scene::IMeshBuffer *buffer;

void init()
{
  buffer = GeometryCreator::createCubeMeshBuffer(
        irr::core::vector3df(1., 1., 1.));
  VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> > vaomanager;
  auto tmp = vaomanager.getBase(buffer);
  printf("%d %d\n", tmp.first, tmp.second);
}

void clean()
{
  delete buffer;
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

  glewExperimental = GL_TRUE;
  glewInit();
  init();

  while (!glfwWindowShouldClose(window))
  {
    glClearColor(1., 1., 0., 1.);
    glClear(GL_COLOR_BUFFER_BIT);
    draw();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  clean();
  glfwTerminate();
  return 0;
}

