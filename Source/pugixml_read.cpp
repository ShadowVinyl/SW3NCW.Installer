#include <stdio.h>
#include <Shlwapi.h>
#include "pugixml.hpp"

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#endif

using namespace std;

pugi::xml_document xdoc;
pugi::xml_parse_result xresult;
pugi::xml_node xtag;

const wchar_t* cfgFile = L"config.xml";

wchar_t* GetXMLConfigFileName()
{
	wchar_t selfdir[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, selfdir, MAX_PATH);
	PathRemoveFileSpecW(selfdir);
	wcscat(selfdir, L"\\\0");
	wcscat(selfdir, cfgFile);
	return selfdir;
}
string ReadXMLConfigTag(const char* TagName)
{
	//wchar_t* filename = GetXMLConfigFileName();
	xresult = xdoc.load_file(cfgFile);
	if (xresult)
	{
		xtag = xdoc.child("Config").child(TagName);
		return xtag.text().as_string();
	}
	else
	{
		printf("PugiXML: Cant load config file for reading!");
		return "";
	}
}
bool WriteXMLConfigTag(const char* TagName, const char* TagContent)
{
	//wchar_t* filename = GetXMLConfigFileName();
	xresult = xdoc.load_file(cfgFile);
	if (xresult)
	{
		xtag = xdoc.child("Config").child(TagName);
		xtag.last_child().set_value(TagContent);
		xdoc.save_file(cfgFile);
	}
	else
	{
		printf("PugiXML: Cant load config file for writting!");
		return FALSE;
	}

	return TRUE;
}