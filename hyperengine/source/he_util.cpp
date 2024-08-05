#include "he_util.hpp"

namespace hyperengine {
	size_t split(std::string const& txt, std::vector<std::string>& strs, char ch) {
		size_t pos = txt.find(ch);
		size_t initialPos = 0;
		strs.clear();

		while (pos != std::string::npos) {
			strs.push_back(txt.substr(initialPos, pos - initialPos));
			initialPos = pos + 1;

			pos = txt.find(ch, initialPos);
		}

		strs.push_back(txt.substr(initialPos, std::min(pos, txt.size()) - initialPos + 1));

		return strs.size();
	}

	void luaDumpstack(lua_State* L) {
		int top = lua_gettop(L);
		for (int i = 1; i <= top; i++) {
			printf("%d\t%s\t", i, luaL_typename(L, i));
			switch (lua_type(L, i)) {
			case LUA_TNUMBER:
				printf("%g\n", lua_tonumber(L, i));
				break;
			case LUA_TSTRING:
				printf("%s\n", lua_tostring(L, i));
				break;
			case LUA_TBOOLEAN:
				printf("%s\n", (lua_toboolean(L, i) ? "true" : "false"));
				break;
			case LUA_TNIL:
				printf("%s\n", "nil");
				break;
			default:
				printf("%p\n", lua_topointer(L, i));
				break;
			}
		}
	}

	glm::vec3 luaToVec3(lua_State* L) {
		glm::vec3 result;

		for (uint_fast8_t i = 0; i < 3; ++i) {
			lua_pushinteger(L, i + 1);
			lua_gettable(L, -2);
			result[i] = static_cast<float>(lua_tonumber(L, -1));
			lua_pop(L, 1);
		}

		return result;
	}

	glm::vec4 luaToVec4(lua_State* L) {
		glm::vec4 result;

		for (uint_fast8_t i = 0; i < 4; ++i) {
			lua_pushinteger(L, i + 1);
			lua_gettable(L, -2);
			result[i] = static_cast<float>(lua_tonumber(L, -1));
			lua_pop(L, 1);
		}

		return result;
	}
}