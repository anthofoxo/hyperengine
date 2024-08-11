#ifdef _WIN32
#	include <Windows.h>
#	include <shlobj_core.h>
#	undef near
#	undef far
#endif

#ifdef _MSC_VER
#	define HE_ALLOCATOR(size) _Ret_notnull_ _Post_writable_byte_size_(size) __declspec(allocator)
#else
#	define HE_ALLOCATOR(size)
#endif

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <string>
#include <optional>
#include <set>
#include <vector>
#include <format>
#include <array>
#include <span>
#include <memory>
#include <sstream>
#include <regex>

#include <debug_trap.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "he_platform.hpp"
#include "he_io.hpp"
#include "he_util.hpp"
#include "he_audio.hpp"

#include "graphics/he_framebuffer.hpp"
#include "graphics/he_gl.hpp"
#include "graphics/he_window.hpp"
#include "graphics/he_rdoc.hpp"
#include "graphics/he_mesh.hpp"
#include "graphics/he_texture.hpp"
#include "graphics/he_shader.hpp"
#include "graphics/he_renderbuffer.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <misc/cpp/imgui_stdlib.h>
#include <ImGuizmo.h>

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

// TODO: We should look into otherr text editing options out there
// stb_textedit Seems like a good base for the text editor
#include <TextEditor.h>

#include <lua.hpp>

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#include <spdlog/spdlog.h>

#include <tinyfiledialogs.h>

#include <filesystem>

[[nodiscard]] HE_ALLOCATOR(count) void* operator new(std::size_t count)
{
	void* ptr = malloc(count);
	if (ptr == nullptr) throw std::bad_alloc();
	TracyAlloc(ptr, count);
	return ptr;
}

void operator delete(void* ptr) noexcept {
	TracyFree(ptr);
	free(ptr);
}

// TODO: Along with internal textures, we should implment internal models, such as planes, cubes and spheres
constexpr std::string_view kInternalTextureBlackName = "internal://black.png";
constexpr std::string_view kInternalTextureWhiteName = "internal://white.png";
constexpr std::string_view kInternalTextureUvName = "internal://uv.png";

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
	io.ConfigWindowsMoveFromTitleBarOnly = true;

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
	ZoneScoped;
	TracyGpuZone(TracyFunction);
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::GetIO().ConfigDebugIsDebuggerPresent = hyperengine::isDebuggerPresent();
	ImGuizmo::BeginFrame();
}

static void imguiEndFrame() {
	ZoneScoped;
	TracyGpuZone(TracyFunction);
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
		"sampler2D", "in", "out", "const", "uniform", "layout", "std140",
		"if", "else", "return"
	};
	for (auto& k : keywords)
		langDef.mKeywords.insert(k);

	char const* const hyperengineMacros[] = {
		"INPUT", "OUTPUT", "VARYING", "VERT", "FRAG"
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
		bool opened = mShaderEditor[pathStr].enabled;
		mShaderEditor[pathStr] = { editor, opened || !parsed.empty() };
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
	hyperengine::Uuid uuid = hyperengine::Uuid::generate();
	Transform transform;

	GameObjectComponent() {
		std::stringstream ss;
		ss << "unnamed " << std::hex << (uint64_t)uuid;
		name = ss.str();
	}
};

struct MeshFilterComponent {
	std::shared_ptr<hyperengine::Mesh> mesh;
};

// TODO: It's possible to get heap corruption when hotswapping shaders with different material sizes,
// need to look into fixing this
struct MeshRendererComponent {
	std::shared_ptr<hyperengine::ShaderProgram> shader;
	std::vector<uint8_t> data;
	GLuint uniformBuffer = 0;
	std::array<std::shared_ptr<hyperengine::Texture>, 8> textures;

	void resetMaterial() {
		for (int i = 0; i < textures.size(); ++i)
			textures[i] = nullptr;

		if (uniformBuffer)
			glDeleteBuffers(1, &uniformBuffer);

		// realloc buffer
		data = std::vector<uint8_t>();

		// Attempt to allocate material data
		allocateMaterialBuffer();
	}

	void allocateMaterialBuffer() {
		if (!shader) return;
		auto allocation = shader->materialAllocationSize();
		data.resize(allocation);
		memset(data.data(), 0, allocation);

		// TODO: Implment buffer abstraction, we actually leak memory a bit rn
		glGenBuffers(1, &uniformBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
		glBufferData(GL_UNIFORM_BUFFER, allocation, nullptr, GL_STREAM_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void uploadAndPrep() {
		glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, data.size(), data.data());
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, uniformBuffer);
	}
};

struct CameraComponent {
	glm::vec2 clippingPlanes = { 0.1f, 100.0f };
	float fov = 80.0f;
};

struct LightComponent final {
	glm::vec3 color = glm::vec3(1.0f);
	float strength = 3.0f;
};

#define CAT_(a, b) a ## b
#define CAT(a, b) CAT_(a, b)
#define VARNAME(Var) CAT(Var, __LINE__)
#define UNNAMABLE VARNAME(_reserved)

struct UniformEngineData {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 lightmat;
	glm::vec3 skyColor;
	float farPlane;
	glm::vec3 sunDirection;
	float UNNAMABLE;
	glm::vec3 sunColor;
	float UNNAMABLE;
};

// Assert layout matches glsl std140
static_assert(sizeof(UniformEngineData) == 240);
static_assert(offsetof(UniformEngineData, projection)   ==   0);
static_assert(offsetof(UniformEngineData, view)         ==  64);
static_assert(offsetof(UniformEngineData, lightmat)     == 128);
static_assert(offsetof(UniformEngineData, skyColor)     == 192);
static_assert(offsetof(UniformEngineData, farPlane)     == 204);
static_assert(offsetof(UniformEngineData, sunDirection) == 208);
static_assert(offsetof(UniformEngineData, sunColor)     == 224);

struct Views final {
	bool hierarchy = true;
	bool properties = true;
	bool viewport = true;
	bool resourceManager = false;
	bool imguiDemoWindow = false;
	bool experimentAudio = false;

