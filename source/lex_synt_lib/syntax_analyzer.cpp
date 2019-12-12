#include <cctype>
#include <map>
#include <set>

#include "assert.hpp"
#include "keywords.hpp"
#include "syntax_analyzer.hpp"

namespace U
{

namespace Synt
{

namespace
{

struct ExpectedLexem
{
	ExpectedLexem( const Lexem::Type in_type ) : type(in_type) {}
	ExpectedLexem( const Keywords keyword ) : type(Lexem::Type::Identifier), text( Keyword( keyword) ) {}

	Lexem::Type type;
	ProgramString text;
};

const std::vector<ExpectedLexem> g_namespace_body_elements_start_lexems
{
	ExpectedLexem(Keywords::namespace_),
	ExpectedLexem(Keywords::class_), ExpectedLexem(Keywords::struct_),
	ExpectedLexem(Keywords::fn_), ExpectedLexem(Keywords::op_),
	ExpectedLexem(Keywords::var_), ExpectedLexem(Keywords::auto_),
	ExpectedLexem(Keywords::static_assert_),
};

const std::vector<ExpectedLexem> g_class_body_elements_control_lexems
{
	ExpectedLexem(Keywords::class_), ExpectedLexem(Keywords::struct_),
	ExpectedLexem(Keywords::fn_), ExpectedLexem(Keywords::op_),
	ExpectedLexem(Keywords::var_), ExpectedLexem(Keywords::auto_),
	ExpectedLexem(Keywords::static_assert_),
	ExpectedLexem(Lexem::Type::BraceRight),
};

const std::vector<ExpectedLexem> g_block_body_elements_control_lexems
{
	ExpectedLexem(Keywords::if_), ExpectedLexem(Keywords::static_if_), ExpectedLexem(Keywords::while_),
	ExpectedLexem(Keywords::return_), ExpectedLexem(Keywords::break_), ExpectedLexem(Keywords::continue_),
	ExpectedLexem(Keywords::var_), ExpectedLexem(Keywords::auto_),
	ExpectedLexem(Keywords::halt_),
	ExpectedLexem(Keywords::static_assert_),
};

const std::vector<ExpectedLexem> g_function_arguments_list_control_lexems
{
	ExpectedLexem(Lexem::Type::Comma), ExpectedLexem(Lexem::Type::BracketRight),
};

const std::vector<ExpectedLexem> g_template_arguments_list_control_lexems
{
	ExpectedLexem(Lexem::Type::Comma), ExpectedLexem(Lexem::Type::TemplateBracketRight),
};

int GetBinaryOperatorPriority( const BinaryOperatorType binary_operator )
{
	#define PRIORITY ( - __LINE__ )

	switch( binary_operator )
	{
	case BinaryOperatorType::Div:
	case BinaryOperatorType::Mul:
	case BinaryOperatorType::Rem:
		return PRIORITY;
	case BinaryOperatorType::Add:
	case BinaryOperatorType::Sub:
		return PRIORITY;
	case BinaryOperatorType::ShiftLeft :
	case BinaryOperatorType::ShiftRight:
		return PRIORITY;
	case BinaryOperatorType::Equal:
	case BinaryOperatorType::NotEqual:
	case BinaryOperatorType::Less:
	case BinaryOperatorType::LessEqual:
	case BinaryOperatorType::Greater:
	case BinaryOperatorType::GreaterEqual:
		return PRIORITY;
	case BinaryOperatorType::And: return PRIORITY;
	case BinaryOperatorType::Or: return PRIORITY;
	case BinaryOperatorType::Xor: return PRIORITY;
	case BinaryOperatorType::LazyLogicalAnd: return PRIORITY;
	case BinaryOperatorType::LazyLogicalOr: return PRIORITY;

	case BinaryOperatorType::Last: break;
	};

	U_ASSERT(false);
	return PRIORITY;

	#undef PRIORITY
}

bool IsBinaryOperator( const Lexem& lexem )
{
	return
		lexem.type == Lexem::Type::Plus ||
		lexem.type == Lexem::Type::Minus ||
		lexem.type == Lexem::Type::Star ||
		lexem.type == Lexem::Type::Slash ||
		lexem.type == Lexem::Type::Percent ||

		lexem.type == Lexem::Type::CompareEqual ||
		lexem.type == Lexem::Type::CompareNotEqual ||
		lexem.type == Lexem::Type::CompareLess ||
		lexem.type == Lexem::Type::CompareLessOrEqual ||
		lexem.type == Lexem::Type::CompareGreater ||
		lexem.type == Lexem::Type::CompareGreaterOrEqual ||

		lexem.type == Lexem::Type::And ||
		lexem.type == Lexem::Type::Or ||
		lexem.type == Lexem::Type::Xor ||

		lexem.type == Lexem::Type::ShiftLeft  ||
		lexem.type == Lexem::Type::ShiftRight ||

		lexem.type == Lexem::Type::Conjunction ||
		lexem.type == Lexem::Type::Disjunction;
}

BinaryOperatorType LexemToBinaryOperator( const Lexem& lexem )
{
	switch( lexem.type )
	{
		case Lexem::Type::Plus: return BinaryOperatorType::Add;
		case Lexem::Type::Minus: return BinaryOperatorType::Sub;
		case Lexem::Type::Star: return BinaryOperatorType::Mul;
		case Lexem::Type::Slash: return BinaryOperatorType::Div;
		case Lexem::Type::Percent: return BinaryOperatorType::Rem;

		case Lexem::Type::CompareEqual: return BinaryOperatorType::Equal;
		case Lexem::Type::CompareNotEqual: return BinaryOperatorType::NotEqual;
		case Lexem::Type::CompareLess: return BinaryOperatorType::Less;
		case Lexem::Type::CompareLessOrEqual: return BinaryOperatorType::LessEqual;
		case Lexem::Type::CompareGreater: return BinaryOperatorType::Greater;
		case Lexem::Type::CompareGreaterOrEqual: return BinaryOperatorType::GreaterEqual;

		case Lexem::Type::And: return BinaryOperatorType::And;
		case Lexem::Type::Or: return BinaryOperatorType::Or;
		case Lexem::Type::Xor: return BinaryOperatorType::Xor;

		case Lexem::Type::ShiftLeft : return BinaryOperatorType::ShiftLeft ;
		case Lexem::Type::ShiftRight: return BinaryOperatorType::ShiftRight;

		case Lexem::Type::Conjunction: return BinaryOperatorType::LazyLogicalAnd;
		case Lexem::Type::Disjunction: return BinaryOperatorType::LazyLogicalOr;

		default:
		U_ASSERT(false);
		return BinaryOperatorType::Add;
	};
}

bool IsAdditiveAssignmentOperator( const Lexem& lexem )
{
	switch(lexem.type)
	{
		case Lexem::Type::AssignAdd:
		case Lexem::Type::AssignSub:
		case Lexem::Type::AssignMul:
		case Lexem::Type::AssignDiv:
		case Lexem::Type::AssignAnd:
		case Lexem::Type::AssignRem:
		case Lexem::Type::AssignOr :
		case Lexem::Type::AssignXor:
		case Lexem::Type::AssignShiftLeft :
		case Lexem::Type::AssignShiftRight:
			return true;
		default: break;
	};

	return false;
}

BinaryOperatorType GetAdditiveAssignmentOperator( const Lexem& lexem )
{
	switch(lexem.type)
	{
		case Lexem::Type::AssignAdd: return BinaryOperatorType::Add;
		case Lexem::Type::AssignSub: return BinaryOperatorType::Sub;
		case Lexem::Type::AssignMul: return BinaryOperatorType::Mul;
		case Lexem::Type::AssignDiv: return BinaryOperatorType::Div;
		case Lexem::Type::AssignAnd: return BinaryOperatorType::And;
		case Lexem::Type::AssignRem: return BinaryOperatorType::Rem;
		case Lexem::Type::AssignOr : return BinaryOperatorType::Or;
		case Lexem::Type::AssignXor: return BinaryOperatorType::Xor;
		case Lexem::Type::AssignShiftLeft : return BinaryOperatorType::ShiftLeft;
		case Lexem::Type::AssignShiftRight: return BinaryOperatorType::ShiftRight;

		default:
		U_ASSERT(false);
		return BinaryOperatorType::Add;
	};
}

double PowI( const uint64_t base, const uint64_t pow )
{
	if( pow == 0u )
		return 1.0;
	if( pow == 1u )
		return double(base);
	if( pow == 2u )
		return double(base * base);

	const uint64_t half_pow= pow / 2u;
	double res= PowI( base, half_pow );
	res= res * res;
	if( half_pow * 2u != pow )
		res*= double(base);
	return res;
}

class SyntaxAnalyzer final
{
public:
	SyntaxAnalyzer();
	SyntaxAnalyzer( const MacrosPtr& macros );

	SyntaxAnalysisResult DoAnalyzis( const Lexems& lexems );
	std::vector<Import> ParseImportsOnly( const Lexems& lexems );

private:
	struct ParsedMacroElement
	{
		Lexems::const_iterator begin;
		Lexems::const_iterator end;
		std::vector< std::map<ProgramString, ParsedMacroElement> > sub_elements;
		Macro::MatchElementKind kind= Macro::MatchElementKind::Lexem;
	};

	struct MacroNamesMap
	{
		const MacroNamesMap* prev= nullptr;
		const std::map<ProgramString, ParsedMacroElement>* names= nullptr;

		const ParsedMacroElement* GetElement( const ProgramString& name ) const
		{
			const auto it= names->find(name);
			if( it != names->end() ) return &it->second;
			if( prev != nullptr ) return prev->GetElement(name);
			return nullptr;
		}
	};

private:
	std::vector<Import> ParseImports();

	void ParseMacro();
	std::vector<Macro::MatchElement> ParseMacroMatchBlock();
	std::vector<Macro::ResultElement> ParseMacroResultBlock();

	ProgramElements ParseNamespaceBody() { return ParseNamespaceBody( Lexem::Type::BraceRight ); }
	ProgramElements ParseNamespaceBody( Lexem::Type end_lexem );

	NumericConstant ParseNumericConstant();

	Expression ParseExpression();

	FunctionArgument ParseFunctionArgument();
	void ParseFunctionTypeEnding( FunctionType& result );
	FunctionTypePtr ParseFunctionType();

	TypeName ParseTypeName();
	std::vector<Expression> ParseTemplateParameters();
	ComplexName ParseComplexName();
	ReferencesTagsList ParseReferencesTagsList();
	FunctionReferencesPollutionList ParseFunctionReferencesPollutionList();

	Initializer ParseInitializer( bool parse_expression_initializer );
	Initializer ParseVariableInitializer();
	Initializer ParseArrayInitializer();
	Initializer ParseStructNamedInitializer();
	Initializer ParseConstructorInitializer();
	Initializer ParseExpressionInitializer();

	VariablesDeclaration ParseVariablesDeclaration();
	AutoVariableDeclaration ParseAutoVariableDeclaration();

	ReturnOperator ParseReturnOperator();
	WhileOperator ParseWhileOperator();
	ForOperator ParseForOperator();
	BreakOperator ParseBreakOperator();
	ContinueOperator ParseContinueOperator();
	IfOperator ParseIfOperator();
	StaticIfOperator ParseStaticIfOperator();
	StaticAssert ParseStaticAssert();
	Enum ParseEnum();
	BlockElement ParseHalt();

	std::vector<BlockElement> ParseBlockElements();
	Block ParseBlock();

	ClassKindAttribute TryParseClassKindAttribute();
	std::vector<ComplexName> TryParseClassParentsList();
	bool TryParseClassSharedState();
	bool TryParseClassFieldsOrdered();

	Typedef ParseTypedef();
	Typedef ParseTypedefBody();
	std::unique_ptr<Function> ParseFunction();
	std::unique_ptr<Class> ParseClass();
	ClassElements ParseClassBodyElements();
	std::unique_ptr<Class> ParseClassBody();

	using TemplateVar=
		std::variant<
			EmptyVariant,
			ClassTemplate,
			TypedefTemplate,
			FunctionTemplate >;
	TemplateVar ParseTemplate();

	const Macro* FetchMacro( const ProgramString& macro_name, const Macro::Context context );

	template<typename ParseFnResult>
	ParseFnResult ExpandMacro( const Macro& macro, ParseFnResult (SyntaxAnalyzer::*parse_fn)() );

	bool MatchMacroBlock(
		const std::vector<Macro::MatchElement>& match_elements,
		const ProgramString& macro_name,
		std::map<ProgramString, ParsedMacroElement>& out_elements );

	Lexems DoExpandMacro(
		const MacroNamesMap& parsed_elements,
		const std::vector<Macro::ResultElement>& result_elements,
		ProgramStringMap<ProgramString>& unique_macro_identifier_map );

	void NextLexem();
	bool NotEndOfFile();

	void TryRecoverAfterError( const std::vector<ExpectedLexem>& expected_lexems );
	void TryRecoverAfterError( const std::vector<ExpectedLexem>& expected_lexems0, const std::vector<ExpectedLexem>& expected_lexems1 );
	void TryRecoverAfterError( const std::vector<ExpectedLexem>& expected_lexems0, const std::vector<ExpectedLexem>& expected_lexems1, const std::vector<ExpectedLexem>& expected_lexems2 );
	void TrySkipBrackets( Lexem::Type bracket_type );
	void PushErrorMessage();

private:
	SyntaxErrorMessages error_messages_;
	Lexems::const_iterator it_;
	Lexems::const_iterator it_end_;

	Lexems::const_iterator last_error_it_;
	size_t last_error_repeats_;

