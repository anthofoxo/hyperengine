#include "he_platform.hpp"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <string>
#include <iostream>
#include <optional>
#include <vector>
#include <fstream>
#include <format>

#include <renderdoc_app.h>

RENDERDOC_API_1_0_0* kRenderDoc = nullptr;

#ifdef _WIN32

#include <Windows.h>
#include <shlobj_core.h>

void setupRenderdoc() {
	HMODULE library = GetModuleHandleA("renderdoc.dll");
	if (library == nullptr) {
		CHAR pf[MAX_PATH];
		SHGetSpecialFolderPathA(nullptr, pf, CSIDL_PROGRAM_FILES, false);
		library = LoadLibraryA(std::format("{}/RenderDoc/renderdoc.dll", pf).c_str());
	}
	if (library == nullptr) return;

	pRENDERDOC_GetAPI getApi = (pRENDERDOC_GetAPI)GetProcAddress(library, "RENDERDOC_GetAPI");
	if (getApi == nullptr) return;
	getApi(eRENDERDOC_API_Version_1_0_0, (void**)&kRenderDoc);
}
#else
#include <dlfcn.h>

void setupRenderdoc() {
	void* library = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
	if (library == nullptr) library = dlopen("librenderdoc.so", RTLD_NOW);
	if (library == nullptr) return;

	pRENDERDOC_GetAPI getApi = (pRENDERDOC_GetAPI)dlsym(library, "RENDERDOC_GetAPI");
	if (getApi == nullptr) return;
	getApi(eRENDERDOC_API_Version_1_0_0, (void**)&kRenderDoc);
}
#endif

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
#define HE_VERT
#define INPUT(type, name, index) layout(location = index) in type name
#define OUTPUT(type, name, index)
#define UNIFORM(type, name) uniform type name
#line 1
)";

char const* fragHeader = R"(#version 330 core
#define HE_FRAG
#define INPUT(type, name, index)
#define OUTPUT(type, name, index) layout(location = index) out type name
#define UNIFORM(type, name) uniform type name
#define MAX(a, b) a > b ? a : b
#line 1
)";

int main(int argc, char* argv[]) {
	setupRenderdoc();

	if (kRenderDoc) {
		kRenderDoc->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
	}

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

	char const* vertSourcePtr = vertSource.data();
	int vertSourceSize = vertSource.size();
	char const* fragSourcePtr = fragSource.data();
	int fragSourceSize = fragSource.size();

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