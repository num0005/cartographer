#include "H2Tweaks.h"
#include "H2Startup.h"
#include "H2Config.h"
#include "GSUtils.h"
#include "Hook.h"
#include "H2OnscreenDebugLog.h"
#include "GSCustomMenu.h"

#pragma region Done_Tweaks

typedef int(__cdecl *thookServ1)(HKEY, LPCWSTR);
thookServ1 phookServ1;
int __cdecl LoadRegistrySettings(HKEY hKey, LPCWSTR lpSubKey) {
	char result =
		phookServ1(hKey, lpSubKey);
	addDebugText("Post Server Registry Read.");
	if (strlen(H2Config_dedi_server_name) > 0) {
		wchar_t* PreLoadServerName = (wchar_t*)((BYTE*)H2BaseAddr + 0x3B49B4);
		swprintf(PreLoadServerName, 15, L"%hs", H2Config_dedi_server_name);
		//char temp[27];
		//snprintf(temp, 27, "%ws", dedi_server_name);
		//MessageBoxA(NULL, temp, "Server Pre name thingy", MB_OK);
		wchar_t* LanServerName = (wchar_t*)((BYTE*)H2BaseAddr + 0x52042A);
		swprintf(LanServerName, 2, L"");
	}
	if (strlen(H2Config_dedi_server_playlist) > 0) {
		wchar_t* ServerPlaylist = (wchar_t*)((BYTE*)H2BaseAddr + 0x3B3704);
		swprintf(ServerPlaylist, 256, L"%hs", H2Config_dedi_server_playlist);
	}
	return result;
}

typedef int(__stdcall *thookServ2)();
thookServ2 phookServ2;
int __stdcall PreReadyLoad() {
	MessageBoxA(NULL, "aaaaaaas", "PreCrash", MB_OK);
	int result =
		phookServ2();
	MessageBoxA(NULL, "aaaaaaasaaaaaa", "PostCrash", MB_OK);
	if (strlen(H2Config_dedi_server_name) > 0) {
		wchar_t* PreLoadServerName = (wchar_t*)((BYTE*)H2BaseAddr + 0x52042A);
		swprintf(PreLoadServerName, 32, L"%hs", H2Config_dedi_server_name);
		char temp[27];
		snprintf(temp, 32, H2Config_dedi_server_name);
		MessageBoxA(NULL, temp, "AAA Server Pre name thingy", MB_OK);
	}
	return result;
}

static bool NotDisplayIngameChat() {
	int GameGlobals = (int)*(int*)((DWORD)H2BaseAddr + 0x482D3C);
	DWORD* GameEngine = (DWORD*)(GameGlobals + 0x8);
	BYTE* GameState = (BYTE*)((DWORD)H2BaseAddr + 0x420FC4);

	if (H2Config_hide_ingame_chat) {
		int GameTimeGlobals = (int)*(int*)((DWORD)H2BaseAddr + 0x4C06E4);
		DWORD* ChatOpened = (DWORD*)(GameTimeGlobals + 0x354);//MP Only?
		if (*ChatOpened == 2) {
			extern void hotkeyFuncToggleHideIngameChat();
			hotkeyFuncToggleHideIngameChat();
		}
	}

	if (H2Config_hide_ingame_chat) {
		return true;
	}
	else if (*GameEngine != 3 && *GameState == 3) {
		//Enable chat in engine mode and game state mp.
		return false;
	}
	else {
		//original test - if is campaign
		return *GameEngine == 1;
	}
}

typedef char(__cdecl *thookChangePrivacy)(int);
thookChangePrivacy phookChangePrivacy;
char __cdecl HookChangePrivacy(int privacy) {
	char result =
		phookChangePrivacy(privacy);
	if (result == 1 && privacy == 0) {
		pushHostLobby();
	}
	return result;
}


