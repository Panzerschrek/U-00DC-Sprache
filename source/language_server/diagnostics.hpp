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
	std::string code;
};

bool operator==( const DocumentDiagnostic& l, const DocumentDiagnostic& r );
bool operator <( const DocumentDiagnostic& l, const DocumentDiagnostic& r );

using DocumentDiagnostics= std::vector<DocumentDiagnostic>;

// Diagnostic in specific document.
// TODO - use unordered_map.
using DiagnosticsByDocument= std::map<Uri, DocumentDiagnostics>;

// Diagnosics maps for each managed document.
// This is needed to manage properly diagnostics in common files.
using DiagnosticsBySourceDocument= std::map<Uri, DiagnosticsByDocument>;

} // namespace LangServer

} // namespace U
