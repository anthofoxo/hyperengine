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
#include <set>
#include <vector>
#include <format>
#include <array>
#include <span>
#include <memory>
#include <sstream>
#include <regex>
#include <fstream>

#include <debug_trap.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "he_common.hpp"
#include "he_platform.hpp"
#include "he_mesh.hpp"
#include "he_window.hpp"
#include "he_rdoc.hpp"
#include "he_texture.hpp"
#include "he_shader.hpp"
#include "he_io.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <misc/cpp/imgui_stdlib.h>

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

#include <TextEditor.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

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
	hyperengine::breakpointIfDebugging();
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

static void transformEditUi(Transform& transform) {
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
	ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.1f);
	ImGui::PopID();
}

static void initImGui(hyperengine::Window& window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
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
}

static void destroyImGui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

static void imguiBeginFrame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
#ifdef _WIN32
	ImGui::GetIO().ConfigDebugIsDebuggerPresent = IsDebuggerPresent();
#endif
}

static void imguiEndFrame() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

auto glslShaderDef() {
	TextEditor::LanguageDefinition langDef;

	const char* const keywords[] = {
		"void", "bool", "int", "uint", "float", "double",
		"bvec2", "bvec3", "bvec4", "ivec2", "ivec3", "ivec4",
		"uvec2", "uvec3", "uvec4", "vec2", "vec3", "vec4",
		"dvec2", "dvec3", "dvec4", "mat2", "mat3", "mat4",
		"mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4",
		"mat4x2", "mat4x3", "mat4x4",
		"sampler2D", "in", "out", "const", "uniform"
	};
	for (auto& k : keywords)
		langDef.mKeywords.insert(k);

	char const* const hyperengineMacros[] = {
		"INPUT", "OUTPUT", "VARYING", "VERT", "FRAG", "UNIFORM", "CONST"
	};

	for (auto& k : hyperengineMacros) {
		TextEditor::Identifier id;
		id.mDeclaration = "HyperEngine macro";
		langDef.mPreprocIdentifiers.insert(std::make_pair(std::string(k), id));
	}

	const char* const builtins[] = {
		"acos", "acosh", "asin", "asinh", "atan", "atanh", "cos", "cosh", "degrees", "radians", "sin", "sinh", "tan", "tanh"
		"abs", "ceil", "clamp", "dFdx", "dFdy", "exp", "exp2", "floor", "fma", "fract", "fwidth", "inversesqrt", "isinf", "isnan",
		"log", "log2", "max", "min", "mix", "mod", "modf", "noise", "pow", "round", "roundEven", "sign", "smoothstep", "sqrt",
		"step", "trunc", "floatBitsToInt", "intBitsToFloat", "gl_ClipDistance", "gl_FragCoord", "gl_FragDepth", "gl_FrontFacing",
		"gl_InstanceID", "gl_InvocationID", "gl_Layer", "gl_PointCoord", "gl_PointSize", "gl_Position", "gl_PrimitiveID",
		"gl_PrimitiveIDIn", "gl_SampleID", "gl_SampleMask", "gl_SampleMaskIn", "gl_SamplePosition", "gl_VertexID", "gl_ViewportIndex"
		"cross", "distance", "dot", "equal", "faceforward", "length", "normalize", "notEqual", "reflect", "refract", "all", "any",
		"greaterThan", "greaterThanEqual", "lessThan", "lessThanEqual", "not", "EmitVertex", "EndPrimitive", "texelFetch",
		"texelFetchOffset", "texture", "textureGrad", "textureGradOffset", "textureLod", "textureLodOffset", "textureOffset",
		"textureProj", "textureProjGrad", "textureProjGradOffset", "textureProjLod", "textureProjLodOffset", "textureProjOffset",
		"textureSize", "determinant", "inverse", "matrixCompMult", "outerProduct", "transpose",
	};
	for (auto& k : builtins) {
		TextEditor::Identifier id;
		id.mDeclaration = "Built-in";
		langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
	}

	langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", TextEditor::PaletteIndex::Preprocessor));
	langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", TextEditor::PaletteIndex::String));
	langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("\\'\\\\?[^\\']\\'", TextEditor::PaletteIndex::CharLiteral));
	langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", TextEditor::PaletteIndex::Number));
	langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
	langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
	langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
	langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", TextEditor::PaletteIndex::Identifier));
	langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", TextEditor::PaletteIndex::Punctuation));

	langDef.mCommentStart = "/*";
	langDef.mCommentEnd = "*/";
	langDef.mSingleLineComment = "//";

	langDef.mCaseSensitive = true;
	langDef.mAutoIndentation = true;

	langDef.mName = "GLSL";

	return langDef;
}

