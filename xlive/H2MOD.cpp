#include <stdafx.h>
#include <windows.h>
#include <iostream>
#include <sstream>
#include "H2MOD.h"
#include "H2MOD_GunGame.h"
#include "H2MOD_Infection.h"
#include "H2MOD_Halo2Final.h"
#include "Network.h"
#include "xliveless.h"
#include "CUser.h"
#include <h2mod.pb.h>
#include <Mmsystem.h>
#include <thread>
#include "Globals.h"
#include "H2OnscreenDebugLog.h"
#include "GSUtils.h"
#include "CustomNetworking.h"

H2MOD *h2mod = new H2MOD();
GunGame *gg = new GunGame();
Infection *inf = new Infection();
Halo2Final *h2f = new Halo2Final();

bool b_Infection = false;
bool b_Halo2Final = false;

extern bool b_GunGame;
extern CUserManagement User;
extern ULONG g_lLANIP;
extern ULONG g_lWANIP;
extern UINT g_port;
extern bool isHost;
extern HANDLE H2MOD_Network;
extern bool NetworkActive;
extern bool Connected;
extern bool ThreadCreated;
extern ULONG broadcast_server;


SOCKET comm_socket = INVALID_SOCKET;
char* NetworkData = new char[255];

HMODULE base;

extern int MasterState;


#pragma region engine calls

int __cdecl call_get_object(signed int object_datum_index, int object_type)
{
	//TRACE_GAME("call_get_object( object_datum_index: %08X, object_type: %08X )", object_datum_index, object_type);

	typedef int(__cdecl *get_object)(signed int object_datum_index, int object_type);
	get_object pget_object = (get_object)((char*)h2mod->GetBase() + ((h2mod->Server) ? 0x11F3A6 : 0x1304E3));

	return pget_object(object_datum_index, object_type);
}

int __cdecl call_unit_reset_equipment(int unit_datum_index)
{
	//TRACE_GAME("unit_reset_equipment(unit_datum_index: %08X)", unit_datum_index);
	typedef int(__cdecl *unit_reset_equipment)(int unit_datum_index);
	unit_reset_equipment punit_reset_equipment = (unit_reset_equipment)(((char*)h2mod->GetBase()) + ((h2mod->Server) ? 0x133030 : 0x1441E0));
	if (unit_datum_index != -1 && unit_datum_index != 0)
	{
		return punit_reset_equipment(unit_datum_index);
	}

	return 0;
}

int __cdecl call_hs_object_destroy(int object_datum_index)
{
	//TRACE_GAME("hs_object_destory(object_datum_index: %08X)", object_datum_index);
	typedef int(__cdecl *hs_object_destroy)(int object_datum_index);
	hs_object_destroy phs_object_destroy = (hs_object_destroy)(((char*)h2mod->GetBase()) + ((h2mod->Server) ? 0x124ED5 : 0x136005));

	return phs_object_destroy(object_datum_index);
}

signed int __cdecl call_unit_inventory_next_weapon(unsigned short unit)
{
	//TRACE_GAME("unit_inventory_next_weapon(unit: %08X)", unit);
	typedef signed int(__cdecl *unit_inventory_next_weapon)(unsigned short unit);
	unit_inventory_next_weapon punit_inventory_next_weapon = (unit_inventory_next_weapon)(((char*)h2mod->GetBase()) + 0x139E04);

	return punit_inventory_next_weapon(unit);
}

bool __cdecl call_assign_equipment_to_unit(int unit, int object_index, short unk)
{
	//TRACE_GAME("assign_equipment_to_unit(unit: %08X,object_index: %08X,unk: %08X)", unit, object_index, unk);
	typedef bool(__cdecl *assign_equipment_to_unit)(int unit, int object_index, short unk);
	assign_equipment_to_unit passign_equipment_to_unit = (assign_equipment_to_unit)(((char*)h2mod->GetBase()) + ((h2mod->Server) ? 0x1330FA : 0x1442AA));

	return passign_equipment_to_unit(unit, object_index, unk);
}

int __cdecl call_object_placement_data_new(void* s_object_placement_data, int object_definition_index, int object_owner, int unk)
{

	//TRACE_GAME("object_placement_data_new(s_object_placement_data: %08X,",s_object_placement_data);
	//TRACE_GAME("object_definition_index: %08X, object_owner: %08X, unk: %08X)", object_definition_index, object_owner, unk);

	typedef int(__cdecl *object_placement_data_new)(void*, int, int, int);
	object_placement_data_new pobject_placement_data_new = (object_placement_data_new)(((char*)h2mod->GetBase()) + ((h2mod->Server) ? 0x121033 : 0x132163));

	return pobject_placement_data_new(s_object_placement_data, object_definition_index, object_owner, unk);


}

signed int __cdecl call_object_new(void* pObject)
{
	//TRACE_GAME("object_new(pObject: %08X)", pObject);

	typedef int(__cdecl *object_new)(void*);
	object_new pobject_new = (object_new)(((char*)h2mod->GetBase()) + ((h2mod->Server) ? 0x125B77 : 0x136CA7));

	return pobject_new(pObject);

}

bool __cdecl call_add_object_to_sync(int gamestate_object_datum)
{
	typedef int(__cdecl  *add_object_to_sync)(int gamestate_object_datum);
	add_object_to_sync p_add_object_to_sync = (add_object_to_sync)((char*)h2mod->GetBase() + ((h2mod->Server) ? 0x1B2C44 : 0x1B8D14));

	return p_add_object_to_sync(gamestate_object_datum);
}

#pragma endregion



//sub_1458759
typedef int(__stdcall *write_chat_text)(void*, int);
write_chat_text write_chat_text_method;

std::string empty("");

int __stdcall write_chat_hook(void* pObject, int a2) {
	/*
	sub_14A7567((int)&v14, 0x79u, a2);            // this function def populates v14 with whatevers in the input component
	sub_13D1855((int)&v7);
	v5 = sub_142B37B(); //can use this to get the this* object we need to even utilize write_chat_text_method
	return sub_1458759(v5, (int)&v7);             // this method that gets invoked here has logic in that that appends the GamerTag : or Server : to the chatbox line
	*/

	//to get the current clients name, call sub_11B57CA(*(_DWORD *)(v2 + 8)); replace v2 with a2
	//which is at method offset 0x2157CA

	wchar_t* chatStringWChar = (wchar_t*)(a2 + 20);
	if (chatStringWChar[0] == L'$') {
		//this be a command, treat it differently
		h2mod->isChatBoxCommand = true;
		char chatStringChar[119];
		wcstombs(chatStringChar, chatStringWChar, 119);
		std::string str(chatStringChar);
		TRACE_GAME_N("chat_string=%s", chatStringChar);
		h2mod->handle_command(str);
		return 1;
	}
	else {
		h2mod->isChatBoxCommand = false;
		return write_chat_text_method(pObject, a2);
	}
}

typedef int(__cdecl *write_inner_chat_text)(int a1, unsigned int a2, int a3);
write_inner_chat_text write_inner_chat_text_method;

int __cdecl write_inner_chat_hook(int a1, unsigned int a2, int a3) {
	return write_inner_chat_text_method(a1, a2, a3);
}

//can write literal and dynamic wchar_t's
//basically a function that calls all the correct functions to write text to the chatbox
//if dedi, writes to the console stdout
void H2MOD::write_inner_chat_dynamic(const wchar_t* data) {
	if (this->Server) {
		h2mod->logToDedicatedServerConsole((wchar_t*)data);
		return;
	}
	//0x2096AE
	typedef int(*sub_2196AE)();

	//0x287BA9
	typedef void(__cdecl *sub_297BA9)(void* a1, int a2, unsigned int a3);
	sub_297BA9 sub_297BA9_method = (sub_297BA9)(h2mod->GetBase() + 0x287BA9);
	sub_2196AE sub_2196AE_method = (sub_2196AE)(h2mod->GetBase() + 0x2096AE);
	DWORD* ptr = (DWORD*)(((char*)h2mod->GetBase()) + 0x00973ac8);

	int a3 = (int)&(*data);
	void* v5 = ptr;
	const unsigned __int16* v3 = (const unsigned __int16*)(a3 - 20);
	int v8 = *(DWORD*)v5;
	*((BYTE*)v5 + 7684) = 1;
	*((DWORD*)v5 + 2) = v8;
	int v10 = *(DWORD*)v5;
	int v11 = (*(DWORD*)v5)++ % 30;
	int v12 = v10 % 30;

	if (*((DWORD *)v5 + v10 % 30 + 4)) {
		//TODO: caused crash for tweek? f tweek
		HeapFree(GetProcessHeap(), 0, ((LPVOID *)v5 + v11 + 4));
	}
	//size in bytes
	unsigned int v13 = wcslen(data) + 256;
	LPVOID v14 = HeapAlloc(GetProcessHeap(), 0, 2 * v13);
	*((DWORD *)v5 + v12 + 4) = (DWORD)v14;
	sub_297BA9_method(v14, 0, v13);

	//write the string
	write_inner_chat_text_method(*((DWORD *)v5 + v12 + 4), v13, a3);

	*((DWORD*)v5 + 3) = sub_2196AE_method();
}