	const MacrosPtr macros_;
};

SyntaxAnalyzer::SyntaxAnalyzer()
	: macros_(std::make_shared<MacrosByContextMap>())
{}

SyntaxAnalyzer::SyntaxAnalyzer( const MacrosPtr& macros )
	: macros_(macros)
{}

SyntaxAnalysisResult SyntaxAnalyzer::DoAnalyzis( const Lexems& lexems )
{
	SyntaxAnalysisResult result;

	it_= lexems.begin();
	it_end_= lexems.end();
	last_error_it_= lexems.end();
	last_error_repeats_= 0u;

	result.imports= ParseImports();
	while( NotEndOfFile() )
	{
		if( !( it_->type == Lexem::Type::MacroIdentifier && it_->text == "?macro" ) )
			break;

		ParseMacro();
	}

	result.program_elements= ParseNamespaceBody( Lexem::Type::EndOfFile );

	result.error_messages.swap( error_messages_ );
	result.macros= macros_;

	return result;
}

std::vector<Import> SyntaxAnalyzer::ParseImportsOnly( const Lexems& lexems )
{
	it_= lexems.begin();
	it_end_= lexems.end();
	last_error_it_= lexems.end();
	last_error_repeats_= 0u;

	return ParseImports();
}

std::vector<Import> SyntaxAnalyzer::ParseImports()
{
	std::vector<Import> imports;

	while( NotEndOfFile() )
	{
		if( !( it_->type == Lexem::Type::Identifier && it_->text == Keywords::import_ ) )
			break;
		NextLexem();

		if( it_->type == Lexem::Type::String )
		{
			imports.emplace_back( it_->file_pos );
			imports.back().import_name= it_->text;
			NextLexem();
		}
		else
		{
			PushErrorMessage();
			TryRecoverAfterError( { ExpectedLexem(Keywords::import_), ExpectedLexem(Lexem::Type::String) }, g_namespace_body_elements_start_lexems );
		}
	}

	return imports;
}

void SyntaxAnalyzer::ParseMacro()
{
	Macro macro;
	Macro::Context macro_context= Macro::Context::Expression;

	U_ASSERT( it_->type == Lexem::Type::MacroIdentifier && it_->text == "?macro" );
	NextLexem();

	// Match body
	if( it_->type != Lexem::Type::MacroBracketLeft )
	{
		PushErrorMessage();
		return;
	}
	NextLexem();

	// MacroName::context
	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return;
	}
	macro.name= it_->text;
	macro.file_pos= it_->file_pos;
	NextLexem();

	if( IsKeyword( macro.name ) )
	{
		SyntaxErrorMessage msg;
		msg.file_pos= macro.file_pos;
		msg.text= "Using keyword as macro name";
		error_messages_.push_back(std::move(msg));
	}

	if( it_->type != Lexem::Type::Colon )
	{
		PushErrorMessage();
		return;
	}
	NextLexem();

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return;
	}
	const ProgramString& context_str= it_->text;
	NextLexem();

	if( context_str == "expr" )
		macro_context= Macro::Context::Expression;
	else if( context_str == "block" )
		macro_context= Macro::Context::Block;
	else if( context_str == "class" )
		macro_context= Macro::Context::Class;
	else if( context_str == "namespace" )
		macro_context= Macro::Context::Namespace;
	else
	{
		SyntaxErrorMessage msg;
		msg.file_pos= macro.file_pos;
		msg.text= "\"" + context_str + "\" unknown macro context";
		error_messages_.push_back(std::move(msg));
	}

	macro.match_template_elements= ParseMacroMatchBlock();
	if( it_->type != Lexem::Type::MacroBracketRight )
	{
		PushErrorMessage();
		return;
	}
	NextLexem();

	if( it_->type != Lexem::Type::RightArrow )
	{
		PushErrorMessage();
		return;
	}
	NextLexem();

	// Result body.
	if( it_->type != Lexem::Type::MacroBracketLeft )
	{
		PushErrorMessage();
		return;
	}
	NextLexem();

	macro.result_template_elements= ParseMacroResultBlock();
	if( it_->type != Lexem::Type::MacroBracketRight )
	{
		PushErrorMessage();
		return;
	}
	NextLexem();

	MacroMap& macro_map= (*macros_)[macro_context];
	if( macro_map.find( macro.name ) != macro_map.end() )
	{
		SyntaxErrorMessage msg;
		msg.file_pos= macro.file_pos;
		msg.text= "\"" + macro.name + "\" macro redefinition.";
		error_messages_.push_back(std::move(msg));
	}
	else
		macro_map[macro.name]= std::move(macro);
}

std::vector<Macro::MatchElement> SyntaxAnalyzer::ParseMacroMatchBlock()
{
	std::vector<Macro::MatchElement> result;
	std::set<ProgramString> elements_set;

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::MacroBracketRight )
			break;
		else if( it_->type == Lexem::Type::MacroBracketLeft )
		{
			PushErrorMessage();
			return result;
		}
		else if( it_->type == Lexem::Type::MacroIdentifier )
		{
			Macro::MatchElement element;
			element.name= it_->text;

			if( elements_set.find( element.name ) != elements_set.end() )
			{
				SyntaxErrorMessage msg;
				msg.file_pos= it_->file_pos;
				msg.text= "\"" + element.name + "\" macro parameter redefinition.";
				error_messages_.push_back(std::move(msg));
			}
			if( IsKeyword( element.name ) )
			{
				SyntaxErrorMessage msg;
				msg.file_pos= it_->file_pos;
				msg.text= "Using keyword as macro element name";
				error_messages_.push_back(std::move(msg));
			}
			NextLexem();

			if( it_->type != Lexem::Type::Colon )
			{
				PushErrorMessage();
				return result;
			}
			NextLexem();

			if( it_->type != Lexem::Type::Identifier )
			{
				PushErrorMessage();
				return result;
			}
			const ProgramString& element_type_str= it_->text;
			NextLexem();

			if( element_type_str == "ident" )
				element.kind= Macro::MatchElementKind::Identifier;
			else if( element_type_str == "ty" )
				element.kind= Macro::MatchElementKind::Typename;
			else if( element_type_str == "expr" )
				element.kind= Macro::MatchElementKind::Expression;
			else if( element_type_str == "block" )
				element.kind= Macro::MatchElementKind::Block;
			else if( element_type_str == "opt" )
				element.kind= Macro::MatchElementKind::Optional;
			else if( element_type_str == "rep" )
				element.kind= Macro::MatchElementKind::Repeated;
			else
			{
				SyntaxErrorMessage msg;
				msg.file_pos= it_->file_pos;
				msg.text= "\"" + element_type_str + "\" unknown macro variable type";
				error_messages_.push_back(std::move(msg));
			}

			if( element.kind == Macro::MatchElementKind::Optional || element.kind == Macro::MatchElementKind::Repeated )
			{
				if( it_->type != Lexem::Type::MacroBracketLeft )
				{
					PushErrorMessage();
					return result;
				}
				NextLexem();

				element.sub_elements= ParseMacroMatchBlock();

				if( it_->type != Lexem::Type::MacroBracketRight )
				{
					PushErrorMessage();
					return result;
				}
				NextLexem();
			}
			// Loop separator.
			if( element.kind == Macro::MatchElementKind::Repeated && it_->type == Lexem::Type::MacroBracketLeft )
			{
				NextLexem();
				if( it_->type == Lexem::Type::MacroIdentifier || it_->type == Lexem::Type::MacroBracketLeft || it_->type == Lexem::Type::MacroBracketRight )
				{
					PushErrorMessage();
					return result;
				}

				element.lexem= *it_;
				NextLexem();

				if( it_->type != Lexem::Type::MacroBracketRight )
				{
					PushErrorMessage();
					return result;
				}
				NextLexem();
			}
			else
				element.lexem.type= Lexem::Type::EndOfFile;

			elements_set.emplace( element.name );
			result.push_back( element );
		}
		else
		{
			Macro::MatchElement element;
			element.kind= Macro::MatchElementKind::Lexem;
			element.lexem= *it_;

			result.push_back(std::move(element));

			NextLexem();
		}
	}

	// Detect kind of end lexems for optionals/loops.
	for( size_t i= 0u; i < result.size(); ++i )
	{
		if( result[i].kind == Macro::MatchElementKind::Optional || result[i].kind == Macro::MatchElementKind::Repeated )
		{
			if( !result[i].sub_elements.empty() && result[i].sub_elements.front().kind == Macro::MatchElementKind::Lexem )
				result[i].block_check_lexem_kind= Macro::BlockCheckLexemKind::LexemAtBlockStart;
			else if( i + 1u < result.size() && result[i+1u].kind == Macro::MatchElementKind::Lexem )
				result[i].block_check_lexem_kind= Macro::BlockCheckLexemKind::LexemAfterBlockEnd;
			else
			{
				SyntaxErrorMessage msg;
				msg.file_pos= it_->file_pos;
				msg.text= "Expected lexem at start or after \"" + result[i].name + "\" element.";
				error_messages_.push_back(std::move(msg));
			}

			if( i + 1u < result.size() && result[i+1u].kind == Macro::MatchElementKind::Lexem )
			{
				if( result[i].kind == Macro::MatchElementKind::Optional &&
					!result[i].sub_elements.empty() && result[i].sub_elements.front().kind == Macro::MatchElementKind::Lexem &&
					result[i].sub_elements.front().lexem.type == result[i+1u].lexem.type && result[i].sub_elements.front().lexem.text == result[i+1u].lexem.text )
				{
					SyntaxErrorMessage msg;
					msg.file_pos= it_->file_pos;
					msg.text= "Start lexem of optional macro block must be different from first lexem after optional block.";
					error_messages_.push_back(std::move(msg));
				}

				if( result[i].kind == Macro::MatchElementKind::Repeated &&
					result[i].lexem.type != Lexem::Type::EndOfFile &&
					result[i].lexem.type == result[i+1].lexem.type && result[i].lexem.text == result[i+1].lexem.text )
				{
					SyntaxErrorMessage msg;
					msg.file_pos= it_->file_pos;
					msg.text= "Separator lexem of repeated macro block must be different from first lexem after repeated block.";
					error_messages_.push_back(std::move(msg));
				}

				if( result[i].kind == Macro::MatchElementKind::Repeated &&
					result[i].lexem.type == Lexem::Type::EndOfFile &&
					!result[i].sub_elements.empty() && result[i].sub_elements.front().kind == Macro::MatchElementKind::Lexem &&
					result[i].sub_elements.front().lexem.type == result[i+1u].lexem.type && result[i].sub_elements.front().lexem.text == result[i+1u].lexem.text )
				{
					SyntaxErrorMessage msg;
					msg.file_pos= it_->file_pos;
					msg.text= "Start lexem of repeated macro block without separator must be different from first lexem after repeated block.";
					error_messages_.push_back(std::move(msg));
				}
			}
		}
	}

	return result;
}

std::vector<Macro::ResultElement> SyntaxAnalyzer::ParseMacroResultBlock()
{
	std::vector<Macro::ResultElement> result;

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::MacroBracketRight )
			break;
		else if( it_->type == Lexem::Type::MacroBracketLeft )
		{
			PushErrorMessage();
			return result;
		}
		else if( it_->type == Lexem::Type::MacroIdentifier )
		{
			Macro::ResultElement element;
			element.kind= Macro::ResultElementKind::VariableElement;
			element.name= it_->text;
			NextLexem();

			// Parse block for optionals/loops
			//  TODO - check if element actually is not optional or loop
			if( it_->type == Lexem::Type::MacroBracketLeft )
			{
				NextLexem();

				element.kind= Macro::ResultElementKind::VariableElementWithMacroBlock;
				element.sub_elements= ParseMacroResultBlock();

				if( it_->type != Lexem::Type::MacroBracketRight )
				{
					PushErrorMessage();
					return result;
				}
				NextLexem();

				if( it_->type == Lexem::Type::MacroBracketLeft )
				{
					NextLexem();
					if( it_->type == Lexem::Type::MacroIdentifier || it_->type == Lexem::Type::MacroBracketLeft || it_->type == Lexem::Type::MacroBracketRight )
					{
						PushErrorMessage();
						return result;
					}

					element.lexem= *it_;
					NextLexem();

					if( it_->type != Lexem::Type::MacroBracketRight )
					{
						PushErrorMessage();
						return result;
					}
					NextLexem();
				}
				else
					element.lexem.type= Lexem::Type::EndOfFile;
			}

			result.push_back(std::move(element));
		}
		else
		{
			Macro::ResultElement element;
			element.kind= Macro::ResultElementKind::Lexem;
			element.lexem= *it_;
			result.push_back(std::move(element));

			NextLexem();
		}
	}

	return result;
}