size_t stringSplit(std::string const& txt, std::vector<std::string>& strs, char ch) {
	size_t pos = txt.find(ch);
	size_t initialPos = 0;
	strs.clear();

	// Decompose statement
	while (pos != std::string::npos) {
		strs.push_back(txt.substr(initialPos, pos - initialPos));
		initialPos = pos + 1;

		pos = txt.find(ch, initialPos);
	}

	strs.push_back(txt.substr(initialPos, std::min(pos, txt.size()) - initialPos + 1));

	return strs.size();
}

struct ResourceManager final {
	hyperengine::UnorderedStringMap<std::weak_ptr<hyperengine::Mesh>> mMeshes;
	hyperengine::UnorderedStringMap<std::weak_ptr<hyperengine::Texture>> mTextures;
	hyperengine::UnorderedStringMap<std::weak_ptr<hyperengine::ShaderProgram>> mShaders;

	hyperengine::UnorderedStringMap<TextEditor> mShaderEditor;

	std::shared_ptr<hyperengine::Mesh> getMesh(std::string_view path) {
		auto it = mMeshes.find(path);

		if (it != mMeshes.end())
			if (std::shared_ptr<hyperengine::Mesh> ptr = it->second.lock())
				return ptr;

		std::string pathStr(path);

		auto optMesh = hyperengine::readMesh(pathStr.c_str());
		if (!optMesh.has_value()) return nullptr;

		std::shared_ptr<hyperengine::Mesh> mesh = std::make_shared<hyperengine::Mesh>(std::move(optMesh.value()));
		mMeshes[std::move(pathStr)] = mesh;
		return mesh;
	}

	std::shared_ptr<hyperengine::Texture> getTexture(std::string_view path) {
		auto it = mTextures.find(path);

		if (it != mTextures.end())
			if (std::shared_ptr<hyperengine::Texture> ptr = it->second.lock())
				return ptr;

		std::string pathStr(path);

		auto opt = hyperengine::readTextureImage(pathStr.c_str());
		if (!opt.has_value()) return nullptr;

		std::shared_ptr<hyperengine::Texture> texture = std::make_shared<hyperengine::Texture>(std::move(opt.value()));
		mTextures[std::move(pathStr)] = texture;
		return texture;
	}

	void reloadShader(std::string const& pathStr, hyperengine::ShaderProgram& program) {
		auto shader = hyperengine::readFileString(pathStr.c_str());
		if (!shader.has_value()) return;

		// reload shader, will repopulate errors
		program = { {.source = shader.value() } };

		// Setup debug editor
		std::unordered_map<int, std::set<std::string>> parsed;
		std::regex match("[0-9]*\\(([0-9]+)\\)");

		// On nvidia drivers, the program info log will include errors from all stages
		if (!program.errors().empty()) {
			for (auto const& error : program.errors()) {
				std::vector<std::string> lines;
				stringSplit(error, lines, '\n');

				// Attempt to parse information from line
				for (auto const& line : lines) {
					auto words_begin = std::sregex_iterator(line.begin(), line.end(), match);
					auto words_end = std::sregex_iterator();

					for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
						std::smatch match = *i;

						int lineMatch = std::stoi(match[1]);
						if (lineMatch == 0) lineMatch = 1; // If no line, push error to top of file
						parsed[lineMatch].insert(line);
						break; //  Only care about the first match
					}
				}
			}
		}

