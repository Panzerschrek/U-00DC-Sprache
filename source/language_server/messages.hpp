#pragma once
#include <variant>
#include <vector>
#include "document_position.hpp"

namespace U
{

namespace LangServer
{

// Pseudo-requests/notifications for cases, where parsing of a request/notification fails.

struct InvalidParams
{
	std::string message;
};

struct MethodNotFound
{
	std::string method_name;
};

namespace Requests
{

struct Initialize{};
struct Shutdown{};

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
	InvalidParams,
	MethodNotFound,
	Requests::Initialize,
	Requests::Shutdown,
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

struct Initialized{};

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

struct TextDocumentWillSave
{
	Uri uri;
};

struct TextDocumentDidSave
{
	Uri uri;
};

struct CancelRequest
{
	RequestId id;
};

} // namespace Notifications

using Notification= std::variant<
	InvalidParams,
	MethodNotFound,
	Notifications::Initialized,
	Notifications::TextDocumentDidOpen,
	Notifications::TextDocumentDidClose,
	Notifications::TextDocumentDidChange,
	Notifications::TextDocumentWillSave,
	Notifications::TextDocumentDidSave,
	Notifications::CancelRequest >;

using Message= std::variant<Request, Notification>;

} // namespaceLangServer

} // namespace U
