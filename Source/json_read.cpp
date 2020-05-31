#include <json/json.h>
#include <json/value.h>
#include <json/json-src/json_tool.h>
#include <Shlwapi.h>
#include <codecvt>
#include <fstream>

#include "json_read.h"
#include "FileClass.h"
#include "CnsClass.h"
#include "LogClass.h"

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#endif

std::string ReadJSONLocTag(const char* tagname)
{
    std::string result;
    std::wstring filename, secPath;

    extern int CurrentLang;
    if (CurrentLang == 0)
        secPath = L"locale\\loc-ru-ru.json";
    else
        secPath = L"locale\\loc-en-us.json";
    
    filename = GetExePath() + secPath;

	Json::Value cfg_root;

    std::ifstream cfgfile(filename);
    if (cfgfile.good())
    {
        cfgfile >> cfg_root;
        result = cfg_root["language"][tagname].asString();
    }
    else
    {
        CnsClass::ShowConsole();
        CnsClass::Print("Red", "Localization file was not found in locale folder! (%d)", GetLastError());
        LogClass::LOG("(ERROR) [Main] Localization file was not found in locale folder! (%d)", GetLastError());
        result = "Loc Error!";
    }

    return result;
}