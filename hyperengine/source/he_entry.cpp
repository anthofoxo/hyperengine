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

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
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

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// ImHex Pattern Matcher
#if 0
#pragma array_limit 2000000
#pragma pattern_limit 2000000

struct Vertex {
	float data[8];
};

struct HeMesh {
	u8 header[16];
	u32 vertexCount;
	Vertex vertices[vertexCount];
	u32 elementCount;
	match(vertexCount) {
		(0x0000 ... 0x00FF) : u8 elements[elementCount];
		(0x0100 ... 0xFFFF) : u16 elements[elementCount];
		(_) : u32 elements[elementCount];
	}
};
#endif

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

struct Transform final {
	glm::vec3 translation;
	glm::quat orientation = glm::identity<glm::quat>();
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

	glm::mat4 get() const {
		glm::vec3 skew = glm::zero<glm::vec3>();
		glm::vec4 perspective(0.0f, 0.0f, 0.0f, 1.0f);
		return glm::recompose(scale, orientation, translation, skew, perspective);
	}

	void set(glm::mat4 const& mat) {
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(mat, scale, orientation, translation, skew, perspective);
	}
};

Transform cameraTransform;
Transform objectTransform;

void transformEditUi(Transform& transform) {
	ImGui::PushID(&transform);
	ImGui::DragFloat3("Translation", glm::value_ptr(transform.translation), 0.1f);
	glm::vec3 oldEuler = glm::degrees(glm::eulerAngles(transform.orientation));
	glm::vec3 newEuler = oldEuler;
	if (ImGui::DragFloat3("Rotation", glm::value_ptr(newEuler))) {
		glm::vec3 deltaEuler = glm::radians(newEuler - oldEuler);

		transform.orientation = glm::rotate(transform.orientation, deltaEuler.x, glm::vec3(1, 0, 0));
		transform.orientation = glm::rotate(transform.orientation, deltaEuler.y, glm::vec3(0, 1, 0));
		transform.orientation = glm::rotate(transform.orientation, deltaEuler.z, glm::vec3(0, 0, 1));
	}
	ImGui::BeginDisabled();
	ImGui::DragFloat4("Orientation", glm::value_ptr(transform.orientation), 0.1f);
	ImGui::EndDisabled();
	ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.1f);
	ImGui::PopID();
}

struct Mesh final {
	GLuint vao, vbo, ebo;
	GLenum type;
	GLsizei count;

	void draw() {
		glDrawElements(GL_TRIANGLES, count, type, nullptr);
	}
};

static std::optional<Mesh> loadMesh(char const* path) {
	static_assert(sizeof(aiVector3D) == sizeof(glm::vec3));

	Assimp::Importer import; 
	aiScene const* scene = import.ReadFile(path, aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_RemoveComponent | aiProcess_FlipUVs | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes | aiProcess_ImproveCacheLocality | aiProcess_FixInfacingNormals);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << import.GetErrorString() << '\n';
		return std::nullopt;
	}

	if (scene->mNumMeshes != 1) {
		std::cerr << "Only one mesh can be exported currently\n";
		return std::nullopt;
	}

	aiMesh* mesh = scene->mMeshes[0];

	std::vector<Vertex> vertices;
	vertices.reserve(mesh->mNumVertices);
	
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		auto& textureCoord = mesh->mTextureCoords[0][i];
		vertices.emplace_back(std::bit_cast<glm::vec3>(mesh->mVertices[i]), std::bit_cast<glm::vec3>(mesh->mNormals[i]), glm::vec2(textureCoord.x, textureCoord.y));
	}

	unsigned char elementPrimitiveWidth;
	if (mesh->mNumVertices <= std::numeric_limits<uint8_t>::max()) elementPrimitiveWidth = sizeof(uint8_t);
	else if (mesh->mNumVertices <= std::numeric_limits<uint16_t>::max()) elementPrimitiveWidth = sizeof(uint16_t);
	else elementPrimitiveWidth = sizeof(uint32_t);

	std::vector<char> elements;
	elements.reserve(mesh->mNumFaces * 3 * elementPrimitiveWidth);

	GLsizei count = 0;

	for (unsigned int iFace = 0; iFace < mesh->mNumFaces; iFace++) {
		aiFace face = mesh->mFaces[iFace];
		if (face.mNumIndices != 3) continue;

		for (unsigned int iElement = 0; iElement < 3; ++iElement) {
			++count;

			for (unsigned int iReserve = 0; iReserve < elementPrimitiveWidth; ++iReserve)
				elements.push_back(0);

			// This method is okay for little endian systems, should verify for big endian
			uint32_t element = face.mIndices[iElement];
			memcpy(elements.data() + elements.size() - elementPrimitiveWidth, &element, elementPrimitiveWidth);
		}
	}

	Mesh glmesh;

	glGenVertexArrays(1, &glmesh.vao);
	glBindVertexArray(glmesh.vao);

	glGenBuffers(1, &glmesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, glmesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(decltype(vertices)::value_type), vertices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &glmesh.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glmesh.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(decltype(elements)::value_type), elements.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void const*)(uintptr_t)offsetof(Vertex, position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void const*)(uintptr_t)offsetof(Vertex, normal));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void const*)(uintptr_t)offsetof(Vertex, uv));

	glmesh.count = count;

	if (elementPrimitiveWidth == 1) glmesh.type = GL_UNSIGNED_BYTE;
	else if (elementPrimitiveWidth == 2) glmesh.type = GL_UNSIGNED_SHORT;
	else glmesh.type = GL_UNSIGNED_INT;

	return glmesh;
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
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplGlfw_InitForOpenGL(window.handle(), true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	glClearColor(0.7f, 0.8f, 0.9f, 1.0f);

	auto optMesh = loadMesh("varoom.obj");
	Mesh mesh = optMesh.value();

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

	bool viewImGuiDemoWindow = false;

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

			if (ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_MenuBar)) {
				if (ImGui::BeginMenuBar()) {
					if (ImGui::BeginMenu("View")) {
						ImGui::MenuItem("ImGui Demo Window", nullptr, &viewImGuiDemoWindow);

						ImGui::EndMenu();
					}

					ImGui::EndMenuBar();
				}

				ImGui::TextUnformatted("Camera");
				ImGui::Separator();
				transformEditUi(cameraTransform);
				ImGui::TextUnformatted("Object");
				ImGui::Separator();
				transformEditUi(objectTransform);
			}
			ImGui::End();

			if (viewImGuiDemoWindow)
				ImGui::ShowDemoWindow(&viewImGuiDemoWindow);

			glViewport(0, 0, width, height);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			glBindVertexArray(mesh.vao);
			texture.bind(0);
			glUseProgram(program);

			glm::mat4 projection = glm::perspective(glm::radians(80.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

			GLint location = glGetUniformLocation(program, "uProjection");
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(projection));

			location = glGetUniformLocation(program, "uTransform");
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(objectTransform.get()));

			location = glGetUniformLocation(program, "uView");
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(glm::inverse(cameraTransform.get())));

			mesh.draw();

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

	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(1, &mesh.vbo);
	glDeleteBuffers(1, &mesh.ebo);
	glDeleteProgram(program);

    return 0;
}