#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <utility>

#include <lua.hpp>
#include <glm/glm.hpp>

namespace hyperengine {
	template<class... Callable>
	struct Visitor : Callable... {
		using Callable::operator()...;
	};

#if 0
	// Example usage of this variant pattern
	using GlTypeVarient = std::variant<float, glm::vec2, glm::vec3, glm::vec4>;

	void editUniformGui(char const* label, GlTypeVarient& variant) {
		std::visit(hyperengine::Visitor{
			[label](float& v)     { ImGui::DragFloat (label, &v); },
			[label](glm::vec2& v) { ImGui::DragFloat2(label, glm::value_ptr(v)); },
			[label](glm::vec3& v) { ImGui::DragFloat3(label, glm::value_ptr(v)); },
			[label](glm::vec4& v) { ImGui::DragFloat4(label, glm::value_ptr(v)); },
		}, variant);
	}
#endif

	namespace {
		struct Hash final {
			using hash_type = std::hash<std::string_view>;
			using is_transparent = void;
			std::size_t operator()(char const* str) const { return hash_type{}(str); }
			std::size_t operator()(std::string_view str) const { return hash_type{}(str); }
			std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
		};
	}

	template<class V>
	using UnorderedStringMap = std::unordered_map<std::string, V, Hash, std::equal_to<>>;

	size_t split(std::string const& txt, std::vector<std::string>& strs, char ch);
	void luaDumpstack(lua_State* L);
	glm::vec2 luaToVec2(lua_State* L);
	glm::vec3 luaToVec3(lua_State* L);
	glm::vec4 luaToVec4(lua_State* L);
}