void postConfig() {

	wchar_t mutexName2[255];
	swprintf(mutexName2, L"Halo2BasePort#%d", H2Config_base_port);
	HANDLE mutex2 = CreateMutex(0, TRUE, mutexName2);
	DWORD lastErr2 = GetLastError();
	if (lastErr2 == ERROR_ALREADY_EXISTS) {
		char NotificationPlayerText[120];
		sprintf(NotificationPlayerText, "Base port %d is already bound to!\nExpect MP to not work!", H2Config_base_port);
		addDebugText(NotificationPlayerText);
		MessageBoxA(NULL, NotificationPlayerText, "BASE PORT BIND WARNING!", MB_OK);
	}
	char NotificationText5[120];
	sprintf(NotificationText5, "Base port: %d.", H2Config_base_port);
	addDebugText(NotificationText5);

	RefreshTogglexDelay();
}

#pragma endregion

int(__cdecl* sub_20E1D8)(int, int, int, int, int, int);

int __cdecl sub_20E1D8_boot(int a1, int a2, int a3, int a4, int a5, int a6) {
	//a2 == 0x5 - system link lost connection
	if (a2 == 0xb9) {
		//boot them offline.
		H2Config_master_ip = inet_addr("127.0.0.1");
		H2Config_master_port_relay = 2001;
		extern int MasterState;
		MasterState = 2;
		extern char* ServerStatus;
		snprintf(ServerStatus, 250, "Status: Offline");
	}
	int result = sub_20E1D8(a1, a2, a3, a4, a5, a6);
	return result;
}

void InitH2Tweaks() {
	postConfig();

	addDebugText("Begin Startup Tweaks.");

	if (H2IsDediServer) {
		DWORD dwBack;

		phookServ1 = (thookServ1)DetourFunc((BYTE*)H2BaseAddr + 0x8EFA, (BYTE*)LoadRegistrySettings, 11);
		VirtualProtect(phookServ1, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//phookServ2 = (thookServ2)DetourFunc((BYTE*)H2BaseAddr + 0xBA3C, (BYTE*)PreReadyLoad, 11);
		//VirtualProtect(phookServ2, 4, PAGE_EXECUTE_READWRITE, &dwBack);
	}
	else {//is client

		DWORD dwBack;
		//Hook a function which changes the party privacy to detect if the lobby becomes open.
		//Problem is if you want to set it via mem poking, it won't push the lobby to the master automatically.
		//phookChangePrivacy = (thookChangePrivacy)DetourFunc((BYTE*)H2BaseAddr + 0x2153ce, (BYTE*)HookChangePrivacy, 11);
		//VirtualProtect(phookChangePrivacy, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//Scrapped for now, maybe.
		DWORD tempResX = 0;
		DWORD tempResY = 0;

		HKEY hKeyResolution = NULL;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Halo 2\\Video Settings", 0, KEY_READ, &hKeyResolution) == ERROR_SUCCESS) {
			GetDWORDRegKey(hKeyResolution, L"ScreenResX", &tempResX);
			GetDWORDRegKey(hKeyResolution, L"ScreenResY", &tempResY);
			RegCloseKey(hKeyResolution);
		}

		if (H2Config_custom_resolution_x > 0 && H2Config_custom_resolution_y > 0) {
			if (H2Config_custom_resolution_x != (int)tempResX || H2Config_custom_resolution_y != (int)tempResY) {
				LSTATUS err;
				if (err = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Halo 2\\Video Settings", 0, KEY_ALL_ACCESS, &hKeyResolution) == ERROR_SUCCESS) {
					RegSetValueEx(hKeyResolution, L"ScreenResX", NULL, REG_DWORD, (const BYTE*)&H2Config_custom_resolution_x, sizeof(H2Config_custom_resolution_x));
					RegSetValueEx(hKeyResolution, L"ScreenResY", NULL, REG_DWORD, (const BYTE*)&H2Config_custom_resolution_y, sizeof(H2Config_custom_resolution_y));
					RegCloseKey(hKeyResolution);
				}
				else {
					char errorMsg[200];
					sprintf(errorMsg, "Error: 0x%x. Unable to make Screen Resolution changes.\nPlease try restarting Halo 2 with Administrator Privileges.", err);
					addDebugText(errorMsg);
					MessageBoxA(NULL, errorMsg, "Registry Write Error", MB_OK);
					exit(EXIT_FAILURE);
				}
			}
		}

		bool IntroHQ = true;//clients should set on halo2.exe -highquality

		if (H2Config_skip_intro) {
			BYTE assmIntroSkip[] = { 0x3F };
			WriteBytes(H2BaseAddr + 0x221C0E, assmIntroSkip, 1);
		}

		if (!H2Config_skip_intro && IntroHQ) {
			BYTE assmIntroHQ[] = { 0xEB };
			WriteBytes(H2BaseAddr + 0x221C29, assmIntroHQ, 1);
		}

		//Allows unlimited clients
		BYTE assmUnlimitedClients[41];
		memset(assmUnlimitedClients, 0x00, 41);
		WriteBytes(H2BaseAddr + 0x39BCF0, assmUnlimitedClients, 41);

		//Allows on a remote desktop connection
		BYTE assmRemoteDesktop[] = { 0xEB };
		WriteBytes(H2BaseAddr + 0x7E54, assmRemoteDesktop, 1);

		//Disables the ESRB warning (only occurs for English Language).
		//disables the one if no intro vid occurs.
		BYTE& ESRB = *((BYTE*)H2BaseAddr + 0x411030);
		ESRB = 0;
		//disables the one after the intro video.
		BYTE assmIntroESRBSkip[] = { 0x00 };
		WriteBytes(H2BaseAddr + 0x3a0fa, assmIntroESRBSkip, 1);
		WriteBytes(H2BaseAddr + 0x3a1ce, assmIntroESRBSkip, 1);

		//Redirects the is_campaign call that the in-game chat renderer makes so we can show/hide it as we like.
		PatchCall(H2BaseAddr + 0x22667B, NotDisplayIngameChat);
		PatchCall(H2BaseAddr + 0x226628, NotDisplayIngameChat);

		//hook the gui popup for when the player is booted.
		sub_20E1D8 = (int(__cdecl*)(int, int, int, int, int, int))((char*)H2BaseAddr + 0x20E1D8);
		PatchCall(H2BaseAddr + 0x21754C, &sub_20E1D8_boot);
	}
	addDebugText("End Startup Tweaks.");
}