ProgramElements SyntaxAnalyzer::ParseNamespaceBody( const Lexem::Type end_lexem )
{
	ProgramElements program_elements;

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::fn_ || it_->text == Keywords::op_ ) )
		{
			if( auto function= ParseFunction() )
				program_elements.emplace_back( std::move( function ) );
		}
		else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::struct_ || it_->text == Keywords::class_ ) )
		{
			if( auto class_= ParseClass() )
				program_elements.emplace_back( std::move( class_ ) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ )
		{
			TemplateVar template_= ParseTemplate();
			if( auto* const class_template= std::get_if<ClassTemplate>(&template_) )
				program_elements.emplace_back( std::move( *class_template ) );
			else if( auto* const typedef_template= std::get_if<TypedefTemplate>(&template_) )
				program_elements.emplace_back( std::move( *typedef_template ) );
			else if( auto* const function_template= std::get_if<FunctionTemplate>(&template_) )
				program_elements.emplace_back( std::move( *function_template ) );
			else if( std::get_if<EmptyVariant>(&template_) )
			{}
			else
			{
				// TODO - push more relevant message.
				PushErrorMessage();
			}
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ )
		{
			program_elements.emplace_back( ParseVariablesDeclaration() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ )
		{
			program_elements.emplace_back( ParseAutoVariableDeclaration() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ )
		{
			program_elements.emplace_back( ParseStaticAssert() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::enum_ )
		{
			program_elements.emplace_back( ParseEnum() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::type_ )
		{
			program_elements.emplace_back(ParseTypedef() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::namespace_ )
		{
			auto namespace_= std::make_unique<Namespace>( it_->file_pos );
			NextLexem();

			ProgramString name;
			if( it_->type == Lexem::Type::Identifier )
			{
				name= it_->text;
				NextLexem();
			}
			else
				PushErrorMessage();

			if( it_->type == Lexem::Type::BraceLeft )
				NextLexem();
			else
			{
				PushErrorMessage();
				TryRecoverAfterError( g_namespace_body_elements_start_lexems );
			}

			namespace_->name_= std::move(name);
			namespace_->elements_= ParseNamespaceBody( Lexem::Type::BraceRight );

			if( it_->type != Lexem::Type::BraceRight )
				PushErrorMessage();
			NextLexem();

			program_elements.push_back( std::move( namespace_ ) );
		}
		else if( it_->type == end_lexem )
		{
			// End of namespace
			return program_elements;
		}
		else
		{
			if( it_->type == Lexem::Type::Identifier )
			{
				if( const Macro* const macro= FetchMacro( it_->text, Macro::Context::Namespace ) )
				{
					for( auto& element : ExpandMacro( *macro, &SyntaxAnalyzer::ParseNamespaceBody ) )
						program_elements.push_back( std::move(element) );
					continue;
				}
			}

			PushErrorMessage();
			TryRecoverAfterError( g_namespace_body_elements_start_lexems );
		}
	} // while not end of file

	return program_elements;
}

NumericConstant SyntaxAnalyzer::ParseNumericConstant()
{
	const ProgramString& text= it_->text;

	uint64_t base= 10;
	uint64_t (*number_func)( sprache_char) =
		[]( sprache_char c ) -> uint64_t
		{
			U_ASSERT( c >= '0' && c <= '9' );
			return c - '0';
		};

	bool (*is_number_func)( sprache_char) =
		[]( sprache_char c ) -> bool { return std::isdigit(c); };

	ProgramString::const_iterator it= text.begin();
	const ProgramString::const_iterator it_end= text.end();

	if( text.size() >= 2 && text[0] == '0' )
	{
		sprache_char d= text[1];
		switch(d)
		{
		case 'b':
			it+= 2;
			base= 2;
			number_func=
				[]( sprache_char c ) -> uint64_t
				{
					U_ASSERT( c >= '0' && c <= '1' );
					return c - '0';
				};
			is_number_func=
				[]( sprache_char c ) -> bool
				{
					return c == '0' || c == '1';
				};
			break;

		case 'o':
			it+= 2;
			base= 8;
			number_func=
				[]( sprache_char c ) -> uint64_t
				{
					U_ASSERT( c >= '0' && c <= '7' );
					return c - '0';
				};
			is_number_func=
				[]( sprache_char c ) -> bool
				{
					return c >= '0' && c <= '7';
				};
			break;

		case 'x':
			it+= 2;
			base= 16;
			number_func=
				[]( sprache_char c ) -> uint64_t
				{
					if( c >= '0' && c <= '9' )
						return c - '0';
					else if( c >= 'a' && c <= 'f' )
						return c - 'a' + 10;
					else
					{
						U_ASSERT( c >= 'A' && c <= 'F' );
						return c - 'A' + 10;
					}
				};
			is_number_func= []( sprache_char c ) -> bool { return std::isxdigit(c); };
			break;

		default: break;
		};
	}

	uint64_t integer_part= 0, fractional_part= 0;
	int fractional_part_digits= 0, exponent= 0;
	bool has_fraction_point= false;

	while( it < it_end && is_number_func( *it ) )
	{
		const uint64_t integer_part_before= integer_part;
		integer_part= integer_part * base + number_func( *it );
		++it;

		if( integer_part < integer_part_before ) // Check overflow
		{
			SyntaxErrorMessage msg;
			msg.file_pos= it_->file_pos;
			msg.text= "Integer part of numeric literal is too long";
			error_messages_.push_back( msg );
			break;
		}
	}

	if( it < it_end && *it == '.' )
	{
		++it;
		has_fraction_point= true;

		while( it < it_end && is_number_func(*it) )
		{
			const uint64_t fractional_part_before= fractional_part;
			fractional_part= fractional_part * base + number_func( *it );
			++fractional_part_digits;
			++it;

			if( fractional_part < fractional_part_before ) // Check overflow
			{
				SyntaxErrorMessage msg;
				msg.file_pos= it_->file_pos;
				msg.text= "Fractional part of numeric literal is too long";
				error_messages_.push_back( msg );
				break;
			}
		}
	}

	// Exponent
	if( it < it_end && *it == 'e' )
	{
		++it;

		U_ASSERT( base == 10 );
		bool is_negative= false;

		if( it < it_end && *it == '-' )
		{
			is_negative= true;
			++it;
		}
		else if( it < it_end && *it == '+' )
			++it;

		while( it < it_end && is_number_func(*it) )
		{
			exponent= exponent * int(base) + int(number_func(*it));
			++it;
		}
		if( is_negative )
			exponent= -exponent;
	}

	NumericConstant result( it_->file_pos );

	// For double calculate only powers > 0, because pow( base, positive ) is always integer and have exact double representation.
	// pow( base, negative ) may have not exact double representation (1/10 for example).
	// Example:
	// 3 / 10 - right
	// 3 * (1/10) - wrong
	if( exponent >= 0 )
		result.value_double_= double(integer_part) * PowI( base, exponent );
	else
		result.value_double_= double(integer_part) / PowI( base, -exponent );
	if( exponent >= fractional_part_digits )
		result.value_double_+= double(fractional_part) * PowI( base, exponent - fractional_part_digits );
	else
		result.value_double_+= double(fractional_part) / PowI( base, fractional_part_digits - exponent );

	result.value_int_= integer_part;
	for( int i= 0; i < exponent; ++i )
		result.value_int_*= base;
	for( int i= 0; i < -exponent; ++i )
		result.value_int_/= base;

	uint64_t fractional_part_corrected= fractional_part;
	for( int i= 0; i < exponent - fractional_part_digits; ++i )
		fractional_part_corrected*= base;
	for( int i= 0; i < fractional_part_digits - exponent; ++i )
		fractional_part_corrected/= base;
	result.value_int_+= fractional_part_corrected;

	result.has_fractional_point_= has_fraction_point;

	if( size_t(it_end - it) > sizeof(TypeSuffix) / sizeof(TypeSuffix::value_type) - 1 )
	{
		SyntaxErrorMessage msg;
		msg.file_pos= it_->file_pos;
		msg.text= "Type suffix of numeric literal is too long";
		error_messages_.push_back( msg );
		return result;
	}
	std::copy( it, it_end, result.type_suffix_.begin() );

	return result;
}

Expression SyntaxAnalyzer::ParseExpression()
{
	Expression root;

	while( NotEndOfFile() )
	{
		std::vector<UnaryPrefixOperator> prefix_operators;

		// Prefix operators.
		while( NotEndOfFile() )
		{
			switch( it_->type )
			{
			case Lexem::Type::Identifier:
			case Lexem::Type::Scope:
			case Lexem::Type::Number:
			case Lexem::Type::String:
			case Lexem::Type::BracketLeft:
			case Lexem::Type::SquareBracketLeft:
				goto parse_operand;

			case Lexem::Type::Plus:
				 prefix_operators.emplace_back( UnaryPlus( it_->file_pos ) );
				NextLexem();
				break;

			case Lexem::Type::Minus:
				prefix_operators.emplace_back( UnaryMinus( it_->file_pos ) );
				NextLexem();
				break;

			case Lexem::Type::Not:
				prefix_operators.emplace_back( LogicalNot( it_->file_pos ) );
				NextLexem();
				break;

			case Lexem::Type::Tilda:
				prefix_operators.emplace_back( BitwiseNot( it_->file_pos ) );
				NextLexem();
				break;

			default:
				if( prefix_operators.empty() )
				{
					if( std::get_if<EmptyVariant>( &root ) != nullptr )
						PushErrorMessage();
					return root;
				}
				else
				{
					PushErrorMessage();
					return EmptyVariant();
				}
			};
		}

	parse_operand:
		Expression current_node;
		ExpressionComponentWithUnaryOperators* current_node_ptr= nullptr;

		// Operand.
		if( it_->type == Lexem::Type::Identifier )
		{
			if( it_->text == Keywords::true_ )
			{
				current_node= BooleanConstant( it_->file_pos, true );
				current_node_ptr= std::get_if<BooleanConstant>( &current_node );
				NextLexem();
			}
			else if( it_->text == Keywords::false_ )
			{
				current_node= BooleanConstant( it_->file_pos, false );
				current_node_ptr= std::get_if<BooleanConstant>( &current_node );
				NextLexem();
			}
			else if( it_->text == Keywords::move_ )
			{
				MoveOperator move_operator( it_->file_pos );

				NextLexem();
				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				if( it_->type != Lexem::Type::Identifier )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				move_operator.var_name_= it_->text;
				NextLexem();

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				current_node= std::move(move_operator);
				current_node_ptr= std::get_if<MoveOperator>( &current_node );
			}
			else if( it_->text == Keywords::select_ )
			{
				TernaryOperator ternary_operator( it_->file_pos );

				NextLexem();
				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();
				ternary_operator.condition= std::make_unique<Expression>( ParseExpression() );

				if( it_->type != Lexem::Type::Question )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();
				ternary_operator.true_branch= std::make_unique<Expression>( ParseExpression() );

				if( it_->type != Lexem::Type::Colon )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();
				ternary_operator.false_branch= std::make_unique<Expression>( ParseExpression() );

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				current_node= std::move(ternary_operator);
				current_node_ptr= std::get_if<TernaryOperator>( &current_node );
			}
			else if( it_->text == Keywords::cast_ref_ )
			{
				CastRef cast( it_->file_pos );

				NextLexem();
				if( it_->type != Lexem::Type::TemplateBracketLeft )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				cast.type_= std::make_unique<TypeName>( ParseTypeName() );
				if( it_->type != Lexem::Type::TemplateBracketRight )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				cast.expression_= std::make_unique<Expression>( ParseExpression() );

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				current_node= std::move(cast);
				current_node_ptr= std::get_if<CastRef>( &current_node );
			}
			else if( it_->text == Keywords::cast_ref_unsafe_ )
			{
				CastRefUnsafe cast( it_->file_pos );

				NextLexem();
				if( it_->type != Lexem::Type::TemplateBracketLeft )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				cast.type_= std::make_unique<TypeName>( ParseTypeName() );
				if( it_->type != Lexem::Type::TemplateBracketRight )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				cast.expression_= std::make_unique<Expression>( ParseExpression() );

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				current_node= std::move(cast);
				current_node_ptr= std::get_if<CastRefUnsafe>( &current_node );
			}
			else if( it_->text == Keywords::cast_imut_ )
			{
				CastImut cast( it_->file_pos );

				NextLexem();

				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				cast.expression_= std::make_unique<Expression>( ParseExpression() );

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				current_node= std::move(cast);
				current_node_ptr= std::get_if<CastImut>( &current_node );
			}
			else if( it_->text == Keywords::cast_mut_ )
			{
				CastMut cast( it_->file_pos );

				NextLexem();

				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				cast.expression_= std::make_unique<Expression>( ParseExpression() );

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				current_node= std::move(cast);
				current_node_ptr= std::get_if<CastMut>( &current_node );
			}
			else if( it_->text == Keywords::typeinfo_ )
			{
				TypeInfo typeinfo_(it_->file_pos );
				NextLexem();

				if( it_->type != Lexem::Type::TemplateBracketLeft )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				typeinfo_.type_= std::make_unique<TypeName>( ParseTypeName() );
				if( it_->type != Lexem::Type::TemplateBracketRight )
				{
					PushErrorMessage();
					return EmptyVariant();
				}
				NextLexem();

				current_node= std::move(typeinfo_);
				current_node_ptr= std::get_if<TypeInfo>( &current_node );
			}
			else if( it_->text == Keywords::fn_ || it_->text == Keywords::typeof_ || it_->text == Keywords::tup_ )
			{
				// Parse function type name: fn( i32 x )
				TypeNameInExpression type_name_in_expression( it_->file_pos );
				type_name_in_expression.type_name= ParseTypeName();
				current_node= std::move(type_name_in_expression);
				current_node_ptr= std::get_if<TypeNameInExpression>( &current_node );
			}
			else
			{
				if( auto macro= FetchMacro( it_->text, Macro::Context::Expression ) )
				{
					const FilePos& macro_file_pos= it_->file_pos;
					Expression macro_expression= ExpandMacro( *macro, &SyntaxAnalyzer::ParseExpression );
					if( std::get_if<EmptyVariant>( &macro_expression ) != nullptr )
						return EmptyVariant();

					BracketExpression bracket_expression( macro_file_pos );
					bracket_expression.expression_= std::make_unique<Expression>( std::move(macro_expression) );

					current_node= std::move(bracket_expression);
					current_node_ptr= std::get_if<BracketExpression>( &current_node );
				}
				else
				{
					current_node= NamedOperand( it_->file_pos, ParseComplexName() );
					current_node_ptr= std::get_if<NamedOperand>( &current_node );
				}
			}
		}
		else if( it_->type == Lexem::Type::Scope )
		{
			current_node= NamedOperand( it_->file_pos, ParseComplexName() );
			current_node_ptr= std::get_if<NamedOperand>( &current_node );
		}
		else if( it_->type == Lexem::Type::Number )
		{
			current_node= ParseNumericConstant();
			current_node_ptr= std::get_if<NumericConstant>( &current_node );
			NextLexem();
		}
		else if( it_->type == Lexem::Type::String )
		{
			StringLiteral string_literal( it_->file_pos );
			string_literal.value_= it_->text;
			NextLexem();

			if( it_->type == Lexem::Type::LiteralSuffix )
			{
				if( it_->text.size() > sizeof(TypeSuffix) / sizeof(TypeSuffix::value_type) - 1 )
				{
					SyntaxErrorMessage msg;
					msg.file_pos= it_->file_pos;
					msg.text= "String literal is too long";
					error_messages_.push_back( msg );
					return EmptyVariant();
				}
				std::copy( it_->text.begin(), it_->text.end(), string_literal.type_suffix_.begin() );
				NextLexem();
			}

			current_node= std::move(string_literal);
			current_node_ptr= std::get_if<StringLiteral>( &current_node );
		}
		else if( it_->type == Lexem::Type::BracketLeft )
		{
			NextLexem();

			BracketExpression bracket_expression( (it_-1)->file_pos );
			bracket_expression.expression_= std::make_unique<Expression>( ParseExpression() );

			current_node= std::move(bracket_expression);
			current_node_ptr= std::get_if<BracketExpression>( &current_node );

			if( it_->type != Lexem::Type::BracketRight )
			{
				PushErrorMessage();
				return EmptyVariant();
			}
			NextLexem();
		}
		else if( it_->type == Lexem::Type::SquareBracketLeft )
		{
			// Parse array type name: [ ElementType, 42 ]
			TypeNameInExpression type_name_in_expression(it_->file_pos );
			type_name_in_expression.type_name= ParseTypeName();
			current_node= std::move(type_name_in_expression);
			current_node_ptr= std::get_if<TypeNameInExpression>( &current_node );
		}
		else U_ASSERT(false);

		U_ASSERT( current_node_ptr != nullptr );
		current_node_ptr->prefix_operators_= std::move( prefix_operators );

		bool is_binary_operator= false;
		// Postfix operators.
		bool end_postfix_operators= false;
		while( !end_postfix_operators )
		{
			switch( it_->type )
			{
			case Lexem::Type::SquareBracketLeft:
				{
					IndexationOperator indexation_opearator( it_->file_pos );
					NextLexem();

					indexation_opearator.index_= ParseExpression();
					current_node_ptr->postfix_operators_.emplace_back( std::move(indexation_opearator) );

					if( it_->type != Lexem::Type::SquareBracketRight )
					{
						PushErrorMessage();
						return EmptyVariant();
					}
					NextLexem();
				}
				break;

			case Lexem::Type::BracketLeft:
				{
					CallOperator call_operator( it_->file_pos );
					NextLexem();

					while( NotEndOfFile() )
					{
						if( it_->type == Lexem::Type::BracketRight )
						{
							NextLexem();
							break;
						}

						call_operator.arguments_.emplace_back( ParseExpression() );

						if( it_->type == Lexem::Type::Comma )
						{
							NextLexem();
							if( it_->type == Lexem::Type::BracketRight )
								PushErrorMessage();
						}
						else if( it_->type == Lexem::Type::BracketRight )
						{
							NextLexem();
							break;
						}
					}

					current_node_ptr->postfix_operators_.emplace_back( std::move(call_operator) );

				} break;

			case Lexem::Type::Dot:
				{
					MemberAccessOperator member_access_operator( it_->file_pos );
					NextLexem();

					if( it_->type != Lexem::Type::Identifier )
					{
						PushErrorMessage();
						return EmptyVariant();
					}

					member_access_operator.member_name_= it_->text;
					NextLexem();

					if( it_->type == Lexem::Type::TemplateBracketLeft )
					{
						member_access_operator.have_template_parameters= true;
						member_access_operator.template_parameters= ParseTemplateParameters();
					}

					current_node_ptr->postfix_operators_.emplace_back( std::move( member_access_operator ) );
				} break;

			default:
				is_binary_operator= IsBinaryOperator( *it_ );
				end_postfix_operators= true;
				break;
			};

			if( end_postfix_operators )
				break;
		}

		if( std::get_if< EmptyVariant >( &root ) != nullptr )
			root= std::move( current_node );
		else
		{
			BinaryOperator* const root_as_binary_operator= std::get_if<BinaryOperator>( &root );
			U_ASSERT( root_as_binary_operator != nullptr );

			// Place to existent tree last component.
			BinaryOperator* most_right_with_null= root_as_binary_operator;
			while( most_right_with_null->right_ != nullptr )
			{
				BinaryOperator* const right_as_binary_operator= std::get_if<BinaryOperator>( most_right_with_null->right_.get() );
				U_ASSERT( right_as_binary_operator != nullptr );
				most_right_with_null= right_as_binary_operator;
			}
			most_right_with_null->right_= std::make_unique<Expression>( std::move( current_node ) );
		}

		if( is_binary_operator )
		{
			const BinaryOperatorType binary_operator_type= LexemToBinaryOperator( *it_ );
			auto binary_operator= std::make_unique<BinaryOperator>( it_->file_pos );
			binary_operator->operator_type_= binary_operator_type;
			NextLexem();

			if( BinaryOperator* const root_as_binary_operator= std::get_if<BinaryOperator>( &root ) )
			{
				BinaryOperator* node_to_replace_parent= nullptr;
				BinaryOperator* node_to_replace= root_as_binary_operator;
				while( GetBinaryOperatorPriority( binary_operator->operator_type_ ) > GetBinaryOperatorPriority( node_to_replace->operator_type_ ) )
				{
					node_to_replace_parent= node_to_replace;
					BinaryOperator* const right_as_binary_operator= std::get_if<BinaryOperator>( node_to_replace->right_.get() );
					if( right_as_binary_operator == nullptr )
						break;
					node_to_replace= right_as_binary_operator;
				}

				if( node_to_replace_parent != nullptr )
				{
					binary_operator->left_= std::move( node_to_replace_parent->right_ );
					node_to_replace_parent->right_= std::make_unique<Expression>( std::move( *binary_operator ) );
				}
				else
				{
					binary_operator->left_= std::make_unique<Expression>( std::move( root ) );
					root= std::move( *binary_operator );
				}
			}
			else
			{
				binary_operator->left_= std::make_unique<Expression>( std::move( root ) );
				root= std::move( *binary_operator );
			}
		}
		else
			break;
	}

	return root;
}

FunctionArgument SyntaxAnalyzer::ParseFunctionArgument()
{
	FunctionArgument result( it_->file_pos );
	result.type_= ParseTypeName();

	result.reference_modifier_= ReferenceModifier::None;
	result.mutability_modifier_= MutabilityModifier::None;

	if( it_->type == Lexem::Type::And )
	{
		result.reference_modifier_= ReferenceModifier::Reference;
		NextLexem();

		if( it_->type == Lexem::Type::Apostrophe )
		{
			NextLexem();

			if( it_->type == Lexem::Type::Identifier )
			{
				result.reference_tag_ = it_->text;
				NextLexem();
			}
			else
			{
				PushErrorMessage();
				TryRecoverAfterError( g_function_arguments_list_control_lexems );
				return result;
			}
		}
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		TryRecoverAfterError( g_function_arguments_list_control_lexems );
		return result;
	}

	if( it_->text == Keywords::mut_ )
	{
		result.mutability_modifier_= MutabilityModifier::Mutable;
		NextLexem();
	}
	else if( it_->text == Keywords::imut_ )
	{
		result.mutability_modifier_= MutabilityModifier::Immutable;
		NextLexem();
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		TryRecoverAfterError( g_function_arguments_list_control_lexems );
		return result;
	}

	result.name_= it_->text;
	NextLexem();

	if( it_->type == Lexem::Type::Apostrophe )
		result.inner_arg_reference_tags_= ParseReferencesTagsList();

	return result;
}

void SyntaxAnalyzer::ParseFunctionTypeEnding( FunctionType& result )
{
	if( it_->type == Lexem::Type::Apostrophe )
		result.referecnces_pollution_list_= ParseFunctionReferencesPollutionList();

	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::unsafe_ )
	{
		result.unsafe_= true;
		NextLexem();
	}

	if( it_->type == Lexem::Type::Colon )
	{
		NextLexem();

		result.return_type_= std::make_unique<TypeName>( ParseTypeName() );

		if( it_->type == Lexem::Type::And )
		{
			result.return_value_reference_modifier_= ReferenceModifier::Reference;
			NextLexem();

			if( it_->type == Lexem::Type::Apostrophe )
			{
				NextLexem();

				if( it_->type == Lexem::Type::Identifier )
				{
					result.return_value_reference_tag_ = it_->text;
					NextLexem();
				}
				else
				{
					PushErrorMessage();
					TryRecoverAfterError( { ExpectedLexem(Lexem::Type::BraceLeft), ExpectedLexem(Lexem::Type::BraceLeft), ExpectedLexem(Lexem::Type::Semicolon) } );
					return;
				}
			}

			if( it_->type == Lexem::Type::Identifier )
			{
				if( it_->text == Keywords::mut_ )
					result.return_value_mutability_modifier_= MutabilityModifier::Mutable;
				else if( it_->text == Keywords::imut_ )
					result.return_value_mutability_modifier_= MutabilityModifier::Immutable;
				else
					PushErrorMessage();
				NextLexem();
			}
		}
		else if( it_->type == Lexem::Type::Apostrophe )
			result.return_value_inner_reference_tags_= ParseReferencesTagsList();
	}
}

FunctionTypePtr SyntaxAnalyzer::ParseFunctionType()
{
	auto result= std::make_unique<FunctionType>( it_->file_pos );

	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::fn_ );
	NextLexem();

	if( it_->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage();
		return result;
	}
	NextLexem();

	while( NotEndOfFile() && it_->type != Lexem::Type::EndOfFile )
	{
		if( it_->type == Lexem::Type::BracketRight )
		{
			NextLexem();
			break;
		}

		result->arguments_.push_back( ParseFunctionArgument() );

		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();
			// Disallov constructions, like "fn f( a : int, ){}"
			if( it_->type == Lexem::Type::BracketRight )
				PushErrorMessage();
		}
		else if( it_->type == Lexem::Type::BracketRight )
		{}
		else
		{
			PushErrorMessage();
			TryRecoverAfterError( g_function_arguments_list_control_lexems );
		}
	} // for arguments

	ParseFunctionTypeEnding( *result );

	return result;
}

TypeName SyntaxAnalyzer::ParseTypeName()
{
	if( it_->type == Lexem::Type::SquareBracketLeft )
	{
		NextLexem();

		ArrayTypeName array_type_name(it_->file_pos);
		array_type_name.element_type= std::make_unique<TypeName>( ParseTypeName() );

		if( it_->type != Lexem::Type::Comma )
		{
			PushErrorMessage();
			return std::move(array_type_name);
		}
		NextLexem();
		array_type_name.size= std::make_unique<Expression>( ParseExpression() );

		if( it_->type != Lexem::Type::SquareBracketRight )
		{
			PushErrorMessage();
			return std::move(array_type_name);
		}
		NextLexem();

		return std::move(array_type_name);
	}
	else if( it_->type == Lexem::Type::BracketLeft )
	{
		// Type name inside (). We needs this for better parsing of function pointer types, for example.
		NextLexem();

		TypeName type_name= ParseTypeName();

		if( it_->type != Lexem::Type::BracketRight )
		{
			PushErrorMessage();
			return type_name;
		}
		NextLexem();

		return type_name;
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::typeof_ )
	{
		NextLexem();

		TypeofTypeName typeof_type_name( it_->file_pos );

		if( it_->type != Lexem::Type::BracketLeft )
		{
			PushErrorMessage();
			return std::move(typeof_type_name);
		}
		NextLexem();

		typeof_type_name.expression= std::make_unique<Expression>( ParseExpression() );

		if( it_->type != Lexem::Type::BracketRight )
		{
			PushErrorMessage();
			return std::move(typeof_type_name);
		}
		NextLexem();

		return std::move(typeof_type_name);
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::tup_ )
	{
		NextLexem();

		TupleType tuple_type( it_->file_pos );

		if( it_->type != Lexem::Type::SquareBracketLeft )
		{
			PushErrorMessage();
			return std::move(tuple_type);
		}
		NextLexem();

		while( NotEndOfFile() )
		{
			if( it_->type == Lexem::Type::SquareBracketRight )
			{
				NextLexem();
				break;
			}

			tuple_type.element_types_.push_back( ParseTypeName() );
			if( it_->type == Lexem::Type::SquareBracketRight )
			{
				NextLexem();
				break;
			}
			else
			{
				if( it_->type != Lexem::Type::Comma )
				{
					PushErrorMessage();
					return std::move(tuple_type);
				}
				NextLexem();

				if( it_->type == Lexem::Type::SquareBracketRight )
				{
					// something, like ,)
					PushErrorMessage();
					return std::move(tuple_type);
				}
			}
		}

		return std::move(tuple_type);
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::fn_ )
		return ParseFunctionType();
	else
	{
		NamedTypeName named_type_name( it_->file_pos );
		named_type_name.name= ParseComplexName();
		return std::move(named_type_name);
	}
}

std::vector<Expression> SyntaxAnalyzer::ParseTemplateParameters()
{
	U_ASSERT( it_->type == Lexem::Type::TemplateBracketLeft );
	NextLexem();

	std::vector<Expression> result;

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::TemplateBracketRight )
		{
			NextLexem();
			break;
		}

		result.push_back( ParseExpression() );

		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();
			if( it_->type == Lexem::Type::TemplateBracketRight )
				PushErrorMessage();
		}
		else if( it_->type == Lexem::Type::TemplateBracketRight )
		{}
		else
		{
			PushErrorMessage();
			TryRecoverAfterError( g_template_arguments_list_control_lexems );
		}
	}

	return result;
}

