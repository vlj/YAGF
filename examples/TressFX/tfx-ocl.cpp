
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Windows.h>
#include <Maths/matrix4.h>

#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <CL/cl_gl_ext.h>

#include <fstream>
#include <iostream>
#include "common.hpp"

#include <Util/Debug.h>

float density = .4;

TressFXAsset tfxassets;

cl_context context;
cl_command_queue queue;
cl_kernel kernel;
cl_program prog;
cl_mem InitialPos;
cl_mem StrandType;
cl_mem PreviousPos;
cl_mem PosBuffer;
cl_mem ConstantSimBuffer;

typedef cl_event(CALLBACK *PFNCreateEventFromGLsyncKHRCustom)(cl_context, cl_GLsync, cl_int *);
PFNCreateEventFromGLsyncKHRCustom clCreateEventFromGLsyncKHRCustom;

void init()
{
  tfxassets = loadTress("..\\examples\\TressFX\\ruby.tfxb");

  initCommon(tfxassets);

  DebugUtil::enableDebugOutput();
  int err;
  cl_platform_id platform;
  clGetPlatformIDs(1, &platform, NULL);
  cl_device_id device;
  clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, 0);

  clCreateEventFromGLsyncKHRCustom = (PFNCreateEventFromGLsyncKHRCustom) clGetExtensionFunctionAddressForPlatform(platform, "clCreateEventFromGLsyncKHR");

  size_t sz;
  clGetDeviceInfo(device, CL_DEVICE_NAME, 0, 0, &sz);
  char *name = new char[sz];
  clGetDeviceInfo(device, CL_DEVICE_NAME, sz, name, 0);
  printf("%s\n", name);
  delete[] name;

  cl_context_properties prop[] = { CL_CONTEXT_PLATFORM, (cl_context_properties) platform,
    CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
    CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
    0
  };

  context = clCreateContext(prop, 1, &device, NULL, NULL, NULL);

  std::ifstream file("..\\examples\\TressFX\\shaders\\Simulation.cl", std::ios::in);

  const std::string &progstr = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  size_t src_sz = progstr.size();
  const char *src = progstr.c_str();

  prog = clCreateProgramWithSource(context, 1, &src, &src_sz, &err);
  err = clBuildProgram(prog, 1, &device, 0, 0, 0);

  size_t log_sz;
  clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_sz);
  char *log = new char[log_sz];
  clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, log_sz, log, 0);
  printf("%s\n", log);
  delete log;

  queue = clCreateCommandQueue(context, device, 0, &err);
  kernel = clCreateKernel(prog, "IntegrationAndGlobalShapeConstraints", &err);
  ConstantSimBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(struct SimulationConstants), 0, &err);

  InitialPos = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pVertices, &err);
  PosBuffer = clCreateFromGLBuffer(context, CL_MEM_READ_WRITE, PosSSBO, &err);
  PreviousPos = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pVertices, &err);
  StrandType = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tfxassets.m_NumTotalHairStrands * sizeof(int), tfxassets.m_pHairStrandType, &err);
}

void clean()
{
  cleanCommon();
}

GLsync syncFirstPassRenderComplete;

void simulate(float time)
{
  struct SimulationConstants cbuf = {};

  irr::core::matrix4 Model;
  if (time < 30.)
    cbuf.bWarp = 1;
  else
    cbuf.bWarp = 0;

  if (time > 180 * 360)
    time = 180 * 360;

  Model.setRotationDegrees(irr::core::vector3df(0., time / 360., 0.));

  memcpy(cbuf.ModelTransformForHead, Model.pointer(), 16 * sizeof(float));
  cbuf.ModelRotateForHead[0] = 0.;
  cbuf.ModelRotateForHead[1] = sin((time / 720) * (3.14 / 180.));
  cbuf.ModelRotateForHead[2] = 0.;
  cbuf.ModelRotateForHead[3] = cos((3.14 / 180.) * time / 720);

  cbuf.timeStep = .016;

  cbuf.Damping0 = 0.125;
  cbuf.StiffnessForGlobalShapeMatching0 = 0.8;
  cbuf.GlobalShapeMatchingEffectiveRange0 = 0.3;
  cbuf.StiffnessForLocalShapeMatching0 = 0.2;
  cbuf.Damping1 = 0.125;
  cbuf.StiffnessForGlobalShapeMatching1 = 0.25;
  cbuf.GlobalShapeMatchingEffectiveRange1 = 0.4;
  cbuf.StiffnessForLocalShapeMatching1 = 0.75;
  cbuf.Damping2 = 0.0199;
  cbuf.StiffnessForGlobalShapeMatching2 = 0.;
  cbuf.GlobalShapeMatchingEffectiveRange2 = 0.;
  cbuf.StiffnessForLocalShapeMatching2 = 0.699;
  cbuf.Damping3 = 0.1;
  cbuf.StiffnessForGlobalShapeMatching3 = 0.2;
  cbuf.GlobalShapeMatchingEffectiveRange3 = 0.3;
  cbuf.StiffnessForLocalShapeMatching3 = 1.;

  cbuf.NumOfStrandsPerThreadGroup = 4;

  cbuf.GravityMagnitude = 10.;
  cbuf.NumLengthConstraintIterations = 2;
  cbuf.NumLocalShapeMatchingIterations = 1;

  memset(cbuf.Wind, 0, 4 * sizeof(float));
  memset(cbuf.Wind1, 0, 4 * sizeof(float));
  memset(cbuf.Wind2, 0, 4 * sizeof(float));
  memset(cbuf.Wind3, 0, 4 * sizeof(float));

  int err = CL_SUCCESS;
  cl_event ev = 0;
  if (syncFirstPassRenderComplete);
    ev = clCreateEventFromGLsyncKHRCustom(context, syncFirstPassRenderComplete, &err);

  glFinish();

  // First pass is done so sim is done too, can safely upload
  err = clEnqueueWriteBuffer(queue, ConstantSimBuffer, CL_FALSE, 0, sizeof(struct SimulationConstants), &cbuf, 0, 0, 0);

  err = clEnqueueAcquireGLObjects(queue, 1, &PosBuffer, 0, 0, 0);
  err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &PosBuffer);
  err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &PreviousPos);
  err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &StrandType);
  err = clSetKernelArg(kernel, 3, sizeof(cl_mem), &InitialPos);
  err = clSetKernelArg(kernel, 4, sizeof(cl_mem), &ConstantSimBuffer);
  size_t local_wg = 64;
  size_t global_wg = density * tfxassets.m_NumTotalHairVertices;
  err = clEnqueueNDRangeKernel(queue, kernel, 1, 0, &global_wg, &local_wg, 0, 0, 0);

  err = clEnqueueReleaseGLObjects(queue, 1, &PosBuffer, 0, 0, 0);
  clFinish(queue);
  if (syncFirstPassRenderComplete);
  {
    glDeleteSync(syncFirstPassRenderComplete);
    clReleaseEvent(ev);
  }
  return;
}

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
      glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
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