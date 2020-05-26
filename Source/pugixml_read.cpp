#include <stdio.h>
#include <Shlwapi.h>
#include "pugixml.hpp"

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#endif

pugi::xml_document xdoc;
pugi::xml_parse_result xresult;
pugi::xml_node xtag;

const wchar_t* cfgFile = L"filecfg.xml";

wchar_t* GetXMLConfigFileName()
{
	wchar_t selfdir[MAX_PATH];
	GetModuleFileNameW(NULL, selfdir, MAX_PATH);
	PathRemoveFileSpecW(selfdir);
	wcscat(selfdir, L"\\");
	wcscat(selfdir, cfgFile);
	return selfdir;
}
std::string ReadXMLConfigTag(const char* TagName)
{
	wchar_t* filename = GetXMLConfigFileName();
	xresult = xdoc.load_file(filename);
	if (xresult)
	{
		xtag = xdoc.child("Config").child(TagName);
		return xtag.text().as_string();
	}
	return "";
}
bool WriteXMLConfigTag(const char* TagName, const char* TagContent)
{
	wchar_t* filename = GetXMLConfigFileName();
	xresult = xdoc.load_file(filename);
	if (xresult)
	{
		xtag = xdoc.child("Config").child(TagName);
		xtag.last_child().set_value(TagContent);
		xdoc.save_file(filename);
		return TRUE;
	}
	return FALSE;
}