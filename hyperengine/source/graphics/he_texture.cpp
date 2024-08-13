#include "he_texture.hpp"

#include "he_gl.hpp"
#include <glm/gtc/type_ptr.hpp>

#include <spdlog/spdlog.h>

namespace hyperengine {
	namespace {
		GLuint getBindingState(GLenum target) {
			GLuint param;

			if (target == GL_TEXTURE_2D)
				glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&param);
			else
				glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, (GLint*)&param);

			return param;
		}
	}

	Texture::Texture(CreateInfo const& info) {
		if (info.depth > 0)
			mTarget = GL_TEXTURE_2D_ARRAY;
		else
			mTarget = GL_TEXTURE_2D;		

		using enum FilterMode;
		int maxLevel = 0;
		float maxAnisotropy = 0.0f;
		
		bool const useAnisotropy = info.minFilter == kNearestMipNearest || info.minFilter == kLinearMipNearest || info.minFilter == kNearestMipLinear || info.minFilter == kLinearMipLinear;

		if (useAnisotropy) {
			maxLevel = static_cast<int>(glm::floor(glm::log2(static_cast<float>(glm::max(info.width, info.height)))));
			maxAnisotropy = glm::min(hyperengine::glContextInfo().maxAnisotropy, info.anisotropy);
		}

		if (GLAD_GL_ARB_direct_state_access) {
			glCreateTextures(mTarget, 1, &mHandle);
			glTextureParameteri(mHandle, GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(info.minFilter));
			glTextureParameteri(mHandle, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(info.magFilter));
			glTextureParameteri(mHandle, GL_TEXTURE_WRAP_S, static_cast<GLenum>(info.wrap));
			glTextureParameteri(mHandle, GL_TEXTURE_WRAP_T, static_cast<GLenum>(info.wrap));
			glTextureParameteri(mHandle, GL_TEXTURE_MAX_LEVEL, maxLevel);
			glTextureParameterfv(mHandle, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(info.border));
			if (maxAnisotropy > 0.0f)
				glTextureParameterf(mHandle, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);

			if (info.depth > 0)
				glTextureStorage3D(mHandle, maxLevel + 1, pixelFormatToInternalFormat(info.format), info.width, info.height, info.depth);
			else
				glTextureStorage2D(mHandle, maxLevel + 1, pixelFormatToInternalFormat(info.format), info.width, info.height);
		}
		else {
			// save state
			GLuint param = getBindingState(mTarget);

			glGenTextures(1, &mHandle);
			glBindTexture(mTarget, mHandle);
			glTexParameteri(mTarget, GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(info.minFilter));
			glTexParameteri(mTarget, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(info.magFilter));
			glTexParameteri(mTarget, GL_TEXTURE_WRAP_S, static_cast<GLenum>(info.wrap));
			glTexParameteri(mTarget, GL_TEXTURE_WRAP_T, static_cast<GLenum>(info.wrap));
			glTexParameteri(mTarget, GL_TEXTURE_MAX_LEVEL, maxLevel);
			glTexParameterfv(mTarget, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(info.border));
			if (maxAnisotropy > 0.0f)
				glTexParameterf(mTarget, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);

			if (GLAD_GL_ARB_texture_storage) {
				if (info.depth > 0)
					glTexStorage3D(mTarget, maxLevel + 1, pixelFormatToInternalFormat(info.format), info.width, info.height, info.depth);
				else
					glTexStorage2D(mTarget, maxLevel + 1, pixelFormatToInternalFormat(info.format), info.width, info.height);
			}
			else {
				if (info.depth > 0)
					glTexImage3D(mTarget, 0, pixelFormatToInternalFormat(info.format), info.width, info.height, info.depth, 0, pixelFormatToFormat(info.format), pixelFormatToType(info.format), nullptr);
				else
					glTexImage2D(mTarget, 0, pixelFormatToInternalFormat(info.format), info.width, info.height, 0, pixelFormatToFormat(info.format), pixelFormatToType(info.format), nullptr);
			}

			// restore state
			glBindTexture(mTarget, param);
		}

		if (GLAD_GL_KHR_debug && !info.label.empty())
			glObjectLabel(GL_TEXTURE, mHandle, static_cast<GLsizei>(info.label.size()), info.label.data());

		mOrigin = std::string(info.origin);
	}

	Texture& Texture::operator=(Texture&& other) noexcept {
		std::swap(mTarget, other.mTarget);
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
			if(info.depth > 0)
				glTextureSubImage3D(mHandle, 0, info.xoffset, info.yoffset, info.zoffset, info.width, info.height, info.depth, pixelFormatToFormat(info.format), pixelFormatToType(info.format), info.pixels);
			else
				glTextureSubImage2D(mHandle, 0, info.xoffset, info.yoffset, info.width, info.height, pixelFormatToFormat(info.format), pixelFormatToType(info.format), info.pixels);

			if (info.mips)
				glGenerateTextureMipmap(mHandle);
		}
		else {
			// save state
			GLuint param = getBindingState(mTarget);

			glBindTexture(mTarget, mHandle);

			if (info.depth > 0)
				glTexSubImage3D(mTarget, 0, info.xoffset, info.yoffset, info.zoffset, info.width, info.height, info.depth, pixelFormatToFormat(info.format), pixelFormatToType(info.format), info.pixels);
			else
				glTexSubImage2D(mTarget, 0, info.xoffset, info.yoffset, info.width, info.height, pixelFormatToFormat(info.format), pixelFormatToType(info.format), info.pixels);

			if (info.mips)
				glGenerateMipmap(mTarget);

			// restore state
			glBindTexture(mTarget, param);
		}
	}

	void Texture::bind(GLuint unit) {
		if (GLAD_GL_ARB_direct_state_access) {
			glBindTextureUnit(unit, mHandle);
		}
		else {
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(mTarget, mHandle);
		}
	}
}