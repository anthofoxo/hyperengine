#pragma once

#include <glad/gl.h>
#include <string_view>

namespace hyperengine {
	struct GlContextInfo final {
		std::string renderer;
		std::string vendor;
		std::string version;
		std::string glslVersion;
		float maxAnisotropy = 0.0f;
	};

	std::string_view glConstantToString(GLuint val);
	void glMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param);
	GlContextInfo const& glContextInfo();
}