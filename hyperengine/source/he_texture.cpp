#include "he_texture.hpp"

#include <glm/glm.hpp>

namespace hyperengine {
	Texture::Texture(CreateInfo const& info) {
		int maxLevel = static_cast<int>(glm::floor(glm::log2(static_cast<float>(glm::max(info.width, info.height)))));

		float maxAnisotropy = 0;

		if (info.minFilter == GL_NEAREST_MIPMAP_NEAREST ||
			info.minFilter == GL_LINEAR_MIPMAP_NEAREST ||
			info.minFilter == GL_NEAREST_MIPMAP_LINEAR ||
			info.minFilter == GL_LINEAR_MIPMAP_LINEAR
			) {
			if (GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic) {
				// These constants should be the same
				static_assert(GL_MAX_TEXTURE_MAX_ANISOTROPY == GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT);
				static_assert(GL_TEXTURE_MAX_ANISOTROPY == GL_TEXTURE_MAX_ANISOTROPY_EXT);
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);
			}
		}
		else {
			maxLevel = 0;
		}

		if (GLAD_GL_ARB_direct_state_access) {
			glCreateTextures(GL_TEXTURE_2D, 1, &mHandle);
			glTextureParameteri(mHandle, GL_TEXTURE_MIN_FILTER, info.minFilter);
			glTextureParameteri(mHandle, GL_TEXTURE_MAG_FILTER, info.magFilter);
			glTextureParameteri(mHandle, GL_TEXTURE_WRAP_S, info.wrap);
			glTextureParameteri(mHandle, GL_TEXTURE_WRAP_T, info.wrap);
			glTextureParameteri(mHandle, GL_TEXTURE_MAX_LEVEL, maxLevel);
			if(maxAnisotropy > 0.0f)
				glTextureParameterf(mHandle, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);
			glTextureStorage2D(mHandle, maxLevel + 1, pixelFormatToInternalFormat(info.format), info.width, info.height);
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
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxLevel);
			if (maxAnisotropy > 0.0f)
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);

			if (GLAD_GL_ARB_texture_storage)
				glTexStorage2D(GL_TEXTURE_2D, maxLevel + 1, pixelFormatToInternalFormat(info.format), info.width, info.height);
			else
				glTexImage2D(GL_TEXTURE_2D, 0, pixelFormatToInternalFormat(info.format), info.width, info.height, 0, pixelFormatToFormat(info.format), pixelFormatToType(info.format), nullptr);

			// restore state
			glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(param));
		}

		if (GLAD_GL_KHR_debug && !info.label.empty())
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

			if (info.mips)
				glGenerateTextureMipmap(mHandle);
		}
		else {
			// save state
			GLint param;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &param);

			glBindTexture(GL_TEXTURE_2D, mHandle);
			glTexSubImage2D(GL_TEXTURE_2D, 0, info.xoffset, info.yoffset, info.width, info.height, pixelFormatToFormat(info.format), pixelFormatToType(info.format), info.pixels);

			if (info.mips)
				glGenerateMipmap(GL_TEXTURE_2D);

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