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
		if (GLAD_GL_ARB_direct_state_access) {
			glCreateTextures(GL_TEXTURE_2D, 1, &mHandle);
			glTextureParameteri(mHandle, GL_TEXTURE_MIN_FILTER, info.minFilter);
			glTextureParameteri(mHandle, GL_TEXTURE_MAG_FILTER, info.magFilter);
			glTextureParameteri(mHandle, GL_TEXTURE_WRAP_S, info.wrap);
			glTextureParameteri(mHandle, GL_TEXTURE_WRAP_T, info.wrap);
			glTextureStorage2D(mHandle, 1, pixelFormatToInternalFormat(info.format), info.width, info.height);
		}
		else {
			// save state
			GLint param;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &param);

			glGenTextures(1, &mHandle);
			glBindTexture(GL_TEXTURE_2D, mHandle);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, info.minFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, info.magFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, info.wrap);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, info.wrap);

			if (GLAD_GL_ARB_texture_storage)
				glTexStorage2D(GL_TEXTURE_2D, 1, pixelFormatToInternalFormat(info.format), info.width, info.height);
			else
				glTexImage2D(GL_TEXTURE_2D, 0, pixelFormatToInternalFormat(info.format), info.width, info.height, 0, pixelFormatToFormat(info.format), pixelFormatToType(info.format), nullptr);

			// restore state
			glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(param));
		}

		if (GLAD_GL_KHR_debug)
			glObjectLabel(GL_TEXTURE, mHandle, static_cast<GLsizei>(info.label.size()), info.label.data());

		mOrigin = std::string(info.origin);
	}

	Texture& Texture::operator=(Texture&& other) noexcept {
		std::swap(mHandle, other.mHandle);
		std::swap(mOrigin, other.mOrigin);
		return *this;
	}

	Texture::~Texture() noexcept {
		if (mHandle)
			glDeleteTextures(1, &mHandle);
	}

	void Texture::upload(UploadInfo const& info) {
		if (GLAD_GL_ARB_direct_state_access) {
			glTextureSubImage2D(mHandle, 0, info.xoffset, info.yoffset, info.width, info.height, pixelFormatToFormat(info.format), pixelFormatToType(info.format), info.pixels);
		}
		else {
			// save state
			GLint param;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &param);

			glBindTexture(GL_TEXTURE_2D, mHandle);
			glTexSubImage2D(GL_TEXTURE_2D, 0, info.xoffset, info.yoffset, info.width, info.height, pixelFormatToFormat(info.format), pixelFormatToType(info.format), info.pixels);

			// restore state
			glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(param));
		}
	}

	void Texture::bind(GLuint unit) {
		if (GLAD_GL_ARB_direct_state_access) {
			glBindTextureUnit(unit, mHandle);
		}
		else {
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_2D, mHandle);
		}
	}
}