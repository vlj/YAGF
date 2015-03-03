#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Util/GeometryCreator.h>
#include <Core/VAO.h>

#include <Maths/matrix4.h>
#include <Core/Shaders.h>

irr::scene::IMeshBuffer *buffer;


const char *vtxshader =
    "#version 330\n"
    "uniform mat4 ModelMatrix;\n"
    "uniform mat4 InverseModelMatrix;\n"
    "layout(location = 0) in vec3 Position;\n"
    "void main(void) {\n"
    "  gl_Position = ModelMatrix * vec4(Position, 1.);\n"
    "}\n";

const char *fragshader =
    "#version 330\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "  FragColor = vec4(1.0);\n"
    "}\n";

class ObjectShader : public ShaderHelperSingleton<ObjectShader, irr::core::matrix4, irr::core::matrix4>, public TextureRead<Trilinear_Anisotropic_Filtered, Trilinear_Anisotropic_Filtered>
{
public:
    ObjectShader()
    {
      Program = ProgramShaderLoading::LoadProgram(
          GL_VERTEX_SHADER, vtxshader,
          GL_FRAGMENT_SHADER, fragshader);
      AssignUniforms("ModelMatrix", "InverseModelMatrix");
    }
};


void init()
{
  glDisable(GL_CULL_FACE);
  buffer = GeometryCreator::createCubeMeshBuffer(
        irr::core::vector3df(1., 1., 1.));
  auto tmp = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> >::getInstance()->getBase(buffer);
  printf("%d %d\n", tmp.first, tmp.second);
}

void clean()
{
  delete buffer;
}

void draw()
{
  glUseProgram(ObjectShader::getInstance()->Program);
  glBindVertexArray(VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex> >::getInstance()->getVAO());
  irr::core::matrix4 tmp;
//  tmp.print();
  ObjectShader::getInstance()->setUniforms(tmp, tmp);
//  printf("index cnt %d\n", buffer->getIndexCount());
  glDrawElementsBaseVertex(GL_TRIANGLES, buffer->getIndexCount(), GL_UNSIGNED_SHORT, 0, 0);
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
    glClearColor(0., 0., 0., 1.);
    glClear(GL_COLOR_BUFFER_BIT);
    draw();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  clean();
  glfwTerminate();
  return 0;
}

