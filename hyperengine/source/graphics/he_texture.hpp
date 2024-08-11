#pragma once

#include <glm/glm.hpp>
#include <glad/gl.h>
#include <utility>
#include <string>
#include <string_view>

#include "he_pixelformat.hpp"

namespace hyperengine {
	class Texture final {
	public:
		enum struct FilterMode : GLenum {
			kNearest = GL_NEAREST,
			kLinear = GL_LINEAR,
			kNearestMipNearest = GL_NEAREST_MIPMAP_NEAREST,
			kLinearMipNearest = GL_LINEAR_MIPMAP_NEAREST,
			kNearestMipLinear = GL_NEAREST_MIPMAP_LINEAR,
			kLinearMipLinear = GL_LINEAR_MIPMAP_LINEAR
		};

		enum struct WrapMode : GLenum {
			kClampEdge = GL_CLAMP_TO_EDGE,
			kClampBorder = GL_CLAMP_TO_BORDER,
			kMirrorRepeat = GL_MIRRORED_REPEAT,
			kRepeat = GL_REPEAT
		};

		struct CreateInfo final {
			GLsizei width = 0, height = 0;
			PixelFormat format = PixelFormat::kRgba8;
			FilterMode minFilter = FilterMode::kLinear;
			FilterMode magFilter = FilterMode::kLinear;
			WrapMode wrap = WrapMode::kRepeat;
			glm::vec4 border = glm::vec4(1.0f);
			float anisotropy = 8.0f;
			std::string_view label;
			std::string_view origin;
		};

		struct UploadInfo final {
			GLint xoffset, yoffset;
			GLsizei width, height;
			PixelFormat format;
			void const* pixels = nullptr;
			bool mips = false;
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