		TextEditor editor;
		editor.SetLanguageDefinition(glslShaderDef());
		editor.SetText(shader.value());
		TextEditor::ErrorMarkers markers;
		for (auto const& [k, v] : parsed) {
			std::string str;
			for (auto const& lineFromSet : v)
				str += lineFromSet + '\n';

			markers.insert(std::make_pair(k, str));
		}
		editor.SetErrorMarkers(markers);
		mShaderEditor[pathStr] = editor;
	}

	std::shared_ptr<hyperengine::ShaderProgram> getShaderProgram(std::string_view path) {
		auto it = mShaders.find(path);

		if (it != mShaders.end())
			if (std::shared_ptr<hyperengine::ShaderProgram> ptr = it->second.lock())
				return ptr;

		std::string pathStr(path);

		hyperengine::ShaderProgram program;
		reloadShader(pathStr, program);

		std::shared_ptr<hyperengine::ShaderProgram> obj = std::make_shared<hyperengine::ShaderProgram>(std::move(program));
		mShaders[std::move(pathStr)] = obj;
		return obj;
	}
};

struct GameObjectComponent final {
	bool spin = false;
	std::string name = "unnamed";
	Transform transform;
	std::shared_ptr<hyperengine::Mesh> mesh;
	std::shared_ptr<hyperengine::Texture> texture;
	std::shared_ptr<hyperengine::ShaderProgram> shader;

	GameObjectComponent() {
		std::stringstream ss;
		ss << "unnamed " << std::hex << (void*)this;
		name = ss.str();
	}
};

struct Engine final {
	void init() {
		mWindow = {{ .width = 1280, .height = 720, .title = "HyperEngine" }};

		glfwMakeContextCurrent(mWindow.handle());
		gladLoadGL(&glfwGetProcAddress);

		if (GLAD_GL_KHR_debug) {
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(&message_callback, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		}

		glfwSetWindowUserPointer(mWindow.handle(), this);
		glfwSetWindowCloseCallback(mWindow.handle(), [](GLFWwindow* window) {
			static_cast<Engine*>(glfwGetWindowUserPointer(window))->mRunning = false;
		});

		initImGui(mWindow);

		glClearColor(0.7f, 0.8f, 0.9f, 1.0f);

		{
			entt::handle entity{ mRegistry, mRegistry.create() };
			GameObjectComponent& gameObject = entity.emplace<GameObjectComponent>();
			gameObject.mesh = mResourceManager.getMesh("varoom.obj");
			gameObject.shader = mResourceManager.getShaderProgram("shader.glsl");
			gameObject.texture = mResourceManager.getTexture("varoom.png");
			gameObject.transform.translation = glm::vec3(-1, 0, 0);
		}

		{
			entt::handle entity{ mRegistry, mRegistry.create() };
			GameObjectComponent& gameObject = entity.emplace<GameObjectComponent>();
			gameObject.mesh = mResourceManager.getMesh("dragon.obj");
			gameObject.shader = mResourceManager.getShaderProgram("shader.glsl");
			gameObject.texture = mResourceManager.getTexture("color.png");
			gameObject.transform.translation = glm::vec3(1, 0, 0);
			gameObject.transform.scale = glm::vec3(0.2f);
			gameObject.spin = true;
		}

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	void run() {
		init();

		while (mRunning) {
			glfwPollEvents();
			glfwGetFramebufferSize(mWindow.handle(), &mFramebufferSize.x, &mFramebufferSize.y);

			if (mFramebufferSize.x > 0 && mFramebufferSize.y > 0) {
				imguiBeginFrame();
				update();
				imguiEndFrame();
			}

			mWindow.swapBuffers();
		}

		destroyImGui();
	}

	void drawUi() {
		if (ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_MenuBar)) {
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("View")) {
					ImGui::MenuItem("ImGui Demo Window", nullptr, &mViewImGuiDemoWindow);

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			ImGui::DragFloat("Fov", &mFov, 1.0f, 10.0f, 170.0f);
			ImGui::DragFloatRange2("Clipping planes", &mNear, &mFar, 1.0f, 0.0f, 1000.0f);

			if (ImGui::CollapsingHeader("Camera"))
				transformEditUi(mCameraTransform);		
		}
		ImGui::End();

		for (auto& [k, v] : mResourceManager.mShaderEditor) {
			if (ImGui::Begin(k.c_str(), nullptr, ImGuiWindowFlags_None)) {
				if (ImGui::Button("Save and reload")) {
					std::ofstream file(k.c_str(), std::ofstream::out | std::ofstream::binary);
					file.write(v.GetText().data(), glm::max<size_t>(v.GetText().size() - 1, 0)); // getText returns an extra endline at the end
					file.close();
					// File is closed and written

					// If shader is in memory, reload it
					auto it = mResourceManager.mShaders.find(k);
					if (it != mResourceManager.mShaders.end()) {
						if (std::shared_ptr<hyperengine::ShaderProgram> program = it->second.lock())
							mResourceManager.reloadShader(k, *program);
					}
				}

				v.Render(k.c_str());
			}
			ImGui::End();
		}

		static entt::entity selected = entt::null;

		if (ImGui::Begin("Hierarchy")) {
			for (auto&& [entity, gameObject] : mRegistry.view<GameObjectComponent>().each()) {
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
				if (selected == entity)
					flags |= ImGuiTreeNodeFlags_Selected;
		
				bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)entity, flags, "%s", gameObject.name.c_str());
				if (ImGui::IsItemActive())
					selected = entity;

				if (opened)
					ImGui::TreePop();
			}
		}
		ImGui::End();