void DeinitH2Tweaks() {

}

void setSens(short input_type, int sens) {
	if (sens == 0)
		return; //return if sensitivity is 0

	if (input_type == 1) { //controller
		*(float*)(H2BaseAddr + 0x4A89BC) = (float)(40.0 + 10.0 * (double)sens); //y-axis
		*(float*)(H2BaseAddr + 0x4A89B8) = (float)(80.0 + 20.0 * (double)sens); //x-axis
	}
	else if (input_type == 0) { //mouse 
		*(float*)(H2BaseAddr + 0x4A89B4) = (float)(25.0 + 10.0 * (double)sens); //y-axis
		*(float*)(H2BaseAddr + 0x4A89B0) = (float)(50.0 + 20.0 * (double)sens); //x-axis
	}
}

void setFOV(int field_of_view)
{
	if (field_of_view > 0 && field_of_view <= 110)
	{
		const UINT CURRENT_FOV_OFFSET = 4883752;

		float defaultRadians = (float)(70 * 3.14159265f / 180);
		float targetRadians = (float)((double)field_of_view * 3.14159265f / 180);
		float targetRadiansVehicle = (float)((double)(field_of_view + 10) * 3.14159265f / 180);
		*(float*)(H2BaseAddr + 0x41D984) = (targetRadians / defaultRadians); //First Person
		*(float*)(H2BaseAddr + 0x413780) = (targetRadians / defaultRadians); //Vehicle
	}
}

void setCrosshairPos(float crosshair_offset) {

	if (!FloatIsNaN(crosshair_offset)) {
	DWORD CrosshairY = *(DWORD*)((char*)H2BaseAddr + 0x479E70) + 0x1AF4 + 0xf0 + 0x1C;
	*(float*)CrosshairY = crosshair_offset;
	}
	
}