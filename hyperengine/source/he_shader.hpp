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
		};

		ShaderProgram() = default;
		ShaderProgram(CreateInfo const& info);
		ShaderProgram(ShaderProgram const&) = delete;
		ShaderProgram& operator=(ShaderProgram const&) = delete;
		inline ShaderProgram(ShaderProgram&& other) noexcept { *this = std::move(other); }
		ShaderProgram& operator=(ShaderProgram&& other) noexcept;
		~ShaderProgram() noexcept;

		inline bool cull() const { return mCull; }
		inline GLuint handle() const { return mHandle; }
		inline std::vector<std::string> const& errors() const { return mErrors; }

		GLint getUniformLocation(std::string_view name) const;
		void uniform1f(std::string_view name, float v0);
		void uniform3f(std::string_view name, glm::vec3 const& v0);
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

		GLuint mHandle = 0;
		std::unordered_map<std::string, int, Hash, std::equal_to<>> mUniforms;
		std::vector<std::string> mErrors;
		bool mCull = true;
	};
}