//0x1BA418
typedef bool(*live_check)();
live_check live_check_method;

bool clientXboxLiveCheck() {
	//lets you access live menu
	return true;
}

//0x1B1643
typedef signed int(*live_check2)();
live_check2 live_check_method2;

signed int clientXboxLiveCheck2() {
	//1 = turns off live? 
	//2 = either not live or can't download maps
	return 2;
}

typedef int(__cdecl *show_error_screen)(int a1, signed int a2, int a3, __int16 a4, int a5, int a6);
show_error_screen show_error_screen_method;

int __cdecl showErrorScreen(int a1, signed int a2, int a3, __int16 a4, int a5, int a6) {
	if (a2 == 280) {
		//280 is special here, the constant is used when a custom map cannot be loaded for clients
		mapManager->startMapDownload();
		return 0;
	}
	return show_error_screen_method(a1, a2, a3, a4, a5, a6);
}

typedef signed int(__cdecl *string_display_hook)(int a3, unsigned int a4, int a5, int a6);
string_display_hook string_display_hook_method;

std::wstring YOU_FAILED_TO_LOAD_MAP_ORG = L"You failed to load the map.";

//lets you follow the call path of any string that is displayed (in a debugger)
signed int __cdecl stringDisplayHook(int a3, unsigned int a4, int a5, int a6) {
	/*
	if (overrideUnicodeMessage) {
	wchar_t* temp = (wchar_t*)a5;
	if (temp[0] != L' ') {
	//const wchar_t* lobbyMessage = mapManager->getCustomLobbyMessage();
	if (lobbyMessage != NULL && wcscmp(temp, yftlm) == 0) {
	//if we detect that we failed to load the map, we display different strings only for the duration of
	//this specific string being displayed
	return string_display_hook_method(a3, a4, (int)(lobbyMessage), a6);
	}

	/*
	if (wcscmp(OCTAGON.c_str(), temp) == 0) {
	__debugbreak();
	}
	if (temp != NULL && temp[0] == L'T' && temp[1] == L'i' && temp[2] == L'e' && temp[3] == L'd') {
	__debugbreak();
	}*/
	//}
	//}*/
	return string_display_hook_method(a3, a4, a5, a6);
}

void H2MOD::handle_command(std::string command) {
	commands->handle_command(command);
}

void H2MOD::handle_command(std::wstring command) {
	commands->handle_command(std::string(command.begin(), command.end()));
}

typedef int(__cdecl *dedi_command_hook)(int a1, int a2, char a3);
dedi_command_hook dedi_command_hook_method;

typedef signed int(*dedi_print)(const char* a1, ...);

void H2MOD::logToDedicatedServerConsole(wchar_t* message) {
	dedi_print dedi_print_method = (dedi_print)(h2mod->GetBase() + 0x2354C8);
	dedi_print_method((const char*)(message));
}

int __cdecl dediCommandHook(int a1, int a2, int a3) {
	h2mod->logToDedicatedServerConsole(L"Dedicated command\n");
	unsigned __int16* ptr = *(unsigned __int16 **)a1;
	const wchar_t* text = (wchar_t*)ptr;
	wchar_t c = text[0];
	if (c == L'$') {
		h2mod->logToDedicatedServerConsole(L"Running chatbox command\n");
		//run the chatbox commands
		h2mod->handle_command(std::wstring(text));
	}

	return dedi_command_hook_method(a1, a2, a3);
}

typedef char(__stdcall *send_text_chat)(char* thisx, int a2, int a3, char a4, char a5);
send_text_chat send_text_chat_method;

char __stdcall sendTextChat(char* thisx, int a2, int a3, char a4, char a5) {
	if (!h2mod->isChatBoxCommand) {
		return send_text_chat_method(thisx, a2, a3, a4, a5);
	}
	//don't send chatbox commands to anyone
	return ' ';
}

//0xD1FA7
typedef void(__thiscall *data_decode_string)(void* thisx, int a2, int a3, int a4);
data_decode_string getDataDecodeStringMethod() {
	return (data_decode_string)(h2mod->GetBase() + 0xD1FA7);
}

//0xD1FFD
typedef int(__thiscall *data_decode_address)(int thisx, int a1, int a2);
data_decode_address getDataDecodeAddressMethod() {
	return (data_decode_address)(h2mod->GetBase() + 0xD1FFD);
}

//0xD1F95
typedef int(__thiscall *data_decode_id)(int thisx, int a1, int a2, int a3);
data_decode_id getDataDecodeId() {
	return (data_decode_id)(h2mod->GetBase() + 0xD1F95);
}

//0xD1EE5
typedef unsigned int(__thiscall *data_decode_integer)(int thisx, int a1, int a2);
data_decode_integer getDataDecodeIntegerMethod() {
	return (data_decode_integer)(h2mod->GetBase() + 0xD1EE5);
}

//0xD1F47
typedef bool(__thiscall *data_decode_bool)(int thisx, int a2);
data_decode_bool getDataDecodeBoolMethod() {
	return (data_decode_bool)(h2mod->GetBase() + 0xD1F47);
}

//0xD114C
//this isn't can_join, its some other shit
typedef bool(__thiscall *can_join)(int thisx);
can_join getCanJoinMethod() {
	return (can_join)(h2mod->GetBase() + 0xD114C);
}

//0x1ECEEB
typedef bool(__cdecl *read_text_chat_packet)(int a1, int a2, int a3);
read_text_chat_packet read_text_chat_packet_method;

bool __cdecl readTextChat(int a1, int a2, int a3) {
	//TODO: from this method you can determine if the server sent you the message
	//could open up scripting

	//TODO: any pieces of text with "$" in front, ignore, since this is the client trying to possibly
	//send a malicious command
	data_decode_integer dataDecodeInteger = getDataDecodeIntegerMethod();
	data_decode_id dataDecodeId = getDataDecodeId();
	data_decode_address dataDecodeAddress = getDataDecodeAddressMethod();
	data_decode_string dataDecodeString = getDataDecodeStringMethod();
	data_decode_bool dataDecodeBool = getDataDecodeBoolMethod();
	can_join sub_45114C_method = getCanJoinMethod();

	bool v3; // al@1
	unsigned int v4; // eax@3
	unsigned int v5; // ebx@4
	int v6; // ebp@5
	bool result; // al@7

	dataDecodeId(a1, (int)"session-id", a3, 64);
	*(DWORD*)(a3 + 8) = dataDecodeInteger(a1, (int)"routed-players", 32);
	*(DWORD*)(a3 + 12) = dataDecodeInteger(a1, (int)"metadata", 8);
	v3 = dataDecodeBool(a1, (int)"source-is-server");
	*(BYTE *)(a3 + 16) = v3;
	if (!v3)
		dataDecodeId(a1, (int)"source-player", a3 + 17, 64);
	v4 = dataDecodeInteger(a1, (int)"destination-player-count", 8);
	*(DWORD*)(a3 + 156) = v4;
	if (v4 > 0x10)
	{
		result = 0;
	}
	else
	{
		v5 = 0;
		if (v4)
		{
			v6 = a3 + 25;
			do
			{
				dataDecodeId(a1, (int)"destination-player", v6, 64);
				++v5;
				v6 += 8;
			} while (v5 < *(DWORD*)(a3 + 156));
		}
		dataDecodeString((void *)a1, (int)"text", a3 + 160, 121);

		char* text = (char*)(a3 + 160);
		char c = text[0];

		TRACE_GAME_N("text_chat_packet=%s", text);
		if (c == '$') {
			if (v3) {
				//TODO: if came from server, run thru server->client command handler
			}
			else {
				//TODO: run thru client->client command handler
			}

			text[0] = ' ';
		}
		//result = sub_45114C_method(a1) == 0;
	}
	return true;
}

#pragma region PlayerFunctions

float H2MOD::get_player_x(int playerIndex, bool resetDynamicBase) {
	int base = get_dynamic_player_base(playerIndex, resetDynamicBase);
	if (base != -1) {
		float buffer;
		ReadProcessMemory(GetCurrentProcess(), (LPVOID)(base + 0x64), &buffer, sizeof(buffer), NULL);
		return buffer;
	}
	return 0.0f;
}

float H2MOD::get_player_y(int playerIndex, bool resetDynamicBase) {
	int base = get_dynamic_player_base(playerIndex, resetDynamicBase);
	if (base != -1) {
		float buffer;
		ReadProcessMemory(GetCurrentProcess(), (LPVOID)(base + 0x68), &buffer, sizeof(buffer), NULL);
		return buffer;
	}
	return 0.0f;
}

float H2MOD::get_player_z(int playerIndex, bool resetDynamicBase) {
	int base = get_dynamic_player_base(playerIndex, resetDynamicBase);
	if (base != -1) {
		float buffer;
		ReadProcessMemory(GetCurrentProcess(), (LPVOID)(base + 0x6C), &buffer, sizeof(buffer), NULL);
		return buffer;
	}
	return 0.0f;
}

