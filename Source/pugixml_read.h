#pragma once

#include "pugixml.hpp"

wchar_t* GetXMLConfigFileName();
std::string ReadXMLConfigTag(const char* TagName);
bool WriteXMLConfigTag(const char* TagName, const char* TagContent);