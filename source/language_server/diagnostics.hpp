#pragma once
#include <map>
#include <vector>
#include "document_position.hpp"

namespace U
{

namespace LangServer
{

struct DocumentDiagnostic
{
	DocumentRange range;
	std::string text;
};

using DocumentDiagnostics= std::vector<DocumentDiagnostic>;

// TODO - use unordered_map.
using DiagnosticsByDocument= std::map<Uri, DocumentDiagnostics>;

} // namespace LangServer

} // namespace U