int H2MOD::get_dynamic_player_base(int playerIndex, bool resetDynamicBase) {
	/*int tempSight = get_unit_datum_from_player_index(playerIndex);
	int cachedDynamicBase = playerIndexToDynamicBase[playerIndex];
	if (tempSight != -1 && tempSight != 0 && (!cachedDynamicBase || resetDynamicBase)) {
	//TODo: this is based on some weird implementation in HaloObjects.cs, need just to find real offsets to dynamic player pointer
	//instead of this garbage
	for (int i = 0; i < 2048; i++) {
	int possiblyDynamicBase = *(DWORD*)(0x3003CF3C + (i * 12) + 8);
	DWORD* possiblyDynamicBasePtr = (DWORD*)(possiblyDynamicBase + 0x3F8);
	if (possiblyDynamicBasePtr) {
	BYTE buffer[4];
	//use readprocess memory since it will set the page memory and read/write
	ReadProcessMemory(GetCurrentProcess(), (LPVOID)(possiblyDynamicBase + 0x3F8), &buffer, sizeof(buffer), NULL);
	//little endian
	int dynamicS2 = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
	if (tempSight == dynamicS2) {
	playerIndexToDynamicBase[playerIndex] = possiblyDynamicBase;
	return possiblyDynamicBase;
	}
	}
	}
	}
	return cachedDynamicBase;*/
	int dyanamic = -1;
	int playerdatum = h2mod->get_unit_datum_from_player_index(playerIndex);
	int DynamicObjBase = *(DWORD *)(((char*)h2mod->GetBase()) + ((h2mod->Server) ? 0x50C8EC : 0x4E461C));
	if (playerdatum != -1)
		dyanamic = *(DWORD *)(*(DWORD *)(DynamicObjBase + 68) + 12 * (unsigned __int16)playerdatum + 8);
	return dyanamic;
}


void GivePlayerWeapon(int PlayerIndex, int WeaponId, bool bReset)
{
	//TRACE_GAME("GivePlayerWeapon(PlayerIndex: %08X, WeaponId: %08X)", PlayerIndex, WeaponId);

	int unit_datum = h2mod->get_unit_datum_from_player_index(PlayerIndex);

	if (unit_datum != -1 && unit_datum != 0)
	{
		char* nObject = new char[0xC4];
		DWORD dwBack;
		VirtualProtect(nObject, 0xC4, PAGE_EXECUTE_READWRITE, &dwBack);

		call_object_placement_data_new(nObject, WeaponId, unit_datum, 0);

		int object_index = call_object_new(nObject);

		if (bReset == true)
			call_unit_reset_equipment(unit_datum);

		call_assign_equipment_to_unit(unit_datum, object_index, 1);
	}

}

wchar_t* H2MOD::get_local_player_name()
{
	return (wchar_t*)(((char*)h2mod->GetBase()) + 0x51A638);
}

int H2MOD::get_player_index_from_name(wchar_t* playername)
{
	DWORD player_table_ptr;
	if (!h2mod->Server)
		player_table_ptr = *(DWORD*)(this->GetBase() + 0x004A8260);
	else
		player_table_ptr = *(DWORD*)(this->GetBase() + 0x004D64C4);
	player_table_ptr += 0x44;

	for (int i = 0; i <= 16; i++)
	{
		wchar_t* comparename = (wchar_t*)(*(DWORD*)player_table_ptr + (i * 0x204) + 0x40);

		TRACE_GAME("[H2MOD]::get_player_index_from_name( %ws : %ws )", playername, comparename);

		if (wcscmp(comparename, playername))
		{
			return i;
		}
	}
}

wchar_t* H2MOD::get_player_name_from_index(int pIndex)
{
	DWORD player_table_ptr;
	if (!h2mod->Server)
		player_table_ptr = *(DWORD*)(this->GetBase() + 0x004A8260);
	else
		player_table_ptr = *(DWORD*)(this->GetBase() + 0x004D64C4);
	player_table_ptr += 0x44;

	return	(wchar_t*)(*(DWORD*)player_table_ptr + (pIndex * 0x204) + 0x40);
}

int H2MOD::get_player_index_from_unit_datum(int unit_datum_index)
{
	int pIndex = 0;
	DWORD player_table_ptr;
	if (!h2mod->Server)
		player_table_ptr = *(DWORD*)(this->GetBase() + 0x004A8260);
	else
		player_table_ptr = *(DWORD*)(this->GetBase() + 0x004D64C4);
	player_table_ptr += 0x44;

	for (pIndex = 0; pIndex <= 16; pIndex++)
	{
		if (unit_datum_index == (int)*(int*)(*(DWORD*)player_table_ptr + (pIndex * 0x204) + 0x28))
		{
			return pIndex;
		}
	}

}


int H2MOD::get_unit_datum_from_player_index(int pIndex)
{
	int unit = 0;
	DWORD player_table_ptr;
	if (!h2mod->Server)
		player_table_ptr = *(DWORD*)(this->GetBase() + 0x004A8260);
	else
		player_table_ptr = *(DWORD*)(this->GetBase() + 0x004D64C4);
	player_table_ptr += 0x44;

	unit = (int)*(int*)(*(DWORD*)player_table_ptr + (pIndex * 0x204) + 0x28);

	return unit;
}

int H2MOD::get_unit_from_player_index(int pIndex)
{
	int unit = 0;
	DWORD player_table_ptr;
	if (!h2mod->Server)
		player_table_ptr = *(DWORD*)(this->GetBase() + 0x004A8260);
	else
		player_table_ptr = *(DWORD*)(this->GetBase() + 0x004D64C4);
	player_table_ptr += 0x44;

	unit = (int)*(short*)(*(DWORD*)player_table_ptr + (pIndex * 0x204) + 0x28);

	return unit;
}

BYTE H2MOD::get_unit_team_index(int unit_datum_index)
{
	BYTE tIndex = 0;
	int unit_object = call_get_object(unit_datum_index, 3);
	if (unit_object)
	{
		tIndex = *(BYTE*)((BYTE*)unit_object + 0x13C);
	}
	return tIndex;
}

void H2MOD::set_unit_team_index(int unit_datum_index, BYTE team)
{
	int unit_object = call_get_object(unit_datum_index, 3);
	if (unit_object)
	{
		*(BYTE*)((BYTE*)unit_object + 0x13C) = team;
	}
}

void H2MOD::set_unit_biped(BYTE biped, int pIndex)
{
	if (pIndex >= 0 && pIndex < 16)
		*(BYTE*)(((char*)((h2mod->Server) ? 0x3000274C : 0x30002BA0) + (pIndex * 0x204))) = biped;
}

void H2MOD::set_unit_speed_patch(bool hackit) {
	//TODO: create a way to undo the patch in the case when more than just infection relies on this.
	//Enable Speed Hacks

	BYTE assmPatchSpeed[8];
	memset(assmPatchSpeed, 0x90, 8);
	WriteBytesASM(h2mod->GetBase() + ((!h2mod->Server) ? 0x6AB7f : 0x6A3BA), assmPatchSpeed, 8);
}

void H2MOD::set_unit_speed(float speed, int pIndex)
{
	if (pIndex >= 0 && pIndex < 16)
		*(float*)(((char*)((h2mod->Server) ? 0x30002848 : 0x30002C9C) + (pIndex * 0x204))) = speed;
}

void H2MOD::set_local_team_index(BYTE team)
{
	*(BYTE*)(((char*)h2mod->GetBase()) + 0x51A6B4) = team;
}

void H2MOD::set_local_grenades(BYTE type, BYTE count, int pIndex)
{
	int unit_datum_index = h2mod->get_unit_datum_from_player_index(pIndex);

	int unit_object = call_get_object(unit_datum_index, 3);

	if (unit_object)
	{
		if (type == GrenadeType::Frag)
			*(BYTE*)((BYTE*)unit_object + 0x252) = count;
		if (type == GrenadeType::Plasma)
			*(BYTE*)((BYTE*)unit_object + 0x253) = count;
	}

}

void H2MOD::set_unit_grenades(BYTE type, BYTE count, int pIndex, bool bReset)
{
	int unit_datum_index = h2mod->get_unit_datum_from_player_index(pIndex);
	wchar_t* pName = h2mod->get_player_name_from_index(pIndex);

	int unit_object = call_get_object(unit_datum_index, 3);
	if (unit_object)
	{
		if (bReset)
			call_unit_reset_equipment(unit_datum_index);

		if (type == GrenadeType::Frag)
		{
			*(BYTE*)((BYTE*)unit_object + 0x252) = count;


		}
		if (type == GrenadeType::Plasma)
		{
			*(BYTE*)((BYTE*)unit_object + 0x253) = count;
		}

		H2ModPacket GrenadePak;
		GrenadePak.set_type(H2ModPacket_Type_set_unit_grenades);
		h2mod_set_grenade *gnade = GrenadePak.mutable_set_grenade();
		gnade->set_count(count);
		gnade->set_pindex(pIndex);
		gnade->set_type(type);

		char* GrenadeBuf = new char[GrenadePak.ByteSize()];
		GrenadePak.SerializeToArray(GrenadeBuf, GrenadePak.ByteSize());

		for (auto it = NetworkPlayers.begin(); it != NetworkPlayers.end(); ++it)
		{
			if (wcscmp(it->first->PlayerName, pName) == 0)
			{
				it->first->PacketData = GrenadeBuf;
				it->first->PacketSize = GrenadePak.ByteSize();
				it->first->PacketsAvailable = true;
			}
		}


	}
}

