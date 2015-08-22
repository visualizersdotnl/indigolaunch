
#include "platform.h"
#include "xmlutil.h"

using namespace pugi;

bool LoadXML(xml_document &document, const std::wstring &path)
{
	const xml_parse_result result = document.load_file(path.c_str());	
	if (result.status != status_ok)
	{
		switch (result.status)
		{
		case status_file_not_found:
		case status_io_error:
			SetLastError(L"Can not open or read XML file from disk: '" + path + L"'");
			return false;

		default:
			ASSERT(0);
		case status_out_of_memory:
		case status_internal_error:
			SetLastError(L"Can not read XML file due to an internal error: '" + path + L"'");
			return false;
		
		// the following will benefit from further specification and a line number!
		case status_unrecognized_tag:
		case status_bad_pi:
		case status_bad_comment:
		case status_bad_cdata:
		case status_bad_doctype:
		case status_bad_pcdata:
		case status_bad_start_element:
		case status_bad_attribute:
		case status_bad_end_element:
		case status_end_element_mismatch:
			SetLastError(L"Can not read XML file due to syntax error: '" + path + L"'");
			return false;
		}
	}
	
	return true;
}

bool LoadAndParseFlatXML(
	xml_document &document, 
	const std::wstring &path,
	const wchar_t *ppNodes[],
	int numNodes,
	std::wstring pValues[])
{
	// load XML
	if (!LoadXML(document, path))
		return false;

	// grab node values (string)
	for (int iNode = 0; iNode < numNodes; ++iNode)
	{
		const xml_node node = document.child(ppNodes[iNode]);
		if (node.empty())
		{
			std::wstring message = L"XML file does not contain required tag <";
			message += ppNodes[iNode];
			message += L">: '" + path + L"'";
			SetLastError(message);
			return false;
		}
			
		pValues[iNode] = node.child_value();
	}

	return true;
}
