#include "he_framebuffer.hpp"

namespace hyperengine {
	Framebuffer::Framebuffer(CreateInfo const& info) {
		if(GLAD_GL_ARB_direct_state_access)
			glCreateFramebuffers(1, &mHandle);
		else
			glGenFramebuffers(1, &mHandle);
	}

	Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
		std::swap(mHandle, other.mHandle);
		return *this;
	}

	Framebuffer::~Framebuffer() noexcept {
		if (mHandle)
			glDeleteFramebuffers(1, &mHandle);
	}

	void Framebuffer::bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, mHandle);
	}

	void Framebuffer::texture2D(GLenum attachment, hyperengine::Texture& texture) {
		if (GLAD_GL_ARB_direct_state_access)
			glNamedFramebufferTexture(mHandle, attachment, texture.handle(), 0);
		else {
			GLuint state;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&state);

			glBindFramebuffer(GL_FRAMEBUFFER, mHandle);
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture.handle(), 0);

			glBindFramebuffer(GL_FRAMEBUFFER, state);
		}
	}

	void Framebuffer::renderbuffer(GLenum attachment, hyperengine::Renderbuffer& renderbuffer) {
		if (GLAD_GL_ARB_direct_state_access)
			glNamedFramebufferRenderbuffer(mHandle, attachment, GL_RENDERBUFFER, renderbuffer.handle());
		else {
			GLuint state;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&state);

			glBindFramebuffer(GL_FRAMEBUFFER, mHandle);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer.handle());

			glBindFramebuffer(GL_FRAMEBUFFER, state);
		}
	}
}