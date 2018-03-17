#include <stdafx.h>
#include "LuaApiInterface.h"
#include "xliveless.h"
#include <string>
#include <filesystem>
#include "H2MOD.h"
#include "H2Config.h"

namespace fs = std::experimental::filesystem;

extern bool isHost;

static void stackDump(lua_State *L) {
#if _DEBUG
	int i = lua_gettop(L);
	lua_logger->log(" --------  Stack Dump --------");
	while (i + 1) {
		int t = lua_type(L, i);
		std::string output = std::to_string(i) + ":";
		switch (t) {
		case LUA_TSTRING:
			output = output + lua_tostring(L, i);
			break;
		case LUA_TBOOLEAN:
			output += lua_toboolean(L, i) ? "true" : "false";
			break;
		case LUA_TNUMBER:
			output += std::to_string(lua_tonumber(L, i));
			break;
		default:
			output += lua_typename(L, t);
			break;
		}
		lua_logger->log(output);
		i--;
	}
	lua_logger->log("-------- Stack Dump Finished --------");
#endif
}

// redirect print to log file
LUA_FUNC(custom_print) {
	int nargs = lua_gettop(L);
	std::string output;
	for (int index = 1; index <= nargs; index++) {
		if (index != 1)
			output += "\t";
		lua_getglobal(L, "tostring");
		lua_pushvalue(L, index);
		lua_call(L, 1, 1);
		output += lua_tostring(L, -1);
		lua_pop(L, 1);
	}
	lua_logger->log("[Lua Print] " + output);
	return 0;
}

static const struct luaL_Reg custom_print[] = {
	{ "print", l_custom_print },
	{ NULL, NULL } /* end of array */
};

LUA_FUNC(is_dedi) {
	lua_pushboolean(L, h2mod->Server);
	return 1;
}

LUA_FUNC(is_host) {
	lua_pushboolean(L, isHost);
	return 1;
}

LUA_FUNC(get_map_name) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> wstring_to_string;

	std::wstring map_name_wide = (wchar_t*)(h2mod->GetBase() + 0x46DAE8);
	std::string map_name_narrow = wstring_to_string.to_bytes(map_name_wide);
	lua_pushstring(L, map_name_narrow.c_str());
	return 1;
}

LUA_FUNC(get_variant_name) {
	std::string variant_name = getVariantName();
	lua_pushstring(L, variant_name.c_str());
	return 1;
}

LUA_FUNC(register) {
	lua_getglobal(L, luaL_checkstring(L, -2));
	size_t len = lua_objlen(L, -1);
	lua_pushvalue(L, -2);
	lua_rawseti(L, -2, ++len);
	return 0;
}

static const struct luaL_Reg halo_api[] = {
	ADD_FUNC(is_dedi),
	ADD_FUNC(is_host),
	ADD_FUNC(get_map_name),
	ADD_FUNC(register),
	ADD_FUNC(get_variant_name),
	{ NULL, NULL } /* end of array */
};

LUA_FUNC(get_version) {
	lua_pushliteral(L, DLL_VERSION_STR);
	return 1;
}

static const struct luaL_Reg carto_api[] = {
	ADD_FUNC(get_version),
	{ NULL, NULL } /* end of array */
};


static const luaL_Reg lualibs[] = {
	{ "", luaopen_base },
	{ LUA_TABLIBNAME, luaopen_table },
	{ LUA_STRLIBNAME, luaopen_string },
	{ LUA_MATHLIBNAME, luaopen_math },
	{ LUA_DBLIBNAME, luaopen_debug },
	{ NULL, NULL }
};

LuaApiInterface::LuaApiInterface()
{
	lua_state = luaL_newstate();
	const luaL_Reg *lib = lualibs;
	for (; lib->func; lib++) {
		lua_pushcfunction(lua_state, lib->func);
		lua_pushstring(lua_state, lib->name);
		lua_call(lua_state, 1, 0);
	}

	lua_getglobal(lua_state, "_G");
	luaL_register(lua_state, NULL, custom_print);
	lua_pop(lua_state, 2); // pop the two refs to _G

	luaL_register(lua_state, "halo_2", halo_api);
	lua_pop(lua_state, 1); // pop the ref to halo_2

	luaL_register(lua_state, "cartograpger", carto_api);
	lua_pop(lua_state, 1); // pop the ref to halo_2

	CREATE_CALLBACK_TABLE("map_load");
	is_running = true;
}


LuaApiInterface::~LuaApiInterface()
{
	lua_close(lua_state);
}

/*  Runs callbacks defined by user code in lua
Expects the arguments that are being passed to the callback to be on top of the stack
All passed arguments are removed from the stack
*/
void LuaApiInterface::run_callbacks(int arg_count, const char *callback_name) {
	int arg_start = lua_gettop(lua_state);
	lua_getglobal(lua_state, "debug");
	lua_getfield(lua_state, -1, "traceback");
	int traceback = lua_gettop(lua_state);

	lua_getglobal(lua_state, callback_name);
	lua_pushnil(lua_state);
	while (lua_next(lua_state, -2)) {
		// push function
		lua_pushvalue(lua_state, lua_gettop(lua_state));

		// push args
		for (int i = arg_count; i > 0; i--) {
			lua_pushvalue(lua_state, arg_start);
		}
		if (lua_pcall(lua_state, arg_count, 0, traceback)) {
			lua_logger->log("[ERROR] runtime error!");
			lua_logger->log("Lua error output: " + std::string(lua_tostring(lua_state, -1)));
			lua_pop(lua_state, 1); // pop the error
			is_running = false;
			return;
		}
		lua_pop(lua_state, 1);
	}

	// pop error handler and args
	lua_pop(lua_state, 3 + arg_count);
}

void LuaApiInterface::loadScript(const std::string &script_path)
{
	if (!is_running)
		return;

	lua_logger->log("Loading script: \"" + script_path + "\"");

	lua_getglobal(lua_state, "debug");
	lua_getfield(lua_state, -1, "traceback");
	int traceback = lua_gettop(lua_state);

	switch (luaL_loadfile(lua_state, script_path.c_str())) {
		case LUA_ERRMEM:
			lua_logger->log("[ERROR] memory allocation error.");
			is_running = false;
			break;
		case LUA_ERRSYNTAX: {
			lua_logger->log("[ERROR] syntax error while loading file");
			lua_logger->log("Lua error output: " + std::string(lua_tostring(lua_state, -1)));
			lua_pop(lua_state, 1); // pop the error
			is_running = false;
			break;
		}
		default: {
			if (lua_pcall(lua_state, 0, 0, traceback)) {
				lua_logger->log("[ERROR] runtime error!");
				lua_logger->log("Lua error output: " + std::string(lua_tostring(lua_state, -1)));
				lua_pop(lua_state, 1); // pop the error
				is_running = false;
			}

		}
	}

	lua_pop(lua_state, 1); // pop debug.traceback
}

void LuaApiInterface::LoadScripts()
{
	for (auto &p : fs::directory_iterator("scripts")) {
		std::string path = p.path().string();
		if (fs::is_regular_file(p) && path.substr(max(4, path.size()) - 4) == ".lua")
			loadScript(path);
	}
}
