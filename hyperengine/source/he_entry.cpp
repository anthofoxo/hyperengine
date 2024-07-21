#include "he_platform.hpp"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <string>
#include <iostream>
#include <optional>
#include <vector>
#include <fstream>

std::optional<std::vector<char>> readFile(char const* path) {
	std::ifstream file;
	file.open(path, std::ios::in | std::ios::binary);
	if (!file) return std::nullopt;

	std::vector<char> content;
	file.seekg(0, std::ios::end);
	auto size = file.tellg();
	content.resize(size);
	file.seekg(0, std::ios::beg);
	file.read(content.data(), content.size());
	file.close();
	return content;
}

int main(int argc, char* argv[]) {
	if (hyperengine::isWsl())
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

	glfwInit();
	GLFWwindow* window = glfwCreateWindow(1280, 720, "HyperEngine", nullptr, nullptr);

	glfwMakeContextCurrent(window);
	gladLoadGL(&glfwGetProcAddress);

	glClearColor(0.7f, 0.8f, 0.9f, 1.0f);

	GLuint vao, vbo;

	GLfloat data[] = {
		-1.0f, 1.0f,
		-1.0f, -1.0f,
		1.0f, 1.0f,
		1.0f, -1.0f,
	};

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof data, data, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void const*)(uintptr_t)0);

	auto vertSource = readFile("shader.vert.glsl");
	auto fragSource = readFile("shader.frag.glsl");

	char const* vertSourcePtr = vertSource.value().data();
	int vertSourceSize = vertSource.value().size();
	char const* fragSourcePtr = fragSource.value().data();
	int fragSourceSize = fragSource.value().size();

	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vert, 1, &vertSourcePtr, &vertSourceSize);
	glShaderSource(frag, 1, &fragSourcePtr, &fragSourceSize);

	glCompileShader(vert);
	glCompileShader(frag);
	
	GLint param;
	glGetShaderiv(vert, GL_INFO_LOG_LENGTH, &param);

	if (param > 0) {
		std::string error;
		error.resize(param);
		glGetShaderInfoLog(vert, param, nullptr, error.data());
		std::cerr << error << '\n';
	}

	GLint param2;
	glGetShaderiv(frag, GL_INFO_LOG_LENGTH, &param2);

	if (param2 > 0) {
		std::string error;
		error.resize(param2);
		glGetShaderInfoLog(frag, param2, nullptr, error.data());
		std::cerr << error << '\n';
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	glDetachShader(program, vert);
	glDetachShader(program, frag);

	glDeleteShader(vert);
	glDeleteShader(frag);

	GLint param3;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &param3);

	if (param3 > 0) {
		std::string error;
		error.resize(param3);
		glGetProgramInfoLog(program, param3, nullptr, error.data());
		std::cerr << error << '\n';
	}

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glBindVertexArray(vao);
		glUseProgram(program);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(program);

	glfwDestroyWindow(window);
	glfwTerminate();
    return 0;
}