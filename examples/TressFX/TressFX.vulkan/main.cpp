#include <Windows.h>
#include "sample.h"

void main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto window = glfwCreateWindow(1280, 1024, "TressFx", nullptr, nullptr);
    sample tressfx(window);
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		tressfx.draw();
		// Keep running
	}
	glfwDestroyWindow(window);
	return;
}
