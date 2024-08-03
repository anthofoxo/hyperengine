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
#include "he_util.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <misc/cpp/imgui_stdlib.h>
#include <ImGuizmo.h>

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

#include <TextEditor.h>

#include <lua.hpp>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

struct AudioEngine final {
	void init(std::string_view preferredDevice) {
		if (ma_context_init(nullptr, 0, nullptr, &mContext) != MA_SUCCESS) {
			// Error.
		}

		ma_device_info* pPlaybackInfos;
		ma_uint32 playbackCount;
		if (ma_context_get_devices(&mContext, &pPlaybackInfos, &playbackCount, nullptr, nullptr) != MA_SUCCESS) {
			// Error.
		}

		bool isDeviceChosen = false;
		ma_uint32 chosenPlaybackDeviceIndex = 0;

		for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
			if (std::string_view(pPlaybackInfos[iDevice].name) == preferredDevice) {
				chosenPlaybackDeviceIndex = iDevice;
				isDeviceChosen = true;
			}
		}

		if (!isDeviceChosen) {
			for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
				if (pPlaybackInfos[iDevice].isDefault) {
					chosenPlaybackDeviceIndex = iDevice;
					isDeviceChosen = true;
					break;
				}
			}
		}

		mDeviceName = pPlaybackInfos[chosenPlaybackDeviceIndex].name;

		ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
		deviceConfig.playback.pDeviceID = &pPlaybackInfos[chosenPlaybackDeviceIndex].id;
		deviceConfig.pUserData = &mEngine;

		deviceConfig.dataCallback = [](ma_device* pDevice, void* pOutput, void const* pInput, ma_uint32 frameCount) {
			ma_engine_read_pcm_frames(static_cast<ma_engine*>(pDevice->pUserData), pOutput, frameCount, nullptr);
		};

		if (ma_device_init(&mContext, &deviceConfig, &mDevice) != MA_SUCCESS) {
			// Error.
		}

		ma_engine_config engineConfig = ma_engine_config_init();
		engineConfig.pDevice = &mDevice;

		if (ma_engine_init(&engineConfig, &mEngine) != MA_SUCCESS) {
			// Error.
		}
	}

	void uninit() {
		ma_engine_uninit(&mEngine);
		ma_device_uninit(&mDevice);
		ma_context_uninit(&mContext);
	}

	std::string mDeviceName;
	ma_context mContext;
	ma_device mDevice;
	ma_engine mEngine;
};

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
	//hyperengine::breakpointIfDebugging();
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
	ImGui::GetIO().ConfigDebugIsDebuggerPresent = hyperengine::isDebuggerPresent();
	ImGuizmo::BeginFrame();
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

struct TextEditorInfo {
	TextEditor editor;
	bool enabled = false;
};

struct ResourceManager final {
	hyperengine::UnorderedStringMap<std::weak_ptr<hyperengine::Mesh>> mMeshes;
	hyperengine::UnorderedStringMap<std::weak_ptr<hyperengine::Texture>> mTextures;
	hyperengine::UnorderedStringMap<std::weak_ptr<hyperengine::ShaderProgram>> mShaders;

	hyperengine::UnorderedStringMap<TextEditorInfo> mShaderEditor;

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
		program = {{ .source = shader.value(), .origin = pathStr }};

		// Setup debug editor
		std::unordered_map<int, std::set<std::string>> parsed;
		std::regex match("[0-9]*\\(([0-9]+)\\)");

