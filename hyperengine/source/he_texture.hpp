#pragma once

#include <glad/gl.h>
#include <utility>

namespace hyperengine {
	class Texture final {
	public:
		struct CreateInfo final {
			GLint internalFormat;
			GLsizei width, height;
			GLenum format, type;
			void const* pixels;
			GLenum minFilter, magFilter;
			GLenum wrap;
		};

		constexpr Texture() noexcept = default;
		Texture(CreateInfo const& info);
		Texture(Texture const&) = delete;
		Texture& operator=(Texture const&) = delete;
		inline Texture(Texture&& other) noexcept { *this = std::move(other); }
		Texture& operator=(Texture&& other) noexcept;
		~Texture() noexcept;

		void bind(GLuint unit);
	private:
		GLuint mHandle = 0;
	};
}