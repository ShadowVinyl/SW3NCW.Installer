#pragma once

#include "pugixml/pugixml.hpp"

std::string ReadXMLConfigTag(const char* TagName);
bool WriteXMLConfigTag(const char* TagName, const char* TagContent);