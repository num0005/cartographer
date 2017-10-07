#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include "H2MOD.h"
#include <iostream>
#include <Shellapi.h>
#include "H2Startup.h"
#include "H2OnscreenDebugLog.h"
#include "GSRunLoop.h"
#include <string>

using namespace std;

#include "Detour.h"

HMODULE hThis = NULL;

CRITICAL_SECTION d_lock;

UINT g_online = 1;
UINT g_debug = 0;
UINT g_port = 1000;
UINT fps_enable = 1;
UINT fps_limit = 60;
UINT field_of_view = 70;
float crosshair_offset = 0.165f;

//map downloading is off by default
UINT map_downloading_enable = 0;

//chatbox commands are off by default
UINT chatbox_commands = 0;

ULONG broadcast_server = inet_addr("149.56.81.89");


UINT g_signin[4] = { 1,0,0,0 };
CHAR g_szUserName[4][16+1] = { "Cartographer", "Cartographer", "Cartographer", "Cartographer" };
CHAR g_szToken[32+1] = { "" };


CHAR g_szWANIP[16+1] = { "127.0.0.1" };
CHAR g_szLANIP[16+1] = { "127.0.0.1" };
ULONG g_lWANIP = inet_addr("127.0.0.1");
ULONG g_lLANIP = inet_addr("127.0.0.1");

WORD g_szWANPORT = 1000;
WORD g_szLANPORT = 1000;

XUID xFakeXuid[4] = { 0xEE000000DEADC0DE, 0xEE000000DEADC0DE, 0xEE000000DEADC0DE, 0xEE000000DEADC0DE };
CHAR g_profileDirectory[512] = "Profiles";

std::wstring dlcbasepath;

#pragma region H2 GunGame Variables
	bool b_GunGame = 0;
	int weapon_one = 0;
	int weapon_two = 0;
	int weapon_three = 0;
	int weapon_four = 0;
	int weapon_five = 0;
	int weapon_six = 0;
	int weapon_seven = 0;
	int weapon_eight = 0;
	int weapon_nine = 0;
	int weapon_ten = 0;
	int weapon_eleven = 0;
	int weapon_tweleve = 0;
	int weapon_thirteen = 0;
	int weapon_fourteen = 0;
	int weapon_fiffteen = 0;
	int weapon_sixteen = 0;
#pragma endregion

std::string ModulePathA(HMODULE hModule = NULL)
{
	static char strPath[MAX_PATH];
	GetModuleFileNameA(hModule, strPath, MAX_PATH);
	return strPath;
}

std::wstring ModulePathW(HMODULE hModule = NULL)
{
	static wchar_t strPath[MAX_PATH];
	GetModuleFileNameW(hModule, strPath, MAX_PATH);
	return strPath;
}

#ifndef NO_TRACE
FILE * logfile = NULL;
FILE * loggame = NULL;
FILE * loggamen = NULL;