ComplexName SyntaxAnalyzer::ParseComplexName()
{
	ComplexName complex_name;

	if( !( it_->type == Lexem::Type::Identifier || it_->type == Lexem::Type::Scope ) )
	{
		PushErrorMessage();
		return complex_name;
	}

	if( it_->type == Lexem::Type::Scope )
	{
		complex_name.components.emplace_back();
		NextLexem();
	}

	do
	{
		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return complex_name;
		}
		complex_name.components.emplace_back();
		complex_name.components.back().name= it_->text;
		NextLexem();

		if( it_->type == Lexem::Type::TemplateBracketLeft )
		{
			complex_name.components.back().have_template_parameters= true;
			complex_name.components.back().template_parameters= ParseTemplateParameters();
		}

		if( it_->type == Lexem::Type::Scope )
			NextLexem();
		else
			break;

	} while( NotEndOfFile() );

	return complex_name;
}

ReferencesTagsList SyntaxAnalyzer::ParseReferencesTagsList()
{
	U_ASSERT( it_->type == Lexem::Type::Apostrophe );
	NextLexem();

	ReferencesTagsList result;

	if( it_->type == Lexem::Type::Apostrophe )
	{
		// Empty list
		NextLexem();
		return result;
	}

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::Identifier )
		{
			result.push_back( it_->text );
			NextLexem();
		}
		else
		{
			PushErrorMessage();
			return result;
		}

		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();
			if( it_->type == Lexem::Type::Apostrophe ) // Disable things, like 'a, b, c,'
			{
				PushErrorMessage();
				return result;
			}
		}
		else if( it_->type == Lexem::Type::Apostrophe )
		{
			NextLexem();
			break;
		}
		else if( it_->type == Lexem::Type::Ellipsis )
		{
			NextLexem();
			result.emplace_back();

			if( it_->type != Lexem::Type::Apostrophe )
			{
				PushErrorMessage();
				return result;
			}
			NextLexem();
			break;
		}
	}

	return result;
}

FunctionReferencesPollutionList SyntaxAnalyzer::ParseFunctionReferencesPollutionList()
{
	U_ASSERT( it_->type == Lexem::Type::Apostrophe );
	NextLexem();

	FunctionReferencesPollutionList result;

	if( it_->type == Lexem::Type::Apostrophe )
	{
		// Empty list
		NextLexem();
		return result;
	}

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::Identifier )
		{
			result.emplace_back();
			result.back().first = it_->text;
			NextLexem();
		}
		else
		{
			PushErrorMessage();
			return result;
		}

		if( it_->type != Lexem::Type::LeftArrow )
		{
			PushErrorMessage();
			return result;
		}
		NextLexem();

		if( it_->type == Lexem::Type::Identifier )
		{
			if( it_->text == Keywords::mut_ )
			{
				result.back().second.is_mutable= true;
				NextLexem();
			}
			else if( it_->text == Keywords::imut_ )
			{
				result.back().second.is_mutable= false;
				NextLexem();
			}
		}

		if( it_->type == Lexem::Type::Identifier )
		{
			result.back().second.name= it_->text;
			NextLexem();
		}
		else
		{
			PushErrorMessage();
			return result;
		}

		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();
			if( it_->type == Lexem::Type::Apostrophe ) // Disable things, like 'a, b, c,'
			{
				PushErrorMessage();
				return result;
			}
		}
		else if( it_->type == Lexem::Type::Apostrophe )
		{
			NextLexem();
			break;
		}
	}

	return result;
}

