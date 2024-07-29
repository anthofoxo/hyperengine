#ifdef _WIN32
#	include <Windows.h>
#endif

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <optional>
#include <vector>
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
#include "he_shader.hpp"
#include "he_io.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

#define HE_IMPL_EXPAND(x) case x: return #x
std::string_view glConstantToString(GLuint val) {
	switch (val) {
	HE_IMPL_EXPAND(GL_DEBUG_SOURCE_API);
	HE_IMPL_EXPAND(GL_DEBUG_SOURCE_WINDOW_SYSTEM);
	HE_IMPL_EXPAND(GL_DEBUG_SOURCE_SHADER_COMPILER);
	HE_IMPL_EXPAND(GL_DEBUG_SOURCE_THIRD_PARTY);
	HE_IMPL_EXPAND(GL_DEBUG_SOURCE_APPLICATION);
	HE_IMPL_EXPAND(GL_DEBUG_SOURCE_OTHER);

	HE_IMPL_EXPAND(GL_DEBUG_TYPE_ERROR);
	HE_IMPL_EXPAND(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR);
	HE_IMPL_EXPAND(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR);
	HE_IMPL_EXPAND(GL_DEBUG_TYPE_PORTABILITY);
	HE_IMPL_EXPAND(GL_DEBUG_TYPE_PERFORMANCE);
	HE_IMPL_EXPAND(GL_DEBUG_TYPE_MARKER);
	HE_IMPL_EXPAND(GL_DEBUG_TYPE_OTHER);

	HE_IMPL_EXPAND(GL_DEBUG_SEVERITY_NOTIFICATION);
	HE_IMPL_EXPAND(GL_DEBUG_SEVERITY_LOW);
	HE_IMPL_EXPAND(GL_DEBUG_SEVERITY_MEDIUM);
	HE_IMPL_EXPAND(GL_DEBUG_SEVERITY_HIGH);
	default: return "?";
	}
}
#undef HE_IMPL_EXPAND

void message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param) {
	std::cout << glConstantToString(source) << ", " << glConstantToString(type) << ", " << glConstantToString(severity) << ", " << id << ": " << message << '\n';
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

bool enableQuaternionEditing = false;

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
	ImGui::BeginDisabled(!enableQuaternionEditing);
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

	if (GLAD_GL_ARB_direct_state_access) {

		glCreateVertexArrays(1, &glmesh.vao);

		glCreateBuffers(1, &glmesh.vbo);
		glNamedBufferStorage(glmesh.vbo, vertices.size() * sizeof(decltype(vertices)::value_type), vertices.data(), 0);

		glCreateBuffers(1, &glmesh.ebo);
		glNamedBufferStorage(glmesh.ebo, elements.size() * sizeof(decltype(elements)::value_type), elements.data(), 0);

		GLuint bindingIndex = 0;
		glVertexArrayVertexBuffer(glmesh.vao, bindingIndex, glmesh.vbo, 0, sizeof(Vertex));
		glVertexArrayElementBuffer(glmesh.vao, glmesh.ebo);

		glEnableVertexArrayAttrib(glmesh.vao, 0);
		glEnableVertexArrayAttrib(glmesh.vao, 1);
		glEnableVertexArrayAttrib(glmesh.vao, 2);

		glVertexArrayAttribFormat(glmesh.vao, 0, 3, GL_FLOAT, GL_FALSE, (GLuint)offsetof(Vertex, position));
		glVertexArrayAttribFormat(glmesh.vao, 1, 3, GL_FLOAT, GL_FALSE, (GLuint)offsetof(Vertex, normal));
		glVertexArrayAttribFormat(glmesh.vao, 2, 2, GL_FLOAT, GL_FALSE, (GLuint)offsetof(Vertex, uv));

		glVertexArrayAttribBinding(glmesh.vao, 0, bindingIndex);
		glVertexArrayAttribBinding(glmesh.vao, 1, bindingIndex);
		glVertexArrayAttribBinding(glmesh.vao, 2, bindingIndex);
	}
	else {
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
	}

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

	hyperengine::ShaderProgram program;

	{
		auto shader = hyperengine::readFileString("shader.glsl");

		if (shader.has_value()) {
			program = {{
				.source = shader.value()
			}};
		}
	}

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

		int width, height;
		glfwGetFramebufferSize(window.handle(), &width, &height);
		
		if (width > 0 && height > 0) {
#ifdef _WIN32
			io.ConfigDebugIsDebuggerPresent = IsDebuggerPresent();
#endif

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

				ImGui::Checkbox("Quaternion editing", &enableQuaternionEditing);

				if (ImGui::CollapsingHeader("Camera")) {
					transformEditUi(cameraTransform);
				}

				if (ImGui::CollapsingHeader("Object")) {
					transformEditUi(cameraTransform);
				}
			}
			ImGui::End();

			if (viewImGuiDemoWindow)
				ImGui::ShowDemoWindow(&viewImGuiDemoWindow);

			glViewport(0, 0, width, height);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			glBindVertexArray(mesh.vao);
			texture.bind(0);

			program.bind();

			glm::mat4 projection = glm::perspective(glm::radians(80.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

			program.uniformMat4f("uProjection", projection);
			program.uniformMat4f("uTransform", objectTransform.get());
			program.uniformMat4f("uView", glm::inverse(cameraTransform.get()));

			mesh.draw();

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}

			window.swapBuffers();
		}

	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(1, &mesh.vbo);
	glDeleteBuffers(1, &mesh.ebo);

    return 0;
}