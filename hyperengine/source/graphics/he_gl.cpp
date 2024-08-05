#include "he_gl.hpp"

#include <iostream>

namespace hyperengine {
#define HE_IMPL_EXPAND(x) case x: return #x
	std::string_view glConstantToString(GLuint val) {
		switch (val) {
			HE_IMPL_EXPAND(GL_DEBUG_SOURCE_API);
			HE_IMPL_EXPAND(GL_DEBUG_SOURCE_WINDOW_SYSTEM);
			HE_IMPL_EXPAND(GL_DEBUG_SOURCE_SHADER_COMPILER);
			HE_IMPL_EXPAND(GL_DEBUG_SOURCE_THIRD_PARTY);
			HE_IMPL_EXPAND(GL_DEBUG_SOURCE_APPLICATION);
			HE_IMPL_EXPAND(GL_DEBUG_SOURCE_OTHER);

			HE_IMPL_EXPAND(GL_DEBUG_TYPE_ERROR);
			HE_IMPL_EXPAND(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR);
			HE_IMPL_EXPAND(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR);
			HE_IMPL_EXPAND(GL_DEBUG_TYPE_PORTABILITY);
			HE_IMPL_EXPAND(GL_DEBUG_TYPE_PERFORMANCE);
			HE_IMPL_EXPAND(GL_DEBUG_TYPE_MARKER);
			HE_IMPL_EXPAND(GL_DEBUG_TYPE_OTHER);

			HE_IMPL_EXPAND(GL_DEBUG_SEVERITY_NOTIFICATION);
			HE_IMPL_EXPAND(GL_DEBUG_SEVERITY_LOW);
			HE_IMPL_EXPAND(GL_DEBUG_SEVERITY_MEDIUM);
			HE_IMPL_EXPAND(GL_DEBUG_SEVERITY_HIGH);
		default: return "?";
		}
	}
#undef HE_IMPL_EXPAND

	void glMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param) {
		std::cout << glConstantToString(source) << ", " << glConstantToString(type) << ", " << glConstantToString(severity) << ", " << id << ": " << message << '\n';
		// hyperengine::breakpointIfDebugging();
	}

	namespace {
		bool gInfoAllocated = false;
		GlContextInfo gInfo;
	}

	GlContextInfo const& glContextInfo() {
		if (!gInfoAllocated) {
			gInfoAllocated = true;

			gInfo.renderer = (char const*)glGetString(GL_RENDERER);
			gInfo.vendor = (char const*)glGetString(GL_VENDOR);
			gInfo.version = (char const*)glGetString(GL_VERSION);
			gInfo.glslVersion = (char const*)glGetString(GL_SHADING_LANGUAGE_VERSION);

			if (GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic) {
				static_assert(GL_MAX_TEXTURE_MAX_ANISOTROPY == GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT);
				static_assert(GL_TEXTURE_MAX_ANISOTROPY == GL_TEXTURE_MAX_ANISOTROPY_EXT);
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &gInfo.maxAnisotropy);
			}
		}

		return gInfo;
	}
}

