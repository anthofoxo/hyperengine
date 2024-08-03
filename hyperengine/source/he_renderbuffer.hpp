#pragma once

#include <utility>
#include <string_view>
#include <glad/gl.h>

#include "he_pixelformat.hpp"

namespace hyperengine {
	class Renderbuffer final {
	public:
		struct CreateInfo final {
			GLsizei width, height;
			PixelFormat format;
			std::string_view label;
		};
		constexpr Renderbuffer() noexcept = default;
		Renderbuffer(CreateInfo const& info);
		Renderbuffer(Renderbuffer const&) = delete;
		Renderbuffer& operator=(Renderbuffer const&) = delete;
		inline Renderbuffer(Renderbuffer&& other) noexcept { *this = std::move(other); }
		Renderbuffer& operator=(Renderbuffer&& other) noexcept;
		~Renderbuffer() noexcept;

		inline GLuint handle() const { return mHandle; }
	private:
		GLuint mHandle = 0;
	};
}