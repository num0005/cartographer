#pragma once
#include <codecvt>

#ifdef XLIVELESS_EXPORTS
#define XLIVELESS_API extern "C" __declspec(dllexport)
#else
#define XLIVELESS_API extern "C" __declspec(dllimport)
#endif

#define XLIVELESS_VERSION   0x00020000  // 2.0.0

//useful macros
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_FREE(p)  { if(p) { free(p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#define ARRAYOF(x) (sizeof(x)/sizeof(*x))
#define IN_RANGE(val, min, max) ((val) > (min) && (val) < (max))
#define IN_RANGE2(val, min, max) ((val) >= (min) && (val) <= (max))
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define BYTESOF(a, b) ((a) * sizeof(b))

extern HMODULE hThis;
extern CRITICAL_SECTION d_lock;
extern bool H2Config_debug_log;
extern int instanceNumber;

class Logger {
public:
	Logger(const wchar_t *log_name)
	{
#ifndef NO_TRACE
		if (H2Config_debug_log) {
			std::wstring filename = log_name;
			if (instanceNumber != 1)
				filename += std::to_wstring(instanceNumber) + L".";
			filename += L".log";
			logfile = _wfopen(filename.c_str(), L"wt");
		}
#endif
	}
	~Logger()
	{
		if (!logfile)
			return;
		fflush(logfile);
		fclose(logfile);
	}
	void log(const wchar_t *message, va_list arg)
	{
		if (!logfile)
			return;

		EnterCriticalSection(&d_lock);
		SYSTEMTIME	t;
		GetLocalTime(&t);

		fwprintf(logfile, L"%02d/%02d/%04d %02d:%02d:%02d.%03d ", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

		vfwprintf(logfile, message, arg);
		fwprintf(logfile, L"\n");

		fflush(logfile);
		LeaveCriticalSection(&d_lock);
	}

	void log(wchar_t *msg, ...)
	{
		if (!logfile)
			return;
		va_list arg;
		va_start(arg, msg);
		log(const_cast<const wchar_t*>(msg), arg);
		va_end(arg);
	}

	void log(std::wstring &msg, ...)
	{
		if (!logfile)
			return;
		va_list arg;
		const wchar_t *wmessage = msg.c_str();
		va_start(arg, wmessage);
		log(wmessage, arg);
		va_end(arg);
	}

	void log(std::string &message, ...)
	{
		if (!logfile)
			return;
		std::wstring_convert<std::codecvt_utf8<wchar_t>> wstring_to_string;
		std::wstring wmessage = wstring_to_string.from_bytes(message);
		va_list arg;
		va_start(arg, wmessage);
		log(wmessage.c_str(), arg);
		va_end(arg);
	}

	void log(const char* message, ...)
	{
		if (!logfile)
			return;
		std::wstring_convert<std::codecvt_utf8<wchar_t>> wstring_to_string;
		std::wstring wmessage = wstring_to_string.from_bytes(message);
		va_list arg;
		va_start(arg, wmessage);
		log(wmessage.c_str(), arg);
		va_end(arg);
	}
private:
	FILE *logfile = nullptr;
};

extern Logger *xlive_logger;
extern Logger *h2mod_logger;
extern Logger *h2network_logger;
extern Logger *lua_logger;

#ifndef NO_TRACE
#define TRACE_GAME(msg, ...) h2mod_logger->log(L ## msg, __VA_ARGS__)
#define TRACE_GAME_NETWORK(msg, ...) h2network_logger->log( ## msg, __VA_ARGS__ )
#define TRACE(msg, ...) xlive_logger->log(L ## msg, __VA_ARGS__)

//#define trace()
#else
#define TRACE()
#define TRACE_GAME()
#define TRACE_GAME()
#define TRACE_GAME_NETWORK()
#endif

void update_player_count();
std::string getVariantName();