BYTE H2MOD::get_local_team_index()
{
	return *(BYTE*)(((char*)h2mod->GetBase() + 0x51A6B4));
}
#pragma endregion

void H2MOD::DisableSound(int sound)
{
	DWORD AddressOffset = *(DWORD*)((char*)this->GetBase() + 0x47CD54);

	TRACE_GAME("AddressOffset: %08X", AddressOffset);
	switch (sound)
	{
	case SoundType::Slayer:
		TRACE_GAME("AddressOffset+0xd7dfb4:", (DWORD*)(AddressOffset + 0xd7dfb4));
		*(DWORD*)(AddressOffset + 0xd7e05c) = -1;
		*(DWORD*)(AddressOffset + 0xd7dfb4) = -1;
		break;

	case SoundType::GainedTheLead:
		*(DWORD*)(AddressOffset + 0xd7ab34) = -1;
		*(DWORD*)(AddressOffset + 0xd7ac84) = -1;
		break;

	case SoundType::LostTheLead:
		*(DWORD*)(AddressOffset + 0xd7ad2c) = -1;
		*(DWORD*)(AddressOffset + 0xd7add4) = -1;
		break;

	case SoundType::TeamChange:
		*(DWORD*)(AddressOffset + 0xd7b9a4) = -1;
		break;
	}
}

void SoundThread(void)
{

	while (1)
	{
		if (h2mod->SoundMap.size() > 0)
		{
			auto it = h2mod->SoundMap.begin();
			while (it != h2mod->SoundMap.end())
			{
				TRACE_GAME("[H2MOD-SoundQueue] - attempting to play sound %ws - delaying for %i miliseconds first", it->first, it->second);
				Sleep(it->second);
				PlaySound(it->first, NULL, SND_FILENAME);
				it = h2mod->SoundMap.erase(it);
			}
		}

		Sleep(100);
	}

}

void Field_of_View(unsigned int field_of_view, bool save)
{
	if (field_of_view > 0 && field_of_view <= 110) {

		if (save) {
			//save to xlive.ini the fov if $setfov command is used or anything else
		}

		const UINT FOV_MULTIPLIER_OFFSET = 4315524;
		const UINT FOV_VEHICLE_MULTIPLIER_OFFSET = 4274048;
		const UINT CURRENT_FOV_OFFSET = 4883752;

		float defaultRadians = (float)(70 * 3.14159265f / 180);
		float targetRadians = (float)((double)field_of_view * 3.14159265f / 180);
		*(float*)(h2mod->GetBase() + FOV_MULTIPLIER_OFFSET) = (targetRadians / defaultRadians);
		*(float*)(h2mod->GetBase() + FOV_VEHICLE_MULTIPLIER_OFFSET) = (targetRadians / defaultRadians);
	}
}

typedef char(__cdecl *tsub_4F17A)(void*, int, int);
tsub_4F17A psub_4F17A;

char __cdecl sub_4F17A(void* thisptr, int a2, int a3) //allows people to load custom maps with custom tags/huds 
{
	int result = psub_4F17A(thisptr, a2, a3);
	return 1;
}

typedef bool(__cdecl *spawn_player)(int a1);
spawn_player pspawn_player;

typedef bool(__cdecl *membership_update_network_decode)(int a1, int a2, int a3);
membership_update_network_decode pmembership_update_network_decode;

typedef int(__cdecl *game_difficulty_get_real_evaluate)(int a1, int a2);
game_difficulty_get_real_evaluate pgame_difficulty_get_real_evaluate;

typedef int(__cdecl *map_intialize)(int a1);
map_intialize pmap_initialize;

typedef char(__cdecl *player_death)(int unit_datum_index, int a2, char a3, char a4);
player_death pplayer_death;

typedef void(__stdcall *update_player_score)(void* thisptr, unsigned short a2, int a3, int a4, int a5, char a6);
update_player_score pupdate_player_score;

typedef void(__stdcall *tjoin_game)(void* thisptr, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, char a12, int a13, int a14);
tjoin_game pjoin_game;

extern SOCKET game_sock;
extern SOCKET game_sock_1000;
extern CUserManagement User;
XNADDR join_game_xn;

typedef int(__cdecl *tconnect_establish_write)(void* a1, int a2, int a3);
tconnect_establish_write pconnect_establish_write;


char __cdecl OnPlayerDeath(int unit_datum_index, int a2, char a3, char a4)
{

	//TRACE_GAME("OnPlayerDeath(unit_datum_index: %08X, a2: %08X, a3: %08X, a4: %08X)", unit_datum_index,a2,a3,a4);
	//TRACE_GAME("OnPlayerDeath() - Team: %i", h2mod->get_unit_team_index(unit_datum_index));

#pragma region GunGame Handler
	if (b_GunGame && (h2mod->Server || isHost))
		gg->PlayerDied(unit_datum_index);
#pragma endregion

#pragma region Infection Handler
	if (b_Infection)
		inf->PlayerInfected(unit_datum_index);
#pragma endregion

	return pplayer_death(unit_datum_index, a2, a3, a4);
}


void __stdcall OnPlayerScore(void* thisptr, unsigned short a2, int a3, int a4, int a5, char a6)
{
	//TRACE_GAME("update_player_score_hook ( thisptr: %08X, a2: %08X, a3: %08X, a4: %08X, a5: %08X, a6: %08X )", thisptr, a2, a3, a4, a5, a6);


#pragma region GunGame Handler
	if (a5 == 7) //player got a kill?
	{
		int PlayerIndex = a2;
		if (b_GunGame && (isHost || h2mod->Server))
			gg->LevelUp(PlayerIndex);
	}

#pragma endregion

	return pupdate_player_score(thisptr, a2, a3, a4, a5, a6);
}
void PatchFixRankIcon() {
	if (!h2mod->Server) {
		int THINGY = (int)*(int*)((char*)h2mod->GetBase() + 0xA40564);
		BYTE* assmOffset = (BYTE*)(THINGY + 0x800);
		const int assmlen = 20;
		BYTE assmOrigRankIcon[assmlen] = { 0x92,0x00,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6D,0x74,0x69,0x62,0xCA,0x02,0xEC,0xE4 };
		BYTE assmPatchFixRankIcon[assmlen] = { 0xCC,0x01,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6D,0x74,0x69,0x62,0xE6,0x02,0x08,0xE5 };
		bool shouldPatch = true;
		for (int i = 0; i < assmlen; i++) {
			if (*(assmOffset + i) != assmOrigRankIcon[i]) {
				shouldPatch = false;
				break;
			}
		}
		if (shouldPatch) {
			WriteBytesASM((DWORD)assmOffset, assmPatchFixRankIcon, assmlen);
			addDebugText("Patching Rank Icon Fix.");
		}
	}
}
void PatchGameDetailsCheck()
{
	BYTE assmPatchGamedetails[2] = { 0x75,0x18};	
	WriteBytesASM(h2mod->GetBase() + 0x219D6D, assmPatchGamedetails, 2);
}

void H2MOD::PatchWeaponsInteraction(bool b_Enable)
{
	//Client Sided Patch
	DWORD offset = h2mod->GetBase() + 0x55EFA;
	BYTE assm[5] = { 0xE8, 0x18, 0xE0,0xFF, 0xFF };
	if (!b_Enable)
	{
		memset(assm, 0x90, 5);
	}
	WriteBytesASM(offset, assm, 5);
}

void PatchPingMeterCheck(bool hackit)
{
	//halo2.exe+1D4E35 

	BYTE assmOrgLine[2] = { 0x74,0x18 };
	BYTE assmPatchPingCheck[2] = { 0x90,0x90 };

	if (hackit)
		WriteBytesASM(h2mod->GetBase() + 0x1D4E35, assmPatchPingCheck, 2);
	else
		WriteBytesASM(h2mod->GetBase() + 0x1D4E35, assmOrgLine, 2);

}

