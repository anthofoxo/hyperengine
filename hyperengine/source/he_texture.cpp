#include "he_texture.hpp"

namespace hyperengine {
	Texture::Texture(CreateInfo const& info) {
		glGenTextures(1, &mHandle);
		glBindTexture(GL_TEXTURE_2D, mHandle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, info.minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, info.magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, info.wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, info.wrap);
		glTexImage2D(GL_TEXTURE_2D, 0, info.internalFormat, info.width, info.height, 0, info.format, info.type, info.pixels);
	}

	Texture& Texture::operator=(Texture&& other) noexcept {
		std::swap(mHandle, other.mHandle);
		return *this;
	}

	Texture::~Texture() noexcept {
		if (mHandle)
			glDeleteTextures(1, &mHandle);
	}

	void Texture::bind(GLuint unit) {
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, mHandle);
	}
}