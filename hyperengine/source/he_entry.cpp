#ifdef _WIN32
#	include <Windows.h>
#	include <shlobj_core.h>
#	undef near
#	undef far
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

#include "gui/he_console.hpp"

#include <btBulletDynamicsCommon.h>

#include <debug_trap.h>

#include "he_resourcemanager.hpp"
#include "gui/he_filesystem.hpp"

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

#include <format>

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

#include <imgui_internal.h>

#include <spdlog/sinks/base_sink.h>

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
constexpr std::string_view kInternalTextureCheckerboardName = "internal://checkerboard.png";
// !!! NOTICE !!! When adding to this list, make sure to update the definition inside gui/he_filesystem.hpp

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

	void translate(glm::vec3 direction) {
		glm::mat4 matrix = get();
		matrix = glm::translate(matrix, direction);
		set(matrix);
	}
};

std::unordered_map<std::string, ImFont*> fonts;

static void initImGui(hyperengine::Window& window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	ImFont* defaultFont = io.Fonts->AddFontDefault();
	ImFont* regularFont = io.Fonts->AddFontFromFileTTF("NotoSans-Regular.ttf", 18.0f);
	ImFont* boldFont = io.Fonts->AddFontFromFileTTF("NotoSans-Bold.ttf", 18.0f);
	ImFont* monoFont = io.Fonts->AddFontFromFileTTF("NotoSansMono-Regular.ttf", 18.0f);
	if (regularFont) io.FontDefault = regularFont;

	fonts["regular"] = regularFont ? regularFont : defaultFont;
	fonts["bold"] = boldFont ? boldFont : defaultFont;
	fonts["mono"] = monoFont ? monoFont : defaultFont;

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

struct GameObjectComponent final {
	std::string name;
	hyperengine::Uuid uuid = hyperengine::Uuid::generate();
	Transform transform;

	GameObjectComponent() {
		std::stringstream ss;
		ss << "unnamed " << std::hex << (uint64_t)uuid;
		name = ss.str();
	}

	// unused rn
	entt::entity parent = entt::null;
	entt::entity first = entt::null;
	entt::entity prev = entt::null;
	entt::entity next = entt::null;
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

struct CameraComponent final {
	glm::vec2 clippingPlanes = { 0.1f, 100.0f };
	float fov = 80.0f;
};

struct LightComponent final {
	glm::vec3 color = glm::vec3(1.0f);
	float strength = 3.0f;
};

struct UniformEngineData final {
	glm::mat4 projection;   
	glm::mat4 view;
	glm::mat4 lightmat;
	glm::vec3 skyColor;
	float farPlane;
	glm::vec3 sunDirection;
	float gTime;
	glm::vec3 sunColor;
	float HE_UNNAMABLE;
};

// Assert layout matches glsl std140
static_assert(sizeof(UniformEngineData) == 240);
static_assert(offsetof(UniformEngineData, projection)   ==   0);
static_assert(offsetof(UniformEngineData, view)         ==  64);
static_assert(offsetof(UniformEngineData, lightmat)     == 128);
static_assert(offsetof(UniformEngineData, skyColor)     == 192);
static_assert(offsetof(UniformEngineData, farPlane)     == 204);
static_assert(offsetof(UniformEngineData, sunDirection) == 208);
static_assert(offsetof(UniformEngineData, gTime)        == 220);
static_assert(offsetof(UniformEngineData, sunColor)     == 224);

struct Views final {
	bool flatHierarchy = true;
	bool properties = true;
	bool viewport = true;
	bool console = true;
	bool filesystem = true;
	bool resourceManager = false;
	bool imguiDemoWindow = false;
	bool experimentAudio = false;

	void drawUi() {
		ImGui::MenuItem("Hierarchy", nullptr, &flatHierarchy);
		ImGui::MenuItem("Properties", nullptr, &properties);
		ImGui::MenuItem("Viewport", nullptr, &viewport);
		ImGui::MenuItem("Console", nullptr, &console);
		ImGui::MenuItem("Filesystem", nullptr, &filesystem);
		ImGui::MenuItem("Resource Manager", nullptr, &resourceManager);
		ImGui::Separator();
		ImGui::MenuItem("Dear ImGui Demo", nullptr, &imguiDemoWindow);
		ImGui::SeparatorText("Experiments");
		ImGui::MenuItem("Audio", nullptr, &experimentAudio);
	}
};

template <class T, class UserPtr>
bool drawComponentEditGui(entt::registry& registry, entt::entity entity, char const* name, UserPtr* ptr, void(*func)(T&, UserPtr*), bool canDeleteCompoent = true) {
	if (entity == entt::null) return false;
	T* component = registry.try_get<T>(entity);
	if (!component) return false;

	bool opened = ImGui::CollapsingHeader(name);

	bool removeComponent = false;
	if (ImGui::BeginPopupContextItem()) {
		if (ImGui::MenuItem("Delete Component", nullptr, nullptr, canDeleteCompoent)) {
			removeComponent = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if(opened)
		func(*component, ptr);

	if (removeComponent) {
		registry.erase<T>(entity);
		return false;
	}

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

bool drawVec3Control(char const* label, glm::vec3& values, float resetValue = 0.0f, bool placeDummy = true, float columnWidth = 100.0f) {
	bool edited = false;

	ImGui::PushID(label);
	ImGui::Columns(2, nullptr, false);

	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text(label);
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize(lineHeight + 3.0f, lineHeight);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
	ImGui::PushFont(fonts["bold"]);
	if (ImGui::Button("X", buttonSize)) { values.x = resetValue; edited = true; }
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	edited |= ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f", 0);
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
	ImGui::PushFont(fonts["bold"]);
	if (ImGui::Button("Y", buttonSize)) { values.y = resetValue; edited = true; }
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	edited |= ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f", 0);
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
	ImGui::PushFont(fonts["bold"]);
	if (ImGui::Button("Z", buttonSize)) { values.z = resetValue; edited = true; }
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	edited |= ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f", 0);
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();
	ImGui::Columns(1);
	if (placeDummy) ImGui::Dummy(ImGui::GetStyle().ItemSpacing);
	ImGui::PopID();
	return edited;
};

static bool canPerformGlobalBinding(ImGuiKeyChord chord) {
	bool isRouted = ImGui::GetShortcutRoutingData(chord)->RoutingCurr != ImGuiKeyOwner_NoOwner;
	return !isRouted && ImGui::IsKeyChordPressed(chord);
}


class PhysicsWorld final {
public:
	PhysicsWorld(btVector3 gravity = btVector3(0, -10, 0)) {
		ZoneScoped;
		mCollisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
		mDispatcher = std::make_unique<btCollisionDispatcher>(mCollisionConfiguration.get());
		mPairCache = std::make_unique<btDbvtBroadphase>(); // can also try btAxis3Sweep
		mConstraintSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
		mDynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(mDispatcher.get(), mPairCache.get(), mConstraintSolver.get(), mCollisionConfiguration.get());
		mDynamicsWorld->setGravity(gravity);
	}

	void stepSimulation(btScalar timeStep, int maxSubSteps = 1, btScalar fixedTimeStep = 1.0f / 60.0f) {
		ZoneScoped;
		mDynamicsWorld->stepSimulation(timeStep, maxSubSteps, fixedTimeStep);
	}
public:
	std::unique_ptr<btDefaultCollisionConfiguration> mCollisionConfiguration;
	std::unique_ptr<btCollisionDispatcher> mDispatcher;
	std::unique_ptr<btBroadphaseInterface> mPairCache;
	std::unique_ptr<btSequentialImpulseConstraintSolver> mConstraintSolver;
	std::unique_ptr<btDiscreteDynamicsWorld> mDynamicsWorld;
};


ATTRIBUTE_ALIGNED16(struct) EngineMotionState : public btMotionState {
	entt::handle mHandle;

	BT_DECLARE_ALIGNED_ALLOCATOR();
	EngineMotionState(entt::handle handle) : mHandle(handle) {}

	virtual void getWorldTransform(btTransform& worldTrans) const override {
		auto transform = mHandle.get<GameObjectComponent>().transform;
		transform.scale = glm::vec3(1.0f);
		worldTrans.setFromOpenGLMatrix(glm::value_ptr(transform.get()));
	}

	virtual void setWorldTransform(btTransform const& worldTrans) override {
		glm::mat4 matrix;
		worldTrans.getOpenGLMatrix(glm::value_ptr(matrix));
		auto& transform = mHandle.get<GameObjectComponent>().transform;
		glm::vec3 oldScale = transform.scale;
		transform.set(matrix);
		transform.scale = oldScale;
	}
};

struct PhysicsComponent final {
	void attach(PhysicsWorld& world) {
		if (attached) return;
		attached = true;
		world.mDynamicsWorld->addRigidBody(mBody.get());
	}

	void detach(PhysicsWorld& world) {
		if (!attached) return;
		attached = false;
		world.mDynamicsWorld->removeCollisionObject(mBody.get());
	}

	std::unique_ptr<btRigidBody> mBody;
	std::unique_ptr<btCollisionShape> mShape;
	std::unique_ptr<EngineMotionState> mMotionState;
	bool attached = false;

	float mass = 1.0f;
	glm::vec3 halfExtents = { 1.0f, 1.0f, 1.0f };
	float scale = 1.0f;

	void updateShape(PhysicsWorld& world) {
		detach(world);

		mShape = std::make_unique<btBoxShape>(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
		mShape->setLocalScaling(btVector3(scale, scale, scale));

		btVector3 localInertia = btVector3(0, 0, 0);
		if (mass > 0.0f) mShape->calculateLocalInertia(mass, localInertia);

		mBody->setCollisionShape(mShape.get());
		mBody->setMassProps(mass, localInertia);
		mBody->updateInertiaTensor();

		attach(world);
	}

	void setup(PhysicsWorld& world, entt::handle entityHandle) {
		mMotionState = std::make_unique<EngineMotionState>(entityHandle);

		mShape = std::make_unique<btBoxShape>(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
		mShape->setLocalScaling(btVector3(scale, scale, scale));

		btVector3 localInertia = btVector3(0, 0, 0);
		if (mass > 0.0f) mShape->calculateLocalInertia(mass, localInertia);

		auto info = btRigidBody::btRigidBodyConstructionInfo{ mass, mMotionState.get(), mShape.get(), localInertia };
		info.m_linearDamping = 0.2f;
		info.m_angularDamping = 0.2f;
		info.m_restitution = 0.4f;
		mBody = std::make_unique<btRigidBody>(info);
		attach(world);
	}
};

struct Engine final {
	void createInternalTextures() {
		mResourceManager.assertTextureLifetime(mResourceManager.getTexture("icons/file.png"));

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
			uint8_t pixels[] = {
				255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 
				191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255,
				191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255,
				191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255,
				191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255, 191, 191, 191, 255, 255, 255, 255, 255,
			};

			hyperengine::Texture tex = { {.width = 8, .height = 8, .format = hyperengine::PixelFormat::kRgba8, .minFilter = kLinear, .magFilter = kNearest, .wrap = kRepeat, .label = kInternalTextureCheckerboardName, .origin = kInternalTextureCheckerboardName } };
			tex.upload({ .xoffset = 0, .yoffset = 0, .width = 8, .height = 8, .format = hyperengine::PixelFormat::kRgba8, .pixels = pixels});
			mInternalTextureCheckerboard = std::make_shared<hyperengine::Texture>(std::move(tex));
			mResourceManager.mTextures[std::string(kInternalTextureCheckerboardName)] = mInternalTextureCheckerboard;
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

		mAcesProgram = mResourceManager.getShaderProgram("shaders/aces.glsl", mFileErrors);
		mShadowProgram = mResourceManager.getShaderProgram("shaders/shadow.glsl", mFileErrors);
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
		editorOpNewScene();

		
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

		mEmptyMesh = {{ .origin = "Empty Mesh" }};

		while (mRunning) {
			glfwPollEvents();
			glfwGetFramebufferSize(mWindow.handle(), &mFramebufferSize.x, &mFramebufferSize.y);

			if (mFramebufferSize.x > 0 && mFramebufferSize.y > 0) {
				hyperengine::Framebuffer().bind();
				imguiBeginFrame();
				mResourceManager.update();
				update();
				mPhysicsWorld.stepSimulation(ImGui::GetIO().DeltaTime, 10);
				hyperengine::Framebuffer().bind();
				imguiEndFrame();
			}

			mWindow.swapBuffers();

			TracyGpuCollect;
			FrameMark;
		}

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
					ImGui::LabelText("Uuid", "%p", (uint64_t)comp.uuid);
					if (ImGui::SmallButton("Copy Uuid")) {
						std::string formatted = std::format("0x{:016x}", (uint64_t)comp.uuid);
						ImGui::SetClipboardText(formatted.c_str());
					}

					bool edited = false;

					edited |= drawVec3Control("Translation", comp.transform.translation, 0.0f, false);

					glm::vec3 oldEuler = glm::degrees(glm::eulerAngles(comp.transform.orientation));
					glm::vec3 newEuler = oldEuler;

					if (edited |= drawVec3Control("Rotation", newEuler, 0.0f, false)) {
						glm::vec3 deltaEuler = glm::radians(newEuler - oldEuler);
						comp.transform.orientation = glm::rotate(comp.transform.orientation, deltaEuler.x, glm::vec3(1, 0, 0));
						comp.transform.orientation = glm::rotate(comp.transform.orientation, deltaEuler.y, glm::vec3(0, 1, 0));
						comp.transform.orientation = glm::rotate(comp.transform.orientation, deltaEuler.z, glm::vec3(0, 0, 1));
					}

					edited |= drawVec3Control("Scale", comp.transform.scale, 1.0f);

					if (edited) {
						auto* physics = ptr->mRegistry.try_get<PhysicsComponent>(ptr->mSelected);
						if (physics) {
							if (ptr->mOperation == ImGuizmo::TRANSLATE) {
								btTransform trans;
								physics->mMotionState->getWorldTransform(trans);
								physics->mBody->setWorldTransform(trans);
							}
							physics->mBody->activate();
							physics->mBody->clearForces();
						}
					}
				}, false);

				bool hasLight = drawComponentEditGui<LightComponent, Engine>(mRegistry, mSelected, "Light", this, [](auto& comp, auto* ptr) {
					ImGui::ColorEdit3("Color", glm::value_ptr(comp.color));
					ImGui::DragFloat("Strength", &comp.strength, 0.1f);
				});

				bool hasMeshFilter = drawComponentEditGui<MeshFilterComponent, Engine>(mRegistry, mSelected, "Mesh Filter", this, [](auto& comp, auto* ptr) {
					if (comp.mesh)
						ImGui::LabelText("Mesh", "%s", comp.mesh->origin().c_str());
					else
						ImGui::LabelText("Mesh", "%s", "<null>");

					if (ImGui::BeginDragDropTarget()) {
						if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload("FilesystemFile")) {
							std::string path((char const*)payload->Data, payload->DataSize);
							comp.mesh = ptr->mResourceManager.getMesh(path);
						}
						ImGui::EndDragDropTarget();
					}
				});

				bool hasMeshRenderer = drawComponentEditGui<MeshRendererComponent, Engine>(mRegistry, mSelected, "Mesh Renderer", this, [](auto& comp, auto* ptr) {
					if (comp.shader)
						ImGui::LabelText("Shader", "%s", comp.shader->origin().c_str());
					else
						ImGui::LabelText("Shader", "%s", "<null>");	

					if (ImGui::BeginDragDropTarget()) {
						if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload("FilesystemFile")) {
							std::string path((char const*)payload->Data, payload->DataSize);
							comp.shader = ptr->mResourceManager.getShaderProgram(path, ptr->mFileErrors);
							comp.resetMaterial();
						}
						ImGui::EndDragDropTarget();
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

							if(!texture)
								ImGui::Image((void*)(uintptr_t)ptr->mInternalTextureCheckerboard->handle(), {64, 64}, {0, 1}, {1, 0});
							else
								ImGui::Image((void*)(uintptr_t)texture->handle(), { 64, 64 }, { 0, 1 }, { 1, 0 });

							if (ImGui::BeginDragDropTarget()) {
								if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload("FilesystemFile")) {
									std::string path((char const*)payload->Data, payload->DataSize);
									comp.textures[v] = ptr->mResourceManager.getTexture(path);
								}
								ImGui::EndDragDropTarget();
							}

							ImGui::PopID();
						}
					}
				});

				bool hasCamera = drawComponentEditGui<CameraComponent, Engine>(mRegistry, mSelected, "Camera", this, [](auto& comp, auto* ptr) {
					ImGui::DragFloat("Fov", &comp.fov, 1.0f, 10.0f, 170.0f);
					ImGui::DragFloatRange2("Clipping planes", &comp.clippingPlanes.x, &comp.clippingPlanes.y, 0.1f, 0.001f, 1000.0f);
				});

				if (ImGui::Button("Add Component")) {
					ImGui::OpenPopup("AddComponent");
				}

				if (ImGui::BeginPopup("AddComponent")) {

					if (ImGui::MenuItem("Game Object", nullptr, nullptr, !hasGameObject)) {
						mRegistry.emplace<GameObjectComponent>(mSelected);
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::MenuItem("Mesh Filter", nullptr, nullptr, !hasMeshFilter)) {
						mRegistry.emplace<MeshFilterComponent>(mSelected);
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::MenuItem("Mesh Renderer", nullptr, nullptr, !hasMeshRenderer)) {
						mRegistry.emplace<MeshRendererComponent>(mSelected);
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::MenuItem("Light", nullptr, nullptr, !hasLight)) {
						mRegistry.emplace<LightComponent>(mSelected);
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::MenuItem("Camera", nullptr, nullptr, !hasCamera)) {
						mRegistry.emplace<CameraComponent>(mSelected);
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}
			}
		}
		ImGui::End();
		
	}

	void drawGuiFlatHierarchy() {
		ZoneScoped;

		if (!mViews.flatHierarchy) return;

		if (ImGui::Begin("Flat Hierarchy", &mViews.flatHierarchy)) {
			for (auto&& [entity, gameObject] : mRegistry.view<GameObjectComponent>().each()) {
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
				if (mSelected == entity)
					flags |= ImGuiTreeNodeFlags_Selected;

				bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)entity, flags, "%s", gameObject.name.c_str());
				if (ImGui::IsItemActive())
					mSelected = entity;

				bool shouldDelete = false;

				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::MenuItem("Delete This")) {
						shouldDelete = true;
					}

					ImGui::EndPopup();
				}

				if (opened)
					ImGui::TreePop();


				if (shouldDelete) {
					if (mSelected == entity)
						mSelected = entt::null;
					mRegistry.destroy(entity);
				}
			}



			if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
				editorOpGuiEntity();
				ImGui::EndPopup();
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

						ImGui::TreePop();
					}
				}
			}
		}
		ImGui::End();
	}

	void loadScene(char const* path) {
		editorOpNewScene();

		// Dont add camera, within the viewport we will use an internal camera, this will be used for the game runtime
		//{
		//	entt::handle entity{ mRegistry, mRegistry.create() };
		//	GameObjectComponent& gameObject = entity.emplace<GameObjectComponent>();
		//	gameObject.transform.translation = glm::vec3(0, 1, 3);
		//	gameObject.name = "MainCamera";
		//	CameraComponent& camera = entity.emplace<CameraComponent>();
		//}

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
			int t = lua_gettop(L);
			lua_pushnil(L);

			// Iterate object list
			while (lua_next(L, t) != 0) {
				entt::entity entity = mRegistry.create();
				GameObjectComponent& gameObject = mRegistry.emplace<GameObjectComponent>(entity); // Enforce all objects to have this component

				lua_getfield(L, -1, "GameObject");
				if (lua_istable(L, -1)) {
					lua_getfield(L, -1, "name");
					if (lua_isstring(L, -1)) {
						gameObject.name = lua_tostring(L, -1);
					}
					lua_pop(L, 1);

					lua_getfield(L, -1, "uuid");
					if (lua_isinteger(L, -1)) {
						gameObject.uuid = static_cast<uint64_t>(lua_tointeger(L, -1));
					}
					else {
						spdlog::warn("object in scene file doesn'r provide uuid");
					}
					lua_pop(L, 1);

					lua_getfield(L, -1, "translation");
					if (lua_istable(L, -1)) {
						gameObject.transform.translation = hyperengine::luaToVec3(L);
					}
					lua_pop(L, 1);

					lua_getfield(L, -1, "orientation");
					if (lua_istable(L, -1)) {
						gameObject.transform.orientation = std::bit_cast<glm::quat>(hyperengine::luaToVec4(L));
					}
					lua_pop(L, 1);

					lua_getfield(L, -1, "scale");
					if (lua_istable(L, -1)) {
						gameObject.transform.scale = hyperengine::luaToVec3(L);
					}
					lua_pop(L, 1);

				}
				lua_pop(L, 1);

				lua_getfield(L, -1, "MeshFilter");
				if (lua_istable(L, -1)) {
					auto& meshFilter = mRegistry.emplace<MeshFilterComponent>(entity);
				
					lua_getfield(L, -1, "resource");
					if (lua_isstring(L, -1)) {
						meshFilter.mesh = mResourceManager.getMesh(lua_tostring(L, -1));
					}
					lua_pop(L, 1);
				}
				lua_pop(L, 1);

				lua_getfield(L, -1, "MeshRenderer");
				if (lua_istable(L, -1)) {
					auto& meshRenderer = mRegistry.emplace<MeshRendererComponent>(entity);

					lua_getfield(L, -1, "shader");
					if (lua_isstring(L, -1)) {
						meshRenderer.shader = mResourceManager.getShaderProgram(lua_tostring(L, -1), mFileErrors);
					}
					lua_pop(L, 1);

					if (meshRenderer.shader) {
						meshRenderer.allocateMaterialBuffer();

						lua_getfield(L, -1, "textures");
						if (lua_istable(L, -1)) {
							int textureTable = lua_gettop(L);
							lua_pushnil(L);
							while (lua_next(L, textureTable) != 0) {
								auto it = meshRenderer.shader->opaqueAssignments().find(lua_tostring(L, -2));
								if (it != meshRenderer.shader->opaqueAssignments().end()) {
									meshRenderer.textures[it->second] = mResourceManager.getTexture(lua_tostring(L, -1));
								}
								lua_pop(L, 1);
							}

						}
						lua_pop(L, 1);

						lua_getfield(L, -1, "material");
						if (lua_istable(L, -1)) {
							int materialTable = lua_gettop(L);
							lua_pushnil(L);

							while (lua_next(L, materialTable) != 0) {
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
				}
				lua_pop(L, 1);

				lua_getfield(L, -1, "Light");
				if (lua_istable(L, -1)) {
					auto& light = mRegistry.emplace<LightComponent>(entity);

					lua_getfield(L, -1, "color");
					if (lua_istable(L, -1)) {
						light.color = hyperengine::luaToVec3(L);
					}
					lua_pop(L, 1);

					lua_getfield(L, -1, "strength");
					if (lua_isnumber(L, -1)) {
						light.strength = static_cast<float>(lua_tonumber(L, -1));
					}
					lua_pop(L, 1);
				}
				lua_pop(L, 1);

				lua_pop(L, 1);
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

	void DetachPhysicsObj(entt::registry& reg, entt::entity e) {

		reg.get<PhysicsComponent>(e).detach(mPhysicsWorld);

		for (int i = 0; i < mPhysicsWorld.mDynamicsWorld->getNumCollisionObjects(); ++i) {
			mPhysicsWorld.mDynamicsWorld->getCollisionObjectArray()[i]->activate(true);
		}
	}

	void editorOpNewScene() {
		mRegistry.clear();
		mRegistry = entt::registry();
		mSelected = entt::null;
		mRegistry.on_destroy<PhysicsComponent>().connect<&Engine::DetachPhysicsObj>(this);

		auto& rootGameObject = mRegistry.emplace<GameObjectComponent>(mRoot);
		rootGameObject.name = "_root";
		rootGameObject.uuid = 0;

		{
			entt::entity entity = mRegistry.create();
			auto& gameObject = mRegistry.emplace<GameObjectComponent>(entity);
			gameObject.transform.scale = { 0, 0, 0 };
			auto& comp = mRegistry.emplace<PhysicsComponent>(entity);
			auto& meshFilter = mRegistry.emplace<MeshFilterComponent>(entity);
			meshFilter.mesh = mResourceManager.getMesh("plane.obj");
			comp.mShape = std::make_unique<btStaticPlaneShape>(btVector3(0, 1, 0), 0.0f);
			auto& meshRenderer = mRegistry.emplace<MeshRendererComponent>(entity);
			meshRenderer.shader = mResourceManager.getShaderProgram("shaders/opaque.glsl", mFileErrors);
			if (meshRenderer.shader) {
				meshRenderer.allocateMaterialBuffer();
				*((glm::vec3*)meshRenderer.data.data()) = glm::vec3(1.0f);
				meshRenderer.textures[meshRenderer.shader->opaqueAssignments().at("tAlbedo")] = mResourceManager.getTexture("rocks.png");
			}
			btScalar mass = 0.0f;
			btVector3 localInertia = btVector3(0, 0, 0);
			if (mass > 0.0f) comp.mShape->calculateLocalInertia(mass, localInertia);
			comp.mMotionState = std::make_unique<EngineMotionState>(entt::handle(mRegistry, entity));
			auto info = btRigidBody::btRigidBodyConstructionInfo{ mass, comp.mMotionState.get(), comp.mShape.get(), localInertia };
			info.m_linearDamping = .2f;
			info.m_angularDamping = .2f;
			info.m_restitution = 1.0f;
			comp.mBody = std::make_unique<btRigidBody>(info);

			// Adjust shape realtime
			// comp.detach(mPhysicsWorld);
			// comp.mBody->setCollisionShape(shape);
			// comp.mBody->setMassProps(mass, inertia);
			// comp.attach(mPhysicsWorld);

			comp.attach(mPhysicsWorld);
		}
		for (int y = 0; y < 20; ++y) {
			entt::entity entity = mRegistry.create();
			auto& gameObject = mRegistry.emplace<GameObjectComponent>(entity);
			gameObject.transform.translation = { 0, 5 * y + 5, 0 };
			auto& meshFilter = mRegistry.emplace<MeshFilterComponent>(entity);
			meshFilter.mesh = mResourceManager.getMesh("cube.obj");
			auto& meshRenderer = mRegistry.emplace<MeshRendererComponent>(entity);
			meshRenderer.shader = mResourceManager.getShaderProgram("shaders/opaque.glsl", mFileErrors);
			if (meshRenderer.shader) {
				meshRenderer.allocateMaterialBuffer();
				*((glm::vec3*)meshRenderer.data.data()) = glm::vec3(1.0f);
				meshRenderer.textures[meshRenderer.shader->opaqueAssignments().at("tAlbedo")] = mResourceManager.getTexture("rocks.png");
			}

			auto& comp = mRegistry.emplace<PhysicsComponent>(entity);
			comp.setup(mPhysicsWorld, entt::handle(mRegistry, entity));
		}
	}

	void editorOpCreateEmpty() {
		mSelected = mRegistry.create();
		mRegistry.emplace<GameObjectComponent>(mSelected);
	}

	void editorOpDeleteSelected() {
		mRegistry.destroy(mSelected);
		mSelected = entt::null;
	}

	void editorOpGuiEntity() {
		if (ImGui::MenuItem("Create Empty", ImGui::GetKeyChordName(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_N))) {
			editorOpCreateEmpty();
		}

		if (ImGui::MenuItem("Delete Selected", nullptr, nullptr, mSelected != entt::null)) {
			editorOpDeleteSelected();
		}
	}

	void editorOpReloadShaders() {
		for (auto& [k, v] : mResourceManager.mShaders) {
			if (std::shared_ptr<hyperengine::ShaderProgram> program = v.lock()) {
				mFileErrors.erase(std::u8string((char8_t const*)k.c_str()));
				mResourceManager.reloadShader(k, *program, mFileErrors);
			}
		}
		spdlog::info("Reloaded Shaders");
	}

	void drawUi() {
		ZoneScoped;

		ImGui::DockSpaceOverViewport();

		if (canPerformGlobalBinding(ImGuiMod_Ctrl | ImGuiKey_O))
			openSceneDialogOption();

		if (canPerformGlobalBinding(ImGuiMod_Ctrl | ImGuiKey_N))
			editorOpNewScene();
			
		if (canPerformGlobalBinding(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_N))
			editorOpCreateEmpty();

		if (canPerformGlobalBinding(ImGuiMod_Shift | ImGuiKey_R))
			editorOpReloadShaders();

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New Scene", ImGui::GetKeyChordName(ImGuiMod_Ctrl | ImGuiKey_N), nullptr))
					editorOpNewScene();
				if (ImGui::MenuItem("Open Scene", ImGui::GetKeyChordName(ImGuiMod_Ctrl | ImGuiKey_O), nullptr))
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
				editorOpGuiEntity();
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug")) {
				if (ImGui::MenuItem("Reload Shaders", ImGui::GetKeyChordName(ImGuiMod_Shift | ImGuiKey_R))) {
					editorOpReloadShaders();
				}

				if (ImGui::MenuItem("Capture Frame", nullptr, nullptr, hyperengine::rdoc::isRunning())) {
					hyperengine::rdoc::triggerCapture();
				}

				if (ImGui::MenuItem("Attach RenderDoc", nullptr, nullptr, hyperengine::rdoc::isRunning() && !hyperengine::rdoc::isTargetControlConnected())) {
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

		hyperengine::getConsole().draw(&mViews.console);

		mGuiFilesystem.draw(&mViews.filesystem, mResourceManager, mTextEditors);

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

		if (!mFileErrors.empty()) {
			if (ImGui::Begin("Errors")) {
				if (ImGui::BeginTabBar("ErrorTabs", ImGuiTabBarFlags_Reorderable)) {
					for (auto& [k, v] : mFileErrors) {
						if (ImGui::BeginTabItem((char const*)k.c_str())) {
							ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 0.0f, 0.0f, 1.0f });
							ImGui::TextWrapped("%s", v.c_str());
							ImGui::PopStyleColor();

							ImGui::EndTabItem();
						}
					}

					ImGui::EndTabBar();
				}
			}
			ImGui::End();
		}

		if (!mTextEditors.empty()) {
			bool opened = true;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
			ImGui::Begin("Text Editors", &opened, ImGuiWindowFlags_MenuBar);
			ImGui::PopStyleVar();

			{
				bool saveCurrentDoc = false;
				bool saveAll = false;

				if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S)) {
					saveCurrentDoc = true;
				}

				if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S)) {
					saveAll = true;
				}

				int line = 0, column = 0, lineCount = 0;
				bool overwriteEnabled = false;
				std::string defName;

				if (ImGui::BeginMenuBar()) {
					if (ImGui::BeginMenu("File")) {
						if (ImGui::MenuItem("Save", ImGui::GetKeyChordName(ImGuiMod_Ctrl | ImGuiKey_S))) saveCurrentDoc = true;
						if (ImGui::MenuItem("Save All", ImGui::GetKeyChordName(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S))) saveAll = true;

						ImGui::EndMenu();
					}

					ImGui::EndMenuBar();
				}

				if (ImGui::BeginTabBar("TextEditorsTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {

					std::u8string toClose;

					for (auto& [k, v] : mTextEditors) {
						bool opened = true;

						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

						ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;

						if (mTextEditorsAdditionalState[k].undoIndexInDisc != v.GetUndoIndex())
							flags |= ImGuiTabItemFlags_UnsavedDocument;

						if (ImGui::BeginTabItem((char const*)k.c_str(), &opened, flags)) {

							v.GetCursorPosition(line, column);
							lineCount = v.GetLineCount();
							overwriteEnabled = v.IsOverwriteEnabled();
							defName = v.GetLanguageDefinitionName();

							ImVec2 availContent = ImGui::GetContentRegionAvail();
							availContent.y -= ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y * 2.0f;

							ImGui::PushFont(fonts["mono"]);
							v.Render((char const*)k.c_str(), false, availContent);
							ImGui::PopFont();

							if (saveCurrentDoc) {
								hyperengine::writeFile((char const*)k.c_str(), v.GetText().data(), v.GetText().size());
								mGuiFilesystem.queueUpdate();
								mTextEditorsAdditionalState[k].undoIndexInDisc = v.GetUndoIndex();
							}

							ImGui::EndTabItem();
						}

						ImGui::PopStyleVar();

						if (!opened)
							toClose = k;

						if (saveAll) {
							hyperengine::writeFile((char const*)k.c_str(), v.GetText().data(), v.GetText().size());
							mGuiFilesystem.queueUpdate();
							mTextEditorsAdditionalState[k].undoIndexInDisc = v.GetUndoIndex();
						}
					}

					if (!toClose.empty()) {
						mTextEditors.erase(toClose);
						mTextEditorsAdditionalState.erase(toClose);
					}

					ImGui::EndTabBar();
				}

				ImGui::Text("%6d/%-6d %6d lines | %s | %s", line + 1, column + 1, lineCount, overwriteEnabled ? "Ovr" : "Ins", defName.c_str());
			}
			ImGui::End();

			if (!opened)
				mTextEditors.clear();
		}

		drawGuiResourceManager();
		drawGuiFlatHierarchy();
		drawGuiProperties();
		audioExperiment();

		if (mViews.imguiDemoWindow)
			ImGui::ShowDemoWindow(&mViews.imguiDemoWindow);
	}

	void resizeFramebuffers(glm::ivec2 targetSize) {
		if (mViewportSize == targetSize) return;
		mViewportSize = targetSize;

		using enum hyperengine::Texture::WrapMode;
		using enum hyperengine::Texture::FilterMode;

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
			.label = "framebuffer viewport depth"
		}};

		std::array<hyperengine::Framebuffer::Attachment, 2> attachments{
			hyperengine::Framebuffer::Attachment(GL_COLOR_ATTACHMENT0, std::ref(mFramebufferColor)),
			hyperengine::Framebuffer::Attachment(GL_DEPTH_ATTACHMENT, std::ref(mFramebufferDepth)),
		};

		mFramebuffer = {{ .attachments = attachments }};

		mPostFramebufferColor = { {
			.width = mViewportSize.x,
			.height = mViewportSize.y,
			.format = hyperengine::PixelFormat::kRgba8,
			.minFilter = kLinear,
			.magFilter = kLinear,
			.wrap = kClampEdge,
			.label = "framebuffer post color"
		} };

		std::array<hyperengine::Framebuffer::Attachment, 1> attachmentsPost{
			hyperengine::Framebuffer::Attachment(GL_COLOR_ATTACHMENT0, std::ref(mPostFramebufferColor))
		};

		mPostFramebuffer = {{ .attachments = attachmentsPost }};
	}

	void drawScene(Transform& cameraTransform, CameraComponent& cameraCamera, glm::vec3 sunDirection, glm::vec3 sunColor) {
		if (mViewportSize.x <= 0 || mViewportSize.y <= 0)  return;

		glm::mat4 cameraProjection = glm::perspective(glm::radians(cameraCamera.fov), static_cast<float>(mViewportSize.x) / static_cast<float>(mViewportSize.y), cameraCamera.clippingPlanes.x, cameraCamera.clippingPlanes.y);

		// Depth only pass ; shadowmap
		{
			mFramebufferShadow.bind();
			glViewport(0, 0, mShadowMapSize, mShadowMapSize);
			glClear(GL_DEPTH_BUFFER_BIT);

			// Calculate center point of adjusted view frustum
			auto adjustedFrustum = glm::perspective(glm::radians(cameraCamera.fov), static_cast<float>(mViewportSize.x) / static_cast<float>(mViewportSize.y), mShadowMapNear, mShadowMapDistance);
			auto corners = getSystemSpaceNdcExtremes(adjustedFrustum * glm::inverse(cameraTransform.get()));
			glm::vec3 center = glm::vec3(0, 0, 0);
			for (const auto& v : corners)
				center += glm::vec3(v);
			center /= static_cast<float>(corners.size());

			// Sun view matrix
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
			mUniformEngineData.view = glm::inverse(cameraTransform.get());
			mUniformEngineData.lightmat = lightProjection * lightView;
			mUniformEngineData.skyColor = mSkyColor;
			mUniformEngineData.farPlane = cameraCamera.clippingPlanes[1];
			mUniformEngineData.sunDirection = sunDirection;
			mUniformEngineData.gTime = static_cast<float>(glfwGetTime());
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

		mPostFramebuffer.bind();
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		mAcesProgram->bind();
		mFramebufferColor.bind(0);
		mEmptyMesh.draw(GL_TRIANGLE_STRIP, 0, 4);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}

	void update() {
		ZoneScoped;
		TracyGpuZone(TracyFunction);

		drawUi();

		if (ImGui::Begin("Passes")) {
			ImGui::Image((void*)(uintptr_t)mFramebufferShadowDepth.handle(), { 256, 256 }, { 0, 1 }, { 1, 0 });
			ImGui::DragFloat("ShadowMap Offset", &mShadowMapOffset);

			ImGui::DragFloat("ShadowMap Near", &mShadowMapNear);
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
					// Resize framebuffers if needed
					resizeFramebuffers(contentAvail);

					// Defaults
					Transform* cameraTransform =  &mEditorCameraTransform;
					CameraComponent* cameraCamera = &mEditorCamera;
					glm::vec3 sunDirection = glm::normalize(glm::vec3(0.0f, -1.0f, 0.01f));
					glm::vec3 sunColor = glm::vec3(1.0f) * 3.0f;

					// Find camera // TODO: Add scene panel and use this to find cameras for that
					//for (auto&& [entity, gameObject, camera] : mRegistry.view<GameObjectComponent, CameraComponent>().each()) {
					//	cameraTransform = &gameObject.transform;
					//	cameraCamera = &camera;
					//	break;
					//}

					// Find sun
					for (auto&& [entity, gameObject, light] : mRegistry.view<GameObjectComponent, LightComponent>().each()) {
						//gameObject.transform.orientation = glm::rotate(gameObject.transform.orientation, glm::radians(ImGui::GetIO().DeltaTime * 10.0f), glm::vec3(1, 0, 0));
						//gameObject.transform.orientation = glm::rotate(gameObject.transform.orientation, glm::radians(ImGui::GetIO().DeltaTime * 1.0f), glm::vec3(0, 1, 0));

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

							glm::mat4 cameraProjection = glm::perspective(glm::radians(cameraCamera->fov), static_cast<float>(mViewportSize.x) / static_cast<float>(mViewportSize.y), cameraCamera->clippingPlanes.x, cameraCamera->clippingPlanes.y);

							if (ImGuizmo::Manipulate(glm::value_ptr(glm::inverse(cameraTransform->get())), glm::value_ptr(cameraProjection), mOperation, mMode, glm::value_ptr(matrix))) {
								gameObject.transform.set(matrix);

								glm::vec2 vec = std::bit_cast<glm::vec2>(ImGui::GetIO().MouseDelta) * 3.0f;
								vec.y *= -1.0f;
								//vec /= glm::vec2(mViewportSize);
								//vec *= 2.0f;
								//vec -= 1.0f;
								glm::vec4 result = glm::inverse(mUniformEngineData.projection) * glm::vec4(vec, 0.0f, 0.0f);
								result = glm::inverse(mUniformEngineData.view) * result;

								

								auto* physics = mRegistry.try_get<PhysicsComponent>(mSelected);
								if (physics) {
									btTransform trans;
									physics->mMotionState->getWorldTransform(trans);
									physics->mBody->setWorldTransform(trans);
									physics->mBody->activate();
									physics->mBody->clearForces();
									physics->mBody->setLinearVelocity(btVector3(result.x, result.y, result.z));
								}
							}
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

								if (glm::length2(direction) > 0)
									cameraTransform->translate(glm::normalize(direction) * 10.0f * ImGui::GetIO().DeltaTime);

								ImVec2 drag = ImGui::GetIO().MouseDelta;
								glm::vec3 worldUp = glm::vec3(glm::vec4(0, 1, 0, 0) * cameraTransform->get());
								cameraTransform->orientation = glm::rotate(cameraTransform->orientation, glm::radians(drag.x * -0.3f), worldUp);
								cameraTransform->orientation = glm::rotate(cameraTransform->orientation, glm::radians(drag.y * -0.3f), { 1, 0, 0 });
							}
							else {
								if (ImGui::IsKeyPressed(ImGuiKey_W)) mOperation = ImGuizmo::TRANSLATE;
								else if (ImGui::IsKeyPressed(ImGuiKey_E)) mOperation = ImGuizmo::ROTATE;
								else if (ImGui::IsKeyPressed(ImGuiKey_R)) mOperation = ImGuizmo::SCALE;
							}
						}

						drawScene(*cameraTransform, *cameraCamera, sunDirection, sunColor);
					}
					else
						ImGui::TextUnformatted("No camera found");
				}
			}
			ImGui::End();
			ImGui::PopStyleVar(2);
		}

		glViewport(0, 0, mFramebufferSize.x, mFramebufferSize.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	hyperengine::Window mWindow;

	Transform mEditorCameraTransform;
	CameraComponent mEditorCamera;

	hyperengine::gui::Filesystem mGuiFilesystem;

	float mShadowMapOffset = 16.0f;
	float mShadowMapNear = 0.1f;
	float mShadowMapDistance = 64.0f;
	int mShadowMapSize = 2048;
	hyperengine::Framebuffer mFramebufferShadow;
	hyperengine::Texture mFramebufferShadowDepth;

	hyperengine::AudioEngine mAudioEngine;
	entt::registry mRegistry;
	ResourceManager mResourceManager;
	entt::entity mSelected = entt::null;
	entt::entity mRoot = entt::null;
	PhysicsWorld mPhysicsWorld;

	GLuint mEngineUniformBuffer = 0;
	UniformEngineData mUniformEngineData;

	hyperengine::Framebuffer mFramebuffer;
	hyperengine::Renderbuffer mFramebufferDepth;
	hyperengine::Texture mFramebufferColor;
	glm::ivec2 mViewportSize{};

	struct ExtraState {
		int undoIndexInDisc = 0;
	};

	std::unordered_map<std::u8string, TextEditor> mTextEditors;
	std::unordered_map<std::u8string, ExtraState> mTextEditorsAdditionalState;
	std::unordered_map<std::u8string, std::string> mFileErrors;

	hyperengine::Framebuffer mPostFramebuffer;
	hyperengine::Texture mPostFramebufferColor;

	glm::vec3 mSkyColor = { 0.7f, 0.8f, 0.9f };
	bool mWireframe = false;

	bool mRunning = true;
	Views mViews;
	glm::ivec2 mFramebufferSize{};
	ImGuizmo::OPERATION mOperation = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE mMode = ImGuizmo::MODE::LOCAL;

	hyperengine::Mesh mEmptyMesh;

	std::shared_ptr<hyperengine::ShaderProgram> mShadowProgram;
	std::shared_ptr<hyperengine::ShaderProgram> mAcesProgram;
	std::shared_ptr<hyperengine::Texture> mInternalTextureBlack;
	std::shared_ptr<hyperengine::Texture> mInternalTextureWhite;
	std::shared_ptr<hyperengine::Texture> mInternalTextureUv;
	std::shared_ptr<hyperengine::Texture> mInternalTextureCheckerboard;
};