static bool OnNewRound(int a1)
{

	bool(__cdecl* CallNewRound)(int a1);
	CallNewRound = (bool(__cdecl*)(int))((char*)h2mod->GetBase() + ((h2mod->Server) ? 0x6A87C : 0x6B1C8));
	//addDebugText("New Round Commencing");
		if (b_Infection)
		inf->NextRound();

	if (b_GunGame)
		gg->NextRound();

	return CallNewRound(a1);


}
void H2MOD::PatchNewRound(bool hackit) //All thanks to Glitchy Scripts who wrote this <3
{
	//Replace the Function call  At Offset with OnNewRound
	DWORD offset = 0;

	if (h2mod->Server)
		offset = 0x700EF;
	else
		offset = 0x715ee;
	if(hackit)	
		PatchCall((DWORD)((char*)h2mod->GetBase() + offset), (DWORD)OnNewRound); 
	else
		PatchCall((DWORD)((char*)h2mod->GetBase() + offset), (DWORD)((char*)h2mod->GetBase() + ((h2mod->Server) ? 0x6A87C : 0x6B1C8)));
}


int __cdecl OnMapLoad(int a1)
{
	overrideUnicodeMessage = false;

	isLobby = true;

	//OnMapLoad is called with 30888 when a game ends
	if (a1 == 30888)
	{
		if (b_Halo2Final && !h2mod->Server)
			h2f->Dispose();
		if (b_Infection) {
			inf->Deinitialize();
		}

		int ret = pmap_initialize(a1);

		PatchFixRankIcon();

		return ret;
	}

	b_Infection = false;
	b_GunGame = false;
	b_Halo2Final = false;

	wchar_t* variant_name = (wchar_t*)(((char*)h2mod->GetBase()) + ((h2mod->Server) ? 0x534A18 : 0x97777C));
	int GameGlobals = (int)*(int*)((char*)h2mod->GetBase() + ((h2mod->Server) ? 0x4CB520 : 0x482D3C));
	DWORD* GameEngine = (DWORD*)(GameGlobals + 0x8);

	BYTE* GameState = (BYTE*)(((char*)h2mod->GetBase()) + ((h2mod->Server) ? 0x3C40AC : 0x420FC4));


	TRACE_GAME("[h2mod] OnMapLoad engine mode %d, variant name %ws", *GameEngine, variant_name);

	if (*GameEngine == 2) {
		if (wcsstr(variant_name, L"zombies") > 0 || wcsstr(variant_name, L"Zombies") > 0 || wcsstr(variant_name, L"Infection") > 0 || wcsstr(variant_name, L"infection") > 0)
		{
			TRACE_GAME("[h2mod] Zombies Turned on!");
			b_Infection = true;
		}

		if (wcsstr(variant_name, L"GunGame") > 0 || wcsstr(variant_name, L"gungame") > 0 || wcsstr(variant_name, L"Gungame") > 0)

		{
			TRACE_GAME("[h2mod] GunGame Turned on!");
			b_GunGame = true;
		}

		if (wcsstr(variant_name, L"H2F") > 0 || wcsstr(variant_name, L"h2f") > 0 || wcsstr(variant_name, L"Halo2Final") > 0 || wcsstr(variant_name, L"halo2final") > 0)
		{
			TRACE_GAME("[h2mod] Halo2Final Turned on!");
			b_Halo2Final = true;
		}
	}
	int ret = pmap_initialize(a1);

#pragma region Apply Hitfix

	int offset = 0x47CD54;
	//TRACE_GAME("[h2mod] Hitfix is being run on Client!");
	if (h2mod->Server)
		offset = 0x4A29BC;
	//TRACE_GAME("[h2mod] Hitfix is being run on the Dedicated Server!");

	DWORD AddressOffset = *(DWORD*)((char*)h2mod->GetBase() + offset);

	*(float*)(AddressOffset + 0xA4EC88) = 2400.0f; // battle_rifle_bullet.proj Initial Velocity 
	*(float*)(AddressOffset + 0xA4EC8C) = 2400.0f; //battle_rifle_bullet.proj Final Velocity
	*(float*)(AddressOffset + 0xB7F914) = 5000.0f; //sniper_bullet.proj Initial Velocity
	*(float*)(AddressOffset + 0xB7F918) = 5000.0f; //sniper_bullet.proj Final Velocity
	*(float*)(AddressOffset + 0xCE4598) = 5000.0f; //beam_rifle_beam.proj Initial Velocity
	*(float*)(AddressOffset + 0xCE459C) = 5000.0f; //beam_rifle_beam.proj Final Velocity
	*(float*)(AddressOffset + 0x81113C) = 200.0f; //gauss_turret.proj Initial Velocity def 90
	*(float*)(AddressOffset + 0x811140) = 200.0f; //gauss_turret.proj Final Velocity def 90
	*(float*)(AddressOffset + 0x97A194) = 800.0f; //magnum_bullet.proj initial def 400
	*(float*)(AddressOffset + 0x97A198) = 800.0f; //magnum_bullet.proj final def 400
	*(float*)(AddressOffset + 0x7E7E20) = 2000.0f; //bullet.proj (chaingun) initial def 800
	*(float*)(AddressOffset + 0x7E7E24) = 2000.0f; //bullet.proj (chaingun) final def 800

#pragma endregion

#pragma region H2v Stuff
	if (!h2mod->Server)
	{

#pragma region Crosshair Offset

		//*(float*)(AddressOffset + 0x3DC00) = crosshair_offset;		
		DWORD CrosshairY = *(DWORD*)((char*)h2mod->GetBase() + 0x479E70) + 0x1AF4 + 0xf0 + 0x1C;
		*(float*)CrosshairY = crosshair_offset;

#pragma endregion

		if (*GameEngine == 3) {
			MasterState = 10;
		}
		else {
			MasterState = 11;
		}

		//Crashfix
		//*(int*)(h2mod->GetBase() + 0x464940) = 0;
		//*(int*)(h2mod->GetBase() + 0x46494C) = 0;
		//*(int*)(h2mod->GetBase() + 0x464958) = 0;
		//*(int*)(h2mod->GetBase() + 0x464964) = 0;
		
		if (*GameEngine != 3 && *GameState == 3)
		{
#pragma region Infection
			if (b_Infection)
				inf->Initialize();
#pragma endregion

#pragma region GunGame Handler
			if (b_GunGame && isHost)
				gg->Initialize();
#pragma endregion

#pragma region Halo2Final
			if (b_Halo2Final && !h2mod->Server)
				h2f->Initialize(isHost);
#pragma endregion
		}


	}
	else {
#pragma endregion

#pragma region H2S Stuff
		if (*GameEngine != 3 && *GameState == 3)
		{

#pragma region Infection
			if (b_Infection)
				inf->Initialize();
#pragma endregion

#pragma region GunGame Handler
			if (b_GunGame)
				gg->Initialize();
#pragma endregion

		}

	}
	return ret;

#pragma endregion 
}

bool __cdecl OnPlayerSpawn(int a1)
{
	overrideUnicodeMessage = false;
	//once players spawn we aren't in lobby anymore ;)
	isLobby = false;
	//TRACE_GAME("OnPlayerSpawn(a1: %08X)", a1);

	int PlayerIndex = a1 & 0x000FFFF;

#pragma region Infection Prespawn Handler
	if (b_Infection)
		inf->PreSpawn(PlayerIndex);
#pragma endregion
	
	int ret = pspawn_player(a1); 

#pragma region Infection Handler
	if (b_Infection)
		inf->SpawnPlayer(PlayerIndex);
#pragma endregion

#pragma region GunGame Handler
	if (b_GunGame && (isHost || h2mod->Server))
		gg->SpawnPlayer(PlayerIndex);
#pragma endregion

	return ret;
}

/* Really need some hooks here,
??_7c_simulation_game_engine_player_entity_definition@@6B@ dd offset sub_11D0018

This area will give us tons of info on the game_engine player, including object index and such linked to secure-address, xnaddr, and wether or not they're host.


; class c_simulation_game_statborg_entity_definition: c_simulation_entity_definition;   (#classinformer)
We'll want to hook this for infection, it handles updating teams it seems we could possibly call it after updating a team to push it to all people.

; class c_simulation_damage_section_response_event_definition: c_simulation_event_definition;   (#classinformer)
Damage handlers... could help for quake sounds and other things like detecting assisnations (We'll probably find this when we track medals in-game as well)

; class c_simulation_unit_entity_definition: c_simulation_object_entity_definition, c_simulation_entity_definition;   (#classinformer)
Ofc we want unit handling.

; class c_simulation_slayer_engine_globals_definition: c_simulation_game_engine_globals_definition, c_simulation_entity_definition;   (#classinformer)
Should take a look here for extended functions on scoring chances are we're already hooking one of these.

*/

