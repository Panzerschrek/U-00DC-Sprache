#include "../lex_synt_lib_common/assert.hpp"
#include "completion.hpp"

namespace U
{

namespace LangServer
{

CompletionItemKind TranslateCompletionItemKind( const CodeBuilder::CompletionItemKind value_kind )
{
	using InKind= CodeBuilder::CompletionItemKind;
	using OutKind= CompletionItemKind;
	switch(value_kind)
	{
	case InKind::Variable: return OutKind::Variable;
	case InKind::FunctionsSet: return OutKind::Function;
	case InKind::Type: return OutKind::Class;
	case InKind::ClassField: return OutKind::Field;
	case InKind::NamesScope: return OutKind::Module; // TODO - maybe use something else here?
	case InKind::TypeTemplatesSet: return OutKind::Class; // TODO - maybe use something else here?
	};

	U_ASSERT(false);
	return CompletionItemKind::None;
}

} // namespace LangServer

} // namespace U
