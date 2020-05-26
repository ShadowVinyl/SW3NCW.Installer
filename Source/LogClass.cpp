#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#endif

#include <Windows.h>
#include <Shlwapi.h>
#include <ctime>

#include "pugixml_read.h"
#include "FileClass.h"
#include "CnsClass.h"
#include "LogClass.h"

extern const wchar_t* progver;
extern const wchar_t* LogFile;

void LogClass::InitLog()
{
	std::string xmltag = ReadXMLConfigTag("DisableLogging");
	if (xmltag == "True") return;

	char TextUI[10];
	std::string xmltagUI = ReadXMLConfigTag("InterfaceLang");
	if (xmltagUI == "Russian" || xmltagUI != "English")
		strcpy(TextUI, "Russian");
	else
		strcpy(TextUI, "English");

	const wchar_t* str = progver;
	char version[32];
	size_t len = wcstombs(version, str, wcslen(str));
	if (len > 0u)
		version[len] = '\0';
	puts(version);

	time_t t = time(0);

	wchar_t selfdir[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, selfdir, MAX_PATH);
	PathRemoveFileSpecW(selfdir);
	wcscat(selfdir, L"\\\0");
	wcscat(selfdir, LogFile);

	FILE* out = _wfopen(selfdir, L"w");
	fprintf(out, "Log started: %s\n", ctime(&t));
	fprintf(out, "Version: %s\n", version);
	fprintf(out, "Interface: %s\n\n", TextUI);
	fclose(out);
}
void LogClass::LOG(const char* format, ...)
{
	std::string xmltag = ReadXMLConfigTag("DisableLogging");
	if (xmltag == "True") return;

	const int MAXLEN = 80;
	char stime[MAXLEN];
	time_t t = time(0);
	strftime(stime, MAXLEN, "(%H:%M:%S)", localtime(&t));

	wchar_t selfdir[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, selfdir, MAX_PATH);
	PathRemoveFileSpecW(selfdir);
	wcscat(selfdir, L"\\\0");
	wcscat(selfdir, LogFile);

	FILE* out = _wfopen(selfdir, L"a");
	if (out)
	{
		va_list args;
		va_start(args, format);
		fprintf(out, "%s ", stime);
		vfprintf(out, format, args);
		fprintf(out, "\n");
		va_end(args);
		fclose(out);
	}
}
void LogClass::LOG(const wchar_t* format, ...)
{
	std::string xmltag = ReadXMLConfigTag("DisableLogging");
	if (xmltag == "True") return;

	const int MAXLEN = 80;
	wchar_t stime[MAXLEN];
	time_t t = time(0);
	wcsftime(stime, MAXLEN, L"(%H:%M:%S)", localtime(&t));

	wchar_t selfdir[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, selfdir, MAX_PATH);
	PathRemoveFileSpecW(selfdir);
	wcscat(selfdir, L"\\\0");
	wcscat(selfdir, LogFile);

	FILE* out = _wfopen(selfdir, L"a");
	if (out)
	{
		va_list args;
		va_start(args, format);
		fwprintf(out, L"%s ", stime);
		vfwprintf(out, format, args);
		fwprintf(out, L"\n");
		va_end(args);
		fclose(out);
	}
}
void LogClass::ReleaseLog()
{
	std::string xmltag = ReadXMLConfigTag("DisableLogging");
	if (xmltag == "True") return;

	const wchar_t* str = progver;
	char buf[32];
	size_t len = wcstombs(buf, str, wcslen(str));
	if (len > 0u)
		buf[len] = '\0';
	puts(buf);

	time_t t = time(0);

	wchar_t selfdir[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, selfdir, MAX_PATH);
	PathRemoveFileSpecW(selfdir);
	wcscat(selfdir, L"\\\0");
	wcscat(selfdir, LogFile);

	FILE* out = _wfopen(selfdir, L"a");
	fprintf(out, "\nLog stopped: %s\n", ctime(&t));
	fclose(out);
}