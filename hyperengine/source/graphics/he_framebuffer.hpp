#pragma once

#include <glad/gl.h>
#include <utility>

#include "he_texture.hpp"
#include "he_renderbuffer.hpp"

#include <variant>
#include <functional>
#include <span>

namespace hyperengine {
	class Framebuffer final {
	public:
		struct Attachment final {
			GLenum attachment;
			std::variant<std::reference_wrapper<hyperengine::Texture>, std::reference_wrapper<hyperengine::Renderbuffer>> source;
		};

		struct CreateInfo final {
			std::span<const Attachment> attachments;
		};

		constexpr Framebuffer() noexcept = default;
		Framebuffer(CreateInfo const& info);
		Framebuffer(Framebuffer const&) = delete;
		Framebuffer& operator=(Framebuffer const&) = delete;
		inline Framebuffer(Framebuffer&& other) noexcept { *this = std::move(other); }
		Framebuffer& operator=(Framebuffer&& other) noexcept;
		~Framebuffer() noexcept;

		inline GLuint handle() const { return mHandle; }

		void bind();
	private:
		GLuint mHandle = 0;
	};
}