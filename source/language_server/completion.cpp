#include "../lex_synt_lib_common/assert.hpp"
#include "completion.hpp"

namespace U
{

namespace LangServer
{

CompletionItemKind TranslateCompletionItemKind( const CodeBuilder::CompletionValueKind value_kind )
{
	switch(value_kind)
	{
	case CodeBuilder::CompletionValueKind::Variable: return CompletionItemKind::Variable;
	case CodeBuilder::CompletionValueKind::FunctionsSet: return CompletionItemKind::Function;
	case CodeBuilder::CompletionValueKind::Type: return CompletionItemKind::Class;
	case CodeBuilder::CompletionValueKind::ClassField: return CompletionItemKind::Field;
	case CodeBuilder::CompletionValueKind::NamesScope: return CompletionItemKind::Module; // TODO - maybe use something else here?
	case CodeBuilder::CompletionValueKind::TypeTemplatesSet: return CompletionItemKind::Class; // TODO - maybe use something else here?
	};

	U_ASSERT(false);
	return CompletionItemKind::None;
}

} // namespace LangServer

} // namespace U
