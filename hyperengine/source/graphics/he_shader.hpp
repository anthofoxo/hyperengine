#pragma once

#include <unordered_map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <glad/gl.h>
#include <glm/glm.hpp>

namespace hyperengine {
	class ShaderProgram final {
	public:
		struct CreateInfo final {
			std::string_view source;
			std::string_view origin;
		};

		enum struct UniformType : GLenum {
			kSampler2D = GL_SAMPLER_2D,
			kFloat = GL_FLOAT,
			kVec2f = GL_FLOAT_VEC2,
			kVec3f = GL_FLOAT_VEC3,
			kVec4f = GL_FLOAT_VEC4,
			kMat2f = GL_FLOAT_MAT2,
			kMat3f = GL_FLOAT_MAT3,
			kMat4f = GL_FLOAT_MAT4,
		};

		struct Uniform final {
			GLint location = -1;
			UniformType type = static_cast<UniformType>(0);
			GLint offset = -1;
			GLint blockIndex = -1;
		};

		ShaderProgram() = default;
		ShaderProgram(CreateInfo const& info);
		ShaderProgram(ShaderProgram const&) = delete;
		ShaderProgram& operator=(ShaderProgram const&) = delete;
		inline ShaderProgram(ShaderProgram&& other) noexcept { *this = std::move(other); }
		ShaderProgram& operator=(ShaderProgram&& other) noexcept;
		~ShaderProgram() noexcept;

		inline std::string const& origin() const { return mOrigin; }
		inline bool cull() const { return mCull; }
		inline GLuint handle() const { return mHandle; }
		inline std::vector<std::string> const& errors() const { return mErrors; }
		inline auto const& opaqueAssignments() const { return mOpaqueAssignments; };
		inline auto const& materialInfo() const { return mMaterialInfo; };
		inline std::string_view editHint(std::string_view val) const { auto it = mEditHints.find(val); if (it == mEditHints.end()) return ""; else return it->second; };
		inline int materialAllocationSize() const { return mMaterialAllocationSize; }
		inline auto const& uniforms() const { return mUniforms; }

		Uniform getUniform(std::string_view name) const;
		GLint getUniformLocation(std::string_view name) const;
		void uniform1i(std::string_view name, int v0);
		void uniform1f(std::string_view name, float v0);
		void uniform2f(std::string_view name, glm::vec2 const& v0);
		void uniform3f(std::string_view name, glm::vec3 const& v0);
		void uniform4f(std::string_view name, glm::vec4 const& v0);
		void uniformMat4f(std::string_view name, glm::mat4 const& v0);

		void bind();
	private:
		struct Hash final {
			using hash_type = std::hash<std::string_view>;
			using is_transparent = void;
			std::size_t operator()(char const* str) const { return hash_type{}(str); }
			std::size_t operator()(std::string_view str) const { return hash_type{}(str); }
			std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
		};

		std::string mOrigin;
		GLuint mHandle = 0;
		std::unordered_map<std::string, Uniform, Hash, std::equal_to<>> mUniforms;
		std::unordered_map<std::string, Uniform, Hash, std::equal_to<>> mMaterialInfo;
		std::unordered_map<std::string, int, Hash, std::equal_to<>> mOpaqueAssignments;
		std::unordered_map<std::string, std::string, Hash, std::equal_to<>> mEditHints;
		std::vector<std::string> mErrors;
		int mMaterialAllocationSize = 0;
		bool mCull = true;
	};
}