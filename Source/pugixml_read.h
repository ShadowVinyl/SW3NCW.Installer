#pragma once

#include "pugixml.hpp"

using namespace std;

wchar_t* GetXMLConfigFileName();
string ReadXMLConfigTag(const char* TagName);
string WriteXMLConfigTag(const char* TagName, const char* TagContent);