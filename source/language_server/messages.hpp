#pragma once
#include <variant>
#include <vector>
#include "document_position.hpp"

namespace U
{

namespace LangServer
{

namespace Requests
{

struct Initialize{};

struct Symbols
{
	Uri uri;
};

struct References
{
	PositionInDocument position;
};

struct Definition
{
	PositionInDocument position;
};

struct Complete
{
	PositionInDocument position;
};

struct Highlight
{
	PositionInDocument position;
};

struct Rename
{
	PositionInDocument position;
	std::string new_name;
};

} // namespace Requests

using RequestId= std::variant<std::string, int64_t>;

using RequestParams= std::variant<
	Requests::Initialize,
	Requests::Symbols,
	Requests::References,
	Requests::Definition,
	Requests::Complete,
	Requests::Highlight,
	Requests::Rename >;

struct Request
{
	RequestId id;
	RequestParams params;
};

namespace Notifications
{

struct TextDocumentDidOpen
{
	Uri uri;
	std::string text;
};

struct TextDocumentDidClose
{
	Uri uri;
};

struct TextDocumentIncrementalChange
{
	DocumentRange range;
	std::string new_text;
};

using TextDocumentChange= std::variant<TextDocumentIncrementalChange, std::string>;

struct TextDocumentDidChange
{
	Uri uri;
	std::vector<TextDocumentChange> changes;
};

struct CancelRequest
{
	RequestId id;
};

} // namespace Notifications

using Notification= std::variant<
	Notifications::TextDocumentDidOpen,
	Notifications::TextDocumentDidClose,
	Notifications::TextDocumentDidChange,
	Notifications::CancelRequest >;


} // namespaceLangServer

} // namespace U