Initializer SyntaxAnalyzer::ParseInitializer( const bool parse_expression_initializer )
{
	if( it_->type == Lexem::Type::SquareBracketLeft )
	{
		return ParseArrayInitializer();
	}
	else if( it_->type == Lexem::Type::BracketLeft )
	{
		return ParseConstructorInitializer();
	}
	else if( it_->type == Lexem::Type::BraceLeft )
	{
		return ParseStructNamedInitializer();
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::zero_init_ )
	{
		const auto prev_it= it_;
		NextLexem();
		return ZeroInitializer( prev_it->file_pos );
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::uninitialized_ )
	{
		const auto prev_it= it_;
		NextLexem();
		return UninitializedInitializer( prev_it->file_pos );
	}
	else if( parse_expression_initializer )
	{
		// In some cases usage of expression in initializer is forbidden.
		return ParseExpressionInitializer();
	}
	else
	{
		PushErrorMessage();
		return EmptyVariant{};
	}
}

Initializer SyntaxAnalyzer::ParseVariableInitializer()
{
	Initializer initializer;
	if( it_->type == Lexem::Type::Assignment )
	{
		NextLexem();
		if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::zero_init_ )
		{
			initializer= ZeroInitializer( it_->file_pos );
			NextLexem();
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::uninitialized_ )
		{
			initializer= UninitializedInitializer( it_->file_pos );
			NextLexem();
		}
		else
		{
			ExpressionInitializer expression_initializer( it_->file_pos );
			expression_initializer.expression= ParseExpression();
			initializer= std::move(expression_initializer);
		}
	}
	else if(
		it_->type == Lexem::Type::BracketLeft ||
		it_->type == Lexem::Type::SquareBracketLeft ||
		it_->type == Lexem::Type::BraceLeft )
		return ParseInitializer( false );

	return initializer;
}

Initializer SyntaxAnalyzer::ParseArrayInitializer()
{
	U_ASSERT( it_->type == Lexem::Type::SquareBracketLeft );

	ArrayInitializer result( it_->file_pos );
	NextLexem();

	while( NotEndOfFile() && it_->type != Lexem::Type::SquareBracketRight )
	{
		result.initializers.push_back( ParseInitializer( true ) );
		if( it_->type == Lexem::Type::Comma )
			NextLexem();
		else
			break;
		// TODO - parse continious flag here
	}
	if( it_->type != Lexem::Type::SquareBracketRight )
	{
		PushErrorMessage();
		return std::move(result);
	}

	NextLexem();

	return std::move(result);
}

Initializer SyntaxAnalyzer::ParseStructNamedInitializer()
{
	U_ASSERT( it_->type == Lexem::Type::BraceLeft );

	StructNamedInitializer result( it_->file_pos );
	NextLexem();

	while( NotEndOfFile() && it_->type != Lexem::Type::BraceRight )
	{
		if( it_->type == Lexem::Type::Dot )
			NextLexem();
		else
		{
			PushErrorMessage();
			return std::move(result);
		}

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return std::move(result);
		}
		ProgramString name= it_->text;
		NextLexem();

		Initializer initializer= ParseVariableInitializer();
		if( std::get_if<EmptyVariant>(&initializer) != nullptr )
			PushErrorMessage();

		result.members_initializers.emplace_back();
		result.members_initializers.back().name= std::move(name);
		result.members_initializers.back().initializer= std::move(initializer);

		if( it_->type == Lexem::Type::Comma )
			NextLexem();
	}
	if( it_->type != Lexem::Type::BraceRight )
	{
		PushErrorMessage();
		return std::move(result);
	}

	NextLexem();

	return std::move(result);
}

Initializer SyntaxAnalyzer::ParseConstructorInitializer()
{
	U_ASSERT( it_->type == Lexem::Type::BracketLeft );
	NextLexem();

	std::vector<Expression> args;
	while( NotEndOfFile() && it_->type != Lexem::Type::BracketRight )
	{
		args.push_back( ParseExpression() );
		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();
			// Disallow comma after closing bracket
			if( it_->type == Lexem::Type::BracketRight )
			{
				PushErrorMessage();
				return Initializer();
			}
		}
		else
			break;
	}
	if( it_->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage();
		return Initializer();
	}
	NextLexem();

	ConstructorInitializer result( it_->file_pos );
	result.call_operator.arguments_= std::move(args);
	return std::move(result);
}

Initializer SyntaxAnalyzer::ParseExpressionInitializer()
{
	ExpressionInitializer initializer( it_->file_pos );
	initializer.expression= ParseExpression();
	return std::move( initializer );
}

VariablesDeclaration SyntaxAnalyzer::ParseVariablesDeclaration()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ );
	VariablesDeclaration decl( it_->file_pos );
	NextLexem();

	decl.type= ParseTypeName();

	do
	{
		decl.variables.emplace_back();
		VariablesDeclaration::VariableEntry& variable_entry= decl.variables.back();

		if( it_->type == Lexem::Type::And )
		{
			variable_entry.reference_modifier= ReferenceModifier::Reference;
			NextLexem();
		}

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return decl;
		}

		if( it_->text == Keywords::mut_ )
		{
			variable_entry.mutability_modifier= MutabilityModifier::Mutable;
			NextLexem();
		}
		else if( it_->text == Keywords::imut_ )
		{
			variable_entry.mutability_modifier= MutabilityModifier::Immutable;
			NextLexem();
		}
		else if( it_->text == Keywords::constexpr_ )
		{
			variable_entry.mutability_modifier= MutabilityModifier::Constexpr;
			NextLexem();
		}

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return decl;
		}

		variable_entry.name= it_->text;
		variable_entry.file_pos= it_->file_pos;
		NextLexem();

		Initializer variable_initializer=  ParseVariableInitializer();
		if( std::get_if<EmptyVariant>( &variable_initializer ) == nullptr )
			variable_entry.initializer= std::make_unique<Initializer>( std::move(variable_initializer) );

		if( it_->type == Lexem::Type::Comma )
			NextLexem();
		else if( it_->type == Lexem::Type::Semicolon )
		{
			NextLexem();
			break;
		}
		else
		{
			PushErrorMessage();
			return decl;
		}

	} while( NotEndOfFile() );

	return decl;
}

AutoVariableDeclaration SyntaxAnalyzer::ParseAutoVariableDeclaration()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ );
	AutoVariableDeclaration result( it_->file_pos );
	NextLexem();

	if (it_->type == Lexem::Type::Identifier && it_->text == Keywords::lock_temps_ )
	{
		NextLexem();
		result.lock_temps= true;
	}

	if( it_->type == Lexem::Type::And )
	{
		result.reference_modifier= ReferenceModifier::Reference;
		NextLexem();
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return result;
	}

	if( it_->text == Keywords::mut_ )
	{
		result.mutability_modifier= MutabilityModifier::Mutable;
		NextLexem();
	}
	else if( it_->text == Keywords::imut_ )
	{
		result.mutability_modifier= MutabilityModifier::Immutable;
		NextLexem();
	}
	else if( it_->text == Keywords::constexpr_ )
	{
		result.mutability_modifier= MutabilityModifier::Constexpr;
		NextLexem();
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return result;
	}

	result.name= it_->text;
	NextLexem();

	if( it_->type != Lexem::Type::Assignment )
	{
		PushErrorMessage();
		return result;
	}
	NextLexem();

	result.initializer_expression = ParseExpression();

	if( it_->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage();
		return result;
	}
	NextLexem();

	return result;
}

ReturnOperator SyntaxAnalyzer::ParseReturnOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::return_ );

	ReturnOperator result( it_->file_pos );
	NextLexem();

	if( it_->type == Lexem::Type::Semicolon )
	{
		NextLexem();
		return result;
	}

	result.expression_= ParseExpression();

	if( it_->type != Lexem::Type::Semicolon  )
	{
		PushErrorMessage();
		return result;
	}

	NextLexem();

	return result;
}

WhileOperator SyntaxAnalyzer::ParseWhileOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::while_ );
	WhileOperator result( it_->file_pos );

	NextLexem();
	if( it_->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage();
		return result;
	}

	NextLexem();

	result.condition_= ParseExpression();

	if( it_->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage();
		return result;
	}

	NextLexem();

	if( it_->type != Lexem::Type::BraceLeft )
	{
		PushErrorMessage();
		return result;
	}

	result.block_= ParseBlock();
	return result;
}

ForOperator SyntaxAnalyzer::ParseForOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::for_ );
	ForOperator result( it_->file_pos );

	NextLexem();
	if( it_->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage();
		return result;
	}

	NextLexem();

	if( it_->type == Lexem::Type::And )
	{
		result.reference_modifier_= ReferenceModifier::Reference;
		NextLexem();
	}
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::mut_ )
	{
		result.mutability_modifier_= MutabilityModifier::Mutable;
		NextLexem();
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::imut_ )
	{
		result.mutability_modifier_= MutabilityModifier::Immutable;
		NextLexem();
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return result;
	}
	result.loop_variable_name_= it_->text;
	NextLexem();

	if( it_->type != Lexem::Type::Colon )
	{
		PushErrorMessage();
		return result;
	}
	NextLexem();

	result.sequence_= ParseExpression();

	if( it_->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage();
		return result;
	}

	NextLexem();

	if( it_->type != Lexem::Type::BraceLeft )
	{
		PushErrorMessage();
		return result;
	}

	result.block_= ParseBlock();
	return result;
}

BreakOperator SyntaxAnalyzer::ParseBreakOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::break_ );
	BreakOperator result( it_->file_pos );
	NextLexem();

	if( it_->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage();
		return result;
	}

	NextLexem();

	return result;
}

ContinueOperator SyntaxAnalyzer::ParseContinueOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::continue_ );
	ContinueOperator result( it_->file_pos );
	NextLexem();

	if( it_->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage();
		return result;
	}

	NextLexem();

	return result;
}

IfOperator SyntaxAnalyzer::ParseIfOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::if_  || it_->text == Keywords::static_if_ ) );
	IfOperator result( it_->file_pos );
	NextLexem();

	if( it_->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage();
		return result;
	}

	NextLexem();

	Expression if_expression= ParseExpression();

	if( it_->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage();
		return result;
	}

	NextLexem();

	if( it_->type != Lexem::Type::BraceLeft )
	{
		PushErrorMessage();
		return result;
	}

	auto& branches= result.branches_;
	branches.emplace_back( IfOperator::Branch{ std::move(if_expression), ParseBlock() } );

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::else_ )
		{
			NextLexem();

			// Optional if.
			Expression condition;
			if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ )
			{
				NextLexem();

				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return result;
				}

				NextLexem();

				condition= ParseExpression();

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return result;
				}

				NextLexem();
			}
			// Block - common for "else" and "else if".
			if( it_->type != Lexem::Type::BraceLeft )
			{
				PushErrorMessage();
				return result;
			}

			branches.emplace_back( IfOperator::Branch{ std::move(condition), ParseBlock() } );

			if( std::get_if<EmptyVariant>( &branches.back().condition ) != nullptr )
				break;
		}
		else
			break;
	}

	result.end_file_pos_= std::prev( it_ )->file_pos;
	return result;
}

StaticIfOperator SyntaxAnalyzer::ParseStaticIfOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_if_ );

	StaticIfOperator result( it_->file_pos  );
	result.if_operator_= ParseIfOperator();
	return result;
}

StaticAssert SyntaxAnalyzer::ParseStaticAssert()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ );
	StaticAssert result( it_->file_pos );
	NextLexem();

	if( it_->type == Lexem::Type::BracketLeft )
		NextLexem();
	else
		PushErrorMessage();

	result.expression= ParseExpression();

	if( it_->type == Lexem::Type::BracketRight )
		NextLexem();
	else
		PushErrorMessage();

	if( it_->type == Lexem::Type::Semicolon )
		NextLexem();
	else
	{
		PushErrorMessage();
		return result;
	}

	return result;
}

Enum SyntaxAnalyzer::ParseEnum()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::enum_ );
	Enum result( it_->file_pos );
	NextLexem();

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return result;
	}
	result.name= it_->text;
	NextLexem();

	if( it_->type == Lexem::Type::Colon )
	{
		NextLexem();
		result.underlaying_type_name= ParseComplexName();
	}

	if( it_->type != Lexem::Type::BraceLeft )
	{
		PushErrorMessage();
		return result;
	}
	NextLexem();

	while( NotEndOfFile() )
	{
		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return result;
		}

		result.members.emplace_back();
		result.members.back().file_pos= it_->file_pos;
		result.members.back().name= it_->text;
		NextLexem();

		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();
			if( it_->type == Lexem::Type::BraceRight )
			{
				NextLexem();
				break;
			}
			continue;
		}
		else if( it_->type == Lexem::Type::BraceRight )
		{
			NextLexem();
			break;
		}
		else
		{
			PushErrorMessage();
			return result;
		}
	}

	return result;
}

BlockElement SyntaxAnalyzer::ParseHalt()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::halt_ );
	const FilePos& file_pos= it_->file_pos;
	NextLexem();

	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ )
	{
		NextLexem();
		HaltIf result( file_pos );

		if( it_->type != Lexem::Type::BracketLeft )
		{
			PushErrorMessage();
			return std::move(result);
		}
		NextLexem();

		result.condition= ParseExpression();

		if( it_->type != Lexem::Type::BracketRight )
		{
			PushErrorMessage();
			return std::move(result);
		}
		NextLexem();

		if( it_->type != Lexem::Type::Semicolon )
		{
			PushErrorMessage();
			return std::move(result);
		}
		NextLexem();

		return std::move(result);
	}
	else if( it_->type == Lexem::Type::Semicolon )
	{
		NextLexem();
		return Halt( file_pos );
	}
	else
	{
		PushErrorMessage();
		return Halt( file_pos );
	}
}

