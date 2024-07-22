#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <optional>
#include <vector>
#include <fstream>
#include <format>

#include <debug-trap.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "he_platform.hpp"
#include "he_rdoc.hpp"
#include "he_texture.hpp"

GLuint makeShader(GLenum type, GLchar const* string, GLint length) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &string, &length);
	glCompileShader(shader);

	GLint param;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &param);

	if (param > 0) {
		std::string error;
		error.resize(param);
		glGetShaderInfoLog(shader, param, nullptr, error.data());
		std::cerr << error << '\n';
		psnip_trap();
	}

	return shader;
}

GLuint makeProgram(GLuint vert, GLuint frag) {
	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	glDetachShader(program, vert);
	glDetachShader(program, frag);

	GLint param;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &param);

	if (param > 0) {
		std::string error;
		error.resize(param);
		glGetProgramInfoLog(program, param, nullptr, error.data());
		std::cerr << error << '\n';
		psnip_trap();
	}

	return program;
}

std::optional<std::string> readFile(char const* path) {
	std::ifstream file;
	file.open(path, std::ios::in | std::ios::binary);
	if (!file) return std::nullopt;

	std::string content;
	file.seekg(0, std::ios::end);
	auto size = file.tellg();
	content.resize(size);
	file.seekg(0, std::ios::beg);
	file.read(content.data(), content.size());
	file.close();
	return content;
}

char const* vertHeader = R"(#version 330 core
#define VERT
#define INPUT(type, name, index) layout(location = index) in type name
#define OUTPUT(type, name, index)
#define VARYING(type, name) out type name
#define UNIFORM(type, name) uniform type name
#line 1
)";

char const* fragHeader = R"(#version 330 core
#define FRAG
#define INPUT(type, name, index)
#define OUTPUT(type, name, index) layout(location = index) out type name
#define VARYING(type, name) in type name
#define UNIFORM(type, name) uniform type name
#line 1
)";



void message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param) {
	auto const src_str = [source]() {
		switch (source)
		{
		case GL_DEBUG_SOURCE_API: return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
		case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
		case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
		case GL_DEBUG_SOURCE_OTHER: return "OTHER";
		default: return "?";
		}
	}();

	auto const type_str = [type]() {
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR: return "ERROR";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
		case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
		case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
		case GL_DEBUG_TYPE_MARKER: return "MARKER";
		case GL_DEBUG_TYPE_OTHER: return "OTHER";
		default: return "?";
		}
	}();

	auto const severity_str = [severity]() {
		switch (severity) {
		case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
		case GL_DEBUG_SEVERITY_LOW: return "LOW";
		case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
		case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
		default: return "?";
		}
	}();
	std::cout << src_str << ", " << type_str << ", " << severity_str << ", " << id << ": " << message << '\n';
	psnip_trap();
}

int main(int argc, char* argv[]) {
	hyperengine::setupRenderDoc(true);

	if (hyperengine::isWsl())
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "HyperEngine", nullptr, nullptr);

	glfwMakeContextCurrent(window);
	gladLoadGL(&glfwGetProcAddress);

	if (GLAD_GL_KHR_debug) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(&message_callback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
	}

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

	auto shader = readFile("shader.glsl");

	auto vertSource = std::string(vertHeader) + shader.value();
	auto fragSource = std::string(fragHeader) + shader.value();

	GLuint vert = makeShader(GL_VERTEX_SHADER, vertSource.data(), static_cast<int>(vertSource.size()));
	GLuint frag = makeShader(GL_FRAGMENT_SHADER, fragSource.data(), static_cast<int>(fragSource.size()));

	GLuint program = makeProgram(vert, frag);

	glDeleteShader(vert);
	glDeleteShader(frag);

	hyperengine::Texture texture;

	int x, y;
	stbi_uc* pixels = stbi_load("texture.png", &x, &y, nullptr, 4);

	if (pixels) {
		texture = {{
				.width = x,
				.height = y,
				.format = hyperengine::PixelFormat::kRgba8,
				.minFilter = GL_LINEAR,
				.magFilter = GL_LINEAR,
				.wrap = GL_CLAMP_TO_EDGE
			}};

		texture.upload({
				.xoffset = 0,
				.yoffset = 0,
				.width = x,
				.height = y,
				.format = hyperengine::PixelFormat::kRgba8,
				.pixels = pixels
			});
	}

	stbi_image_free(pixels);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		
		if (width > 0 && height > 0) {
			glViewport(0, 0, width, height);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			glBindVertexArray(vao);
			texture.bind(0);
			glUseProgram(program);

			glm::mat4 projection = glm::perspective(glm::radians(80.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

			GLint location = glGetUniformLocation(program, "uProjection");
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(projection));

			glm::mat4 transform = glm::identity<glm::mat4>();
			transform = glm::rotate(transform, glm::radians(static_cast<float>(glfwGetTime() * 240.0f)), glm::vec3(1, 2, 1));

			location = glGetUniformLocation(program, "uTransform");
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(transform));

			glm::mat4 camera = glm::identity<glm::mat4>();
			camera = glm::translate(camera, glm::vec3(0, 0, 3));

			location = glGetUniformLocation(program, "uView");
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(glm::inverse(camera)));

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glfwSwapBuffers(window);
		}
	}

	// Have to manually dealloacte this,
	// the window would be destroyed before the destructor would run
	texture = {};

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(program);

	glfwMakeContextCurrent(nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
    return 0;
}