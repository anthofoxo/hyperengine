#include "he_renderbuffer.hpp"

namespace hyperengine {
	Renderbuffer::Renderbuffer(CreateInfo const& info) {
		if (GLAD_GL_ARB_direct_state_access) {
			glCreateRenderbuffers(1, &mHandle);
			glNamedRenderbufferStorage(mHandle, pixelFormatToInternalFormat(info.format), info.width, info.height);
		}
		else {
			GLint param;
			glGetIntegerv(GL_RENDERBUFFER_BINDING, &param);
			glGenRenderbuffers(1, &mHandle);
			glBindRenderbuffer(GL_RENDERBUFFER, mHandle);
			glRenderbufferStorage(GL_RENDERBUFFER, pixelFormatToInternalFormat(info.format), info.width, info.height);
			glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(param));
		}

		if (GLAD_GL_KHR_debug)
			glObjectLabel(GL_RENDERBUFFER, mHandle, static_cast<GLsizei>(info.label.size()), info.label.data());
	}

	Renderbuffer& Renderbuffer::operator=(Renderbuffer&& other) noexcept {
		std::swap(mHandle, other.mHandle);
		return *this;
	}

	Renderbuffer::~Renderbuffer() noexcept {
		if (mHandle)
			glDeleteRenderbuffers(1, &mHandle);
	}
}