std::vector<BlockElement> SyntaxAnalyzer::ParseBlockElements()
{
	std::vector<BlockElement> elements;

	while( NotEndOfFile() && it_->type != Lexem::Type::EndOfFile )
	{
		if( it_->type == Lexem::Type::BraceLeft )
			elements.emplace_back( ParseBlock() );

		else if( it_->type == Lexem::Type::BraceRight )
			break;

		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ )
			elements.emplace_back( ParseVariablesDeclaration() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ )
			elements.emplace_back( ParseAutoVariableDeclaration() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::return_ )
			elements.emplace_back( ParseReturnOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::while_ )
			elements.emplace_back( ParseWhileOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::for_ )
			elements.emplace_back( ParseForOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::break_ )
			elements.emplace_back( ParseBreakOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::continue_ )
			elements.emplace_back( ParseContinueOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ )
			elements.emplace_back( ParseIfOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_if_ )
			elements.emplace_back( ParseStaticIfOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ )
			elements.emplace_back( ParseStaticAssert() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::halt_ )
			elements.emplace_back( ParseHalt() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::safe_ )
		{
			NextLexem();
			Block block= ParseBlock();
			block.safety_= Block::Safety::Safe;
			elements.emplace_back( std::move( block ) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::unsafe_ )
		{
			NextLexem();
			Block block= ParseBlock();
			block.safety_= Block::Safety::Unsafe;
			elements.emplace_back( std::move( block ) );
		}
		else if( it_->type == Lexem::Type::Increment )
		{
			IncrementOperator op( it_->file_pos );
			NextLexem();

			op.expression= ParseExpression();
			elements.push_back( std::move(op) );

			if( it_->type != Lexem::Type::Semicolon )
			{
				PushErrorMessage();
				return elements;
			}
			NextLexem();
		}
		else if( it_->type == Lexem::Type::Decrement )
		{
			DecrementOperator op( it_->file_pos );
			NextLexem();

			op.expression= ParseExpression();
			elements.push_back( std::move(op) );

			if( it_->type != Lexem::Type::Semicolon )
			{
				PushErrorMessage();
				return elements;
			}
			NextLexem();
		}
		else
		{
			if( it_->type == Lexem::Type::Identifier )
			{
				if( const auto macro= FetchMacro( it_->text, Macro::Context::Block ) )
				{
					std::vector<BlockElement> macro_elements= ExpandMacro( *macro, &SyntaxAnalyzer::ParseBlockElements );
					for( auto& element : macro_elements )
						elements.push_back( std::move(element) );
					continue;
				}
			}

			Expression l_expression= ParseExpression();

			if( it_->type == Lexem::Type::Assignment )
			{
				AssignmentOperator assignment_operator( it_->file_pos );
				NextLexem();

				assignment_operator.l_value_= std::move(l_expression);
				assignment_operator.r_value_= ParseExpression();

				if( it_->type != Lexem::Type::Semicolon )
				{
					PushErrorMessage();
					TryRecoverAfterError( g_block_body_elements_control_lexems );
					continue;
				}
				NextLexem();

				elements.emplace_back( std::move(assignment_operator) );
			}
			else if( IsAdditiveAssignmentOperator( *it_ ) )
			{
				AdditiveAssignmentOperator op( it_->file_pos );

				op.additive_operation_= GetAdditiveAssignmentOperator( *it_ );
				NextLexem();

				op.l_value_= std::move(l_expression);
				op.r_value_= ParseExpression();

				elements.push_back( std::move(op) );

				if( it_->type != Lexem::Type::Semicolon )
				{
					PushErrorMessage();
					TryRecoverAfterError( g_block_body_elements_control_lexems );
					continue;
				}
				NextLexem();
			}
			else if( it_->type == Lexem::Type::Semicolon )
			{
				SingleExpressionOperator expr(it_->file_pos );
				NextLexem();

				expr.expression_= std::move(l_expression);
				elements.emplace_back( std::move(expr) );
			}
			else
			{
				PushErrorMessage();
				TryRecoverAfterError( g_block_body_elements_control_lexems );
				continue;
			}
		}
	}

	return elements;
}

Block SyntaxAnalyzer::ParseBlock()
{
	U_ASSERT( it_->type == Lexem::Type::BraceLeft );

	Block block( it_->file_pos );

	NextLexem();

	block.elements_= ParseBlockElements();

	if( it_->type == Lexem::Type::BraceRight )
	{
		block.end_file_pos_= it_->file_pos;
		NextLexem();
	}
	else
	{
		PushErrorMessage();
		return block;
	}

	return block;
}

ClassKindAttribute SyntaxAnalyzer::TryParseClassKindAttribute()
{
	ClassKindAttribute class_kind_attribute= ClassKindAttribute::Class;
	if( it_->type == Lexem::Type::Identifier )
	{
		if( it_->text == Keywords::final_ )
			class_kind_attribute= ClassKindAttribute::Final;
		if( it_->text == Keywords::polymorph_ )
			class_kind_attribute= ClassKindAttribute::Polymorph;
		if( it_->text == Keywords::interface_ )
			class_kind_attribute= ClassKindAttribute::Interface;
		if( it_->text == Keywords::abstract_ )
			class_kind_attribute= ClassKindAttribute::Abstract;

		if( class_kind_attribute != ClassKindAttribute::Class )
			NextLexem();
	}

	return class_kind_attribute;
}

std::vector<ComplexName> SyntaxAnalyzer::TryParseClassParentsList()
{
	std::vector<ComplexName> result;

	if( it_->type != Lexem::Type::Colon )
		return result;

	NextLexem();

	while( NotEndOfFile() )
	{
		result.push_back(ParseComplexName());
		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();
			continue;
		}
		else
			break;
	}

	return result;
}

bool SyntaxAnalyzer::TryParseClassSharedState()
{
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::shared_ )
	{
		NextLexem();
		return true;
	}
	return false;
}

bool SyntaxAnalyzer::TryParseClassFieldsOrdered()
{
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::ordered_ )
	{
		NextLexem();
		return true;
	}
	return false;
}

Typedef SyntaxAnalyzer::ParseTypedef()
{
	U_ASSERT( it_->text == Keywords::type_ );

	NextLexem();

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return Typedef( it_->file_pos );
	}

	const ProgramString& name= it_->text;
	NextLexem();

	Typedef result= ParseTypedefBody();
	result.name= name;
	return result;
}

Typedef SyntaxAnalyzer::ParseTypedefBody()
{
	// Parse something like "- i32;"

	Typedef result( it_->file_pos );

	if( it_->type != Lexem::Type::Assignment )
	{
		PushErrorMessage();
		return result;
	}
	NextLexem();

	result.value= ParseTypeName();

	if( it_->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage();
		return result;
	}
	NextLexem();

	return result;
}

std::unique_ptr<Function> SyntaxAnalyzer::ParseFunction()
{
	U_ASSERT( it_->text == Keywords::fn_ || it_->text == Keywords::op_ );

	auto result= std::make_unique<Function>( it_->file_pos );

	const ProgramString& function_defenition_lexem= it_->text;
	NextLexem();

	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::virtual_ )
	{
		NextLexem();
		result->virtual_function_kind_= VirtualFunctionKind::DeclareVirtual;
		if( it_->type == Lexem::Type::Identifier )
		{
			if( it_->text == Keywords::override_ )
			{
				result->virtual_function_kind_= VirtualFunctionKind::VirtualOverride;
				NextLexem();
			}
			else if( it_->text == Keywords::final_ )
			{
				result->virtual_function_kind_= VirtualFunctionKind::VirtualFinal;
				NextLexem();
			}
			else if( it_->text == Keywords::pure_ )
			{
				result->virtual_function_kind_= VirtualFunctionKind::VirtualPure;
				NextLexem();
			}
		}
	}
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::constexpr_ )
	{
		NextLexem();
		result->constexpr_= true;
	}
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::nomangle_ )
	{
		NextLexem();
		result->no_mangle_= true;
	}
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::enable_if_ )
	{
		NextLexem();

		if( it_->type != Lexem::Type::BracketLeft )
		{
			PushErrorMessage();
			return result;
		}
		NextLexem();

		result->condition_= ParseExpression();

		if( it_->type != Lexem::Type::BracketRight )
		{
			PushErrorMessage();
			return result;
		}
		NextLexem();
	}

	if( function_defenition_lexem == Keywords::fn_ )
	{
		result->name_= ParseComplexName();
		if( result->name_.components.back().name == Keywords::conversion_constructor_ )
		{
			result->name_.components.back().name= Keyword( Keywords::constructor_ );
			result->is_conversion_constructor_= true;
		}
	}
	else
	{
		if( it_->type == Lexem::Type::Identifier || it_->type == Lexem::Type::Scope )
		{
			// Parse complex name before op name - such "op MyStruct::+"
			if( it_->type == Lexem::Type::Scope )
			{
				result->name_.components.emplace_back();
				NextLexem();
			}

			while( NotEndOfFile() )
			{
				if( it_->type != Lexem::Type::Identifier )
				{
					PushErrorMessage();
					return nullptr;
				}
				result->name_.components.emplace_back();
				result->name_.components.back().name= it_->text;
				NextLexem();

				if( it_->type == Lexem::Type::Scope )
					NextLexem();

				if( it_->type == Lexem::Type::Identifier )
					continue;
				else
					break;
			}
		}

		OverloadedOperator overloaded_operator= OverloadedOperator::None;
		switch( it_->type )
		{
		case Lexem::Type::Plus   : overloaded_operator= OverloadedOperator::Add; break;
		case Lexem::Type::Minus  : overloaded_operator= OverloadedOperator::Sub; break;
		case Lexem::Type::Star   : overloaded_operator= OverloadedOperator::Mul; break;
		case Lexem::Type::Slash  : overloaded_operator= OverloadedOperator::Div; break;
		case Lexem::Type::Percent: overloaded_operator= OverloadedOperator::Rem; break;
		case Lexem::Type::CompareEqual   : overloaded_operator= OverloadedOperator::Equal   ; break;
		case Lexem::Type::CompareNotEqual: overloaded_operator= OverloadedOperator::NotEqual; break;
		case Lexem::Type::CompareLess          : overloaded_operator= OverloadedOperator::Less        ; break;
		case Lexem::Type::CompareLessOrEqual   : overloaded_operator= OverloadedOperator::LessEqual   ; break;
		case Lexem::Type::CompareGreater       : overloaded_operator= OverloadedOperator::Greater     ; break;
		case Lexem::Type::CompareGreaterOrEqual: overloaded_operator= OverloadedOperator::GreaterEqual; break;
		case Lexem::Type::And: overloaded_operator= OverloadedOperator::And; break;
		case Lexem::Type::Or : overloaded_operator= OverloadedOperator::Or ; break;
		case Lexem::Type::Xor: overloaded_operator= OverloadedOperator::Xor; break;
		case Lexem::Type::ShiftLeft : overloaded_operator= OverloadedOperator::ShiftLeft ; break;
		case Lexem::Type::ShiftRight: overloaded_operator= OverloadedOperator::ShiftRight; break;
		case Lexem::Type::AssignAdd: overloaded_operator= OverloadedOperator::AssignAdd; break;
		case Lexem::Type::AssignSub: overloaded_operator= OverloadedOperator::AssignSub; break;
		case Lexem::Type::AssignMul: overloaded_operator= OverloadedOperator::AssignMul; break;
		case Lexem::Type::AssignDiv: overloaded_operator= OverloadedOperator::AssignDiv; break;
		case Lexem::Type::AssignRem: overloaded_operator= OverloadedOperator::AssignRem; break;
		case Lexem::Type::AssignAnd: overloaded_operator= OverloadedOperator::AssignAnd; break;
		case Lexem::Type::AssignOr : overloaded_operator= OverloadedOperator::AssignOr ; break;
		case Lexem::Type::AssignXor: overloaded_operator= OverloadedOperator::AssignXor; break;
		case Lexem::Type::AssignShiftLeft : overloaded_operator= OverloadedOperator::AssignShiftLeft ; break;
		case Lexem::Type::AssignShiftRight: overloaded_operator= OverloadedOperator::AssignShiftRight; break;
		case Lexem::Type::Not  : overloaded_operator= OverloadedOperator::LogicalNot; break;
		case Lexem::Type::Tilda: overloaded_operator= OverloadedOperator::BitwiseNot; break;
		case Lexem::Type::Assignment: overloaded_operator= OverloadedOperator::Assign; break;
		case Lexem::Type::Increment: overloaded_operator= OverloadedOperator::Increment; break;
		case Lexem::Type::Decrement: overloaded_operator= OverloadedOperator::Decrement; break;

		case Lexem::Type::SquareBracketLeft:
			NextLexem();
			if( it_->type != Lexem::Type::SquareBracketRight )
			{
				PushErrorMessage();
				return nullptr;
			}
			overloaded_operator= OverloadedOperator::Indexing;
			break;

		case Lexem::Type::BracketLeft:
			NextLexem();
			if( it_->type != Lexem::Type::BracketRight )
			{
				PushErrorMessage();
				return nullptr;
			}
			overloaded_operator= OverloadedOperator::Call;
			break;

		default:
			PushErrorMessage();
			return nullptr;
		};

		result->name_.components.emplace_back();
		result->name_.components.back().name= OverloadedOperatorToString( overloaded_operator );
		result->overloaded_operator_= overloaded_operator;

		NextLexem();
	}

	if( it_->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage();
		return nullptr;
	}

	NextLexem();

	FunctionArgumentsDeclaration& arguments= result->type_.arguments_;

	// Try parse "this"
	if( it_->type == Lexem::Type::Identifier )
	{
		bool is_this= false;
		MutabilityModifier mutability_modifier= MutabilityModifier::None;
		if( it_->text == Keywords::mut_ )
		{
			NextLexem();
			if( !( it_->type == Lexem::Type::Identifier && it_->text == Keywords::this_ ) )
			{
				PushErrorMessage();
				return nullptr;
			}
			NextLexem();

			is_this= true;
			mutability_modifier= MutabilityModifier::Mutable;
		}
		else if( it_->text == Keywords::imut_ )
		{
			NextLexem();
			if( !( it_->type == Lexem::Type::Identifier && it_->text == Keywords::this_ ) )
			{
				PushErrorMessage();
				return nullptr;
			}
			NextLexem();

			is_this= true;
			mutability_modifier= MutabilityModifier::Immutable;
		}
		else if( it_->text == Keywords::this_ )
		{
			is_this= true;
			NextLexem();
		}

		if( is_this )
		{
			const FilePos& file_pos= it_->file_pos;

			ReferencesTagsList tags_list;
			if( it_->type == Lexem::Type::Apostrophe )
				tags_list= ParseReferencesTagsList();

			if( it_->type == Lexem::Type::Comma )
			{
				NextLexem();
				// Disallov constructions, like "fn f( mut this, ){}"
				if( it_->type == Lexem::Type::BracketRight )
					PushErrorMessage();
			}

			FunctionArgument this_argument( file_pos );
			this_argument.name_= Keyword( Keywords::this_ );
			this_argument.mutability_modifier_= mutability_modifier;
			this_argument.reference_modifier_= ReferenceModifier::Reference;
			this_argument.reference_tag_= Keyword( Keywords::this_ ); // Implicit set name for tag of "this" to "this".
			this_argument.inner_arg_reference_tags_= std::move(tags_list);
			arguments.push_back( std::move( this_argument ) );
		}
	}

	while( NotEndOfFile() && it_->type != Lexem::Type::EndOfFile )
	{
		if( it_->type == Lexem::Type::BracketRight )
		{
			NextLexem();
			break;
		}

		arguments.push_back( ParseFunctionArgument() );

		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();
			// Disallov constructions, like "fn f( a : int, ){}"
			if( it_->type == Lexem::Type::BracketRight )
				PushErrorMessage();
		}
		else if( it_->type == Lexem::Type::BracketRight )
		{}
		else
		{
			PushErrorMessage();
			TryRecoverAfterError( g_function_arguments_list_control_lexems );
		}
	}

	// If method is constructor or destructor and "this" not explicitly specified, add it.
	// It's easier add "this" here, than dealing with implicit "this" in CodeBuilder.
	if( ( result->name_.components.back().name == Keywords::constructor_ || result->name_.components.back().name == Keywords::destructor_ ) &&
		( arguments.empty() || arguments.front().name_ != Keywords::this_ ) )
	{
		FunctionArgument this_argument( result->file_pos_ );
		this_argument.name_= Keyword( Keywords::this_ );
		this_argument.mutability_modifier_= MutabilityModifier::Mutable;
		this_argument.reference_modifier_= ReferenceModifier::Reference;
		this_argument.reference_tag_= Keyword( Keywords::this_ );
		arguments.insert( arguments.begin(), std::move( this_argument ) );
	}

	ParseFunctionTypeEnding( result->type_ );

	if( it_->type == Lexem::Type::Semicolon )
	{
		// function prototype
		NextLexem();
	}
	else if( it_->type == Lexem::Type::Assignment )
	{
		// =default;

		NextLexem();

		if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::default_ || it_->text == Keywords::delete_ ) )
			result->body_kind= it_->text == Keywords::default_ ? Function::BodyKind::BodyGenerationRequired : Function::BodyKind::BodyGenerationDisabled;
		else
		{
			PushErrorMessage();
			return nullptr;
		}
		NextLexem();

		if( it_->type != Lexem::Type::Semicolon )
		{
			PushErrorMessage();
			return nullptr;
		}
		NextLexem();
	}
	else
	{
		if( it_->type == Lexem::Type::BracketLeft )
		{
			result->constructor_initialization_list_= std::make_unique<StructNamedInitializer>( it_->file_pos );
			NextLexem();

			while( NotEndOfFile() )
			{
				if( it_->type == Lexem::Type::BracketRight )
				{
					NextLexem();
					break;
				}

				if( it_->type != Lexem::Type::Identifier )
				{
					PushErrorMessage();
					return nullptr;
				}
				result->constructor_initialization_list_->members_initializers.emplace_back();
				result->constructor_initialization_list_->members_initializers.back().name= it_->text;
				Initializer& initializer= result->constructor_initialization_list_->members_initializers.back().initializer;

				NextLexem();
				initializer= ParseVariableInitializer();
				if( std::get_if<EmptyVariant>(&initializer) != nullptr )
					PushErrorMessage();

				if( it_->type == Lexem::Type::Comma )
					NextLexem();
			}
		}

		if( it_->type == Lexem::Type::BraceLeft )
			result->block_= std::make_unique<Block>( ParseBlock() );
		else
		{
			PushErrorMessage();
			return nullptr;
		}
	}

	return result;
}

