
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

float density = .4f;

TressFXAsset tfxassets;

cl_context context;
cl_device_id device;
cl_command_queue queue;
cl_kernel kernel_Global;
cl_kernel kernel_Local;
cl_program prog;
cl_mem InitialPos;
cl_mem StrandType;
cl_mem PreviousPos;
cl_mem PosBuffer;
cl_mem GlobalRotations;
cl_mem HairRefVecs;
cl_mem ConstantSimBuffer;

typedef cl_event(CALLBACK *PFNCreateEventFromGLsyncKHRCustom)(cl_context, cl_GLsync, cl_int *);
PFNCreateEventFromGLsyncKHRCustom clCreateEventFromGLsyncKHRCustom;

void init()
{
  tfxassets = loadTress("..\\examples\\TressFX\\ruby.tfxb");

  initCommon(tfxassets);

  DebugUtil::enableDebugOutput();
  int err;
  cl_platform_id platforms[2];
  clGetPlatformIDs(2, platforms, NULL);
  cl_platform_id platform = platforms[0];
  clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, 0);

  clCreateEventFromGLsyncKHRCustom = (PFNCreateEventFromGLsyncKHRCustom) clGetExtensionFunctionAddressForPlatform(platform, "clCreateEventFromGLsyncKHR");

  size_t sz;
  clGetDeviceInfo(device, CL_DEVICE_NAME, 0, 0, &sz);
  char *name = new char[sz];
  clGetDeviceInfo(device, CL_DEVICE_NAME, sz, name, 0);
  printf("%s\n", name);
  delete[] name;

  cl_context_properties prop[] = { CL_CONTEXT_PLATFORM, (cl_context_properties) platform,
//    CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
//    CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
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

  cl_queue_properties qprop[] = {
//    CL_QUEUE_PROPERTIES, CL_QUEUE_ON_DEVICE,
    CL_QUEUE_SIZE, CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE,
    0
  };
  queue = clCreateCommandQueueWithProperties(context, device, qprop, &err);
  kernel_Global = clCreateKernel(prog, "IntegrationAndGlobalShapeConstraints", &err);
  kernel_Local = clCreateKernel(prog, "LocalShapeConstraintsWithIteration", &err);
  ConstantSimBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(struct SimulationConstants), 0, &err);

  InitialPos = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pVertices, &err);
//  PosBuffer = clCreateFromGLBuffer(context, CL_MEM_READ_WRITE, PosSSBO, &err);
  PosBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pVertices, &err);
  PreviousPos = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pVertices, &err);
  StrandType = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tfxassets.m_NumTotalHairStrands * sizeof(int), tfxassets.m_pHairStrandType, &err);
  GlobalRotations = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pGlobalRotations, &err);
  HairRefVecs = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tfxassets.m_pRefVectors, &err);
}

void clean()
{
  clReleaseMemObject(InitialPos);
  clReleaseMemObject(PosBuffer);
  clReleaseMemObject(PreviousPos);
  clReleaseMemObject(StrandType);
  clReleaseMemObject(GlobalRotations);
  clReleaseMemObject(HairRefVecs);
  clReleaseMemObject(ConstantSimBuffer);
  clReleaseKernel(kernel_Global);
  clReleaseKernel(kernel_Local);
  clReleaseProgram(prog);
  clReleaseCommandQueue(queue);
  clReleaseDevice(device);
  clReleaseContext(context);
  cleanCommon();
}

