#pragma once

#include <glad/gl.h>
#include <utility>

namespace hyperengine {
	enum struct PixelFormat {
		kRgba8
	};

	class Texture final {
	public:
		struct CreateInfo final {
			GLsizei width, height;
			PixelFormat format;
			GLenum minFilter, magFilter;
			GLenum wrap;
		};

		struct UploadInfo final {
			GLint xoffset, yoffset;
			GLsizei width, height;
			PixelFormat format;
			void const* pixels;
		};

		constexpr Texture() noexcept = default;
		Texture(CreateInfo const& info);
		Texture(Texture const&) = delete;
		Texture& operator=(Texture const&) = delete;
		inline Texture(Texture&& other) noexcept { *this = std::move(other); }
		Texture& operator=(Texture&& other) noexcept;
		~Texture() noexcept;

		void upload(UploadInfo const& info);
		void bind(GLuint unit);
	private:
		GLuint mHandle = 0;
	};
}