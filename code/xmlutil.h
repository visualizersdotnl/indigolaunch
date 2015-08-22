
#pragma once

#include "../3rdparty/pugixml-1.2/src/pugixml.hpp"

// load XML
bool LoadXML(pugi::xml_document &document, const std::wstring &path);

// load XML, parse and copy requested node values to strings (flat hierarchy only!)
bool LoadAndParseFlatXML(
	pugi::xml_document &document, 
	const std::wstring &path,
	const wchar_t *ppNodes[],
	int numNodes,
	std::wstring pValues[]);
