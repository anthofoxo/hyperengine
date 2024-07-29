#include "he_shader.hpp"

#include <iostream>
#include <debug_trap.h>
#include <glm/gtc/type_ptr.hpp>

namespace {
	std::string_view gVertHeader = R"(#version 330 core
#define VERT
#define INPUT(type, name, index) layout(location = index) in type name
#define OUTPUT(type, name, index)
#define VARYING(type, name) out type name
#define UNIFORM(type, name) uniform type name
#line 1
)";

	std::string_view gFragHeader = R"(#version 330 core
#define FRAG
#define INPUT(type, name, index)
#define OUTPUT(type, name, index) layout(location = index) out type name
#define VARYING(type, name) in type name
#define UNIFORM(type, name) uniform type name
#line 1
)";

	GLuint makeShader(GLenum type, GLchar const* string, GLint length) {
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &string, &length);
		glCompileShader(shader);

		GLint param;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &param);

		if (param > 0) {
			std::string error;
			error.resize(param);
			glGetShaderInfoLog(shader, param, nullptr, error.data());
			std::cerr << error << '\n';
			psnip_trap();
		}

		return shader;
	}
}

namespace hyperengine {
	ShaderProgram::ShaderProgram(CreateInfo const& info) {
		// Theres probably a better way to do this, look into it sometime plz
		std::string vertSource = std::string(gVertHeader) + std::string(info.source);
		std::string fragSource = std::string(gFragHeader) + std::string(info.source);
		//

		GLuint vert = makeShader(GL_VERTEX_SHADER, vertSource.data(), static_cast<int>(vertSource.size()));
		GLuint frag = makeShader(GL_FRAGMENT_SHADER, fragSource.data(), static_cast<int>(fragSource.size()));

		mHandle = glCreateProgram();
		glAttachShader(mHandle, vert);
		glAttachShader(mHandle, frag);
		glLinkProgram(mHandle);
		glDetachShader(mHandle, vert);
		glDetachShader(mHandle, frag);
		glDeleteShader(vert);
		glDeleteShader(frag);

		GLint param;
		glGetProgramiv(mHandle, GL_INFO_LOG_LENGTH, &param);

		if (param > 0) {
			std::string error;
			error.resize(param);
			glGetProgramInfoLog(mHandle, param, nullptr, error.data());
			std::cerr << error << '\n';
			psnip_trap();
		}

		// Read all uniforms ahead of time, no need to constantly look these up every frame
		GLint uniformCount;
		glGetProgramiv(mHandle, GL_ACTIVE_UNIFORMS, &uniformCount);

		if (uniformCount != 0) {
			GLint maxNameLength;
			glGetProgramiv(mHandle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
			std::unique_ptr<char[]> uniformName = std::make_unique<char[]>(maxNameLength);

			for (GLint i = 0; i < uniformCount; ++i) {
				GLsizei length;
				GLsizei count;
				GLenum type;
				glGetActiveUniform(mHandle, i, maxNameLength, &length, &count, &type, uniformName.get());

				GLint location = glGetUniformLocation(mHandle, uniformName.get());
				mUniforms.insert(std::make_pair(std::string(uniformName.get(), length), location));
			}
		}
	}

	ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
		std::swap(mHandle, other.mHandle);
		std::swap(mUniforms, other.mUniforms);
		return *this;
	}

	ShaderProgram::~ShaderProgram() noexcept {
		if (mHandle) {
			glDeleteProgram(mHandle);
		}
	}

	GLint ShaderProgram::getUniformLocation(std::string_view name) const {
		auto it = mUniforms.find(name);
		if (it == mUniforms.end()) return -1;
		return it->second;
	}

	void ShaderProgram::uniformMat4f(std::string_view name, glm::mat4 const& v0) {
		bind();
		glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(v0));
	}

	void ShaderProgram::bind() {
		glUseProgram(mHandle);
	}
}