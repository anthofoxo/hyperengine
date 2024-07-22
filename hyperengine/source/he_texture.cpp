#include "he_texture.hpp"

namespace hyperengine {
	namespace {
		GLint pixelFormatToInternalFormat(PixelFormat format) {
			using enum PixelFormat;

			switch (format) {
			case kRgba8: return GL_RGBA8;
			default: std::unreachable();
			}
		}

		GLenum pixelFormatToFormat(PixelFormat format) {
			using enum PixelFormat;

			switch (format) {
			case kRgba8: return GL_RGBA;
			default: std::unreachable();
			}
		}

		GLenum pixelFormatToType(PixelFormat format) {
			using enum PixelFormat;

			switch (format) {
			case kRgba8: return GL_UNSIGNED_BYTE;
			default: std::unreachable();
			}
		}
	}

	Texture::Texture(CreateInfo const& info) {
		glGenTextures(1, &mHandle);
		glBindTexture(GL_TEXTURE_2D, mHandle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, info.minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, info.magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, info.wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, info.wrap);
		glTexImage2D(GL_TEXTURE_2D, 0, pixelFormatToInternalFormat(info.format), info.width, info.height, 0, pixelFormatToFormat(info.format), pixelFormatToType(info.format), nullptr);
	}

	Texture& Texture::operator=(Texture&& other) noexcept {
		std::swap(mHandle, other.mHandle);
		return *this;
	}

	Texture::~Texture() noexcept {
		if (mHandle)
			glDeleteTextures(1, &mHandle);
	}

	void Texture::upload(UploadInfo const& info) {
		glBindTexture(GL_TEXTURE_2D, mHandle);
		glTexSubImage2D(GL_TEXTURE_2D, 0, info.xoffset, info.yoffset, info.width, info.height, pixelFormatToFormat(info.format), pixelFormatToType(info.format), info.pixels);
	}

	void Texture::bind(GLuint unit) {
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, mHandle);
	}
}