#include "he_texture.hpp"

#include "he_gl.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace hyperengine {
	Texture::Texture(CreateInfo const& info) {
		using enum FilterMode;
		int maxLevel = 0;
		float maxAnisotropy = 0.0f;
		
		bool const useAnisotropy = info.minFilter == kNearestMipNearest || info.minFilter == kLinearMipNearest || info.minFilter == kNearestMipLinear || info.minFilter == kLinearMipLinear;

		if (useAnisotropy) {
			maxLevel = static_cast<int>(glm::floor(glm::log2(static_cast<float>(glm::max(info.width, info.height)))));
			maxAnisotropy = glm::min(hyperengine::glContextInfo().maxAnisotropy, info.anisotropy);
		}

		if (GLAD_GL_ARB_direct_state_access) {
			glCreateTextures(GL_TEXTURE_2D, 1, &mHandle);
			glTextureParameteri(mHandle, GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(info.minFilter));
			glTextureParameteri(mHandle, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(info.magFilter));
			glTextureParameteri(mHandle, GL_TEXTURE_WRAP_S, static_cast<GLenum>(info.wrap));
			glTextureParameteri(mHandle, GL_TEXTURE_WRAP_T, static_cast<GLenum>(info.wrap));
			glTextureParameteri(mHandle, GL_TEXTURE_MAX_LEVEL, maxLevel);
			glTextureParameterfv(mHandle, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(info.border));
			if(maxAnisotropy > 0.0f)
				glTextureParameterf(mHandle, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);
			glTextureStorage2D(mHandle, maxLevel + 1, pixelFormatToInternalFormat(info.format), info.width, info.height);
		}
		else {
			// save state
			GLuint param;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&param);

			glGenTextures(1, &mHandle);
			glBindTexture(GL_TEXTURE_2D, mHandle);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(info.minFilter));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(info.magFilter));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLenum>(info.wrap));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLenum>(info.wrap));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxLevel);
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(info.border));
			if (maxAnisotropy > 0.0f)
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);

			if (GLAD_GL_ARB_texture_storage)
				glTexStorage2D(GL_TEXTURE_2D, maxLevel + 1, pixelFormatToInternalFormat(info.format), info.width, info.height);
			else
				glTexImage2D(GL_TEXTURE_2D, 0, pixelFormatToInternalFormat(info.format), info.width, info.height, 0, pixelFormatToFormat(info.format), pixelFormatToType(info.format), nullptr);

			// restore state
			glBindTexture(GL_TEXTURE_2D, param);
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
			GLuint param;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&param);

			glBindTexture(GL_TEXTURE_2D, mHandle);
			glTexSubImage2D(GL_TEXTURE_2D, 0, info.xoffset, info.yoffset, info.width, info.height, pixelFormatToFormat(info.format), pixelFormatToType(info.format), info.pixels);

			if (info.mips)
				glGenerateMipmap(GL_TEXTURE_2D);

			// restore state
			glBindTexture(GL_TEXTURE_2D, param);
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