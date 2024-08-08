#include "he_window.hpp"

#include <tracy/Tracy.hpp>

#include "he_platform.hpp"
#include <cstdint>
#include <spdlog/spdlog.h>

namespace hyperengine {
	namespace {
		uint_fast16_t gWindowCount = 0;

		void errorCallback(int error, char const* description) {
			spdlog::error("GLFW Error {}: {}", error, description);
		}
	}

	Window::Window(CreateInfo const& info) {
		ZoneScoped;

		if (!gWindowCount) {
			glfwSetErrorCallback(&errorCallback);

			if (hyperengine::isWsl())
				glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

			if (!glfwInit()) return;
		}

#ifdef _DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
		glfwWindowHint(GLFW_MAXIMIZED, info.maximized);

		mHandle = glfwCreateWindow(info.width, info.height, info.title, nullptr, nullptr);

		if (!mHandle && !gWindowCount) {
			glfwTerminate();
			return;
		}

		++gWindowCount;
	}

	Window& Window::operator=(Window&& other) noexcept {
		std::swap(mHandle, other.mHandle);
		return *this;
	}

	Window::~Window() noexcept {
		if (mHandle) {
			glfwMakeContextCurrent(nullptr);
			glfwDestroyWindow(mHandle);

			if (!--gWindowCount)
				glfwTerminate();
		}
	}

	void Window::swapBuffers() {
		ZoneScoped;
		glfwSwapBuffers(mHandle);
	}
}