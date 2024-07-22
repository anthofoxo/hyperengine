#ifdef _WIN32
#	include <Windows.h>
#endif

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <optional>
#include <vector>
#include <fstream>
#include <format>

#include <debug_trap.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "he_window.hpp"
#include "he_rdoc.hpp"
#include "he_texture.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

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


std::optional<std::vector<char>> readFileBinary(char const* path) {
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

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

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

void loadMesh(GLuint& vao, GLuint& vbo, GLuint& ebo, GLsizei& count, GLenum& type, char const* file) {
	std::vector<char> modelData = readFileBinary("varoom.hemesh").value();

	struct Header {
		char bytes[16];
	} header;

	char* ptr = modelData.data();
	memcpy(&header, ptr, 16);
	ptr += 16;

	uint32_t numverts;
	memcpy(&numverts, ptr, 4);
	ptr += 4;

	std::vector<Vertex> vertices;
	vertices.resize(numverts);
	memcpy(vertices.data(), ptr, numverts * sizeof(Vertex));
	ptr += numverts * sizeof(Vertex);

	uint32_t numelements;
	memcpy(&numelements, ptr, 4);
	ptr += 4;

	unsigned int elementwidth;
	if (vertices.size() < std::numeric_limits<uint8_t>::max()) elementwidth = 1;
	else if (vertices.size() < std::numeric_limits<uint16_t>::max()) elementwidth = 2;
	else elementwidth = 4;

	std::vector<uint8_t> elements;
	elements.resize(numelements * elementwidth);
	memcpy(elements.data(), ptr, elements.size());

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(decltype(vertices)::value_type), vertices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(decltype(elements)::value_type), elements.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void const*)(uintptr_t)offsetof(Vertex, position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void const*)(uintptr_t)offsetof(Vertex, normal));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void const*)(uintptr_t)offsetof(Vertex, uv));

	count = numelements;

	if (elementwidth == 1) type = GL_UNSIGNED_BYTE;
	else if (elementwidth == 2) type = GL_UNSIGNED_SHORT;
	else type = GL_UNSIGNED_INT;
}

int main(int argc, char* argv[]) {
	hyperengine::setupRenderDoc(true);

	hyperengine::Window window{{ .width = 1280, .height = 720, .title = "HyperEngine" }};

	glfwMakeContextCurrent(window.handle());
	gladLoadGL(&glfwGetProcAddress);

	if (GLAD_GL_KHR_debug) std::cout << "KHR_debug\n";
	if (GLAD_GL_ARB_texture_storage) std::cout << "ARB_texture_storage\n";
	if (GLAD_GL_ARB_texture_storage) std::cout << "ARB_direct_state_access\n";

	if (GLAD_GL_KHR_debug) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(&message_callback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplGlfw_InitForOpenGL(window.handle(), true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	glClearColor(0.7f, 0.8f, 0.9f, 1.0f);

	GLuint vao, vbo, ebo;
	GLsizei count;
	GLenum type;
	loadMesh(vao, vbo, ebo, count, type, "varoom.hemesh");

	auto shader = readFile("shader.glsl");

	auto vertSource = std::string(vertHeader) + shader.value();
	auto fragSource = std::string(fragHeader) + shader.value();

	GLuint vert = makeShader(GL_VERTEX_SHADER, vertSource.data(), static_cast<int>(vertSource.size()));
	GLuint frag = makeShader(GL_FRAGMENT_SHADER, fragSource.data(), static_cast<int>(fragSource.size()));

	GLuint program = makeProgram(vert, frag);

	glDeleteShader(vert);
	glDeleteShader(frag);

	hyperengine::Texture texture;

	stbi_set_flip_vertically_on_load(true);

	int x, y;
	stbi_uc* pixels = stbi_load("varoom.png", &x, &y, nullptr, 4);

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

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	while (!glfwWindowShouldClose(window.handle())) {
		glfwPollEvents();

#ifdef _WIN32
		io.ConfigDebugIsDebuggerPresent = IsDebuggerPresent();
#endif

		int width, height;
		glfwGetFramebufferSize(window.handle(), &width, &height);
		
		if (width > 0 && height > 0) {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::ShowDemoWindow();

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

			glDrawElements(GL_TRIANGLES, count, type, nullptr);

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}
		}

		glfwSwapBuffers(window.handle());
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(program);

    return 0;
}