#include "sample.h"
#include <Windows.h>
#include <gflags/gflags.h>

void main(int argc, char *argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  auto window = glfwCreateWindow(1280, 1024, "TressFx", nullptr, nullptr);
  sample tressfx(window);
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    tressfx.draw();
    // Keep running
  }
  glfwDestroyWindow(window);
  return;
}
