#ifdef _WIN32
#	include <Windows.h>
#	undef near
#	undef far
#endif

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <optional>
#include <vector>
#include <format>
#include <array>
#include <span>

#include <debug_trap.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "he_mesh.hpp"
#include "he_window.hpp"
#include "he_rdoc.hpp"
#include "he_texture.hpp"
#include "he_shader.hpp"
#include "he_io.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#define HE_IMPL_EXPAND(x) case x: return #x
static std::string_view glConstantToString(GLuint val) {
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

static void message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param) {
	std::cout << glConstantToString(source) << ", " << glConstantToString(type) << ", " << glConstantToString(severity) << ", " << id << ": " << message << '\n';
	psnip_trap();
}

struct Transform final {
	glm::vec3 translation{};
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

static void transformEditUi(Transform& transform, bool enableQuaternionEditing) {
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

int main(int argc, char* argv[]) {
	Transform cameraTransform;
	Transform objectTransform;

	hyperengine::setupRenderDoc(true);

	hyperengine::Window window{{ .width = 1280, .height = 720, .title = "HyperEngine" }};

	glfwMakeContextCurrent(window.handle());
	gladLoadGL(&glfwGetProcAddress);
	
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

	auto optMesh = hyperengine::readMesh("varoom.obj");
	hyperengine::Mesh& mesh = optMesh.value();

	hyperengine::ShaderProgram program;

	{
		auto shader = hyperengine::readFileString("shader.glsl");

		if (shader.has_value()) {
			program = {{
				.source = shader.value()
			}};
		}
	}

	hyperengine::Texture texture = hyperengine::readTextureImage("varoom.png").value_or(hyperengine::Texture());

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	bool enableQuaternionEditing = false;
	bool viewImGuiDemoWindow = false;
	float fov = 80.0f;
	float near = 0.1f;
	float far = 100.0f;

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

				ImGui::DragFloat("Fov", &fov, 1.0f, 10.0f, 170.0f);
				ImGui::DragFloatRange2("Clipping planes", &near, &far, 1.0f, 0.0f, 1000.0f);

				if (ImGui::CollapsingHeader("Camera")) {
					transformEditUi(cameraTransform, enableQuaternionEditing);
				}

				if (ImGui::CollapsingHeader("Object")) {
					transformEditUi(objectTransform, enableQuaternionEditing);
				}
			}
			ImGui::End();

			if (viewImGuiDemoWindow)
				ImGui::ShowDemoWindow(&viewImGuiDemoWindow);

			glViewport(0, 0, width, height);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			mesh.draw();
			texture.bind(0);

			program.bind();

			glm::mat4 projection = glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), near, far);

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

    return 0;
}