	void drawUi() {
		ImGui::MenuItem("Hierarchy", nullptr, &hierarchy);
		ImGui::MenuItem("Properties", nullptr, &properties);
		ImGui::MenuItem("Viewport", nullptr, &viewport);
		ImGui::MenuItem("Resource Manager", nullptr, &resourceManager);
		ImGui::Separator();
		ImGui::MenuItem("Dear ImGui Demo", nullptr, &imguiDemoWindow);
		ImGui::SeparatorText("Experiments");
		ImGui::MenuItem("Audio", nullptr, &experimentAudio);
	}
};

template <class T, class UserPtr>
bool drawComponentEditGui(entt::registry& registry, entt::entity entity, char const* name, UserPtr* ptr, void(*func)(T&, UserPtr*)) {
	if (entity == entt::null) return false;
	T* component = registry.try_get<T>(entity);
	if (!component) return false;

	if (ImGui::CollapsingHeader(name))
		func(*component, ptr);

	return true;
}

static std::array<glm::vec3, 8> getSystemSpaceNdcExtremes(glm::mat4 const& matrix) {
	auto const inv = glm::inverse(matrix);
	std::array<glm::vec3, 8> corners;
	
	for (unsigned int x = 0; x < 2; ++x)
		for (unsigned int y = 0; y < 2; ++y)
			for (unsigned int z = 0; z < 2; ++z) {
				glm::vec4 const pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
				corners[z * 4 + y * 2 + x] = glm::vec3(pt) / pt.w;
			}

	return corners;
}

struct Engine final {
	void createInternalTextures() {
		using enum hyperengine::Texture::WrapMode;
		using enum hyperengine::Texture::FilterMode;
		{
			unsigned char pixels[] = { 0, 0, 0, 255 };
			hyperengine::Texture tex = { {.width = 1, .height = 1, .format = hyperengine::PixelFormat::kRgba8, .minFilter = kNearest, .magFilter = kNearest, .wrap = kClampEdge, .label = kInternalTextureBlackName, .origin = kInternalTextureBlackName } };
			tex.upload({ .xoffset = 0, .yoffset = 0, .width = 1, .height = 1, .format = hyperengine::PixelFormat::kRgba8, .pixels = pixels });
			mInternalTextureBlack = std::make_shared<hyperengine::Texture>(std::move(tex));
			mResourceManager.mTextures[std::string(kInternalTextureBlackName)] = mInternalTextureBlack;
		}
		{
			unsigned char pixels[] = { 255, 255, 255, 255 };
			hyperengine::Texture tex = { {.width = 1, .height = 1, .format = hyperengine::PixelFormat::kRgba8, .minFilter = kNearest, .magFilter = kNearest, .wrap = kClampEdge, .label = kInternalTextureWhiteName, .origin = kInternalTextureWhiteName } };
			tex.upload({ .xoffset = 0, .yoffset = 0, .width = 1, .height = 1, .format = hyperengine::PixelFormat::kRgba8, .pixels = pixels });
			mInternalTextureWhite = std::make_shared<hyperengine::Texture>(std::move(tex));
			mResourceManager.mTextures[std::string(kInternalTextureWhiteName)] = mInternalTextureWhite;
		}
		{
			std::unique_ptr<uint8_t[]> pixels = std::make_unique<uint8_t[]>(256 * 256 * 4); // Large object, heap alloc this

			for (int y = 0; y < 256; ++y)
				for (int x = 0; x < 256; ++x) {
					pixels.get()[x * 4 + y * 256 * 4 + 0] = x;
					pixels.get()[x * 4 + y * 256 * 4 + 1] = y;
					pixels.get()[x * 4 + y * 256 * 4 + 2] = 0;
					pixels.get()[x * 4 + y * 256 * 4 + 3] = 255;
				}

			hyperengine::Texture tex = { {.width = 256, .height = 256, .format = hyperengine::PixelFormat::kRgba8, .minFilter = kLinear, .magFilter = kLinear, .wrap = kRepeat, .label = kInternalTextureUvName, .origin = kInternalTextureUvName } };
			tex.upload({ .xoffset = 0, .yoffset = 0, .width = 256, .height = 256, .format = hyperengine::PixelFormat::kRgba8, .pixels = pixels.get()});
			mInternalTextureUv = std::make_shared<hyperengine::Texture>(std::move(tex));
			mResourceManager.mTextures[std::string(kInternalTextureUvName)] = mInternalTextureUv;
		}

		mAcesProgram = mResourceManager.getShaderProgram("shaders/aces.glsl");
		mShadowProgram = mResourceManager.getShaderProgram("shaders/shadow.glsl");
	}