void trace(LPWSTR message, ...)
{
	if (!logfile)
		return;

	EnterCriticalSection (&d_lock);
	SYSTEMTIME	t;
	GetLocalTime (&t);

	fwprintf (logfile, L"%02d/%02d/%04d %02d:%02d:%02d.%03d ", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
	
	va_list	arg;
	va_start (arg, message);

	vfwprintf (logfile, message, arg);
	fwprintf(logfile, L"\n");

	fflush (logfile);
	va_end (arg);
	LeaveCriticalSection (&d_lock);
}

void trace2(LPWSTR message, ...)
{
	if (!logfile)
		return;

	EnterCriticalSection (&d_lock);
	SYSTEMTIME	t;
	GetLocalTime (&t);

	fwprintf (logfile, L"%02d/%02d/%04d %02d:%02d:%02d.%03d ", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

	va_list	arg;
	va_start (arg, message);

	vfwprintf (logfile, message, arg);
	fwprintf(logfile, L"\n");

	fflush(logfile);
	va_end (arg);
	LeaveCriticalSection (&d_lock);
}

void trace_game(LPWSTR message, ...)
{
	if (!loggame)
		return;

	EnterCriticalSection(&d_lock);
	SYSTEMTIME	t;
	GetLocalTime(&t);

	fwprintf(loggame, L"%02d/%02d/%04d %02d:%02d:%02d.%03d ", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

	va_list	arg;
	va_start(arg, message);

	vfwprintf(loggame, message, arg);
	fwprintf(loggame, L"\n");


	fflush(loggame);
	va_end(arg);
	LeaveCriticalSection(&d_lock);
}

void trace_game_network(LPSTR message, ...)
{
	if (!loggamen)
		return;

	EnterCriticalSection(&d_lock);
	SYSTEMTIME	t;
	GetLocalTime(&t);

	fprintf(loggamen, "%02d/%02d/%04d %02d:%02d:%02d.%03d ", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

	va_list	arg;
	va_start(arg, message);

	vfprintf(loggamen, message, arg);
	fprintf(loggamen, "\n");

	fflush(loggamen);
	va_end(arg);
	LeaveCriticalSection(&d_lock);
}

void trace_game_narrow(LPSTR message, ...)
{
	if (!loggame)
		return;


	EnterCriticalSection(&d_lock);
	SYSTEMTIME	t;
	GetLocalTime(&t);

	fprintf(loggame, "%02d/%02d/%04d %02d:%02d:%02d.%03d ", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

	va_list	arg;
	va_start(arg, message);

	vfprintf(loggame, message, arg);
	fprintf(loggame, "\n");

	fflush(loggame);
	va_end(arg);
	LeaveCriticalSection(&d_lock);
}
#endif

FILE* OpenLog(std::wstring name)
{
	if (getPlayerNumber() > 1)
		name += std::to_wstring(getPlayerNumber());
	name += L".log";
	return _wfopen(name.c_str(), L"wt");
}

void InitInstance()
{
	static bool init = true;

	if(init)
	{
		init = false;

#ifdef _DEBUG
		int CurrentFlags;
		CurrentFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		CurrentFlags |= _CRTDBG_DELAY_FREE_MEM_DF;
		CurrentFlags |= _CRTDBG_LEAK_CHECK_DF;
		CurrentFlags |= _CRTDBG_CHECK_ALWAYS_DF;
		_CrtSetDbgFlag(CurrentFlags);
#endif
		InitializeCriticalSection (&d_lock);

		dlcbasepath = L"DLC";


		strcpy( g_szUserName[0], "Player1" );
		strcpy( g_szUserName[1], "Player2" );
		strcpy( g_szUserName[2], "Player3" );
		strcpy( g_szUserName[3], "Player4" );



		LPWSTR iniFile = new WCHAR[256];
		if (getPlayerNumber() > 1) {
			swprintf(iniFile, L"xlive%d.ini", getPlayerNumber());
		}
		else {
			lstrcpyW(iniFile, L"xlive.ini");
		}

		int ArgCnt;
		LPWSTR* ArgList = CommandLineToArgvW(GetCommandLineW(), &ArgCnt);
		if (ArgList != NULL)
		{
			for (int i = 0; i < ArgCnt; i++)
			{
				if (wcsstr(ArgList[i], L"-h2online=") != NULL)
				{
					if (wcslen(ArgList[i]) < 255)
						lstrcpyW(iniFile, ArgList[i] + 10);
				}
			}
		}

		FILE *fp;
		fp = _wfopen( iniFile, L"r" );

		char iniabnorm[255];
		sprintf(iniabnorm, "c:\\xlive%d.ini", getPlayerNumber());
		if (getPlayerNumber() > 1) {
			if (!fp)
				fp = fopen(iniabnorm, "r");
		}
		else {
			if (!fp)
				fp = fopen("c:\\xlive.ini", "r");
		}

		if( fp )
		{
			while( !feof(fp) )
			{
				char str[256];


				fgets( str, 256, fp );



#define CHECK_ARG_STR(x,y) \
	if( strstr( str,x ) == str ) \
	{ \
		sscanf( str + strlen(x), "%s", &y ); \
		continue; \
	}


#define CHECK_ARG(x,y) \
	if( strstr( str,x ) == str ) \
	{ \
		sscanf( str + strlen(x), "%d", &y ); \
		continue; \
	}

#define CHECK_ARG_FLOAT(x,y) \
	if( strstr(str, x) == str) \
	{ \
		sscanf(str + strlen(x), "%f", &y ); \
		continue; \
	}

#define CHECK_ARG_I64(x,y) \
	if( strstr( str,x ) == str ) \
	{ \
		sscanf( str + strlen(x), "%I64x", &y ); \
		continue; \
	}

				CHECK_ARG_STR("login_token =", g_szToken);
				CHECK_ARG_STR("WANIP =", g_szWANIP);
				CHECK_ARG_STR("LANIP =", g_szLANIP);
				CHECK_ARG("debug_log =", g_debug);
				CHECK_ARG("port =", g_port);
				CHECK_ARG("fps_enable = ", fps_enable);
				CHECK_ARG("fps_limit = ", fps_limit);
				CHECK_ARG("field_of_view = ", field_of_view);
				CHECK_ARG_FLOAT("crosshair_offset = ", crosshair_offset);
				CHECK_ARG("map_downloading_enable = ", map_downloading_enable);
				CHECK_ARG("chatbox_commands = ", chatbox_commands);
			}

			
			fclose(fp);
		}

#pragma region GunGame Levels
		if (b_GunGame == 1)
		{
			FILE* gfp;
			gfp = fopen("gungame.ini", "r");
			
			if (gfp)
			{
				TRACE("[GunGame Enabled] - Opened GunGame.ini!");
				while (!feof(gfp))
				{
					char gstr[256];


					fgets(gstr, 256, gfp);

#define gCHECK_ARG(x,y) \
	if( strstr( gstr,x ) == gstr ) \
	{ \
		sscanf( gstr + strlen(x), "%d", &y ); \
		continue; \
	}

					gCHECK_ARG("weapon_one =", weapon_one);
					gCHECK_ARG("weapon_two =", weapon_two);
					gCHECK_ARG("weapon_three =", weapon_three);
					gCHECK_ARG("weapon_four =", weapon_four);
					gCHECK_ARG("weapon_five =", weapon_five);
					gCHECK_ARG("weapon_six =", weapon_six);
					gCHECK_ARG("weapon_seven =", weapon_seven);
					gCHECK_ARG("weapon_eight =", weapon_eight);
					gCHECK_ARG("weapon_nine =", weapon_nine);
					gCHECK_ARG("weapon_ten =", weapon_ten);
					gCHECK_ARG("weapon_eleven =", weapon_eleven);
					gCHECK_ARG("weapon_tweleve =", weapon_tweleve);
					gCHECK_ARG("weapon_thirteen =", weapon_thirteen);
					gCHECK_ARG("weapon_fourteen =", weapon_fourteen);
					gCHECK_ARG("weapon_fifteen =", weapon_fiffteen);
					gCHECK_ARG("weapon_sixteen =", weapon_sixteen);

				}
				
				fclose(gfp);
			}
		}
		
#pragma endregion
		g_lLANIP = inet_addr(g_szLANIP);
		g_lWANIP = inet_addr(g_szWANIP);

		wchar_t mutexName[255];
		swprintf(mutexName, L"Halo2Login#%s", g_szToken);
		HANDLE mutex = CreateMutex(0, TRUE, mutexName);
		DWORD lastErr = GetLastError();
		char token_censored[33];
		strncpy(token_censored, g_szToken, 32);
		memset(token_censored + 32, 0, 1);
		memset(token_censored + 4, '*', 24);
		if (lastErr == ERROR_ALREADY_EXISTS) {
			//CloseHandle(mutex);
			char NotificationPlayerText[120];
			sprintf(NotificationPlayerText, "Player Login Session %s already exists!\nOld session has been invalidated!", token_censored);
			addDebugText(NotificationPlayerText);
			MessageBoxA(NULL, NotificationPlayerText, "LOGIN OVERRIDDEN WARNING!", MB_OK);
		}
		char NotificationText4[120];
		sprintf(NotificationText4, "Login Token: %s.", token_censored);
		addDebugText(NotificationText4);

		wchar_t mutexName2[255];
		swprintf(mutexName2, L"Halo2BasePort#%d", g_port);
		HANDLE mutex2 = CreateMutex(0, TRUE, mutexName2);
		DWORD lastErr2 = GetLastError();
		if (lastErr2 == ERROR_ALREADY_EXISTS) {
			//CloseHandle(mutex);
			char NotificationPlayerText[120];
			sprintf(NotificationPlayerText, "Base port %d is already bound to!\nExpect MP to not work!", g_port);
			addDebugText(NotificationPlayerText);
			MessageBoxA(NULL, NotificationPlayerText, "BASE PORT BIND WARNING!", MB_OK);
		}
		char NotificationText5[120];
		sprintf(NotificationText5, "Base port: %d.", g_port);
		addDebugText(NotificationText5);

		if (g_debug)
		{
			if (logfile = OpenLog(L"xlive_trace"))
			{
				TRACE("Log started (xLiveLess 2.0a4)\n");
				TRACE("g_port: %i", g_port);
				TRACE("inifile: %ws", iniFile);
				TRACE("g_lWANIP: %08X", g_lWANIP);
				TRACE("g_lLANIP: %08X", g_lLANIP);

			}

			if (loggame = OpenLog(L"h2mod"))
				TRACE_GAME("Log started (H2MOD 0.1a1)\n");

			if (loggamen = OpenLog(L"h2network.log"))
				TRACE_GAME_NETWORK("Log started (H2MOD - Network 0.1a1)\n");
		}

			if (h2mod)
				h2mod->Initialize();
			else
				TRACE("H2MOD Failed to intialize");

			TRACE("[GunGame] : %i", b_GunGame);
			if (b_GunGame == 1)
			{
				TRACE("[GunGame] - weapon_one: %i", weapon_one);
				TRACE("[GunGame] - weapon_two: %i", weapon_two);
				TRACE("[GunGame] - weapon_three: %i", weapon_three);
			}

			WCHAR gameName[256];
			
			GetModuleFileNameW( NULL, (LPWCH) &gameName, sizeof(gameName) );
			TRACE( "%s", gameName );

		


		extern void LoadAchievements();
		LoadAchievements();
	}
}

void ExitInstance()
{
	EnterCriticalSection (&d_lock);

	if (logfile)
	{
	
		TRACE("Shutting down");

		fflush (logfile);
		fclose (logfile);
		fflush (loggame);
		fclose (loggame);
		fclose (loggamen);
		fflush (loggamen);
		
		logfile = NULL;
		loggame = NULL;
		loggamen = NULL;
	}



	LeaveCriticalSection (&d_lock);
	DeleteCriticalSection (&d_lock);



	extern void SaveAchievements();
	SaveAchievements();

	ExitProcess(0);
}

//=============================================================================
// Entry Point
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hThis = hModule;
		srand((unsigned int)time(NULL));
		ProcessH2Startup();
		//system("update.bat"); // fucking broken h2online.exe -_- This will update that...
		Detour();
		break;


	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;


	case DLL_PROCESS_DETACH:
		ExitInstance();
		break;
	}
	return TRUE;
}