namespace {
	template<typename Mutex>
	class HyperEngineSink : public spdlog::sinks::base_sink<Mutex> {
	protected:
		void sink_it_(spdlog::details::log_msg const& msg) override {
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

			ImVec4 constexpr colors[] = {
				{ 204.0f / 255.0f, 204.0f / 255.0f, 204.0f / 255.0f, 1.0f },
				{  58.0f / 255.0f, 150.0f / 255.0f, 221.0f / 255.0f, 1.0f },
				{  19.0f / 255.0f, 161.0f / 255.0f,  14.0f / 255.0f, 1.0f },
				{ 249.0f / 255.0f, 241.0f / 255.0f, 165.0f / 255.0f, 1.0f },
				{ 231.0f / 255.0f,  72.0f / 255.0f,  86.0f / 255.0f, 1.0f },
				{ 197.0f / 255.0f,  15.0f / 255.0f,  31.0f / 255.0f, 1.0f }
			};

			hyperengine::getConsole().addLog(fmt::to_string(formatted), colors[msg.level - spdlog::level::trace]);
		}

		void flush_() override {}
	};

	void setupLogger() {
		spdlog::default_logger()->sinks().push_back(std::make_shared<HyperEngineSink<std::mutex>>());
		spdlog::set_level(spdlog::level::trace);
	}
}

int main(int argc, char* argv[]) {
	TracySetProgramName("HyperEngine");
	setupLogger();
	hyperengine::rdoc::setup(true);
#if 0
	Engine engine;
	engine.run();
#else
	extern int vulkanMain();
	vulkanMain();
#endif

	spdlog::shutdown();
    return 0;
}