	void init() {
		mWindow = {{ .width = 1280, .height = 720, .title = "HyperEngine", .maximized = true }};

		glfwMakeContextCurrent(mWindow.handle());
		gladLoadGL(&glfwGetProcAddress);

		TracyGpuContext;

		if (GLAD_GL_KHR_debug) {
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(&hyperengine::glMessageCallback, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		}

		glfwSetWindowUserPointer(mWindow.handle(), this);
		glfwSetWindowCloseCallback(mWindow.handle(), [](GLFWwindow* window) {
			static_cast<Engine*>(glfwGetWindowUserPointer(window))->mRunning = false;
		});

		glfwSetKeyCallback(mWindow.handle(), [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			if (action != GLFW_PRESS) return;

			if (key == GLFW_KEY_O && mods == GLFW_MOD_CONTROL)
				static_cast<Engine*>(glfwGetWindowUserPointer(window))->openSceneDialogOption();

			if (key == GLFW_KEY_N && mods == GLFW_MOD_CONTROL)
				static_cast<Engine*>(glfwGetWindowUserPointer(window))->fileNewScene();

			if (key == GLFW_KEY_N && mods == (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT))
				static_cast<Engine*>(glfwGetWindowUserPointer(window))->fileEntityCreateEmpty();
		});

		initImGui(mWindow);
		mAudioEngine.init("");

		createInternalTextures();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDisable(GL_MULTISAMPLE);
		glCullFace(GL_BACK);

		// TODO: Implment buffer abstraction, we actually leak memory a bit rn
		glGenBuffers(1, &mEngineUniformBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, mEngineUniformBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(decltype(mUniformEngineData)), nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, mEngineUniformBuffer);
		genShadowmap();
	}

	void genShadowmap() {
		using enum hyperengine::Texture::WrapMode;
		using enum hyperengine::Texture::FilterMode;

		mFramebufferShadowDepth = {{
				.width = mShadowMapSize,
				.height = mShadowMapSize,
				.format = hyperengine::PixelFormat::kD24,
				.minFilter = kNearest,
				.magFilter = kNearest,
				.wrap = kClampBorder,
				.label = "shadow depth"
			}};

		std::array<hyperengine::Framebuffer::Attachment, 1> attachments{
			hyperengine::Framebuffer::Attachment(GL_DEPTH_ATTACHMENT, std::ref(mFramebufferShadowDepth)),
		};

		mFramebufferShadow = {{ .attachments = attachments }};
	}

	void run() {
		init();

		glGenVertexArrays(1, &mEmptyVao);

		while (mRunning) {
			glfwPollEvents();
			glfwGetFramebufferSize(mWindow.handle(), &mFramebufferSize.x, &mFramebufferSize.y);

			if (mFramebufferSize.x > 0 && mFramebufferSize.y > 0) {
				hyperengine::Framebuffer().bind();
				imguiBeginFrame();
				update();
				hyperengine::Framebuffer().bind();
				imguiEndFrame();
			}

			mWindow.swapBuffers();

			TracyGpuCollect;
			FrameMark;
		}

		glDeleteVertexArrays(1, &mEmptyVao);

		destroyImGui();
		mAudioEngine.uninit();

		TracyGpuCollect;
		FrameMark;
	}

	void audioExperiment() {
		if (!mViews.experimentAudio) return;

		if (ImGui::Begin("Audio Experiment", &mViews.experimentAudio)) {
			static std::string source;
			ImGui::InputText("Source", &source);
			if (ImGui::Button("Play")) {
				ma_engine_play_sound(&mAudioEngine.mEngine, source.c_str(), nullptr);
			}
		}
		ImGui::End();
	}

	void drawGuiProperties() {
		if (!mViews.properties) return;

		if (ImGui::Begin("Properties", &mViews.properties)) {
			if (mSelected != entt::null) {

				bool hasGameObject = drawComponentEditGui<GameObjectComponent, Engine>(mRegistry, mSelected, "Game Object", this, [](auto& comp, auto* ptr) {
					ImGui::InputText("Name", &comp.name);
					ImGui::LabelText("Uuid", "%p", comp.uuid);
					ImGui::DragFloat3("Translation", glm::value_ptr(comp.transform.translation), 0.1f);
					glm::vec3 oldEuler = glm::degrees(glm::eulerAngles(comp.transform.orientation));
					glm::vec3 newEuler = oldEuler;
					if (ImGui::DragFloat3("Rotation", glm::value_ptr(newEuler))) {
						glm::vec3 deltaEuler = glm::radians(newEuler - oldEuler);
						comp.transform.orientation = glm::rotate(comp.transform.orientation, deltaEuler.x, glm::vec3(1, 0, 0));
						comp.transform.orientation = glm::rotate(comp.transform.orientation, deltaEuler.y, glm::vec3(0, 1, 0));
						comp.transform.orientation = glm::rotate(comp.transform.orientation, deltaEuler.z, glm::vec3(0, 0, 1));
					}
					ImGui::DragFloat3("Scale", glm::value_ptr(comp.transform.scale), 0.1f);
				});

				bool hasLight = drawComponentEditGui<LightComponent, Engine>(mRegistry, mSelected, "Light", this, [](auto& comp, auto* ptr) {
					ImGui::ColorEdit3("Color", glm::value_ptr(comp.color));
					ImGui::DragFloat("Strength", &comp.strength, 0.1f);
				});

				bool hasMeshFilter = drawComponentEditGui<MeshFilterComponent, Engine>(mRegistry, mSelected, "Mesh Filter", this, [](auto& comp, auto* ptr) {
					if (comp.mesh)
						ImGui::LabelText("Resource", "%s", comp.mesh->origin().c_str());
					else
						ImGui::LabelText("Resource", "%s", "<null>");

					if (ImGui::Button("Select..."))
						ImGui::OpenPopup("meshFilterSelect");

					if (ImGui::BeginPopup("meshFilterSelect")) {
						static std::string path;
						ImGui::InputText("Resource", &path);
						if (ImGui::Button("Apply")) {
							comp.mesh = ptr->mResourceManager.getMesh(path);
							if (comp.mesh) ImGui::CloseCurrentPopup();
						}

						ImGui::EndPopup();
					}
				});

				bool hasMeshRenderer = drawComponentEditGui<MeshRendererComponent, Engine>(mRegistry, mSelected, "Mesh Renderer", this, [](auto& comp, auto* ptr) {
					if (comp.shader)
						ImGui::LabelText("Resource", "%s", comp.shader->origin().c_str());
					else
						ImGui::LabelText("Resource", "%s", "<null>");

					if (ImGui::Button("Select shader..."))
						ImGui::OpenPopup("meshRendererShaderSelect");

					if (ImGui::BeginPopup("meshRendererShaderSelect")) {
						static std::string path;
						ImGui::InputText("Resource", &path);
						if (ImGui::Button("Apply")) {
							comp.shader = ptr->mResourceManager.getShaderProgram(path);
							comp.resetMaterial();
							if (comp.shader) ImGui::CloseCurrentPopup();
						}

						ImGui::EndPopup();
					}

					if(comp.shader)
						for (auto const& [k, v] : comp.shader->materialInfo()) {

							using enum hyperengine::ShaderProgram::UniformType;

							void* ptr = comp.data.data() + v.offset;

							switch (v.type) {
								case kFloat:
									ImGui::DragFloat(k.c_str(), (float*)ptr);
									break;
								case kVec2f:
									ImGui::DragFloat2(k.c_str(), (float*)ptr);
									break;
								case kVec3f:
								{
									if (comp.shader->editHint(k) == "color")
										ImGui::ColorEdit3(k.c_str(), (float*)ptr);
									else
										ImGui::DragFloat3(k.c_str(), (float*)ptr);
									break;
								}
								case kVec4f:
									ImGui::DragFloat4(k.c_str(), (float*)ptr);
									break;
							default:
								ImGui::Text("Unsupported property %s", k.c_str());
							}
						}

					// Present texture options
					if (comp.shader) {
						for (auto const& [k, v] : comp.shader->opaqueAssignments()) {
							if (comp.shader->editHint(k) == "hidden") continue;

							ImGui::PushID(v);

							auto& texture = comp.textures[v];
							
							ImGui::LabelText(k.c_str(), "%s", texture ? texture->origin().c_str() : "<null>");

							bool pressed = false;

							if (texture)
								pressed = ImGui::ImageButton((void*)(uintptr_t)texture->handle(), { 64, 64 }, { 0, 1 }, { 1, 0 });
							else 
								pressed = ImGui::Button("Select");

							if(pressed)
								ImGui::OpenPopup("meshRendererTextureSelect");

							if (ImGui::BeginPopup("meshRendererTextureSelect")) {
								static std::string path;
								ImGui::InputText("Resource", &path);
								if (ImGui::Button("Apply")) {
									comp.textures[v] = ptr->mResourceManager.getTexture(path);
									if (comp.textures[v]) ImGui::CloseCurrentPopup();
								}
							
								ImGui::EndPopup();
							}

							ImGui::PopID();
						}
					}
				});

				bool hasCamera = drawComponentEditGui<CameraComponent, Engine>(mRegistry, mSelected, "Camera", this, [](auto& comp, auto* ptr) {
					ImGui::DragFloat("Fov", &comp.fov, 1.0f, 10.0f, 170.0f);
					ImGui::DragFloatRange2("Clipping planes", &comp.clippingPlanes.x, &comp.clippingPlanes.y, 0.1f, 0.001f, 1000.0f);
				});

				ImGui::SeparatorText("Add Component");

				if (!hasGameObject && ImGui::Button("Game Object")) mRegistry.emplace<GameObjectComponent>(mSelected);
				if (!hasMeshFilter && ImGui::Button("Mesh Filter")) mRegistry.emplace<MeshFilterComponent>(mSelected);
				if (!hasMeshRenderer && ImGui::Button("Mesh Renderer")) mRegistry.emplace<MeshRendererComponent>(mSelected);
				if (!hasLight && ImGui::Button("Light")) mRegistry.emplace<LightComponent>(mSelected);
				if (!hasCamera && ImGui::Button("Camera")) mRegistry.emplace<CameraComponent>(mSelected);
			}
		}
		ImGui::End();
		
	}

	void drawGuiHierarchy() {
		ZoneScoped;

		if (!mViews.hierarchy) return;

		if (ImGui::Begin("Hierarchy", &mViews.hierarchy)) {

			ImGui::BeginDisabled(mSelected == entt::null);
			if (ImGui::SmallButton("Delete Selected")) {
				mRegistry.destroy(mSelected);
				mSelected = entt::null;
			}
			ImGui::EndDisabled();

			for (auto&& [entity, gameObject] : mRegistry.view<GameObjectComponent>().each()) {
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
				if (mSelected == entity)
					flags |= ImGuiTreeNodeFlags_Selected;

				bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)entity, flags, "%s", gameObject.name.c_str());
				if (ImGui::IsItemActive())
					mSelected = entity;

				if (opened)
					ImGui::TreePop();
			}
		}
		ImGui::End();
	}