void __stdcall join_game(void* thisptr, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, char a12, int a13, int a14)
{
	isServer = false;
	Connected = false;
	NetworkActive = false;

	XNADDR* host_xn = (XNADDR*)a6;
	memcpy(&join_game_xn, host_xn, sizeof(XNADDR));

	trace(L"join_game host_xn->ina.s_addr: %08X ", host_xn->ina.s_addr);

	sockaddr_in SendStruct;

	if (host_xn->ina.s_addr != g_lWANIP)
		SendStruct.sin_addr.s_addr = host_xn->ina.s_addr;
	else
		SendStruct.sin_addr.s_addr = g_lLANIP;

	short nPort = (ntohs(host_xn->wPortOnline) + 1);

	TRACE("join_game nPort: %i", nPort);

	SendStruct.sin_port = htons(nPort);

	TRACE("join_game SendStruct.sin_port: %i", ntohs(SendStruct.sin_port));
	TRACE("join_game xn_port: %i", ntohs(host_xn->wPortOnline));

	//SendStruct.sin_port = htons(1001); // These kinds of things need to be fixed too cause we would have the port in the XNADDR struct...
	SendStruct.sin_family = AF_INET;

	int securitysend_1001 = sendto(game_sock, (char*)User.SecurityPacket, 8 + sizeof(XNADDR), 0, (SOCKADDR *)&SendStruct, sizeof(SendStruct));

	User.CreateUser(host_xn, FALSE);

	if (securitysend_1001 == SOCKET_ERROR)
	{
		TRACE("join_game Security Packet - Socket Error True");
		TRACE("join_game Security Packet - WSAGetLastError(): %08X", WSAGetLastError());
	}


	int Data_of_network_Thread = 1;
	H2MOD_Network = CreateThread(NULL, 0, NetworkThread, &Data_of_network_Thread, 0, NULL);

	return pjoin_game(thisptr, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
}

int __cdecl connect_establish_write(void* a1, int a2, int a3)
{

	TRACE("connect_establish_write(a1: %08X,a2 %08X, a3: %08X)", a1, a2, a3);

	if (!isHost)
	{
		sockaddr_in SendStruct;

		if (join_game_xn.ina.s_addr != g_lWANIP)
			SendStruct.sin_addr.s_addr = join_game_xn.ina.s_addr;
		else
			SendStruct.sin_addr.s_addr = g_lLANIP;

		SendStruct.sin_port = join_game_xn.wPortOnline;
		SendStruct.sin_family = AF_INET;

		int securitysend_1000 = sendto(game_sock_1000, (char*)User.SecurityPacket, 8 + sizeof(XNADDR), 0, (SOCKADDR *)&SendStruct, sizeof(SendStruct));
	}

	return pconnect_establish_write(a1, a2, a3);
}

typedef int(__cdecl *object_p_hook)(int s_object_placement_data, int a2, signed int a3, int a4);
object_p_hook object_p_hook_method;

int __cdecl objectPHook(int s_object_placement_data, int object_definition_index, int object_owner, int unk) {
	if (h2mod->hookedObjectDefs.find(object_definition_index) == h2mod->hookedObjectDefs.end()) {
		//ingame only
		wchar_t* mapName = (wchar_t*)(h2mod->GetBase() + 0x97737C);
		TRACE_GAME("MapName=%s, object_definition_index: %08X", mapName, object_definition_index);
		h2mod->hookedObjectDefs.insert(object_definition_index);
	}
	return object_p_hook_method(s_object_placement_data, object_definition_index, object_owner, unk);
}

typedef BOOL(__stdcall *is_debugger_present)();
is_debugger_present is_debugger_present_method;

BOOL __stdcall isDebuggerPresent() {
	return false;
}

typedef void*(__stdcall *tload_wgit)(void* thisptr, int a2, int a3, int a4, unsigned short a5);
tload_wgit pload_wgit;
void* __stdcall OnWgitLoad(void* thisptr, int a2, int a3, int a4, unsigned short a5) {
	int wgit = a2;

	//char NotificationPlayerText[20];
	//sprintf(NotificationPlayerText, "WGIT ID: %d", a2);
	//MessageBoxA(NULL, NotificationPlayerText, "WGITness", MB_OK);

	//Removed the ESRB warning after intro video (only occurs for English Lang).
	if (wgit == 292) {
		wgit = -1;
	}
	//void* thisptr = 
	pload_wgit(thisptr, wgit, a3, a4, a5);
	return thisptr;
}

typedef int(__cdecl *build_gui_list)(int a1, int a2, int a3);
build_gui_list build_gui_list_method;

int __cdecl buildGuiList(int a1, int a2, int a3) {
	if (b_Infection && a1 == (DWORD)(h2mod->GetBase() + 0x3d3620) && !isHost) {
		a2 = 1;
	}
	return build_gui_list_method(a1, a2, a3);
}

typedef NetworkPacket *(__cdecl *register_packet_group)(NetworkPacket *packet_table);
register_packet_group register_test_packet;

NetworkPacket* __cdecl register_test_packet_hook(NetworkPacket *packet_table) {
	return register_test_packet(packet_table);
}

PacketGenerator GameLeaveSessionPacketGenerator;
bool __cdecl leaveSessionPacketGenerator(int a1, int size, void *data)
{
	TRACE("leaveSessionPacketGenerator");
	return GameLeaveSessionPacketGenerator(a1, size, data);
}

PacketHandler GameleaveSessionPacketHandler;
bool __cdecl leaveSessionPacketHandler(int a1, int a2, int a3)
{
	TRACE("leaveSessionPacketHandler");
	return GameleaveSessionPacketHandler(a1, a2, a3);
}

int __cdecl PacketUnknownFunctionHandler(int a1, int a2, int a3)
{
	TRACE("PacketUnknownFunctionHandler");
	return 0;
}

DWORD packet_table_offset;
DWORD push_packet_table;
void __stdcall clear_packet_table_hook(NetworkPacket *old_packet_table) {
	CustomNetwork::ClearPacketTable();
	NetworkPacket* packet_table = CustomNetwork::GetPacketTable();
	NetworkPacket** packet_table_ptr = &packet_table;
	WriteBytesASM(h2mod->GetBase() + packet_table_offset, packet_table_ptr, sizeof(packet_table_ptr));
	WriteBytesASM(h2mod->GetBase() + push_packet_table, packet_table_ptr, sizeof(packet_table_ptr));

	// test packets
	CustomNetwork::Register_Packet(packet_table, CustomNetwork::leave_session_new_test, "leave-session", 8, 8, leaveSessionPacketGenerator, leaveSessionPacketHandler, PacketUnknownFunctionHandler);
}

void H2MOD::ApplyHooks() {
	/* Should store all offsets in a central location and swap the variables based on h2server/halo2.exe*/
	/* We also need added checks to see if someone is the host or not, if they're not they don't need any of this handling. */
	if (this->Server == false) {
		TRACE_GAME("Applying client hooks...");
		/* These hooks are only built for the client, don't enable them on the server! */

#pragma region H2V Hooks
		DWORD dwBack;

		//pload_wgit = (tload_wgit)DetourClassFunc((BYTE*)this->GetBase() + 0x2106A2, (BYTE*)OnWgitLoad, 13);
		//VirtualProtect(pload_wgit, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		psub_4F17A = (tsub_4F17A)DetourFunc((BYTE*)this->GetBase() + 0x4F17A, (BYTE*)sub_4F17A, 13);
		VirtualProtect(psub_4F17A, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		pjoin_game = (tjoin_game)DetourClassFunc((BYTE*)this->GetBase() + 0x1CDADE, (BYTE*)join_game, 13);
		VirtualProtect(pjoin_game, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		pspawn_player = (spawn_player)DetourFunc((BYTE*)this->GetBase() + 0x55952, (BYTE*)OnPlayerSpawn, 6);
		VirtualProtect(pspawn_player, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		pmap_initialize = (map_intialize)DetourFunc((BYTE*)this->GetBase() + 0x5912D, (BYTE*)OnMapLoad, 10);
		VirtualProtect(pmap_initialize, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		pupdate_player_score = (update_player_score)DetourClassFunc((BYTE*)this->GetBase() + 0xD03ED, (BYTE*)OnPlayerScore, 12);
		VirtualProtect(pupdate_player_score, 4, PAGE_EXECUTE_READWRITE, &dwBack);


		pplayer_death = (player_death)DetourFunc((BYTE*)this->GetBase() + 0x17B674, (BYTE*)OnPlayerDeath, 9);
		VirtualProtect(pplayer_death, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		pconnect_establish_write = (tconnect_establish_write)DetourFunc((BYTE*)this->GetBase() + 0x1F1A2D, (BYTE*)connect_establish_write, 5);
		VirtualProtect(pconnect_establish_write, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//chatbox command hooks below
		
		//raw log line (without Server: or GAMER_TAG: prefix)	
		write_inner_chat_text_method = (write_inner_chat_text)DetourFunc((BYTE*)this->GetBase() + 0x287669, (BYTE*)write_inner_chat_hook, 8);	
		VirtualProtect(write_inner_chat_text_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);	

		//read text packet	
		read_text_chat_packet_method = (read_text_chat_packet)DetourFunc((BYTE*)this->GetBase() + 0x1ECEEB, (BYTE*)readTextChat, 6);	
		VirtualProtect(read_text_chat_packet_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);	

		//0x1C7FE0	
		send_text_chat_method = (send_text_chat)DetourClassFunc((BYTE*)h2mod->GetBase() + 0x1C7FE0, (BYTE*)sendTextChat, 11);	
		VirtualProtect(send_text_chat_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);	

		//lobby chatbox	
		write_chat_text_method = (write_chat_text)DetourClassFunc((BYTE*)this->GetBase() + 0x238759, (BYTE*)write_chat_hook, 8);	
		VirtualProtect(write_chat_text_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);	
		
		if (map_downloading_enable) {
			//0x20E15A
			show_error_screen_method = (show_error_screen)DetourFunc((BYTE*)h2mod->GetBase() + 0x20E15A, (BYTE*)showErrorScreen, 8);
			VirtualProtect(show_error_screen_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);
		}

		//TODO: turn on if you want to debug halo2.exe from start of process
		//is_debugger_present_method = (is_debugger_present)DetourFunc((BYTE*)h2mod->GetBase() + 0x39B394, (BYTE*)isDebuggerPresent, 5);
		//VirtualProtect(is_debugger_present_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//TODO: use for object spawn hooking
		//0x132163
		//object_p_hook_method = (object_p_hook)DetourFunc((BYTE*)this->GetBase() + 0x132163, (BYTE*)objectPHook, 6);
		//VirtualProtect(object_p_hook_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//TODO: expensive, use for debugging/searching
		//string_display_hook_method = (string_display_hook)DetourFunc((BYTE*)h2mod->GetBase() + 0x287AB5, (BYTE*)stringDisplayHook, 5);
		//VirtualProtect(string_display_hook_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//TODO: for when live list is ready
		//live checks removed will make users exit to live menu instead of network browser :(
		//live_check_method = (live_check)DetourFunc((BYTE*)this->GetBase() + 0x1BA418, (BYTE*)clientXboxLiveCheck, 9);
		//VirtualProtect(live_check_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//live_check_method2 = (live_check2)DetourFunc((BYTE*)this->GetBase() + 0x1B1643, (BYTE*)clientXboxLiveCheck2, 9);
		//VirtualProtect(live_check_method2, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//pResetRound=(ResetRounds)DetourFunc((BYTE*)this->GetBase() + 0x6B1C8, (BYTE*)OnNextRound, 7);
		//VirtualProtect(pResetRound, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		build_gui_list_method = (build_gui_list)DetourFunc((BYTE*)this->GetBase() + 0x20D1FD, (BYTE*)buildGuiList, 8);
		VirtualProtect(build_gui_list_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);
		// Replace the games packet table with our own one and register our packets
		packet_table_offset = 0x51C464;
		push_packet_table = 0x1AC8F8;
		BYTE packet_count = CustomNetwork::packet_count;
		DetourClassFunc((BYTE*)this->GetBase() + 0x1E81C5, (BYTE*)clear_packet_table_hook, 5);
		WriteBytesASM(this->GetBase() + 0x1E825E, &packet_count, 1);

		// packet test
		GameLeaveSessionPacketGenerator = reinterpret_cast<PacketGenerator>(this->GetBase() + 0x1F0F14);
		GameleaveSessionPacketHandler = reinterpret_cast<PacketHandler>(this->GetBase() + 0x1F0F2A);

		auto new_packet_id = CustomNetwork::leave_session_new_test;
		WriteBytesASM(this->GetBase() + 0x1CA2A3, &new_packet_id, 1);
		WriteBytesASM(this->GetBase() + 0x1CA2C1, &new_packet_id, 1);

		// Patch out the code that displays the "Invalid Checkpoint" error
		// Start
		NopFill(this->GetBase() + 0x30857, 0x41);
		// Respawn
		NopFill(this->GetBase() + 0x8BB98, 0x2b);
	}
#pragma endregion

#pragma region H2ServerHooks
	else {

		DWORD dwBack;

		if (chatbox_commands) {
			dedi_command_hook_method = (dedi_command_hook)DetourFunc((BYTE*)this->GetBase() + 0x1CCFC, (BYTE*)dediCommandHook, 7);
			VirtualProtect(dedi_command_hook_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);
		}

		if (map_downloading_enable) {
			//dedis have map downloading thread turned on by default if configured to do so
			std::thread t1(&MapManager::startListeningForClients, mapManager);
			t1.detach();
		}

		pspawn_player = (spawn_player)DetourFunc((BYTE*)this->GetBase() + 0x5DE4A, (BYTE*)OnPlayerSpawn, 6);
		VirtualProtect(pspawn_player, 4, PAGE_EXECUTE_READWRITE, &dwBack);//

		pmap_initialize = (map_intialize)DetourFunc((BYTE*)this->GetBase() + 0x4E43C, (BYTE*)OnMapLoad, 10);
		VirtualProtect(pmap_initialize, 4, PAGE_EXECUTE_READWRITE, &dwBack);//


		pupdate_player_score = (update_player_score)DetourClassFunc((BYTE*)this->GetBase() + 0x8C84C, (BYTE*)OnPlayerScore, 12);
		VirtualProtect(pupdate_player_score, 4, PAGE_EXECUTE_READWRITE, &dwBack);//


		pplayer_death = (player_death)DetourFunc((BYTE*)this->GetBase() + 0x152ED4, (BYTE*)OnPlayerDeath, 9);
		VirtualProtect(pplayer_death, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		// Replace the games packet table with our own one and register our packets
		packet_table_offset = 0x520B84;
		push_packet_table = 0x1ACAC6;
		BYTE packet_count = CustomNetwork::packet_count;
		DetourClassFunc((BYTE*)this->GetBase() + 0x1AC188, (BYTE*)clear_packet_table_hook, 5);
		WriteBytesASM(this->GetBase() + 0x1AC221, &packet_count, 1);

		// packet test
		GameLeaveSessionPacketGenerator = reinterpret_cast<PacketGenerator>(this->GetBase() + 0x1D18CD);
		GameleaveSessionPacketHandler = reinterpret_cast<PacketHandler>(this->GetBase() + 0x1D18E3);
	}
#pragma endregion
}



DWORD WINAPI NetworkThread(LPVOID lParam)
{
	TRACE_GAME("[h2mod-network] NetworkThread Initializing");

	if (comm_socket == INVALID_SOCKET)
	{
		comm_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (comm_socket == INVALID_SOCKET)
		{
			TRACE_GAME("[h2mod-network] Socket is invalid even after socket()");
		}

		SOCKADDR_IN RecvStruct;
		RecvStruct.sin_port = htons(((short)g_port) + 7);
		RecvStruct.sin_addr.s_addr = htonl(INADDR_ANY);

		RecvStruct.sin_family = AF_INET;

		if (bind(comm_socket, (const sockaddr*)&RecvStruct, sizeof RecvStruct) == -1)
		{
			TRACE_GAME("[h2mod-network] Would not bind socket!");
		}

		DWORD dwTime = 20;

		if (setsockopt(comm_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTime, sizeof(dwTime)) < 0)
		{
			TRACE_GAME("[h2mod-network] Socket Error on register request");
		}

		TRACE_GAME("[h2mod-network] Listening on port: %i", ntohs(RecvStruct.sin_port));
	}
	else
	{
		TRACE_GAME("[h2mod-network] socket already existed continuing without attempting bind...");
	}

	TRACE_GAME("[h2mod-network] Are we host? %i", isHost);

	NetworkActive = true;

	if (isHost)
	{
		TRACE_GAME("[h2mod-network] We're host waiting for a authorization packet from joining clients...");

		while (1)
		{

			if (NetworkActive == false)
			{
				isHost = false;
				Connected = false;
				ThreadCreated = false;
				H2MOD_Network = 0;
				TRACE_GAME("[h2mod-network] Killing host thread NetworkActive == false");
				return 0;
			}
			sockaddr_in SenderAddr;
			int SenderAddrSize = sizeof(SenderAddr);

			memset(NetworkData, 0x00, 255);
			int recvresult = recvfrom(comm_socket, NetworkData, 255, 0, (sockaddr*)&SenderAddr, &SenderAddrSize);

			if (h2mod->NetworkPlayers.size() > 0)
			{
				auto it = h2mod->NetworkPlayers.begin();
				while (it != h2mod->NetworkPlayers.end())
				{
					if (it->second == 0)
					{
						TRACE_GAME("[h2mod-network] Deleting player %ws as their value was set to 0", it->first->PlayerName);

						if (it->first->PacketsAvailable == true)
							delete[] it->first->PacketData; // Delete packet data if there is any.


						delete[] it->first; // Clear NetworkPlayer object.

						it = h2mod->NetworkPlayers.erase(it);


					}
					else
					{
						if (it->first->PacketsAvailable == true) // If there's a packet available we set this to true already.
						{
							TRACE_GAME("[h2mod-network] Sending player %ws data", it->first->PlayerName);

							SOCKADDR_IN QueueSock;
							QueueSock.sin_port = it->first->port; // We can grab the port they connected from.
							QueueSock.sin_addr.s_addr = it->first->addr; // Address they connected from.
							QueueSock.sin_family = AF_INET;

							sendto(comm_socket, it->first->PacketData, it->first->PacketSize, 0, (sockaddr*)&QueueSock, sizeof(QueueSock)); // Just send the already serialized data over the socket.

							it->first->PacketsAvailable = false;
							delete[] it->first->PacketData; // Delete packet data we've sent it already.
						}
						it++;
					}
				}
			}


			if (recvresult > 0)
			{
				bool already_authed = false;
				H2ModPacket recvpak;
				recvpak.ParseFromArray(NetworkData, recvresult);


				if (recvpak.has_type())
				{
					switch (recvpak.type())
					{
					case H2ModPacket_Type_authorize_client:
						TRACE_GAME("[h2mod-network] Player Connected!");
						if (recvpak.has_h2auth())
						{
							if (recvpak.h2auth().has_name())
							{
								wchar_t* PlayerName = new wchar_t[36];
								memcpy(PlayerName, recvpak.h2auth().name().c_str(), 36);

								for (auto it = h2mod->NetworkPlayers.begin(); it != h2mod->NetworkPlayers.end(); ++it)
								{
									if (wcscmp(it->first->PlayerName, PlayerName) == 0)
									{

										TRACE_GAME("[h2mod-network] This player was already connected, sending them another packet letting them know they're authed already.");

										char* SendBuf = new char[recvpak.ByteSize()];
										recvpak.SerializeToArray(SendBuf, recvpak.ByteSize());

										sendto(comm_socket, SendBuf, recvpak.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));


										already_authed = true;

										delete[] SendBuf;
									}
								}

								if (already_authed == false)
								{
									NetworkPlayer *nPlayer = new NetworkPlayer;


									TRACE_GAME("[h2mod-network] PlayerName: %ws", PlayerName);
									TRACE_GAME("[h2mod-network] IP:PORT: %08X:%i", SenderAddr.sin_addr.s_addr, ntohs(SenderAddr.sin_port));

									nPlayer->addr = SenderAddr.sin_addr.s_addr;
									nPlayer->port = SenderAddr.sin_port;
									nPlayer->PlayerName = PlayerName;
									nPlayer->secure = recvpak.h2auth().secureaddr();
									h2mod->NetworkPlayers[nPlayer] = 1;

									char* SendBuf = new char[recvpak.ByteSize()];
									recvpak.SerializeToArray(SendBuf, recvpak.ByteSize());

									sendto(comm_socket, SendBuf, recvpak.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));

									delete[] SendBuf;
								}
							}
						}
						break;
					case H2ModPacket_Type_h2mod_ping:
						H2ModPacket pongpak;
						pongpak.set_type(H2ModPacket_Type_h2mod_pong);

						char* pongdata = new char[pongpak.ByteSize()];
						pongpak.SerializeToArray(pongdata, pongpak.ByteSize());
						sendto(comm_socket, pongdata, recvpak.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));

						delete[] pongdata;
						break;

					}
				}

				Sleep(500);
			}
		}
	}
	else
	{
		Connected = false;
		TRACE_GAME("[h2mod-network] We're a client connecting to server...");


		SOCKADDR_IN SendStruct;
		SendStruct.sin_port = htons(ntohs(join_game_xn.wPortOnline) + 7);
		SendStruct.sin_addr.s_addr = join_game_xn.ina.s_addr;
		SendStruct.sin_family = AF_INET;

		TRACE_GAME("[h2mod-network] Connecting to server on %08X:%i", SendStruct.sin_addr.s_addr, ntohs(SendStruct.sin_port));

		H2ModPacket h2pak;
		h2pak.set_type(H2ModPacket_Type_authorize_client);

		h2mod_auth *authpak = h2pak.mutable_h2auth();
		authpak->set_name((char*)h2mod->get_local_player_name(), 32);
		authpak->set_secureaddr(User.LocalSec);

		char* SendBuf = new char[h2pak.ByteSize()];
		h2pak.SerializeToArray(SendBuf, h2pak.ByteSize());

		sendto(comm_socket, SendBuf, h2pak.ByteSize(), 0, (SOCKADDR*)&SendStruct, sizeof(SendStruct));

		while (1)
		{
			if (NetworkActive == false)
			{
				isHost = false;
				Connected = false;
				ThreadCreated = false;
				H2MOD_Network = 0;
				TRACE_GAME("[h2mod-network] Networkactive == false ending client thread.");
				return 0;
			}

			if (Connected == false)
			{
				TRACE_GAME("[h2mod-network] Client - we're not connected re-sending our auth..");
				sendto(comm_socket, SendBuf, h2pak.ByteSize(), 0, (SOCKADDR*)&SendStruct, sizeof(SendStruct));
			}

			sockaddr_in SenderAddr;
			int SenderAddrSize = sizeof(SenderAddr);

			memset(NetworkData, 0x00, 255);
			int recvresult = recvfrom(comm_socket, NetworkData, 255, 0, (sockaddr*)&SenderAddr, &SenderAddrSize);

			if (recvresult > 0)
			{
				H2ModPacket recvpak;
				recvpak.ParseFromArray(NetworkData, recvresult);

				if (recvpak.has_type())
				{
					switch (recvpak.type())
					{
					case H2ModPacket_Type_authorize_client:

						if (Connected == false)
						{
							TRACE("[h2mod-network] Got the auth packet back!, We're connected!");
							Connected = true;
						}

						break;

					case H2ModPacket_Type_set_player_team:

						if (recvpak.has_h2_set_player_team())
						{

							BYTE TeamIndex = recvpak.h2_set_player_team().team();

							TRACE_GAME("[h2mod-network] Got a set team request from server! TeamIndex: %i", TeamIndex);
							h2mod->set_local_team_index(TeamIndex);
						}

						break;

					case H2ModPacket_Type_set_unit_grenades:
						if (recvpak.has_set_grenade())
						{
							BYTE type = recvpak.set_grenade().type();
							BYTE count = recvpak.set_grenade().count();
							BYTE pIndex = recvpak.set_grenade().pindex();

							h2mod->set_local_grenades(type, count, pIndex);
						}
						break;
					}
				}

			}

			if (Connected == true)
			{
				H2ModPacket pack;
				pack.set_type(H2ModPacket_Type_h2mod_ping);

				char* SendBuf = new char[pack.ByteSize()];
				pack.SerializeToArray(SendBuf, pack.ByteSize());

				sendto(comm_socket, SendBuf, pack.ByteSize(), 0, (SOCKADDR*)&SendStruct, sizeof(SendStruct));

				delete[] SendBuf;
			}


			Sleep(500);
		}

	}

	return 0;
}

DWORD WINAPI Thread1(LPVOID lParam)
{
	char *binarydata = new char[0xAA8 + 1];
	FILE* binarydump = fopen("binarydump.bin", "r");
	fread(binarydata, 0xAA8, 1, binarydump);

	while (1)
	{

		DWORD Base = (DWORD)GetModuleHandleA("halo2.exe");

		DWORD *ServerList = (DWORD*)(*(DWORD*)(Base + 0x96743C));
		if (ServerList > 0)
		{
			memcpy(ServerList, binarydata, 0xAA8);
			memcpy(ServerList + 0xAA8, binarydata, 0xAA8);
		}

		//fread((ServerList + 0xAA8), 0xAA8, 1, BinaryDump);
		//TRACE("ServerList: %08X\n", ServerList);
		//fwrite(ServerList, 0xAA8, 1, BinaryDump);	
	}
}

void H2MOD::Initialize()
{

	if (GetModuleHandleA("H2Server.exe"))
	{
		this->Base = (DWORD)GetModuleHandleA("H2Server.exe");
		this->Server = TRUE;
	}
	else
	{
		this->Base = (DWORD)GetModuleHandleA("halo2.exe");
		this->Server = FALSE;
		//HANDLE Handle_Of_Sound_Thread = 0;
		//int Data_Of_Sound_Thread = 1;
		std::thread SoundT(SoundThread);
		SoundT.detach();
		//Handle_Of_Sound_Thread = CreateThread(NULL, 0, SoundQueue, &Data_Of_Sound_Thread, 0, NULL);
		Field_of_View(field_of_view, 0);

		PatchGameDetailsCheck();
		//PatchPingMeterCheck(true);
	}
	
	TRACE_GAME("H2MOD - Initialized v0.1a");
	TRACE_GAME("H2MOD - BASE ADDR %08X", this->Base);

	//Network::Initialize();
	h2mod->ApplyHooks();
}

void H2MOD::Deinitialize() {

}

DWORD H2MOD::GetBase()
{
	return this->Base;
}

void H2MOD::IndicatorVisibility(bool toggle)
{
	BYTE* indicatorPointer = ((BYTE*)h2mod->GetBase() + 0x111197);
	DWORD dwback;
	VirtualProtect(indicatorPointer, 1, PAGE_EXECUTE_READWRITE, &dwback);

	if (toggle)
		*indicatorPointer = 0x74;
	else
		*indicatorPointer = 0xEB;

	VirtualProtect(indicatorPointer, 1, dwback, NULL);
}