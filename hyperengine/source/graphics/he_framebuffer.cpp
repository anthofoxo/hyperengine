#include "he_framebuffer.hpp"

#include "he_util.hpp"
#include "he_gl.hpp"

#include <spdlog/spdlog.h>

namespace hyperengine {
	Framebuffer::Framebuffer(CreateInfo const& info) {
		if (GLAD_GL_ARB_direct_state_access) {
			glCreateFramebuffers(1, &mHandle);

			for (auto const& texrb : info.attachments) {
				auto attachment = texrb.attachment;

				std::visit(hyperengine::Visitor{
					[this, attachment](hyperengine::Texture& v) { glNamedFramebufferTexture(mHandle, attachment, v.handle(), 0); },
					[this, attachment](hyperengine::Renderbuffer& v) { glNamedFramebufferRenderbuffer(mHandle, attachment, GL_RENDERBUFFER, v.handle()); },
				}, texrb.source);
			}

			auto status = glCheckNamedFramebufferStatus(mHandle, GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				spdlog::error("Incomplete framebuffer: {}", glConstantToString(status));
			}
		}
		else {
			GLuint state;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&state);

			glGenFramebuffers(1, &mHandle);
			glBindFramebuffer(GL_FRAMEBUFFER, mHandle);

			for (auto const& texrb : info.attachments) {
				auto attachment = texrb.attachment;

				std::visit(hyperengine::Visitor{
					[attachment](hyperengine::Texture& v) { glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, v.handle(), 0); },
					[attachment](hyperengine::Renderbuffer& v) { glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, v.handle()); },
				}, texrb.source);
			}

			auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				spdlog::error("Incomplete framebuffer: {}", glConstantToString(status));
			}

			glBindFramebuffer(GL_FRAMEBUFFER, state);
		}
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
}