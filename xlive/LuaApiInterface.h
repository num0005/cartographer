#pragma once
#include <string>
#include "LuaJIT\lua.hpp"

#define LUA_FUNC(name) extern "C" static int l_##name(lua_State *L)

#define CREATE_CALLBACK_TABLE(callback_name) \
lua_newtable(lua_state); \
lua_setglobal(lua_state, (callback_name));

#define ADD_FUNC(func) { #func, l_##func }

class LuaApiInterface
{
public:
	LuaApiInterface();
	~LuaApiInterface();
	void LoadScripts();
	inline bool running() { return is_running; }

	// callbacks
	inline void on_map_load(const std::string &mapname)
	{
		lua_pushstring(lua_state, mapname.c_str());
		run_callbacks(1, "map_load");
	}
protected:
	void loadScript(const std::string &path);
	void run_callbacks(int arg_count, const char *callback_name);
	lua_State *lua_state;
	inline void luaHalt() { is_running = false; }
private:
	bool is_running = false;
};

