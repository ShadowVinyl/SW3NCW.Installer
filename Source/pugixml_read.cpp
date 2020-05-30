#include <stdio.h>
#include <Shlwapi.h>
#include "pugixml/pugixml.hpp"

#include "./FileClass.h"

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#endif

pugi::xml_document xdoc;
pugi::xml_parse_result xresult;
pugi::xml_node xtag;

extern const wchar_t* cfgFile;

std::string ReadXMLConfigTag(const char* TagName)
{
	std::wstring filename = GetExePath() + cfgFile;
	xresult = xdoc.load_file(filename.c_str());
	if (xresult)
	{
		xtag = xdoc.child("Config").child(TagName);
		return xtag.text().as_string();
	}
	return "";
}
bool WriteXMLConfigTag(const char* TagName, const char* TagContent)
{
	std::wstring filename = GetExePath() + cfgFile;
	xresult = xdoc.load_file(filename.c_str());
	if (xresult)
	{
		xtag = xdoc.child("Config").child(TagName);
		xtag.last_child().set_value(TagContent);
		xdoc.save_file(filename.c_str());
		return TRUE;
	}
	return FALSE;
}