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
	string xmltag = ReadXMLConfigTag("DisableLogging");
	if (xmltag == "True") return;

	char TextUI[10];
	string xmltagUI = ReadXMLConfigTag("InterfaceLang");
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
void LogClass::LogMessage(const char* message, bool ErrorFlag, const char* param1, double param2)
{
	string xmltag = ReadXMLConfigTag("DisableLogging");
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
		char format[256];
		if (ErrorFlag)
		{
			if (param1)
			{
				strcpy(format, "%s %s %s %d\n");
				fprintf(out, format, stime, message, param1, GetLastError());
			}
			else
			{
				strcpy(format, "%s %s %d\n");
				fprintf(out, format, stime, message, GetLastError());
			}
		}
		else
		{
			if (param1 && param2)
			{
				strcpy(format, "%s %s %s %.1f\n");
				fprintf(out, format, stime, message, param1, param2);
			}
			else if (param1 && !param2)
			{
				strcpy(format, "%s %s %s\n");
				fprintf(out, format, stime, message, param1);
			}
			else if (!param1 && param2)
			{
				strcpy(format, "%s %s %.1f\n");
				fprintf(out, format, stime, message, param2);
			}
			else
			{
				strcpy(format, "%s %s\n");
				fprintf(out, format, stime, message);
			}
		}
		fclose(out);
	}
}
void LogClass::LogMessage(const wchar_t* message, bool ErrorFlag, const wchar_t* param1, double param2)
{
	string xmltag = ReadXMLConfigTag("DisableLogging");
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
		wchar_t format[256];
		if (ErrorFlag)
		{
			if (param1)
			{
				wcscpy(format, L"%s %s %s %d\n");
				fwprintf(out, format, stime, message, param1, GetLastError());
			}
			else
			{
				wcscpy(format, L"%s %s %d\n");
				fwprintf(out, format, stime, message, GetLastError());
			}
		}
		else
		{
			if (param1 && param2)
			{
				wcscpy(format, L"%s %s %s %.1f\n");
				fwprintf(out, format, stime, message, param1, param2);
			}
			else if (param1 && !param2)
			{
				wcscpy(format, L"%s %s %s\n");
				fwprintf(out, format, stime, message, param1);
			}
			else if (!param1 && param2)
			{
				wcscpy(format, L"%s %s %.1f\n");
				fwprintf(out, format, stime, message, param2);
			}
			else
			{
				wcscpy(format, L"%s %s\n");
				fwprintf(out, format, stime, message);
			}
		}
		fclose(out);
	}
}
void LogClass::ReleaseLog()
{
	string xmltag = ReadXMLConfigTag("DisableLogging");
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