		if (ImGui::Begin("Properties")) {
			if (selected != entt::null) {
				GameObjectComponent* gameObject = mRegistry.try_get<GameObjectComponent>(selected);
				if (gameObject) {
					ImGui::InputText("Name", &gameObject->name);
					if (ImGui::CollapsingHeader("Transform"))
						transformEditUi(gameObject->transform);
				}
			}
		}
		ImGui::End();

		if (mViewImGuiDemoWindow)
			ImGui::ShowDemoWindow(&mViewImGuiDemoWindow);
	}

	void update() {
		drawUi();

		glm::mat4 projection = glm::perspective(glm::radians(mFov), static_cast<float>(mFramebufferSize.x) / static_cast<float>(mFramebufferSize.y), mNear, mFar);

		glViewport(0, 0, mFramebufferSize.x, mFramebufferSize.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		
		for(auto&& [entity, gameObject] : mRegistry.view<GameObjectComponent>().each()) {
			if (!gameObject.texture) continue;
			if (!gameObject.shader) continue;
			if (!gameObject.mesh) continue;

			if (gameObject.spin) {
				gameObject.transform.orientation = glm::rotate(gameObject.transform.orientation, glm::radians(1.0f), glm::vec3(0, 1, 0));
			}

			gameObject.texture->bind(0);
			gameObject.shader->bind();
			gameObject.shader->uniformMat4f("uProjection", projection);
			gameObject.shader->uniformMat4f("uTransform", gameObject.transform.get());
			gameObject.shader->uniformMat4f("uView", glm::inverse(mCameraTransform.get()));
			gameObject.mesh->draw();
		}
	}

	hyperengine::Window mWindow;
	Transform mCameraTransform;

	entt::registry mRegistry;
	ResourceManager mResourceManager;

	bool mRunning = true;
	bool mViewImGuiDemoWindow = false;
	float mFov = 80.0f;
	float mNear = 0.1f;
	float mFar = 100.0f;
	glm::ivec2 mFramebufferSize;
};

int main(int argc, char* argv[]) {
	hyperengine::setupRenderDoc(true);
	Engine engine;
	engine.run();
    return 0;
}