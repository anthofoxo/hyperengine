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
#line 1
)";

	std::string_view gFragHeader = R"(#version 330 core
#define FRAG
#define INPUT(type, name, index)
#define OUTPUT(type, name, index) layout(location = index) out type name
#define VARYING(type, name) in type name
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

				if (match[1] == "edithint")
					mEditHints[match[2]] = match[3];
			}
			source = std::regex_replace(source, regex, "// ENGINE PRAGMA APPLIED // $&");
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

		GLint state;
		glGetIntegerv(GL_CURRENT_PROGRAM, &state);
		glUseProgram(mHandle);

		// Read all uniforms ahead of time, no need to constantly look these up every frame
		GLuint uniformCount;
		glGetProgramiv(mHandle, GL_ACTIVE_UNIFORMS, reinterpret_cast<GLint*>(&uniformCount));

		if (uniformCount != 0) {
			int opaqueAssignment = 0;

			GLint maxNameLength;
			glGetProgramiv(mHandle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
			std::unique_ptr<char[]> uniformName = std::make_unique<char[]>(maxNameLength);

			for (GLuint i = 0; i < uniformCount; ++i) {
				GLsizei length;
				GLsizei count;
				GLenum type;
				
				glGetActiveUniform(mHandle, i, maxNameLength, &length, &count, &type, uniformName.get());

				GLint location = glGetUniformLocation(mHandle, uniformName.get());

				// This uniform is not used or is part of a uniform block
				if (location != -1) {
					GLint blockIndex;
					glGetActiveUniformsiv(mHandle, 1, &i, GL_UNIFORM_BLOCK_INDEX, &blockIndex);

					GLint offset;
					glGetActiveUniformsiv(mHandle, 1, &i, GL_UNIFORM_OFFSET, &offset);

					mUniforms.insert(std::make_pair(std::string(uniformName.get(), length), Uniform(location, UniformType(type), offset, blockIndex)));

					// Automatically assign opaques
					if (type == GL_SAMPLER_2D) {
						glUniform1i(location, opaqueAssignment);
						mOpaqueAssignments[std::string(uniformName.get(), length)] = opaqueAssignment;
						++opaqueAssignment;
					}
				}

				
			}
		}

		GLuint blockCount;
		glGetProgramiv(mHandle, GL_ACTIVE_UNIFORM_BLOCKS, reinterpret_cast<GLint*>(&blockCount));

		if (blockCount != 0) {
			GLint maxNameLength;
			glGetProgramiv(mHandle, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &maxNameLength);
			std::unique_ptr<char[]> blockName = std::make_unique<char[]>(maxNameLength);


			for (GLuint i = 0; i < blockCount; ++i) {
				GLsizei length;
				
				glGetActiveUniformBlockName(mHandle, i, maxNameLength, &length, blockName.get());

				if (std::string_view(blockName.get()) == "EngineData") {
					glUniformBlockBinding(mHandle, i, 0);
				}

				if (std::string_view(blockName.get()) == "Material") {
					glUniformBlockBinding(mHandle, i, 1);

					GLint memorySize;
					glGetActiveUniformBlockiv(mHandle, i, GL_UNIFORM_BLOCK_DATA_SIZE, &memorySize);
					mMaterialAllocationSize = memorySize;

					GLuint activeUniformCount;
					glGetActiveUniformBlockiv(mHandle, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, reinterpret_cast<GLint*>(&activeUniformCount));

					std::vector<GLuint> activeUniforms;
					activeUniforms.resize(activeUniformCount);
					glGetActiveUniformBlockiv(mHandle, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, (GLint*)activeUniforms.data());

					GLint uniformMaxNameLength;
					glGetProgramiv(mHandle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformMaxNameLength);
					std::unique_ptr<char[]> uniformName = std::make_unique<char[]>(uniformMaxNameLength);

					for (GLuint iUniform = 0; iUniform < activeUniformCount; ++iUniform) {
						GLsizei length;
						GLsizei count;
						GLenum type;
						glGetActiveUniform(mHandle, activeUniforms[iUniform], uniformMaxNameLength, &length, &count, &type, uniformName.get());

						GLint offset;
						glGetActiveUniformsiv(mHandle, 1, &activeUniforms[iUniform], GL_UNIFORM_OFFSET, &offset);


						mMaterialInfo[std::string(uniformName.get(), length)] = { .location = 0, .type = UniformType(type), .offset = offset, .blockIndex = -1 };
					}
				}

				
				
			}
		}

		glUseProgram(static_cast<GLuint>(state));
		mOrigin = std::string(info.origin);
	}

	ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
		std::swap(mOrigin, other.mOrigin);
		std::swap(mHandle, other.mHandle);
		std::swap(mUniforms, other.mUniforms);
		std::swap(mErrors, other.mErrors);
		std::swap(mCull, other.mCull);
		std::swap(mOpaqueAssignments, other.mOpaqueAssignments);
		std::swap(mMaterialInfo, other.mMaterialInfo);
		std::swap(mMaterialAllocationSize, other.mMaterialAllocationSize);
		std::swap(mEditHints, other.mEditHints);
		return *this;
	}

	ShaderProgram::~ShaderProgram() noexcept {
		if (mHandle) {
			glDeleteProgram(mHandle);
		}
	}

	ShaderProgram::Uniform ShaderProgram::getUniform(std::string_view name) const {
		auto it = mUniforms.find(name);
		if (it == mUniforms.end()) return Uniform();
		return it->second;
	}

	GLint ShaderProgram::getUniformLocation(std::string_view name) const {
		auto it = mUniforms.find(name);
		if (it == mUniforms.end()) return -1;
		return it->second.location;
	}

	void ShaderProgram::uniform2f(std::string_view name, glm::vec2 const& v0) {
		bind();
		glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(v0));
	}

	void ShaderProgram::uniform3f(std::string_view name, glm::vec3 const& v0) {
		bind();
		glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(v0));
	}

	void ShaderProgram::uniform4f(std::string_view name, glm::vec4 const& v0) {
		bind();
		glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(v0));
	}

	void ShaderProgram::uniform1i(std::string_view name, int v0) {
		bind();
		glUniform1i(getUniformLocation(name), v0);
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