	void drawGuiResourceManager() {
		ZoneScoped;

		if (!mViews.resourceManager) return;

		if (ImGui::Begin("Resource Manager", &mViews.resourceManager)) {
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

	void loadScene(char const* path) {
		fileNewScene();


		{
			entt::handle entity{ mRegistry, mRegistry.create() };
			GameObjectComponent& gameObject = entity.emplace<GameObjectComponent>();
			gameObject.transform.translation = glm::vec3(0, 1, 3);
			gameObject.name = "MainCamera";
			CameraComponent& camera = entity.emplace<CameraComponent>();
		}

		lua_State* L = luaL_newstate();

		static const luaL_Reg loadedlibs[] = {
			//{LUA_GNAME, luaopen_base},
			//{LUA_LOADLIBNAME, luaopen_package},
			//{LUA_COLIBNAME, luaopen_coroutine},
			{LUA_TABLIBNAME, luaopen_table},
			//{LUA_IOLIBNAME, luaopen_io},
			//{LUA_OSLIBNAME, luaopen_os},
			//{LUA_STRLIBNAME, luaopen_string},
			//{LUA_MATHLIBNAME, luaopen_math},
			//{LUA_UTF8LIBNAME, luaopen_utf8},
			//{LUA_DBLIBNAME, luaopen_debug},
			{NULL, NULL}
		};


		const luaL_Reg* lib;
		/* "require" functions from 'loadedlibs' and set results to global table */
		for (lib = loadedlibs; lib->func; lib++) {
			luaL_requiref(L, lib->name, lib->func, 1);
			lua_pop(L, 1);  /* remove lib */
		}


		if (luaL_dofile(L, path) != LUA_OK) {
			spdlog::error("Lua error: {}", lua_tostring(L, -1));
		}
		else {
			int t = lua_gettop(L); // Table index
			lua_pushnil(L); // First key
			while (lua_next(L, t) != 0) {
				/* 'key' (at index -2) and 'value' (at index -1) */
				entt::entity entity = mRegistry.create();
				GameObjectComponent& gameObject = mRegistry.emplace<GameObjectComponent>(entity);
				MeshFilterComponent& meshFilter = mRegistry.emplace<MeshFilterComponent>(entity);
				MeshRendererComponent& meshRenderer = mRegistry.emplace<MeshRendererComponent>(entity);

				lua_getfield(L, -1, "name");
				if (lua_isstring(L, -1))
					gameObject.name = lua_tostring(L, -1);
				lua_pop(L, 1);

				lua_getfield(L, -1, "mesh");
				if (lua_isstring(L, -1))
					meshFilter.mesh = mResourceManager.getMesh(lua_tostring(L, -1));
				lua_pop(L, 1);

				lua_getfield(L, -1, "shader");
				if (lua_isstring(L, -1))
					meshRenderer.shader = mResourceManager.getShaderProgram(lua_tostring(L, -1));
				lua_pop(L, 1);

				// Apply stored material information
				if (meshRenderer.shader) {

					meshRenderer.allocateMaterialBuffer();

					lua_getfield(L, -1, "material");

					// Apply material settings if present
					if (lua_istable(L, -1)) {

						int materialTable = lua_gettop(L); // Table index
						lua_pushnil(L);

						while (lua_next(L, materialTable) != 0) {
							/* 'key' (at index -2) and 'value' (at index -1) */

							// Find material property in shader
							auto it = meshRenderer.shader->materialInfo().find(lua_tostring(L, -2));
							if (it != meshRenderer.shader->materialInfo().end()) {
								auto offset = it->second.offset;
								auto type = it->second.type;

								if (type == hyperengine::ShaderProgram::UniformType::kFloat && lua_isnumber(L, -1)) {
									*((float*)(meshRenderer.data.data() + offset)) = static_cast<float>(lua_tonumber(L, -1));
								}
								else if (type == hyperengine::ShaderProgram::UniformType::kVec2f && lua_istable(L, -1)) {
									*((glm::vec2*)(meshRenderer.data.data() + offset)) = hyperengine::luaToVec2(L);
								}
								else if (type == hyperengine::ShaderProgram::UniformType::kVec3f && lua_istable(L, -1)) {
									*((glm::vec3*)(meshRenderer.data.data() + offset)) = hyperengine::luaToVec3(L);
								}
								else if (type == hyperengine::ShaderProgram::UniformType::kVec4f && lua_istable(L, -1)) {
									*((glm::vec4*)(meshRenderer.data.data() + offset)) = hyperengine::luaToVec4(L);
								}
								else
									spdlog::warn("Unsupported uniform type");
							}

							lua_pop(L, 1);
						}
					}

					lua_pop(L, 1);
				}

				if (meshRenderer.shader) {
					lua_getfield(L, -1, "textures");

					// Apply textures
					if (lua_istable(L, -1)) {

						int textureTable = lua_gettop(L); // Table index
						lua_pushnil(L);

						while (lua_next(L, textureTable) != 0) {
							/* 'key' (at index -2) and 'value' (at index -1) */

							// Find texture property in shader
							auto it = meshRenderer.shader->opaqueAssignments().find(lua_tostring(L, -2));
							if (it != meshRenderer.shader->opaqueAssignments().end()) {
								meshRenderer.textures[it->second] = mResourceManager.getTexture(lua_tostring(L, -1));
							}

							lua_pop(L, 1);
						}

					}

					lua_pop(L, 1);
				}


				lua_getfield(L, -1, "translation");
				if (lua_istable(L, -1))
					gameObject.transform.translation = hyperengine::luaToVec3(L);
				lua_pop(L, 1);

				lua_getfield(L, -1, "scale");
				if (lua_istable(L, -1))
					gameObject.transform.scale = hyperengine::luaToVec3(L);
				lua_pop(L, 1);

				lua_pop(L, 1); // remove value
			}
		}

		lua_pop(L, 1); // Pop table

		lua_close(L);
	}

	void openSceneDialogOption() {
		char const* filter = "*.lua";
		char* source = tinyfd_openFileDialog("Open Scene", "./", 1, &filter, "Lua Files (*.lua)", false);

		if (source)
			loadScene(source);
	}

	void fileNewScene() {
		mRegistry.clear();
		mSelected = entt::null;
	}

	void fileEntityCreateEmpty() {
		mSelected = mRegistry.create();
		mRegistry.emplace<GameObjectComponent>(mSelected);
	}

	void drawUi() {
		ZoneScoped;

		ImGui::DockSpaceOverViewport();

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New Scene", "Ctrl+N", nullptr))
					fileNewScene();
				if (ImGui::MenuItem("Open Scene", "Ctrl+O", nullptr))
					openSceneDialogOption();

				ImGui::Separator();

				if (ImGui::MenuItem("Exit", "Alt+F4", nullptr))
					mRunning = false;

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View")) {
				mViews.drawUi();
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Entity")) {
				if (ImGui::MenuItem("Create Empty", "Ctrl+Shift+N", nullptr))
					fileEntityCreateEmpty();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug")) {
				if (ImGui::MenuItem("Capture Frame", nullptr, nullptr, hyperengine::isRenderDocRunning())) {
					hyperengine::rdoc::triggerCapture();
				}

				if (ImGui::MenuItem("Attach RenderDoc", nullptr, nullptr, hyperengine::isRenderDocRunning() && !hyperengine::rdoc::isTargetControlConnected())) {
					std::thread([](){
#ifdef _WIN32
						CHAR pf[MAX_PATH];
						SHGetSpecialFolderPathA(nullptr, pf, CSIDL_PROGRAM_FILES, false);
						std::string path = std::format("\"{}/RenderDoc/qrenderdoc.exe\" --targetcontrol", pf);
						system(path.c_str());
#else
						system("/bin/qrenderdoc --targetcontrol");
#endif
					}).detach();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		if (ImGui::Begin("Debug")) {
			ImGui::SeparatorText("GPU Info");

			auto const& info = hyperengine::glContextInfo();
			ImGui::LabelText("Renderer", "%s", info.renderer.c_str());
			ImGui::LabelText("Vendor", "%s", info.vendor.c_str());
			ImGui::LabelText("Version", "%s", info.version.c_str());
			ImGui::LabelText("GLSL Version", "%s", info.glslVersion.c_str());

			ImGui::SeparatorText("Rendering values");

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

			if (mOperation != ImGuizmo::SCALE) {
				if (ImGui::RadioButton("Local", mMode == ImGuizmo::LOCAL))
					mMode = ImGuizmo::LOCAL;
				ImGui::SameLine();
				if (ImGui::RadioButton("World", mMode == ImGuizmo::WORLD))
					mMode = ImGuizmo::WORLD;
			}
		}
		ImGui::End();

		for (auto& [k, v] : mResourceManager.mShaderEditor) {
			if (v.enabled) {
				if (ImGui::Begin(k.c_str(), &v.enabled, ImGuiWindowFlags_None)) {
					if (ImGui::Button("Save and reload")) {
						hyperengine::writeFile(k.c_str(), v.editor.GetText().data(), glm::max<size_t>(v.editor.GetText().size() - 1, 0));

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

		drawGuiResourceManager();
		drawGuiHierarchy();
		drawGuiProperties();
		audioExperiment();

		if (mViews.imguiDemoWindow)
			ImGui::ShowDemoWindow(&mViews.imguiDemoWindow);
	}

	void update() {
		ZoneScoped;
		TracyGpuZone(TracyFunction);

		drawUi();

		if (ImGui::Begin("Passes")) {
			ImGui::Image((void*)(uintptr_t)mFramebufferShadowDepth.handle(), { 256, 256 }, { 0, 1 }, { 1, 0 });
			ImGui::DragFloat("ShadowMap Offset", &mShadowMapOffset);
			ImGui::DragFloat("ShadowMap Distance", &mShadowMapDistance);

			if(ImGui::DragInt("ShadowMap Size", &mShadowMapSize, 1.0f, 32, 16384))
				genShadowmap();
		}
		ImGui::End();

		if (mViews.viewport) {
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
			if (ImGui::Begin("Viewport", &mViews.viewport)) {
				static_assert(sizeof(ImVec2) == sizeof(glm::vec2));

				glm::ivec2 contentAvail = std::bit_cast<glm::vec2>(ImGui::GetContentRegionAvail());

				if (contentAvail.x > 0 && contentAvail.y > 0) {

					// Different sizes, gotta resize the viewport buffer
					if (contentAvail != mViewportSize) {
						using enum hyperengine::Texture::WrapMode;
						using enum hyperengine::Texture::FilterMode;

						mViewportSize = contentAvail;

						mFramebufferColor = {{
							.width = mViewportSize.x,
							.height = mViewportSize.y,
							.format = hyperengine::PixelFormat::kRgba32f,
							.minFilter = kLinear,
							.magFilter = kLinear,
							.wrap = kClampEdge,
							.label = "framebuffer viewport color"
						}};

						mFramebufferDepth = {{
							.width = mViewportSize.x,
							.height = mViewportSize.y,
							.format = hyperengine::PixelFormat::kD24,
							.label =  "framebuffer viewport depth"
						}};

						std::array<hyperengine::Framebuffer::Attachment, 2> attachments{
							hyperengine::Framebuffer::Attachment(GL_COLOR_ATTACHMENT0, std::ref(mFramebufferColor)),
							hyperengine::Framebuffer::Attachment(GL_DEPTH_ATTACHMENT, std::ref(mFramebufferDepth)),
						};

						mFramebuffer = {{ .attachments = attachments }};

						mPostFramebufferColor = {{
							.width = mViewportSize.x,
							.height = mViewportSize.y,
							.format = hyperengine::PixelFormat::kRgba8,
							.minFilter = kLinear,
							.magFilter = kLinear,
							.wrap = kClampEdge,
							.label = "framebuffer post color"
						}};

						std::array<hyperengine::Framebuffer::Attachment, 1> attachmentsPost{
							hyperengine::Framebuffer::Attachment(GL_COLOR_ATTACHMENT0, std::ref(mPostFramebufferColor))
						};

						mPostFramebuffer = {{ .attachments = attachmentsPost }};
					}

					mFramebuffer.bind();

					glm::mat4 cameraProjection;
					Transform* cameraTransform =  nullptr;
					CameraComponent* cameraCamera = nullptr;

					// In absence of a sun, assert a white directional light pointing directly down
					glm::vec3 sunDirection = glm::normalize(glm::vec3(0.0f, -1.0f, 0.01f));
					glm::vec3 sunColor = glm::vec3(1.0f) * 3.0f;

					{
						// Find camera, set vals
						for (auto&& [entity, gameObject, camera] : mRegistry.view<GameObjectComponent, CameraComponent>().each()) {
							cameraTransform = &gameObject.transform;
							cameraCamera = &camera;
							cameraProjection = glm::perspective(glm::radians(camera.fov), static_cast<float>(mViewportSize.x) / static_cast<float>(mViewportSize.y), camera.clippingPlanes.x, camera.clippingPlanes.y);
							break;
						}

						// Find sun
						for (auto&& [entity, gameObject, light] : mRegistry.view<GameObjectComponent, LightComponent>().each()) {
							sunDirection = glm::normalize(glm::vec3(gameObject.transform.get() * glm::vec4(0, 0, -1, 0)));
							sunColor = light.color * light.strength;
							break;
						}

						if (cameraTransform && cameraCamera) {
							// Draw image
							ImGui::Image((void*)(uintptr_t)mPostFramebufferColor.handle(), ImGui::GetContentRegionAvail(), { 0, 1 }, { 1, 0 });

							// Draw transformation widget
							if (mSelected != entt::null) {
								GameObjectComponent& gameObject = mRegistry.get<GameObjectComponent>(mSelected);
								glm::mat4 matrix = gameObject.transform.get();

								ImGuizmo::SetDrawlist();
								ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
								ImGuizmo::Enable(ImGui::IsWindowFocused());

								if (ImGuizmo::Manipulate(glm::value_ptr(glm::inverse(cameraTransform->get())), glm::value_ptr(cameraProjection), mOperation, mMode, glm::value_ptr(matrix)))
									gameObject.transform.set(matrix);
							}

							// Move camera in scene
							if (ImGui::IsWindowFocused() && ImGui::IsWindowHovered()) {
								if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
									glm::vec3 direction{};

									if (ImGui::IsKeyDown(ImGuiKey_A)) --direction.x;
									if (ImGui::IsKeyDown(ImGuiKey_D)) ++direction.x;
									if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) --direction.y;
									if (ImGui::IsKeyDown(ImGuiKey_Space)) ++direction.y;
									if (ImGui::IsKeyDown(ImGuiKey_W)) --direction.z;
									if (ImGui::IsKeyDown(ImGuiKey_S)) ++direction.z;

									if (glm::length2(direction) > 0) {
										direction = glm::normalize(direction);

										glm::mat4 matrix = cameraTransform->get();
										matrix = glm::translate(matrix, direction * 10.0f * ImGui::GetIO().DeltaTime);
										cameraTransform->set(matrix);
									}

									ImVec2 drag = ImGui::GetIO().MouseDelta;
									glm::vec4 worldUp = glm::vec4(0, 1, 0, 0) * cameraTransform->get();
									cameraTransform->orientation = glm::rotate(cameraTransform->orientation, glm::radians(drag.x * -0.3f), glm::vec3(worldUp));
									cameraTransform->orientation = glm::rotate(cameraTransform->orientation, glm::radians(drag.y * -0.3f), { 1, 0, 0 });
								}
								else {
									if (ImGui::IsKeyPressed(ImGuiKey_W)) mOperation = ImGuizmo::TRANSLATE;
									else if (ImGui::IsKeyPressed(ImGuiKey_E)) mOperation = ImGuizmo::ROTATE;
									else if (ImGui::IsKeyPressed(ImGuiKey_R)) mOperation = ImGuizmo::SCALE;
								}
							}

							// Depth pass
							mFramebufferShadow.bind();
							glViewport(0, 0, mShadowMapSize, mShadowMapSize);
							glClear(GL_DEPTH_BUFFER_BIT);

							{

								auto lightLimied = glm::perspective(glm::radians(cameraCamera->fov), static_cast<float>(mViewportSize.x) / static_cast<float>(mViewportSize.y), cameraCamera->clippingPlanes.x, mShadowMapDistance);
								auto corners = getSystemSpaceNdcExtremes(lightLimied * glm::inverse(cameraTransform->get()));
								glm::vec3 center = glm::vec3(0, 0, 0);
								for (const auto& v : corners) {
									center += glm::vec3(v);
								}
								center /= static_cast<float>(corners.size());

								glm::mat4 lightView = glm::lookAt(-sunDirection + center, center, glm::vec3(0.0f, 1.0f, 0.0f));

								glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
								glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

								for (const auto& v : corners) {
									const auto trf = lightView * glm::vec4(v, 1.0f);
									min.x = glm::min(min.x, trf.x);
									max.x = glm::max(max.x, trf.x);
									min.y = glm::min(min.y, trf.y);
									max.y = glm::max(max.y, trf.y);
									min.z = glm::min(min.z, trf.z);
									max.z = glm::max(max.z, trf.z);
								}
								
								min.z -= mShadowMapOffset;

								glm::mat4 lightProjection = glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z);

								// Update engine uniform data
								mUniformEngineData.projection = cameraProjection;
								mUniformEngineData.view = glm::inverse(cameraTransform->get());
								mUniformEngineData.lightmat = lightProjection * lightView;
								mUniformEngineData.skyColor = mSkyColor;
								mUniformEngineData.farPlane = cameraCamera->clippingPlanes[1];
								mUniformEngineData.sunDirection = sunDirection;
								mUniformEngineData.sunColor = sunColor;
								// Upload buffer
								glBindBuffer(GL_UNIFORM_BUFFER, mEngineUniformBuffer);
								glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(decltype(mUniformEngineData)), &mUniformEngineData);
								glBindBuffer(GL_UNIFORM_BUFFER, 0);

								// setup drawing
								mShadowProgram->bind();
								
								glDisable(GL_CULL_FACE);

								for (auto&& [entity, gameObject, meshFilter, meshRenderer] : mRegistry.view<GameObjectComponent, MeshFilterComponent, MeshRendererComponent>().each()) {
									if (!meshRenderer.shader) continue;
									if (!meshFilter.mesh) continue;

									// Assign all needed textures
									for (auto const& [k, v] : meshRenderer.shader->opaqueAssignments()) {
										if (meshRenderer.textures[v])
											meshRenderer.textures[v]->bind(v);
										else
											mInternalTextureBlack->bind(v);
									}

									mShadowProgram->uniformMat4f("uTransform", gameObject.transform.get());
									meshFilter.mesh->draw();
									
								}

								glEnable(GL_CULL_FACE);
							}

							// Render scene
							mFramebuffer.bind();
							glViewport(0, 0, mViewportSize.x, mViewportSize.y);
							glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
							glClearColor(mSkyColor.r, mSkyColor.g, mSkyColor.b, 1.0f);

							if (mWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

							for (auto&& [entity, gameObject, meshFilter, meshRenderer] : mRegistry.view<GameObjectComponent, MeshFilterComponent, MeshRendererComponent>().each()) {
								if (!meshRenderer.shader) continue;
								if (!meshFilter.mesh) continue;

								// Assign all needed textures
								for (auto const& [k, v] : meshRenderer.shader->opaqueAssignments()) {

									if (k == "tShadowMap") {
										mFramebufferShadowDepth.bind(v);
									}
									else {
										if (meshRenderer.textures[v])
											meshRenderer.textures[v]->bind(v);
										else
											mInternalTextureBlack->bind(v);
									}

									
								}

								// Upload material changes and bind buffer for prep
								meshRenderer.uploadAndPrep();

								if (!meshRenderer.shader->cull()) glDisable(GL_CULL_FACE);
								meshRenderer.shader->bind();
								meshRenderer.shader->uniformMat4f("uTransform", gameObject.transform.get());
								meshRenderer.shader->uniform3f("uSkyColor", mSkyColor);
								meshFilter.mesh->draw();
								if (!meshRenderer.shader->cull()) glEnable(GL_CULL_FACE);
							}

							if (mWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

							// Tonemap // aces
							glBindVertexArray(mEmptyVao);
							mPostFramebuffer.bind();
							glDisable(GL_CULL_FACE);
							glDisable(GL_DEPTH_TEST);
							
							mAcesProgram->bind();
							mFramebufferColor.bind(0);
							glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

							glEnable(GL_DEPTH_TEST);
							glEnable(GL_CULL_FACE);
							glBindFramebuffer(GL_FRAMEBUFFER, 0);
						}
						else
							ImGui::TextUnformatted("No camera found");
					}
					
				}
			}
			ImGui::End();
			ImGui::PopStyleVar(2);
		}

		glViewport(0, 0, mFramebufferSize.x, mFramebufferSize.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	hyperengine::Window mWindow;

	float mShadowMapOffset = 10.0f;
	float mShadowMapDistance = 50.0f;
	int mShadowMapSize = 2048;
	hyperengine::Framebuffer mFramebufferShadow;
	hyperengine::Texture mFramebufferShadowDepth;

	hyperengine::AudioEngine mAudioEngine;
	entt::registry mRegistry;
	ResourceManager mResourceManager;
	entt::entity mSelected = entt::null;

	GLuint mEngineUniformBuffer = 0;
	UniformEngineData mUniformEngineData;

	hyperengine::Framebuffer mFramebuffer;
	hyperengine::Renderbuffer mFramebufferDepth;
	hyperengine::Texture mFramebufferColor;
	glm::ivec2 mViewportSize{};

	hyperengine::Framebuffer mPostFramebuffer;
	hyperengine::Texture mPostFramebufferColor;

	glm::vec3 mSkyColor = { 0.7f, 0.8f, 0.9f };
	bool mWireframe = false;

	bool mRunning = true;
	Views mViews;
	glm::ivec2 mFramebufferSize{};
	ImGuizmo::OPERATION mOperation = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE mMode = ImGuizmo::MODE::LOCAL;

	GLuint mEmptyVao;

	std::shared_ptr<hyperengine::ShaderProgram> mShadowProgram;
	std::shared_ptr<hyperengine::ShaderProgram> mAcesProgram;
	std::shared_ptr<hyperengine::Texture> mInternalTextureBlack;
	std::shared_ptr<hyperengine::Texture> mInternalTextureWhite;
	std::shared_ptr<hyperengine::Texture> mInternalTextureUv;
};

int main(int argc, char* argv[]) {
	TracySetProgramName("HyperEngine");
	spdlog::set_level(spdlog::level::trace);
	hyperengine::setupRenderDoc(true);
	Engine engine;
	engine.run();
	spdlog::shutdown();
    return 0;
}