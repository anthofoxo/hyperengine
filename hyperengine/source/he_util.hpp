#pragma once

#include <string>
#include <vector>

#include <lua.hpp>
#include <glm/glm.hpp>

namespace hyperengine {
	size_t split(std::string const& txt, std::vector<std::string>& strs, char ch);
	void luaDumpstack(lua_State* L);
	glm::vec3 luaToVec3(lua_State* L);
	glm::vec4 luaToVec4(lua_State* L);
}