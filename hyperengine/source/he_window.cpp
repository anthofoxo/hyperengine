#include "he_window.hpp"

#include "he_platform.hpp"
#include <cstdint>

namespace hyperengine {
	namespace {
		uint_fast16_t gWindowCount = 0;
	}

	Window::Window(CreateInfo const& info) {
		if (!gWindowCount) {
			if (hyperengine::isWsl())
				glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

			if (!glfwInit()) return;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

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
}