std::unique_ptr<Class> SyntaxAnalyzer::ParseClass()
{
	U_ASSERT( it_->text == Keywords::struct_ || it_->text == Keywords::class_ );
	const bool is_class= it_->text == Keywords::class_;
	const FilePos& class_file_pos= it_->file_pos;
	NextLexem();

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return nullptr;
	}
	ProgramString name= it_->text;
	NextLexem();

	ClassKindAttribute class_kind_attribute= ClassKindAttribute::Struct;
	std::vector<ComplexName> parents_list;
	if( is_class )
	{
		class_kind_attribute= TryParseClassKindAttribute();
		parents_list= TryParseClassParentsList();
	}
	const bool have_shared_state= TryParseClassSharedState();
	const bool keep_fields_order= TryParseClassFieldsOrdered();

	std::unique_ptr<Class> result= ParseClassBody();
	if( result != nullptr )
	{
		result->file_pos_= class_file_pos;
		result->name_= std::move(name);
		result->kind_attribute_= class_kind_attribute;
		result->have_shared_state_= have_shared_state;
		result->keep_fields_order_= keep_fields_order;
		result->parents_= std::move(parents_list);
	}

	return result;
}

ClassElements SyntaxAnalyzer::ParseClassBodyElements()
{
	ClassElements result;

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::BraceRight )
			break;
		else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::fn_ || it_->text == Keywords::op_ ) )
		{
			result.emplace_back( ParseFunction() );
		}
		else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::struct_ || it_->text == Keywords::class_ ) )
		{
			result.emplace_back( ParseClass() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ )
		{
			result.emplace_back( ParseVariablesDeclaration() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ )
		{
			result.emplace_back( ParseAutoVariableDeclaration() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ )
		{
			result.emplace_back( ParseStaticAssert() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::enum_ )
		{
			result.emplace_back( ParseEnum() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::type_ )
		{
			result.emplace_back( ParseTypedef() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ )
		{
			TemplateVar template_= ParseTemplate();
			
			if( auto* const class_template= std::get_if<ClassTemplate>(&template_) )
				result.emplace_back( std::move( *class_template ) );
			else if( auto* const typedef_template= std::get_if<TypedefTemplate>(&template_) )
				result.emplace_back( std::move( *typedef_template ) );
			else if( auto* const function_template= std::get_if<FunctionTemplate>(&template_) )
				result.emplace_back( std::move( *function_template ) );
			else if( std::get_if<EmptyVariant>(&template_) )
			{}
			else
			{
				// TODO - push more relevant message.
				PushErrorMessage();
			}
		}
		else if( it_->type == Lexem::Type::Identifier &&
			( it_->text == Keywords::public_ || it_->text == Keywords::private_ || it_->text == Keywords::protected_ ) )
		{
			ClassMemberVisibility visibility= ClassMemberVisibility::Public;
			if( it_->text == Keywords::private_ )
				visibility= ClassMemberVisibility::Private;
			if( it_->text == Keywords::protected_ )
				visibility= ClassMemberVisibility::Protected;

			result.emplace_back( ClassVisibilityLabel( it_->file_pos, visibility ) );

			NextLexem();
			if( it_->type != Lexem::Type::Colon )
				PushErrorMessage();
			NextLexem();
		}
		else
		{
			if( it_->type == Lexem::Type::Identifier )
			{
				if( const Macro* const macro= FetchMacro( it_->text, Macro::Context::Class ) )
				{
					for( auto& element : ExpandMacro( *macro, &SyntaxAnalyzer::ParseClassBodyElements ) )
						result.push_back( std::move(element) );
					continue;
				}
			}

			ClassField field( it_->file_pos );

			field.type= ParseTypeName();

			bool is_reference= false;
			if( it_->type == Lexem::Type::And )
			{
				is_reference= true;
				NextLexem();
				field.reference_modifier= ReferenceModifier::Reference;
			}

			if( it_->type == Lexem::Type::Identifier )
			{
				if( it_->text == Keywords::mut_ )
				{
					NextLexem();
					field.mutability_modifier= MutabilityModifier::Mutable;
				}
				if( it_->text == Keywords::imut_ )
				{
					NextLexem();
					field.mutability_modifier= MutabilityModifier::Immutable;
				}
				if( is_reference && it_->text == Keywords::constexpr_ ) // Allow "constexpr" modifier only for reference fields.
				{
					NextLexem();
					field.mutability_modifier= MutabilityModifier::Constexpr;
				}
			}

			if( it_->type == Lexem::Type::Identifier )
			{
				field.name= it_->text;
				NextLexem();
			}
			else
			{
				PushErrorMessage();
				TryRecoverAfterError( g_class_body_elements_control_lexems );
				continue;
			}

			Initializer field_initializer= ParseVariableInitializer();
			if( std::get_if<EmptyVariant>( &field_initializer ) == nullptr )
				field.initializer= std::make_unique<Initializer>( std::move(field_initializer) );

			if( it_->type == Lexem::Type::Semicolon )
				NextLexem();
			else
			{
				PushErrorMessage();
				TryRecoverAfterError( g_class_body_elements_control_lexems );
				continue;
			}

			result.emplace_back( std::move( field ) );
		}
	}

	return result;
}

std::unique_ptr<Class> SyntaxAnalyzer::ParseClassBody()
{
	auto result= std::make_unique<Class>( it_->file_pos );

	if( it_->type == Lexem::Type::Semicolon )
	{
		NextLexem();
		result->is_forward_declaration_= true;
		return result;
	}
	else if( it_->type == Lexem::Type::BraceLeft )
	{
		NextLexem();
	}
	else
	{
		PushErrorMessage();
		return result;
	}

	result->elements_= ParseClassBodyElements();

	if( it_->type != Lexem::Type::BraceRight )
	{
		PushErrorMessage();
		return result;
	}
	NextLexem();

	return result;
}

SyntaxAnalyzer::TemplateVar SyntaxAnalyzer::ParseTemplate()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ );
	const FilePos& template_file_pos= it_->file_pos;
	NextLexem();

	// TemplateBase parameters
	std::vector<TemplateBase::Arg> args;

	if( it_->type != Lexem::Type::TemplateBracketLeft )
	{
		PushErrorMessage();
		return EmptyVariant();
	}
	NextLexem();

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::TemplateBracketRight )
		{
			NextLexem();
			break;
		}

		args.emplace_back();

		if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::type_ )
			NextLexem();
		else
		{
			auto arg_type= std::make_unique<Expression>( NamedOperand( it_->file_pos, ParseComplexName() ) );
			args.back().arg_type= &std::get_if<NamedOperand>(arg_type.get())->name_;
			args.back().arg_type_expr= std::move(arg_type);
		}

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return EmptyVariant();
		}

		ComplexName name;
		name.components.emplace_back();
		name.components.back().name= it_->text;
		auto name_ptr= std::make_unique<Expression>( NamedOperand( it_->file_pos, std::move(name) ) );
		args.back().name= &std::get_if<NamedOperand>(name_ptr.get())->name_;
		args.back().name_expr= std::move(name_ptr);

		NextLexem();

		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();
			if( it_->type == Lexem::Type::TemplateBracketRight )
				PushErrorMessage();
		}
		else if( it_->type == Lexem::Type::TemplateBracketRight )
		{}
		else
		{
			PushErrorMessage();
			TryRecoverAfterError( g_template_arguments_list_control_lexems );
		}
	} // for arg parameters

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return EmptyVariant();
	}

	enum class TemplateKind
	{
		Invalid,
		Struct,
		Class,
		Typedef,
	};
	TemplateKind template_kind= TemplateKind::Invalid;

	ProgramString name;
	const FilePos& template_thing_file_pos= it_->file_pos;
	if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::struct_ || it_->text == Keywords::class_ ) )
	{
		template_kind= it_->text == Keywords::struct_ ? TemplateKind::Struct : TemplateKind::Class;
		NextLexem();

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return EmptyVariant();
		}
		name= it_->text;
		NextLexem();
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::type_ )
	{
		NextLexem();

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return EmptyVariant();
		}
		name= it_->text;
		NextLexem();
		template_kind= TemplateKind::Typedef;
	}
	else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::fn_ || it_->text == Keywords::op_ ) )
	{
		FunctionTemplate function_template( template_file_pos );
		function_template.args_= std::move(args);
		function_template.function_= ParseFunction();
		if( function_template.function_ != nullptr )
		{
			function_template.function_->is_template_= true;
			return std::move(function_template);
		}
		else
			return EmptyVariant();
	}
	else
	{
		PushErrorMessage();
		return EmptyVariant();
	}

	// TypeTemplateBase parameters
	std::vector<TypeTemplateBase::SignatureArg> signature_args;
	bool is_short_form= false;

	if( it_->type == Lexem::Type::TemplateBracketLeft )
	{
		// Parse signature args
		NextLexem();
		while( NotEndOfFile() )
		{
			if( it_->type == Lexem::Type::TemplateBracketRight )
			{
				NextLexem();
				break;
			}

			signature_args.emplace_back();
			signature_args.back().name= ParseExpression();

			if( it_->type == Lexem::Type::Assignment )
			{
				NextLexem();
				signature_args.back().default_value= ParseExpression();
			}

			if( it_->type == Lexem::Type::Comma )
			{
				NextLexem();
				if( it_->type == Lexem::Type::TemplateBracketRight )
					PushErrorMessage();
			}
			else if( it_->type == Lexem::Type::TemplateBracketRight )
			{}
			else
			{
				PushErrorMessage();
				TryRecoverAfterError( g_template_arguments_list_control_lexems );
			}
		} // for signature args
	}
	else
		is_short_form= true;

	switch( template_kind )
	{
	case TemplateKind::Class:
	case TemplateKind::Struct:
		{
			ClassTemplate class_template( template_file_pos );
			class_template.args_= std::move(args);
			class_template.signature_args_= std::move(signature_args);
			class_template.name_= name;
			class_template.is_short_form_= is_short_form;

			ClassKindAttribute class_kind_attribute= ClassKindAttribute::Struct;
			std::vector<ComplexName> class_parents_list;
			if( template_kind == TemplateKind::Class )
			{
				class_kind_attribute= TryParseClassKindAttribute();
				class_parents_list= TryParseClassParentsList();
			}
			const bool have_shared_state= TryParseClassSharedState();
			const bool keep_fields_order= TryParseClassFieldsOrdered();
			class_template.class_= ParseClassBody();
			if( class_template.class_ != nullptr )
			{
				class_template.class_->file_pos_= template_thing_file_pos;
				class_template.class_->name_= std::move(name);
				class_template.class_->kind_attribute_= class_kind_attribute;
				class_template.class_->have_shared_state_= have_shared_state;
				class_template.class_->keep_fields_order_= keep_fields_order;
				class_template.class_->parents_= std::move(class_parents_list);
			}
			return std::move(class_template);
		}

	case TemplateKind::Typedef:
		{
			TypedefTemplate typedef_template( template_file_pos );
			typedef_template.args_= std::move(args);
			typedef_template.signature_args_= std::move(signature_args);
			typedef_template.name_= name;
			typedef_template.is_short_form_= is_short_form;

			typedef_template.typedef_= std::make_unique<Typedef>( ParseTypedefBody() );
			typedef_template.typedef_->name= std::move(name);
			return std::move(typedef_template);
		}

	case TemplateKind::Invalid:
		break;
	};

	U_ASSERT(false);
	return EmptyVariant();
}

const Macro* SyntaxAnalyzer::FetchMacro( const ProgramString& macro_name, const Macro::Context context )
{
	const MacroMap& macro_map= (*macros_)[context];
	const auto it= macro_map.find(macro_name);
	if( it != macro_map.end() )
		return &it->second;

	return nullptr;
}