		// On nvidia drivers, the program info log will include errors from all stages
		if (!program.errors().empty()) {
			for (auto const& error : program.errors()) {
				std::vector<std::string> lines;
				hyperengine::split(error, lines, '\n');

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
		mShaderEditor[pathStr] = { editor, !parsed.empty() };
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
	std::string name;
	Transform transform;

	GameObjectComponent() {
		std::stringstream ss;
		ss << "unnamed 0x" << std::hex << (void*)this;
		name = ss.str();
	}
};

struct MeshFilterComponent {
	std::shared_ptr<hyperengine::Mesh> mesh;
};

struct MeshRendererComponent {
	std::shared_ptr<hyperengine::Texture> texture;
	std::shared_ptr<hyperengine::ShaderProgram> shader;
};

struct CameraComponent {
	glm::vec2 clippingPlanes = { 0.1f, 100.0f };
	float fov = 80.0f;
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

		std::cout << glGetString(GL_RENDERER) << '\n';
		std::cout << glGetString(GL_VENDOR) << '\n';
		std::cout << glGetString(GL_VERSION) << '\n';
		std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';

		glfwSetWindowUserPointer(mWindow.handle(), this);
		glfwSetWindowCloseCallback(mWindow.handle(), [](GLFWwindow* window) {
			static_cast<Engine*>(glfwGetWindowUserPointer(window))->mRunning = false;
		});

		initImGui(mWindow);
		mAudioEngine.init("");

		{
			entt::handle entity{ mRegistry, mRegistry.create() };
			GameObjectComponent& gameObject = entity.emplace<GameObjectComponent>();
			gameObject.transform.translation = glm::vec3(0, 1, 3);
			gameObject.name = "MainCamera";
			CameraComponent& camera = entity.emplace<CameraComponent>();
		}

		lua_State* L = luaL_newstate();
		luaL_dofile(L, "scene.lua"); // Run file, resulting table is pushed on stack
		int t = lua_gettop(L); // Table index
		lua_pushnil(L); // First key
		while (lua_next(L, t) != 0) {
			/* 'key' (at index -2) and 'value' (at index -1) */
			entt::entity entity = mRegistry.create();
			GameObjectComponent& gameObject = mRegistry.emplace<GameObjectComponent>(entity);
			MeshFilterComponent& meshFilter = mRegistry.emplace<MeshFilterComponent>(entity);
			MeshRendererComponent& meshRenderer = mRegistry.emplace<MeshRendererComponent>(entity);

			lua_getfield(L, -1, "name");
			gameObject.name = lua_tostring(L, -1);
			lua_pop(L, 1);

			lua_getfield(L, -1, "mesh");
			meshFilter.mesh = mResourceManager.getMesh(lua_tostring(L, -1));
			lua_pop(L, 1);

			lua_getfield(L, -1, "texture");
			meshRenderer.texture = mResourceManager.getTexture(lua_tostring(L, -1));
			lua_pop(L, 1);

			lua_getfield(L, -1, "shader");
			meshRenderer.shader = mResourceManager.getShaderProgram(lua_tostring(L, -1));
			lua_pop(L, 1);

			lua_getfield(L, -1, "translation");
			gameObject.transform.translation = hyperengine::luaToVec3(L);
			lua_pop(L, 1);

			lua_getfield(L, -1, "scale");
			gameObject.transform.scale = hyperengine::luaToVec3(L);
			lua_pop(L, 1);

			lua_pop(L, 1); // remove value
		}

		lua_pop(L, 1); // Pop table

		lua_close(L);

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
		mAudioEngine.uninit();
	}

	void drawGuiProperties() {
		if (!mViewProperties) return;

		if (ImGui::Begin("Properties", &mViewProperties)) {
			if (selected != entt::null) {
				if (GameObjectComponent* component = mRegistry.try_get<GameObjectComponent>(selected)) {
					ImGui::InputText("Name", &component->name);
					if (ImGui::CollapsingHeader("Transform")) {
						ImGui::DragFloat3("Translation", glm::value_ptr(component->transform.translation), 0.1f);
						glm::vec3 oldEuler = glm::degrees(glm::eulerAngles(component->transform.orientation));
						glm::vec3 newEuler = oldEuler;
						if (ImGui::DragFloat3("Rotation", glm::value_ptr(newEuler))) {
							glm::vec3 deltaEuler = glm::radians(newEuler - oldEuler);
							component->transform.orientation = glm::rotate(component->transform.orientation, deltaEuler.x, glm::vec3(1, 0, 0));
							component->transform.orientation = glm::rotate(component->transform.orientation, deltaEuler.y, glm::vec3(0, 1, 0));
							component->transform.orientation = glm::rotate(component->transform.orientation, deltaEuler.z, glm::vec3(0, 0, 1));
						}
						ImGui::DragFloat3("Scale", glm::value_ptr(component->transform.scale), 0.1f);
					}
				}
				else if (ImGui::Button("Add Game Object")) {
					mRegistry.emplace<GameObjectComponent>(selected);
				}

				if (MeshFilterComponent* component = mRegistry.try_get<MeshFilterComponent>(selected)) {
					if (ImGui::CollapsingHeader("Mesh Filter")) {
						if (component->mesh)
							ImGui::LabelText("Resource", "%s", component->mesh->origin().c_str());
						else
							ImGui::LabelText("Resource", "%s", "<null>");

						if (ImGui::Button("Select..."))
							ImGui::OpenPopup("meshFilterSelect");

						if (ImGui::BeginPopup("meshFilterSelect")) {
							static std::string path;
							ImGui::InputText("Resource", &path);
							if (ImGui::Button("Apply")) {
								component->mesh = mResourceManager.getMesh(path);
								if (component->mesh) ImGui::CloseCurrentPopup();
							}

							ImGui::EndPopup();
						}
					}
				}
				else if (ImGui::Button("Add Mesh Filter")) {
					mRegistry.emplace<MeshFilterComponent>(selected);
				}

				if (MeshRendererComponent* component = mRegistry.try_get<MeshRendererComponent>(selected)) {
					if (ImGui::CollapsingHeader("Mesh Renderer")) {
						if (component->shader)
							ImGui::LabelText("Resource", "%s", component->shader->origin().c_str());
						else
							ImGui::LabelText("Resource", "%s", "<null>");

						if (ImGui::Button("Select shader..."))
							ImGui::OpenPopup("meshRendererShaderSelect");

						if (ImGui::BeginPopup("meshRendererShaderSelect")) {
							static std::string path;
							ImGui::InputText("Resource", &path);
							if (ImGui::Button("Apply")) {
								component->shader = mResourceManager.getShaderProgram(path);
								if (component->shader) ImGui::CloseCurrentPopup();
							}

							ImGui::EndPopup();
						}

						if (component->texture)
							ImGui::LabelText("Resource", "%s", component->texture->origin().c_str());
						else
							ImGui::LabelText("Resource", "%s", "<null>");

						if (ImGui::Button("Select texture..."))
							ImGui::OpenPopup("meshRendererTextureSelect");

						if (ImGui::BeginPopup("meshRendererTextureSelect")) {
							static std::string path;
							ImGui::InputText("Resource", &path);
							if (ImGui::Button("Apply")) {
								component->texture = mResourceManager.getTexture(path);
								if (component->texture) ImGui::CloseCurrentPopup();
							}

							ImGui::EndPopup();
						}
					}
				}
				else if (ImGui::Button("Add Mesh Renderer")) {
					mRegistry.emplace<MeshRendererComponent>(selected);
				}

				if (CameraComponent* component = mRegistry.try_get<CameraComponent>(selected)) {
					if (ImGui::CollapsingHeader("Camera")) {
						ImGui::DragFloat("Fov", &component->fov, 1.0f, 10.0f, 170.0f);
						ImGui::DragFloatRange2("Clipping planes", &component->clippingPlanes.x, &component->clippingPlanes.y, 0.1f, 0.001f, 1000.0f);
					}
				}
				else if (ImGui::Button("Add Camera")) {
					mRegistry.emplace<CameraComponent>(selected);
				}
			}
		}
		ImGui::End();
		
	}

	void drawGuiHierarchy() {
		if (!mViewHierarchy) return;

		if (ImGui::Begin("Hierarchy", &mViewHierarchy)) {
			if (ImGui::Button("Create empty")) {
				entt::entity entity = mRegistry.create();
				mRegistry.emplace<GameObjectComponent>(entity);
			}

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
	}

	void drawUi() {
		ImGui::DockSpaceOverViewport();

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Quit", "Alt+F4", nullptr))
					mRunning = false;

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View")) {
				ImGui::MenuItem("Hierarchy", nullptr, &mViewHierarchy);
				ImGui::MenuItem("Properties", nullptr, &mViewProperties);
				ImGui::MenuItem("Viewport", nullptr, &mViewViewport);
				ImGui::MenuItem("Resource Manager", nullptr, &mViewResourceManager);
				ImGui::Separator();
				ImGui::MenuItem("Dear ImGui Demo", nullptr, &mViewImGuiDemoWindow);

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		if (ImGui::Begin("Debug")) {
			ImGui::Checkbox("Wireframe", &mWireframe);
			ImGui::ColorEdit3("Sky color", glm::value_ptr(mSkyColor));

			ImGui::SeparatorText("Gizmos");

			if (ImGui::RadioButton("Translate", mOperation == ImGuizmo::TRANSLATE))
				mOperation = ImGuizmo::TRANSLATE;
			ImGui::SameLine();
			if (ImGui::RadioButton("Rotate", mOperation == ImGuizmo::ROTATE))
				mOperation = ImGuizmo::ROTATE;
			ImGui::SameLine();
			if (ImGui::RadioButton("Scale", mOperation == ImGuizmo::SCALE))
				mOperation = ImGuizmo::SCALE;

			if (ImGui::RadioButton("Local", mMode == ImGuizmo::LOCAL))
				mMode = ImGuizmo::LOCAL;
			ImGui::SameLine();
			if (ImGui::RadioButton("World", mMode == ImGuizmo::WORLD))
				mMode = ImGuizmo::WORLD;
		}
		ImGui::End();

		for (auto& [k, v] : mResourceManager.mShaderEditor) {
			if (v.enabled) {
				if (ImGui::Begin(k.c_str(), &v.enabled, ImGuiWindowFlags_None)) {
					if (ImGui::Button("Save and reload")) {
						std::ofstream file(k.c_str(), std::ofstream::out | std::ofstream::binary);
						file.write(v.editor.GetText().data(), glm::max<size_t>(v.editor.GetText().size() - 1, 0)); // getText returns an extra endline at the end
						file.close();
						// File is closed and written

						// If shader is in memory, reload it
						auto it = mResourceManager.mShaders.find(k);
						if (it != mResourceManager.mShaders.end()) {
							if (std::shared_ptr<hyperengine::ShaderProgram> program = it->second.lock())
								mResourceManager.reloadShader(k, *program);
						}
					}

					v.editor.Render(k.c_str());
				}
				ImGui::End();
			}
		}

		if (mViewResourceManager) {
			if (ImGui::Begin("Resource Manager"), &mViewResourceManager) {
				if (ImGui::CollapsingHeader("Meshes")) {
					for (auto& [k, v] : mResourceManager.mMeshes) {
						if (ImGui::TreeNodeEx(k.c_str())) {
							ImGui::LabelText("Strong refs", "%d", v.use_count());
							ImGui::TreePop();
						}
					}
				}
				if (ImGui::CollapsingHeader("Textures")) {
					for (auto& [k, v] : mResourceManager.mTextures) {
						if (ImGui::TreeNodeEx(k.c_str())) {
							ImGui::LabelText("Strong refs", "%d", v.use_count());
							if (auto strongRef = v.lock())
								ImGui::Image((void*)(uintptr_t)strongRef->handle(), { 128, 128 }, { 0, 1 }, { 1, 0 });;
							ImGui::TreePop();
						}
					}
				}
				if (ImGui::CollapsingHeader("Shaders")) {
					for (auto& [k, v] : mResourceManager.mShaders) {
						if (ImGui::TreeNodeEx(k.c_str())) {
							ImGui::LabelText("Strong refs", "%d", v.use_count());
							ImGui::Checkbox("Editor enabled", &mResourceManager.mShaderEditor[k].enabled);

							ImGui::TreePop();
						}
					}
				}
			}
			ImGui::End();
		}

		

		drawGuiHierarchy();
		drawGuiProperties();

		if (mViewImGuiDemoWindow)
			ImGui::ShowDemoWindow(&mViewImGuiDemoWindow);
	}

	void update() {
		drawUi();

		if (mViewViewport) {
			if (ImGui::Begin("Viewport", &mViewViewport)) {
				static_assert(sizeof(ImVec2) == sizeof(glm::vec2));

				glm::ivec2 contentAvail = std::bit_cast<glm::vec2>(ImGui::GetContentRegionAvail());

				if (contentAvail.x > 0 && contentAvail.y > 0) {

					// Different sizes, gotta resize the viewport buffer
					if (contentAvail != mViewportSize) {
						mViewportSize = contentAvail;

						mFramebufferColor = { {
							.width = mViewportSize.x,
							.height = mViewportSize.y,
							.format = hyperengine::PixelFormat::kRgba8,
							.minFilter = GL_LINEAR,
							.magFilter = GL_LINEAR,
							.wrap = GL_CLAMP_TO_EDGE,
							.label = "framebuffer color"
						} };

						if (mFramebufferDepth)
							glDeleteRenderbuffers(1, &mFramebufferDepth);
						if (mFramebuffer)
							glDeleteFramebuffers(1, &mFramebuffer);

						glGenRenderbuffers(1, &mFramebufferDepth);
						glBindRenderbuffer(GL_RENDERBUFFER, mFramebufferDepth);
						glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mViewportSize.x, mViewportSize.y);

						glGenFramebuffers(1, &mFramebuffer);
						glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFramebufferColor.handle(), 0);
						glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mFramebufferDepth);
					}

					glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);

					// RENDER THE SCENE
					bool cameraFound = false;
					glm::mat4 cameraTransform;
					glm::mat4 cameraProjection;
					float farPlane;
					{
						// Find camera, set vals
						for (auto&& [entity, gameObject, camera] : mRegistry.view<GameObjectComponent, CameraComponent>().each()) {
							cameraTransform = gameObject.transform.get();
							cameraProjection = glm::perspective(glm::radians(camera.fov), static_cast<float>(mViewportSize.x) / static_cast<float>(mViewportSize.y), camera.clippingPlanes.x, camera.clippingPlanes.y);
							farPlane = camera.clippingPlanes.y;
							cameraFound = true;
							break;
						}

						if (cameraFound) {
							glViewport(0, 0, mViewportSize.x, mViewportSize.y);
							glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
							glClearColor(mSkyColor.r, mSkyColor.g, mSkyColor.b, 1.0f);

							if (mWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

							for (auto&& [entity, gameObject, meshFilter, meshRenderer] : mRegistry.view<GameObjectComponent, MeshFilterComponent, MeshRendererComponent>().each()) {
								if (!meshRenderer.texture) continue;
								if (!meshRenderer.shader) continue;
								if (!meshFilter.mesh) continue;

								//if (gameObject.spin) {
								//	gameObject.transform.orientation = glm::rotate(gameObject.transform.orientation, glm::radians(1.0f), glm::vec3(0, 1, 0));
								//}

								if (!meshRenderer.shader->cull()) glDisable(GL_CULL_FACE);
								meshRenderer.texture->bind(0);
								meshRenderer.shader->bind();
								meshRenderer.shader->uniformMat4f("uProjection", cameraProjection);
								meshRenderer.shader->uniformMat4f("uTransform", gameObject.transform.get());
								meshRenderer.shader->uniformMat4f("uView", glm::inverse(cameraTransform));
								meshRenderer.shader->uniform1f("uFarPlane", farPlane);
								meshRenderer.shader->uniform3f("uSkyColor", mSkyColor);
								meshFilter.mesh->draw();
								if (!meshRenderer.shader->cull()) glEnable(GL_CULL_FACE);
							}

							if (mWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
						}
					}

					glBindFramebuffer(GL_FRAMEBUFFER, 0);

					if (cameraFound) {
						ImGui::Image((void*)(uintptr_t)mFramebufferColor.handle(), ImGui::GetContentRegionAvail(), { 0, 1 }, { 1, 0 });

						if (selected != entt::null) {
							GameObjectComponent& gameObject = mRegistry.get<GameObjectComponent>(selected);
							glm::mat4 matrix = gameObject.transform.get();

							ImGuizmo::SetDrawlist();
							ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

							if (ImGuizmo::Manipulate(glm::value_ptr(glm::inverse(cameraTransform)), glm::value_ptr(cameraProjection), mOperation, mMode, glm::value_ptr(matrix))) {
								gameObject.transform.set(matrix);
							}
						}
					}
					else
						ImGui::TextUnformatted("No camera found");
				}
			}
			ImGui::End();
		}

		glViewport(0, 0, mFramebufferSize.x, mFramebufferSize.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	hyperengine::Window mWindow;

	AudioEngine mAudioEngine;
	entt::registry mRegistry;
	ResourceManager mResourceManager;
	entt::entity selected = entt::null;

	GLuint mFramebuffer = 0;
	GLuint mFramebufferDepth = 0;
	hyperengine::Texture mFramebufferColor;
	glm::ivec2 mViewportSize{};

	glm::vec3 mSkyColor = { 0.7f, 0.8f, 0.9f };
	bool mWireframe = false;

	bool mRunning = true;
	bool mViewHierarchy = true;
	bool mViewProperties = true;
	bool mViewViewport = true;
	bool mViewResourceManager = false;
	bool mViewImGuiDemoWindow = false;
	glm::ivec2 mFramebufferSize;
	ImGuizmo::OPERATION mOperation = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE mMode = ImGuizmo::MODE::LOCAL;
};

int main(int argc, char* argv[]) {
	hyperengine::setupRenderDoc(true);
	Engine engine;
	engine.run();
    return 0;
}