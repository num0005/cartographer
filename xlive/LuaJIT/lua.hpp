#pragma once
// lua.hpp
// Lua header files for C++
// <<extern "C">> not supplied automatically because Lua also compiles as C++

extern "C" {
	#include "LuaJIT\lua.h"
	#include "LuaJIT\lauxlib.h"
	#include "LuaJIT\lualib.h"
	#include "LuaJIT\luaconf.h"
}