template<typename ParseFnResult>
ParseFnResult SyntaxAnalyzer::ExpandMacro( const Macro& macro, ParseFnResult (SyntaxAnalyzer::*parse_fn)() )
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == macro.name );
	NextLexem();

	std::map<ProgramString, ParsedMacroElement> elements_map;
	if( !MatchMacroBlock( macro.match_template_elements, macro.name, elements_map ) )
		return ParseFnResult();

	MacroNamesMap names_map;
	names_map.prev= nullptr;
	names_map.names= &elements_map;

	ProgramStringMap<ProgramString> unique_macro_identifier_map;
	Lexems result_lexems= DoExpandMacro( names_map, macro.result_template_elements, unique_macro_identifier_map );

	Lexem eof;
	eof.type= Lexem::Type::EndOfFile;
	result_lexems.push_back(eof);

	SyntaxAnalyzer result_analyzer( macros_ );
	result_analyzer.it_= result_lexems.begin();
	result_analyzer.it_end_= result_lexems.end();

	auto element= (result_analyzer.*parse_fn)();
	error_messages_.insert( error_messages_.end(), result_analyzer.error_messages_.begin(), result_analyzer.error_messages_.end() );
	return std::move(element);
}

bool SyntaxAnalyzer::MatchMacroBlock(
	const std::vector<Macro::MatchElement>& match_elements,
	const ProgramString& macro_name,
	std::map<ProgramString, ParsedMacroElement>& out_elements )
{
	const auto push_macro_error=
	[&]
	{
		SyntaxErrorMessage msg;
		msg.file_pos= it_->file_pos;
		msg.text= "Unexpected lexem - \"" + it_->text + "\". (In expansion of macro \"" + macro_name + "\")";
		error_messages_.push_back(std::move(msg));
	};

	for( size_t i= 0u; i < match_elements.size(); ++i )
	{
		const Macro::MatchElement& match_element= match_elements[i];

		switch( match_element.kind )
		{
		case Macro::MatchElementKind::Lexem:
			if( it_->type == match_element.lexem.type &&
				it_->text == match_element.lexem.text )
				NextLexem();
			else
			{
				push_macro_error();
				return false;
			}
			break;

		case Macro::MatchElementKind::Identifier:
			if( it_->type == Lexem::Type::Identifier )
			{
				ParsedMacroElement element;
				element.begin= it_;
				NextLexem();
				element.end= it_;
				element.kind= match_element.kind;
				out_elements[match_element.name]= std::move(element);
			}
			else
			{
				push_macro_error();
				return false;
			}
			break;

		case Macro::MatchElementKind::Typename:
			{
				ParsedMacroElement element;
				element.begin= it_;
				ParseTypeName();
				element.end= it_;
				element.kind= match_element.kind;
				out_elements[match_element.name]= std::move(element);
			}
			break;

		case Macro::MatchElementKind::Expression:
			{
				ParsedMacroElement element;
				element.begin= it_;
				const Expression expression= ParseExpression();
				if( std::get_if<EmptyVariant>( &expression ) != nullptr )
				{
					push_macro_error();
					return false;
				}
				element.end= it_;
				element.kind= match_element.kind;
				out_elements[match_element.name]= std::move(element);
			}
			break;

		case Macro::MatchElementKind::Block:
			{
				ParsedMacroElement element;
				element.begin= it_;
				ParseBlock();
				element.end= it_;
				element.kind= match_element.kind;
				out_elements[match_element.name]= std::move(element);
			}
			break;

		case Macro::MatchElementKind::Optional:
			{
				ParsedMacroElement element;
				element.kind= match_element.kind;

				if( i + 1u < match_elements.size() && match_element.block_check_lexem_kind == Macro::BlockCheckLexemKind::LexemAfterBlockEnd )
				{
					const Lexem& terminator_lexem= match_elements[i+1u].lexem;
					if( it_->type == terminator_lexem.type && it_->text == terminator_lexem.text )
					{} // Optional is empty
					else
					{
						std::map<ProgramString, ParsedMacroElement> optional_elements;
						if( MatchMacroBlock( match_element.sub_elements, macro_name, optional_elements ) )
							element.sub_elements.push_back( std::move(optional_elements) );
						else
							return false;
					}
				}
				else if( match_element.block_check_lexem_kind == Macro::BlockCheckLexemKind::LexemAtBlockStart )
				{
					U_ASSERT( ! match_element.sub_elements.empty() && match_element.sub_elements.front().kind == Macro::MatchElementKind::Lexem );
					const Lexem& check_lexem= match_element.sub_elements.front().lexem;
					if( it_->type == check_lexem.type && it_->text == check_lexem.text )
					{
						std::map<ProgramString, ParsedMacroElement> optional_elements;
						if( MatchMacroBlock( match_element.sub_elements, macro_name, optional_elements ) )
							element.sub_elements.push_back( std::move(optional_elements) );
						else
							return false;
					}
					else {} // Optional is empty.
				}
				else U_ASSERT(false);

				out_elements[match_element.name]= std::move(element);
			}
			break;

		case Macro::MatchElementKind::Repeated:
			{
				ParsedMacroElement element;
				element.kind= match_element.kind;

				if( i + 1u < match_elements.size() && match_element.block_check_lexem_kind == Macro::BlockCheckLexemKind::LexemAfterBlockEnd )
				{
					const Lexem& terminator_lexem= match_elements[i+1u].lexem;
					while(NotEndOfFile())
					{
						if( it_->type == terminator_lexem.type && it_->text == terminator_lexem.text )
							break;

						std::map<ProgramString, ParsedMacroElement> optional_elements;
						if( MatchMacroBlock( match_element.sub_elements, macro_name, optional_elements ) )
							element.sub_elements.push_back( std::move(optional_elements) );
						else
							return false;

						// Process separator.
						if( match_element.lexem.type != Lexem::Type::EndOfFile )
						{
							if( it_->type == match_element.lexem.type && it_->text == match_element.lexem.text )
							{
								NextLexem();
								if( it_->type == terminator_lexem.type && it_->text == terminator_lexem.text )
								{
									// Disable end lexem after separator.
									push_macro_error();
									return false;
								}
							}
							else
								break;
						}
					}
				}
				else if( match_element.block_check_lexem_kind == Macro::BlockCheckLexemKind::LexemAtBlockStart )
				{
					U_ASSERT( ! match_element.sub_elements.empty() && match_element.sub_elements.front().kind == Macro::MatchElementKind::Lexem );
					const Lexem& check_lexem= match_element.sub_elements.front().lexem;
					while(NotEndOfFile())
					{
						if( !( it_->type == check_lexem.type && it_->text == check_lexem.text ) )
							break;

						std::map<ProgramString, ParsedMacroElement> optional_elements;
						if( MatchMacroBlock( match_element.sub_elements, macro_name, optional_elements ) )
							element.sub_elements.push_back( std::move(optional_elements) );
						else
							return false;

						// Process separator.
						if( match_element.lexem.type != Lexem::Type::EndOfFile )
						{
							if( it_->type == match_element.lexem.type && it_->text == match_element.lexem.text )
							{
								NextLexem();
								if( !( it_->type == check_lexem.type && it_->text == check_lexem.text ) )
								{
									// After separator must be start lexem of block.
									push_macro_error();
									return false;
								}
							}
							else
								break;
						}
					}
				}
				else U_ASSERT(false);

				out_elements[match_element.name]= std::move(element);
			}
			break;
		};
	}

	return true;
}

Lexems SyntaxAnalyzer::DoExpandMacro(
	const MacroNamesMap& parsed_elements,
	const std::vector<Macro::ResultElement>& result_elements,
	ProgramStringMap<ProgramString>& unique_macro_identifier_map )
{
	Lexems result_lexems;
	for( const Macro::ResultElement& result_element : result_elements )
	{
		switch( result_element.kind )
		{
		case Macro::ResultElementKind::Lexem:
			if( result_element.lexem.type == Lexem::Type::MacroUniqueIdentifier )
			{
				// Replace ??lexems inside macro with unique identifiers.
				Lexem l;
				l.type= Lexem::Type::Identifier;
				l.file_pos= result_element.lexem.file_pos;

				const auto it= unique_macro_identifier_map.find( result_element.lexem.text );
				if( it != unique_macro_identifier_map.end() )
					l.text= it->second;
				else
				{
					// TODO - Do not use pointer here.
					l.text= "macro_ident_" + std::to_string( reinterpret_cast<uintptr_t>( &unique_macro_identifier_map ) ) + "_" + std::to_string( unique_macro_identifier_map.size() );
					unique_macro_identifier_map[ result_element.lexem.text ]= l.text;
				}

				result_lexems.push_back( std::move(l) );
			}
			else
				result_lexems.push_back( result_element.lexem );
			break;

		case Macro::ResultElementKind::VariableElement:
			{
				const auto element= parsed_elements.GetElement( result_element.name );
				if( element == nullptr )
				{
					SyntaxErrorMessage msg;
					msg.file_pos= result_element.lexem.file_pos;
					msg.text= result_element.name + " not found";
					error_messages_.push_back( std::move(msg) );
					return result_lexems;
				}
				else
					result_lexems.insert( result_lexems.end(), element->begin, element->end );
			}
			break;

		case Macro::ResultElementKind::VariableElementWithMacroBlock:
			{
				const auto element= parsed_elements.GetElement( result_element.name );
				if( element == nullptr )
				{
					SyntaxErrorMessage msg;
					msg.file_pos= result_element.lexem.file_pos;
					msg.text= result_element.name + " not found";
					error_messages_.push_back( std::move(msg) );
					return result_lexems;
				}
				else
				{
					if( result_element.kind == Macro::ResultElementKind::VariableElementWithMacroBlock )
					{
						for( const auto& sub_elements : element->sub_elements )
						{
							MacroNamesMap sub_elements_map;
							sub_elements_map.prev= &parsed_elements;
							sub_elements_map.names= &sub_elements;

							Lexems element_lexems= DoExpandMacro( sub_elements_map, result_element.sub_elements, unique_macro_identifier_map );
							result_lexems.insert( result_lexems.end(), element_lexems.begin(), element_lexems.end() );

							// Push separator.
							if( &sub_elements != &element->sub_elements.back() && result_element.lexem.type != Lexem::Type::EndOfFile )
								result_lexems.push_back( result_element.lexem );
						}
					}
					else
					{
						SyntaxErrorMessage msg;
						msg.file_pos= result_element.lexem.file_pos;
						msg.text= "Expected optional or loop.";
						error_messages_.push_back( std::move(msg) );
						return result_lexems;
					}
				}
			}
			break;
		};
	}

	return result_lexems;
}

void SyntaxAnalyzer::NextLexem()
{
	if( NotEndOfFile() )
		++it_;
}

bool SyntaxAnalyzer::NotEndOfFile()
{
	// ise std::next, because we need possibility to dereference iterator after "end".
	return std::next(it_) < it_end_;
}

void SyntaxAnalyzer::TryRecoverAfterError( const std::vector<ExpectedLexem>& expected_lexems0 )
{
	return TryRecoverAfterError( expected_lexems0, std::vector<ExpectedLexem>(), std::vector<ExpectedLexem>() );
}

void SyntaxAnalyzer::TryRecoverAfterError( const std::vector<ExpectedLexem>& expected_lexems0, const std::vector<ExpectedLexem>& expected_lexems1 )
{
	return TryRecoverAfterError( expected_lexems0, expected_lexems1, std::vector<ExpectedLexem>() );
}

void SyntaxAnalyzer::TryRecoverAfterError( const std::vector<ExpectedLexem>& expected_lexems0, const std::vector<ExpectedLexem>& expected_lexems1, const std::vector<ExpectedLexem>& expected_lexems2 )
{
	const auto eq=
	[]( const Lexem& lexem, const ExpectedLexem& expected_lexem ) -> bool
	{
		if( lexem.type == Lexem::Type::Identifier )
			return lexem.type == expected_lexem.type && lexem.text == expected_lexem.text;
		return lexem.type == expected_lexem.type;
	};

	while( std::next(it_) < it_end_ )
	{
		if( it_->type == Lexem::Type::BracketLeft || it_->type == Lexem::Type::SquareBracketLeft ||
			it_->type == Lexem::Type::BraceLeft || it_->type == Lexem::Type::TemplateBracketLeft )
		{
			TrySkipBrackets( it_->type );
			if( std::next(it_) >= it_end_ )
				return;
		}

		for( const ExpectedLexem& expected_lexem : expected_lexems0 )
			if( eq( *it_, expected_lexem ) )
				return;
		for( const ExpectedLexem& expected_lexem : expected_lexems1 )
			if( eq( *it_, expected_lexem ) )
				return;
		for( const ExpectedLexem& expected_lexem : expected_lexems2 )
			if( eq( *it_, expected_lexem ) )
				return;

		++it_;
	}
	return;
}

void SyntaxAnalyzer::TrySkipBrackets( Lexem::Type bracket_type )
{
	++it_;
	while( std::next(it_) < it_end_ )
	{
		if( it_->type == Lexem::Type::BracketLeft || it_->type == Lexem::Type::SquareBracketLeft ||
			it_->type == Lexem::Type::BraceLeft || it_->type == Lexem::Type::TemplateBracketLeft )
		{
			TrySkipBrackets( it_->type );
			if( std::next(it_) >= it_end_ )
				return;
		}
		if( it_->type == static_cast<Lexem::Type>(int(bracket_type) + 1) )
		{
			++it_;
			return;
		}
		++it_;
	}
}

void SyntaxAnalyzer::PushErrorMessage()
{
	if( error_messages_.empty() || error_messages_.back().file_pos != it_->file_pos )
	{
		SyntaxErrorMessage error_message;
		error_message.file_pos= it_->file_pos;
		error_message.text= "Syntax error - unexpected lexem \"" + it_->text + "\"";
		error_messages_.push_back( std::move(error_message) );
	}

	// HACK!
	// If we report about error in same position multiple times, advance lexems iterator for looping prevention.

	if( it_ == last_error_it_ )
		++last_error_repeats_;
	else
	{
		last_error_it_= it_;
		last_error_repeats_= 0u;
	}

	if( last_error_repeats_ > 10u )
	{
		NextLexem();
		last_error_it_= it_;
		last_error_repeats_= 0u;
	}
}

} // namespace

std::vector<Import> ParseImports( const Lexems& lexems )
{
	return SyntaxAnalyzer().ParseImportsOnly( lexems );
}

SyntaxAnalysisResult SyntaxAnalysis( const Lexems& lexems, const MacrosPtr& macros )
{
	MacrosPtr macros_copy= std::make_shared<MacrosByContextMap>( *macros );
	return SyntaxAnalyzer( std::move(macros_copy) ).DoAnalyzis( lexems );
}

} // namespace Synt

} // namespace U
