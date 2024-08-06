#pragma once

#include <glad/gl.h>
#include <utility>

#include "he_texture.hpp"
#include "he_renderbuffer.hpp"

namespace hyperengine {
	class Framebuffer final {
	public:
		struct CreateInfo final {
		};

		constexpr Framebuffer() noexcept = default;
		Framebuffer(CreateInfo const& info);
		Framebuffer(Framebuffer const&) = delete;
		Framebuffer& operator=(Framebuffer const&) = delete;
		inline Framebuffer(Framebuffer&& other) noexcept { *this = std::move(other); }
		Framebuffer& operator=(Framebuffer&& other) noexcept;
		~Framebuffer() noexcept;

		inline GLuint handle() const { return mHandle; }

		void texture2D(GLenum attachment, hyperengine::Texture& texture);
		void renderbuffer(GLenum attachment, hyperengine::Renderbuffer& renderbuffer);

		void bind();
	private:
		GLuint mHandle = 0;
	};
}