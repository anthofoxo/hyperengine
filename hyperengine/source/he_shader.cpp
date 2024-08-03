#include "he_shader.hpp"

#include <regex>
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
#define CONST(type, name, value) const type name = value
#line 1
)";

	std::string_view gFragHeader = R"(#version 330 core
#define FRAG
#define INPUT(type, name, index)
#define OUTPUT(type, name, index) layout(location = index) out type name
#define VARYING(type, name) in type name
#define UNIFORM(type, name) uniform type name
#define CONST(type, name, value) const type name = value
#line 1
)";

	GLuint makeShader(GLenum type, GLchar const* string, GLint length, std::vector<std::string>& errors) {
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &string, &length);
		glCompileShader(shader);

		GLint param;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &param);

		if (param > 0) {
			std::string error;
			error.resize(param);
			glGetShaderInfoLog(shader, param, nullptr, error.data());
			errors.push_back(error);
		}

		return shader;
	}
}

namespace hyperengine {
	ShaderProgram::ShaderProgram(CreateInfo const& info) {
		std::string source = std::string(info.source);

		// Match engine pragmas
		{
			std::regex regex("@(\\w+)\\s+(\\w+)\\s*=\\s*(\\w+)");

			for (std::sregex_iterator i = std::sregex_iterator(source.begin(), source.end(), regex); i != std::sregex_iterator(); ++i) {
				std::smatch match = *i;

				if (match[1] == "property" && match[2] == "cull")
					mCull = std::stoi(match[3]);
			}
			source = std::regex_replace(source, regex, "//$&");
		}

		// Theres probably a better way to do this, look into it sometime plz
		std::string vertSource = std::string(gVertHeader) + source;
		std::string fragSource = std::string(gFragHeader) + source;
		//

		GLuint vert = makeShader(GL_VERTEX_SHADER, vertSource.data(), static_cast<int>(vertSource.size()), mErrors);
		GLuint frag = makeShader(GL_FRAGMENT_SHADER, fragSource.data(), static_cast<int>(fragSource.size()), mErrors);

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
			mErrors.push_back(error);
		}

		if (!mErrors.empty()) {
			for(auto const& error : mErrors)
				std::cerr << error << '\n';
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
		std::swap(mErrors, other.mErrors);
		std::swap(mCull, other.mCull);
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

	void ShaderProgram::uniform3f(std::string_view name, glm::vec3 const& v0) {
		bind();
		glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(v0));
	}

	void ShaderProgram::uniform1f(std::string_view name, float v0) {
		bind();
		glUniform1f(getUniformLocation(name), v0);
	}

	void ShaderProgram::uniformMat4f(std::string_view name, glm::mat4 const& v0) {
		bind();
		glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(v0));
	}

	void ShaderProgram::bind() {
		glUseProgram(mHandle);
	}
}