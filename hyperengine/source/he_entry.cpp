#include "he_platform.hpp"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

int main(int argc, char* argv[]) {
	if (hyperengine::isWsl())
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

	glfwInit();
	GLFWwindow* window = glfwCreateWindow(1280, 720, "HyperEngine", nullptr, nullptr);

	glfwMakeContextCurrent(window);
	gladLoadGL(&glfwGetProcAddress);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwDestroyWindow(window);
	glfwTerminate();
    return 0;
}