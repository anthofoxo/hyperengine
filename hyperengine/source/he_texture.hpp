#pragma once

#include <glad/gl.h>
#include <utility>
#include <string>
#include <string_view>

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
			std::string_view label;
			std::string_view origin;
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

		inline std::string const& origin() const { return mOrigin; }
		inline GLuint handle() const { return mHandle; }

		void upload(UploadInfo const& info);
		void bind(GLuint unit);
	private:
		std::string mOrigin;
		GLuint mHandle = 0;
	};
}