#include "he_platform.hpp"

#include <GLFW/glfw3.h>

int main(int argc, char* argv[]) {
	if (hyperengine::isWsl())
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

	glfwInit();
	GLFWwindow* window = glfwCreateWindow(1280, 720, "HyperEngine", nullptr, nullptr);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
    return 0;
}