#pragma once

#include <utility>
#include <GLFW/glfw3.h>

namespace hyperengine {
	class Window final {
	public:
		struct CreateInfo final {
			int width, height;
			char const* title;
		};

		constexpr Window() noexcept = default;
		Window(CreateInfo const& info);
		Window(Window const&) = delete;
		Window& operator=(Window const&) = delete;
		inline Window(Window&& other) noexcept { *this = std::move(other); }
		Window& operator=(Window&& other) noexcept;
		~Window() noexcept;

		inline GLFWwindow* handle() const { return mHandle; }

		void swapBuffers();
	private:
		GLFWwindow* mHandle = nullptr;
	};
}