void simulate(float time)
{
  struct SimulationConstants cbuf = {};

  irr::core::matrix4 Model;
  if (time < 30.)
    cbuf.bWarp = 1;
  else
    cbuf.bWarp = 0;

  if (time > 180.f * 360.f)
    time = 180.f * 360.f;

  Model.setRotationDegrees(irr::core::vector3df(0.f, time / 360.f, 0.f));

  memcpy(cbuf.ModelTransformForHead, Model.pointer(), 16 * sizeof(float));
  cbuf.ModelRotateForHead[0] = 0.f;
  cbuf.ModelRotateForHead[1] = sinf((time / 720.f) * (3.14f / 180.f));
  cbuf.ModelRotateForHead[2] = 0.f;
  cbuf.ModelRotateForHead[3] = cosf((3.14f / 180.f) * time / 720.f);

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

  cbuf.GravityMagnitude = 10.f;
  cbuf.NumLengthConstraintIterations = 2;
  cbuf.NumLocalShapeMatchingIterations = 1;

  memset(cbuf.Wind, 0, 4 * sizeof(float));
  memset(cbuf.Wind1, 0, 4 * sizeof(float));
  memset(cbuf.Wind2, 0, 4 * sizeof(float));
  memset(cbuf.Wind3, 0, 4 * sizeof(float));

  int err = CL_SUCCESS;
  cl_event ev = 0;
//  if (syncobj)
//    ev = clCreateEventFromGLsyncKHRCustom(context, syncobj, &err);

  glFinish();

  // First pass is done so sim is done too, can safely upload
  err = clEnqueueWriteBuffer(queue, ConstantSimBuffer, CL_TRUE, 0, sizeof(struct SimulationConstants), &cbuf, 0, 0, 0);
  unsigned int maxId = tfxassets.m_NumTotalHairVertices;
//  err = clEnqueueAcquireGLObjects(queue, 1, &PosBuffer, 0, 0, 0);
  size_t local_wg = 64;
  size_t global_wg_kern_g = (size_t)(density * tfxassets.m_NumTotalHairVertices);
  err = clSetKernelArg(kernel_Global, 0, sizeof(cl_mem), &PosBuffer);
  err = clSetKernelArg(kernel_Global, 1, sizeof(cl_mem), &PreviousPos);
  err = clSetKernelArg(kernel_Global, 2, sizeof(cl_mem), &StrandType);
  err = clSetKernelArg(kernel_Global, 3, sizeof(cl_mem), &InitialPos);
  err = clSetKernelArg(kernel_Global, 4, sizeof(cl_mem), &ConstantSimBuffer);
  err = clSetKernelArg(kernel_Global, 5, sizeof(unsigned int), &maxId);
  err = clEnqueueNDRangeKernel(queue, kernel_Global, 1, 0, &global_wg_kern_g, &local_wg, 0, 0, &ev);

  size_t global_wg_kern_l = (size_t)(density * tfxassets.m_NumTotalHairStrands);
  err = clSetKernelArg(kernel_Local, 0, sizeof(cl_mem), &PosBuffer);
  err = clSetKernelArg(kernel_Local, 1, sizeof(cl_mem), &StrandType);
  err = clSetKernelArg(kernel_Local, 2, sizeof(cl_mem), &GlobalRotations);
  err = clSetKernelArg(kernel_Local, 3, sizeof(cl_mem), &HairRefVecs);
  err = clSetKernelArg(kernel_Local, 4, sizeof(cl_mem), &ConstantSimBuffer);
  err = clSetKernelArg(kernel_Local, 5, sizeof(unsigned int), &maxId);
  err = clEnqueueNDRangeKernel(queue, kernel_Local, 1, 0, &global_wg_kern_l, &local_wg, 0, 0, 0);

  float *tmp = new float[tfxassets.m_NumTotalHairVertices * 4];
  err = clEnqueueReadBuffer(queue, PosBuffer, CL_TRUE, 0, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tmp, 0, 0, 0);
  clFinish(queue);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, PosSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, tfxassets.m_NumTotalHairVertices * 4 * sizeof(float), tmp, GL_STATIC_DRAW);

//  err = clEnqueueReleaseGLObjects(queue, 1, &PosBuffer, 0, 0, 0);

  if (syncobj)
  {
    glDeleteSync(syncobj);
    //clReleaseEvent(ev);
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