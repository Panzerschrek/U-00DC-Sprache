#include <cctype>
#include <cmath>
#include <map>
#include <set>

#include "assert.hpp"
#include "keywords.hpp"
#include "syntax_analyzer.hpp"

namespace U
{

namespace Synt
{

struct ExpectedLexem
{
	ExpectedLexem( const Lexem::Type in_type ) : type(in_type) {}
	ExpectedLexem( const Keywords keyword ) : type(Lexem::Type::Identifier), text( Keyword( keyword) ) {}

	Lexem::Type type;
	ProgramString text;
};

static const std::vector<ExpectedLexem> g_namespace_body_elements_start_lexems
{
	ExpectedLexem(Keywords::namespace_),
	ExpectedLexem(Keywords::class_), ExpectedLexem(Keywords::struct_),
	ExpectedLexem(Keywords::fn_), ExpectedLexem(Keywords::op_),
	ExpectedLexem(Keywords::var_), ExpectedLexem(Keywords::auto_),
	ExpectedLexem(Keywords::static_assert_),
};

static const std::vector<ExpectedLexem> g_class_body_elements_control_lexems
{
	ExpectedLexem(Keywords::class_), ExpectedLexem(Keywords::struct_),
	ExpectedLexem(Keywords::fn_), ExpectedLexem(Keywords::op_),
	ExpectedLexem(Keywords::var_), ExpectedLexem(Keywords::auto_),
	ExpectedLexem(Keywords::static_assert_),
	ExpectedLexem(Lexem::Type::BraceRight),
};

static const std::vector<ExpectedLexem> g_block_body_elements_control_lexems
{
	ExpectedLexem(Keywords::if_), ExpectedLexem(Keywords::static_if_), ExpectedLexem(Keywords::while_),
	ExpectedLexem(Keywords::return_), ExpectedLexem(Keywords::break_), ExpectedLexem(Keywords::continue_),
	ExpectedLexem(Keywords::var_), ExpectedLexem(Keywords::auto_),
	ExpectedLexem(Keywords::halt_),
	ExpectedLexem(Keywords::static_assert_),
};

static const std::vector<ExpectedLexem> g_function_arguments_list_control_lexems
{
	ExpectedLexem(Lexem::Type::Comma), ExpectedLexem(Lexem::Type::BracketRight),
};

static const std::vector<ExpectedLexem> g_template_arguments_list_control_lexems
{
	ExpectedLexem(Lexem::Type::Comma), ExpectedLexem(Lexem::Type::TemplateBracketRight),
};

static int GetBinaryOperatorPriority( const BinaryOperatorType binary_operator )
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
};

static bool IsBinaryOperator( const Lexem& lexem )
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

static BinaryOperatorType LexemToBinaryOperator( const Lexem& lexem )
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

static bool IsAdditiveAssignmentOperator( const Lexem& lexem )
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

static BinaryOperatorType GetAdditiveAssignmentOperator( const Lexem& lexem )
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

	std::unique_ptr<NumericConstant> ParseNumericConstant();

	IExpressionComponentPtr ParseExpression();

	FunctionArgumentPtr ParseFunctionArgument();
	void ParseFunctionTypeEnding( FunctionType& result );
	std::unique_ptr<FunctionType> ParseFunctionType();

	ITypeNamePtr ParseTypeName();
	std::vector<IExpressionComponentPtr> ParseTemplateParameters();
	ComplexName ParseComplexName();
	ReferencesTagsList ParseReferencesTagsList();
	FunctionReferencesPollutionList ParseFunctionReferencesPollutionList();

	IInitializerPtr ParseInitializer( bool parse_expression_initializer );
	std::unique_ptr<ArrayInitializer> ParseArrayInitializer();
	std::unique_ptr<StructNamedInitializer> ParseStructNamedInitializer();
	std::unique_ptr<ConstructorInitializer> ParseConstructorInitializer();
	std::unique_ptr<ExpressionInitializer> ParseExpressionInitializer();

	VariablesDeclarationPtr ParseVariablesDeclaration();
	std::unique_ptr<AutoVariableDeclaration> ParseAutoVariableDeclaration();

	IBlockElementPtr ParseReturnOperator();
	IBlockElementPtr ParseWhileOperator();
	IBlockElementPtr ParseBreakOperator();
	IBlockElementPtr ParseContinueOperator();
	std::unique_ptr<IfOperator> ParseIfOperator();
	IBlockElementPtr ParseStaticIfOperator();
	std::unique_ptr<StaticAssert> ParseStaticAssert();
	std::unique_ptr<Enum> ParseEnum();
	IBlockElementPtr ParseHalt();

	std::vector<IBlockElementPtr> ParseBlockElements();
	BlockPtr ParseBlock();

	ClassKindAttribute TryParseClassKindAttribute();
	std::vector<ComplexName> TryParseClassParentsList();

	std::unique_ptr<Typedef> ParseTypedef();
	std::unique_ptr<Typedef> ParseTypedefBody();
	std::unique_ptr<Function> ParseFunction();
	std::unique_ptr<Class> ParseClass();
	ClassElements ParseClassBodyElements();
	std::unique_ptr<Class> ParseClassBody();

	TemplateBasePtr ParseTemplate();

	const Macro* FetchMacro( const ProgramString& macro_name, const Macro::Context context );

	template<typename ParseFnResult>
	ParseFnResult ExpandMacro( const Macro& macro, ParseFnResult (SyntaxAnalyzer::*parse_fn)() );

	bool MatchMacroBlock(
		const std::vector<Macro::MatchElement>& match_elements,
		const ProgramString& macro_name,
		std::map<ProgramString, ParsedMacroElement>& out_elements );

	Lexems DoExpandMacro(
		const MacroNamesMap& parsed_elements,
		const std::vector<Macro::ResultElement>& result_elements );

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
		if( !( it_->type == Lexem::Type::MacroIdentifier && it_->text == "?macro"_SpC ) )
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

	U_ASSERT( it_->type == Lexem::Type::MacroIdentifier && it_->text == "?macro"_SpC );
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
		msg.text= "Using keyword as macro name"_SpC;
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

	if( context_str == "expr"_SpC )
		macro_context= Macro::Context::Expression;
	else if( context_str == "block"_SpC )
		macro_context= Macro::Context::Block;
	else if( context_str == "class"_SpC )
		macro_context= Macro::Context::Class;
	else if( context_str == "namespace"_SpC )
		macro_context= Macro::Context::Namespace;
	else
	{
		SyntaxErrorMessage msg;
		msg.file_pos= macro.file_pos;
		msg.text= "\""_SpC + context_str + "\" unknown macro context"_SpC;
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
		msg.text= "\""_SpC + macro.name + "\" macro redefinition."_SpC;
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
				msg.text= "\""_SpC + element.name + "\" macro parameter redefinition."_SpC;
				error_messages_.push_back(std::move(msg));
			}
			if( IsKeyword( element.name ) )
			{
				SyntaxErrorMessage msg;
				msg.file_pos= it_->file_pos;
				msg.text= "Using keyword as macro element name"_SpC;
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

			if( element_type_str == "ident"_SpC )
				element.kind= Macro::MatchElementKind::Identifier;
			else if( element_type_str == "ty"_SpC )
				element.kind= Macro::MatchElementKind::Typename;
			else if( element_type_str == "expr"_SpC )
				element.kind= Macro::MatchElementKind::Expression;
			else if( element_type_str == "block"_SpC )
				element.kind= Macro::MatchElementKind::Block;
			else if( element_type_str == "opt"_SpC )
				element.kind= Macro::MatchElementKind::Optional;
			else if( element_type_str == "rep"_SpC )
				element.kind= Macro::MatchElementKind::Repeated;
			else
			{
				SyntaxErrorMessage msg;
				msg.file_pos= it_->file_pos;
				msg.text= "\""_SpC + element_type_str + "\" unknown macro variable type"_SpC;
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
				msg.text= "Expected lexem at start or after \""_SpC + result[i].name + "\" element."_SpC;
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
					msg.text= "Start lexem of optional macro block must be different from first lexem after optional block."_SpC;
					error_messages_.push_back(std::move(msg));
				}

				if( result[i].kind == Macro::MatchElementKind::Repeated &&
					result[i].lexem.type != Lexem::Type::EndOfFile &&
					result[i].lexem.type == result[i+1].lexem.type && result[i].lexem.text == result[i+1].lexem.text )
				{
					SyntaxErrorMessage msg;
					msg.file_pos= it_->file_pos;
					msg.text= "Separator lexem of repeated macro block must be different from first lexem after repeated block."_SpC;
					error_messages_.push_back(std::move(msg));
				}

				if( result[i].kind == Macro::MatchElementKind::Repeated &&
					result[i].lexem.type == Lexem::Type::EndOfFile &&
					!result[i].sub_elements.empty() && result[i].sub_elements.front().kind == Macro::MatchElementKind::Lexem &&
					result[i].sub_elements.front().lexem.type == result[i+1u].lexem.type && result[i].sub_elements.front().lexem.text == result[i+1u].lexem.text )
				{
					SyntaxErrorMessage msg;
					msg.file_pos= it_->file_pos;
					msg.text= "Start lexem of repeated macro block without separator must be different from first lexem after repeated block."_SpC;
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
			if( IProgramElementPtr program_element= ParseFunction() )
				program_elements.emplace_back( std::move( program_element ) );
		}
		else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::struct_ || it_->text == Keywords::class_ ) )
		{
			if( IProgramElementPtr program_element= ParseClass() )
				program_elements.emplace_back( std::move( program_element ) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ )
		{
			if( TemplateBasePtr template_= ParseTemplate() )
			{
				if( IProgramElement* const program_element= dynamic_cast<IProgramElement*>(template_.get()) )
				{
					template_.release();
					program_elements.emplace_back( program_element );
				}
				else
				{
					// TODO - push more relevant message.
					PushErrorMessage();
				}
			}
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ )
		{
			if( IProgramElementPtr program_element= ParseVariablesDeclaration() )
				program_elements.emplace_back( std::move(program_element) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ )
		{
			if( IProgramElementPtr program_element= ParseAutoVariableDeclaration() )
				program_elements.emplace_back( std::move(program_element) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ )
		{
			if( IProgramElementPtr program_element= ParseStaticAssert() )
				program_elements.emplace_back( std::move(program_element) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::enum_ )
		{
			if( IProgramElementPtr program_element= ParseEnum() )
				program_elements.emplace_back( std::move(program_element) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::type_ )
		{
			if( IProgramElementPtr program_element= ParseTypedef() )
				program_elements.emplace_back( std::move(program_element) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::namespace_ )
		{
			std::unique_ptr<Namespace> namespace_( new Namespace( it_->file_pos ) );
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

std::unique_ptr<NumericConstant> SyntaxAnalyzer::ParseNumericConstant()
{
	const ProgramString& text= it_->text;

	NumericConstant::LongFloat base= 10;
	unsigned int (*number_func)( sprache_char) =
		[]( sprache_char c ) -> unsigned int
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
				[]( sprache_char c ) -> unsigned int
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
				[]( sprache_char c ) -> unsigned int
				{
					U_ASSERT( c >= '0' && c <= '7' );
					return c - '0';
				};
			is_number_func=
				[]( sprache_char c ) -> bool
				{
					return c >= '0' && c <= '9';
				};
			break;

		case 'x':
			it+= 2;
			base= 16;
			number_func=
				[]( sprache_char c ) -> unsigned int
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

	NumericConstant::LongFloat number{ 0 };
	bool has_fraction_point= false;

	ProgramString::const_iterator integer_part_end= it;
	while( integer_part_end < it_end &&
		is_number_func( *integer_part_end ) )
		++integer_part_end;

	{
		NumericConstant::LongFloat pow{ 1 };
		for( auto n= integer_part_end - 1; n >= it; --n, pow*= base )
			number+= number_func(*n) * pow;
	}

	it= integer_part_end;
	if( it < it_end && *it == '.' )
	{
		++it;
		has_fraction_point= true;

		NumericConstant::LongFloat fractional_part{ 0 };
		NumericConstant::LongFloat pow{ 1 / base };

		while( it < it_end && is_number_func(*it) )
		{
			fractional_part+= number_func( *it ) * pow;
			pow/= base;
			++it;
		}

		number+= fractional_part;
	}

	// Exponent
	if( it < it_end && *it == 'e' )
	{
		++it;

		U_ASSERT( base == 10 );
		int power= 0;
		bool is_negative= false;

		if( it < it_end && *it == '-' )
		{
			is_negative= true;
			++it;
		}

		while( it < it_end && is_number_func(*it) )
		{
			power*= int(base);
			power+= number_func(*it);
			++it;
		}
		if( is_negative ) power= -power;

		number*= std::pow<NumericConstant::LongFloat>( base, power );
	}

	ProgramString type_suffix( it, it_end );

	return
		std::unique_ptr<NumericConstant>(
			new NumericConstant(
				it_->file_pos,
				number,
				std::move( type_suffix ),
				has_fraction_point ) );
}

IExpressionComponentPtr SyntaxAnalyzer::ParseExpression()
{
	IExpressionComponentPtr root;

	while( NotEndOfFile() )
	{
		PrefixOperators prefix_operators;

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
				 prefix_operators.emplace_back( new UnaryPlus( it_->file_pos ) );
				NextLexem();
				break;

			case Lexem::Type::Minus:
				prefix_operators.emplace_back( new UnaryMinus( it_->file_pos ) );
				NextLexem();
				break;

			case Lexem::Type::Not:
				prefix_operators.emplace_back( new LogicalNot( it_->file_pos ) );
				NextLexem();
				break;

			case Lexem::Type::Tilda:
				prefix_operators.emplace_back( new BitwiseNot( it_->file_pos ) );
				NextLexem();
				break;

			default:
				if( prefix_operators.empty() )
				{
					if( root == nullptr )
						PushErrorMessage();
					return root;
				}
				else
				{
					PushErrorMessage();
					return nullptr;
				}
			};
		}

	parse_operand:
		ExpressionComponentWithUnaryOperatorsPtr current_node;

		// Operand.
		if( it_->type == Lexem::Type::Identifier )
		{
			if( it_->text == Keywords::true_ )
			{
				current_node.reset( new BooleanConstant( it_->file_pos, true ) );
				NextLexem();
			}
			else if( it_->text == Keywords::false_ )
			{
				current_node.reset( new BooleanConstant( it_->file_pos, false ) );
				NextLexem();
			}
			else if( it_->text == Keywords::move_ )
			{
				std::unique_ptr<MoveOperator> move_operator( new MoveOperator( it_->file_pos ) );

				NextLexem();
				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				if( it_->type != Lexem::Type::Identifier )
				{
					PushErrorMessage();
					return nullptr;
				}
				move_operator->var_name_= it_->text;
				NextLexem();

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				current_node= std::move(move_operator);
			}
			else if( it_->text == Keywords::cast_ref )
			{
				std::unique_ptr<CastRef> cast( new CastRef( it_->file_pos ) );

				NextLexem();
				if( it_->type != Lexem::Type::TemplateBracketLeft )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				cast->type_= ParseTypeName();
				if( it_->type != Lexem::Type::TemplateBracketRight )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				cast->expression_= ParseExpression();

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				current_node= std::move(cast);
			}
			else if( it_->text == Keywords::cast_ref_unsafe )
			{
				std::unique_ptr<CastRefUnsafe> cast( new CastRefUnsafe( it_->file_pos ) );

				NextLexem();
				if( it_->type != Lexem::Type::TemplateBracketLeft )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				cast->type_= ParseTypeName();
				if( it_->type != Lexem::Type::TemplateBracketRight )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				cast->expression_= ParseExpression();

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				current_node= std::move(cast);
			}
			else if( it_->text == Keywords::cast_imut )
			{
				std::unique_ptr<CastImut> cast( new CastImut( it_->file_pos ) );

				NextLexem();

				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				cast->expression_= ParseExpression();

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				current_node= std::move(cast);
			}
			else if( it_->text == Keywords::cast_mut )
			{
				std::unique_ptr<CastMut> cast( new CastMut( it_->file_pos ) );

				NextLexem();

				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				cast->expression_= ParseExpression();

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				current_node= std::move(cast);
			}
			else if( it_->text == Keywords::typeinfo_ )
			{
				std::unique_ptr<TypeInfo> typeinfo_( new TypeInfo( it_->file_pos ) );
				NextLexem();

				if( it_->type != Lexem::Type::TemplateBracketLeft )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				typeinfo_->type_= ParseTypeName();
				if( it_->type != Lexem::Type::TemplateBracketRight )
				{
					PushErrorMessage();
					return nullptr;
				}
				NextLexem();

				current_node= std::move(typeinfo_);
			}
			else if( it_->text == Keywords::fn_ )
			{
				// Parse function type name: fn( i32 x )
				std::unique_ptr<TypeNameInExpression> type_name_in_expression( new TypeNameInExpression( it_->file_pos ) );
				type_name_in_expression->type_name= ParseTypeName();
				current_node= std::move(type_name_in_expression);
			}
			else
			{
				if( auto macro= FetchMacro( it_->text, Macro::Context::Expression ) )
				{
					IExpressionComponentPtr macro_expression= ExpandMacro( *macro, &SyntaxAnalyzer::ParseExpression );
					if( macro_expression == nullptr )
						return nullptr;
					current_node.reset( new BracketExpression( macro_expression->GetFilePos(), std::move(macro_expression ) ) );
				}
				else
					current_node.reset( new NamedOperand( it_->file_pos, ParseComplexName() ) );
			}
		}
		else if( it_->type == Lexem::Type::Scope )
		{
			current_node.reset( new NamedOperand( it_->file_pos, ParseComplexName() ) );
		}
		else if( it_->type == Lexem::Type::Number )
		{
			current_node= ParseNumericConstant();
			NextLexem();
		}
		else if( it_->type == Lexem::Type::String )
		{
			std::unique_ptr<StringLiteral> string_literal( new StringLiteral( it_->file_pos ) );
			string_literal->value_= it_->text;
			NextLexem();

			if( it_->type == Lexem::Type::LiteralSuffix )
			{
				string_literal->type_suffix_= it_->text;
				NextLexem();
			}

			current_node= std::move(string_literal);
		}
		else if( it_->type == Lexem::Type::BracketLeft )
		{
			NextLexem();

			current_node.reset(
				new BracketExpression(
					(it_-1)->file_pos,
					ParseExpression() ) );

			if( it_->type != Lexem::Type::BracketRight )
			{
				PushErrorMessage();
				return nullptr;
			}
			NextLexem();
		}
		else if( it_->type == Lexem::Type::SquareBracketLeft )
		{
			// Parse array type name: [ ElementType, 42 ]
			std::unique_ptr<TypeNameInExpression> type_name_in_expression( new TypeNameInExpression( it_->file_pos ) );
			type_name_in_expression->type_name= ParseTypeName();
			current_node= std::move(type_name_in_expression);
		}
		else U_ASSERT(false);

		current_node->prefix_operators_= std::move( prefix_operators );

		bool is_binary_operator= false;
		// Postfix operators.
		bool end_postfix_operators= false;
		while( !end_postfix_operators )
		{
			switch( it_->type )
			{
			case Lexem::Type::SquareBracketLeft:
				{
					NextLexem();

					current_node->postfix_operators_.emplace_back(
						new IndexationOperator(
							(it_-1)->file_pos,
							ParseExpression() ) );

					if( it_->type != Lexem::Type::SquareBracketRight )
					{
						PushErrorMessage();
						return nullptr;
					}
					NextLexem();
				}
				break;

			case Lexem::Type::BracketLeft:
				{
					const FilePos& call_operator_pos= it_->file_pos;
					NextLexem();

					std::vector<IExpressionComponentPtr> arguments;
					while( NotEndOfFile() )
					{
						if( it_->type == Lexem::Type::BracketRight )
						{
							NextLexem();
							break;
						}

						arguments.emplace_back( ParseExpression() );

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

					current_node->postfix_operators_.emplace_back(
						new CallOperator(
							call_operator_pos,
							std::move( arguments ) ) );

				} break;

			case Lexem::Type::Dot:
				{
					std::unique_ptr<MemberAccessOperator> member_access_operator( new MemberAccessOperator( it_->file_pos ) );
					NextLexem();

					if( it_->type != Lexem::Type::Identifier )
					{
						PushErrorMessage();
						return nullptr;
					}

					member_access_operator->member_name_= it_->text;
					NextLexem();

					if( it_->type == Lexem::Type::TemplateBracketLeft )
					{
						member_access_operator->have_template_parameters= true;
						member_access_operator->template_parameters= ParseTemplateParameters();
					}

					current_node->postfix_operators_.push_back( std::move( member_access_operator ) );
				} break;

			default:
				is_binary_operator= IsBinaryOperator( *it_ );
				end_postfix_operators= true;
				break;
			};

			if( end_postfix_operators )
				break;
		}

		if( root == nullptr )
		{
			root= std::move( current_node );
		}
		else
		{
			BinaryOperator* const root_as_binary_operator= dynamic_cast<BinaryOperator*>( root.get() );
			U_ASSERT( root_as_binary_operator != nullptr );

			// Place to existent tree last component.
			BinaryOperator* most_right_with_null= root_as_binary_operator;
			while( most_right_with_null->right_ != nullptr )
			{
				BinaryOperator* const right_as_binary_operator= dynamic_cast<BinaryOperator*>( most_right_with_null->right_.get() );
				U_ASSERT( right_as_binary_operator != nullptr );
				most_right_with_null= right_as_binary_operator;
			}
			most_right_with_null->right_= std::move( current_node );
		}

		if( is_binary_operator )
		{
			const BinaryOperatorType binary_operator_type= LexemToBinaryOperator( *it_ );
			std::unique_ptr<BinaryOperator> binary_operator( new BinaryOperator( it_->file_pos ) );
			binary_operator->operator_type_= binary_operator_type;
			NextLexem();

			if( BinaryOperator* const root_as_binary_operator= dynamic_cast<BinaryOperator*>( root.get() ) )
			{
				BinaryOperator* node_to_replace_parent= nullptr;
				BinaryOperator* node_to_replace= root_as_binary_operator;
				while( GetBinaryOperatorPriority( binary_operator->operator_type_ ) > GetBinaryOperatorPriority( node_to_replace->operator_type_ ) )
				{
					node_to_replace_parent= node_to_replace;
					BinaryOperator* const right_as_binary_operator= dynamic_cast<BinaryOperator*>( node_to_replace->right_.get() );
					if( right_as_binary_operator == nullptr )
						break;
					node_to_replace= right_as_binary_operator;
				}

				if( node_to_replace_parent != nullptr )
				{
					binary_operator->left_= std::move( node_to_replace_parent->right_ );
					node_to_replace_parent->right_= std::move( binary_operator );
				}
				else
				{
					binary_operator->left_= std::move( root );
					root= std::move( binary_operator );
				}
			}
			else
			{
				binary_operator->left_= std::move( root );
				root= std::move( binary_operator );
			}
		}
		else
			break;
	}

	return root;
}

FunctionArgumentPtr SyntaxAnalyzer::ParseFunctionArgument()
{
	ITypeNamePtr arg_type= ParseTypeName();

	ReferenceModifier reference_modifier= ReferenceModifier::None;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ProgramString reference_tag;
	ReferencesTagsList tags_list;

	if( it_->type == Lexem::Type::And )
	{
		reference_modifier= ReferenceModifier::Reference;
		NextLexem();

		if( it_->type == Lexem::Type::Apostrophe )
		{
			NextLexem();

			if( it_->type == Lexem::Type::Identifier )
			{
				reference_tag = it_->text;
				NextLexem();
			}
			else
			{
				PushErrorMessage();
				TryRecoverAfterError( g_function_arguments_list_control_lexems );
				return nullptr;
			}
		}
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		TryRecoverAfterError( g_function_arguments_list_control_lexems );
		return nullptr;
	}

	if( it_->text == Keywords::mut_ )
	{
		mutability_modifier= MutabilityModifier::Mutable;
		NextLexem();
	}
	else if( it_->text == Keywords::imut_ )
	{
		mutability_modifier= MutabilityModifier::Immutable;
		NextLexem();
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		TryRecoverAfterError( g_function_arguments_list_control_lexems );
		return nullptr;
	}

	const FilePos& arg_file_pos= it_->file_pos;
	const ProgramString& arg_name= it_->text;
	NextLexem();

	if( it_->type == Lexem::Type::Apostrophe )
		tags_list= ParseReferencesTagsList();

	return FunctionArgumentPtr(
		new FunctionArgument(
			arg_file_pos,
			arg_name,
			std::move(arg_type),
			mutability_modifier,
			reference_modifier,
			std::move(reference_tag),
			std::move(tags_list) ) );
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

		result.return_type_= ParseTypeName();

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

std::unique_ptr<FunctionType> SyntaxAnalyzer::ParseFunctionType()
{
	std::unique_ptr<FunctionType> result( new FunctionType( it_->file_pos ) );

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

		auto arg= ParseFunctionArgument();
		if( arg != nullptr )
			result->arguments_.push_back( std::move(arg) );

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

ITypeNamePtr SyntaxAnalyzer::ParseTypeName()
{
	if( it_->type == Lexem::Type::SquareBracketLeft )
	{
		NextLexem();

		std::unique_ptr<ArrayTypeName> array_type_name( new ArrayTypeName(it_->file_pos) );
		array_type_name->element_type= ParseTypeName();

		if( it_->type != Lexem::Type::Comma )
		{
			PushErrorMessage();
			return std::move(array_type_name);
		}
		NextLexem();
		array_type_name->size= ParseExpression();

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

		ITypeNamePtr type_name= ParseTypeName();

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

		std::unique_ptr<TypeofTypeName> typeof_type_name( new TypeofTypeName(it_->file_pos) );

		if( it_->type != Lexem::Type::BracketLeft )
		{
			PushErrorMessage();
			return typeof_type_name;
		}
		NextLexem();

		typeof_type_name->expression= ParseExpression();

		if( it_->type != Lexem::Type::BracketRight )
		{
			PushErrorMessage();
			return typeof_type_name;
		}
		NextLexem();

		return typeof_type_name;
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::fn_ )
		return ParseFunctionType();
	else
	{
		std::unique_ptr<NamedTypeName> named_type_name( new NamedTypeName(it_->file_pos) );
		named_type_name->name= ParseComplexName();
		return std::move(named_type_name);
	}
}

std::vector<IExpressionComponentPtr> SyntaxAnalyzer::ParseTemplateParameters()
{
	U_ASSERT( it_->type == Lexem::Type::TemplateBracketLeft );
	NextLexem();

	std::vector<IExpressionComponentPtr> result;

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

IInitializerPtr SyntaxAnalyzer::ParseInitializer( const bool parse_expression_initializer )
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
		return IInitializerPtr( new ZeroInitializer( prev_it->file_pos ) );
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::uninitialized_ )
	{
		const auto prev_it= it_;
		NextLexem();
		return IInitializerPtr( new UninitializedInitializer( prev_it->file_pos ) );
	}
	else if( parse_expression_initializer )
	{
		// In some cases usage of expression in initializer is forbidden.
		return ParseExpressionInitializer();
	}
	else
	{
		PushErrorMessage();
		return nullptr;
	}
}

std::unique_ptr<ArrayInitializer> SyntaxAnalyzer::ParseArrayInitializer()
{
	U_ASSERT( it_->type == Lexem::Type::SquareBracketLeft );

	std::unique_ptr<ArrayInitializer> result( new ArrayInitializer( it_->file_pos ) );
	NextLexem();

	while( NotEndOfFile() && it_->type != Lexem::Type::SquareBracketRight )
	{
		result->initializers.push_back( ParseInitializer( true ) );
		if( it_->type == Lexem::Type::Comma )
			NextLexem();
		else
			break;
		// TODO - parse continious flag here
	}
	if( it_->type != Lexem::Type::SquareBracketRight )
	{
		PushErrorMessage();
		return result;
	}

	NextLexem();

	return result;
}

std::unique_ptr<StructNamedInitializer> SyntaxAnalyzer::ParseStructNamedInitializer()
{
	U_ASSERT( it_->type == Lexem::Type::BraceLeft );

	std::unique_ptr<StructNamedInitializer> result( new StructNamedInitializer( it_->file_pos ) );
	NextLexem();

	while( NotEndOfFile() && it_->type != Lexem::Type::BraceRight )
	{
		if( it_->type == Lexem::Type::Dot )
			NextLexem();
		else
		{
			PushErrorMessage();
			return result;
		}

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return result;
		}
		ProgramString name= it_->text;
		NextLexem();

		IInitializerPtr initializer;
		if( it_->type == Lexem::Type::Assignment )
		{
			NextLexem();
			if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::zero_init_ )
			{
				initializer.reset( new ZeroInitializer( it_->file_pos ) );
				NextLexem();
			}
			else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::uninitialized_ )
			{
				initializer.reset( new UninitializedInitializer( it_->file_pos ) );
				NextLexem();
			}
			else
			{
				initializer.reset( new ExpressionInitializer( it_->file_pos, ParseExpression() ) );
			}
		}
		else if(
			it_->type == Lexem::Type::BracketLeft ||
			it_->type == Lexem::Type::SquareBracketLeft ||
			it_->type == Lexem::Type::BraceLeft )
		{
			initializer= ParseInitializer( false );
		}

		result->members_initializers.emplace_back();
		result->members_initializers.back().name= std::move(name);
		result->members_initializers.back().initializer= std::move(initializer);

		if( it_->type == Lexem::Type::Comma )
			NextLexem();
	}
	if( it_->type != Lexem::Type::BraceRight )
	{
		PushErrorMessage();
		return result;
	}

	NextLexem();

	return result;
}

std::unique_ptr<ConstructorInitializer> SyntaxAnalyzer::ParseConstructorInitializer()
{
	U_ASSERT( it_->type == Lexem::Type::BracketLeft );

	NextLexem();

	std::vector<IExpressionComponentPtr> args;
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
				return nullptr;
			}
		}
		else
			break;
	}
	if( it_->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage();
		return nullptr;
	}
	NextLexem();

	std::unique_ptr<ConstructorInitializer> result(
		new ConstructorInitializer( it_->file_pos, std::move(args) ) );

	return result;
}

std::unique_ptr<ExpressionInitializer> SyntaxAnalyzer::ParseExpressionInitializer()
{
	std::unique_ptr<ExpressionInitializer> result(
		new ExpressionInitializer(
			it_->file_pos,
			ParseExpression() ) );

	return result;
}

VariablesDeclarationPtr SyntaxAnalyzer::ParseVariablesDeclaration()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ );
	VariablesDeclarationPtr decl( new VariablesDeclaration( it_->file_pos ) );
	NextLexem();

	decl->type= ParseTypeName();

	do
	{
		decl->variables.emplace_back();
		VariablesDeclaration::VariableEntry& variable_entry= decl->variables.back();

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

		if( it_->type == Lexem::Type::Assignment )
		{
			NextLexem();
			if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::zero_init_ )
			{
				variable_entry.initializer.reset( new ZeroInitializer( it_->file_pos ) );
				NextLexem();
			}
			else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::uninitialized_ )
			{
				variable_entry.initializer.reset( new UninitializedInitializer( it_->file_pos ) );
				NextLexem();
			}
			else
			{
				variable_entry.initializer.reset( new ExpressionInitializer( it_->file_pos, ParseExpression() ) );
			}
		}
		else if(
			it_->type == Lexem::Type::BracketLeft ||
			it_->type == Lexem::Type::SquareBracketLeft ||
			it_->type == Lexem::Type::BraceLeft )
		{
			variable_entry.initializer= ParseInitializer( false );
		}

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
			return nullptr;
		}

	} while( NotEndOfFile() );

	return decl;
}

std::unique_ptr<AutoVariableDeclaration> SyntaxAnalyzer::ParseAutoVariableDeclaration()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ );
	std::unique_ptr<AutoVariableDeclaration> result( new AutoVariableDeclaration( it_->file_pos ) );
	NextLexem();

	if (it_->type == Lexem::Type::Identifier && it_->text == Keywords::lock_temps_ )
	{
		NextLexem();
		result->lock_temps= true;
	}

	if( it_->type == Lexem::Type::And )
	{
		result->reference_modifier= ReferenceModifier::Reference;
		NextLexem();
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return result;
	}

	if( it_->text == Keywords::mut_ )
	{
		result->mutability_modifier= MutabilityModifier::Mutable;
		NextLexem();
	}
	else if( it_->text == Keywords::imut_ )
	{
		result->mutability_modifier= MutabilityModifier::Immutable;
		NextLexem();
	}
	else if( it_->text == Keywords::constexpr_ )
	{
		result->mutability_modifier= MutabilityModifier::Constexpr;
		NextLexem();
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return result;
	}

	result->name= it_->text;
	NextLexem();

	if( it_->type != Lexem::Type::Assignment )
	{
		PushErrorMessage();
		return result;
	}
	NextLexem();

	result->initializer_expression = ParseExpression();

	if( it_->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage();
		return result;
	}
	NextLexem();

	return result;
}

IBlockElementPtr SyntaxAnalyzer::ParseReturnOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::return_ );

	const FilePos& op_pos= it_->file_pos;

	NextLexem();

	if( it_->type == Lexem::Type::Semicolon )
	{
		NextLexem();
		return IBlockElementPtr( new ReturnOperator( op_pos, nullptr ) );
	}

	IExpressionComponentPtr expression= ParseExpression();

	if( it_->type != Lexem::Type::Semicolon  )
	{
		PushErrorMessage();
		return nullptr;
	}

	NextLexem();

	return IBlockElementPtr( new ReturnOperator( op_pos, std::move( expression ) ) );
}

IBlockElementPtr SyntaxAnalyzer::ParseWhileOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::while_ );

	const FilePos& op_pos= it_->file_pos;

	NextLexem();
	if( it_->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage();
		return nullptr;
	}

	NextLexem();

	IExpressionComponentPtr condition= ParseExpression();

	if( it_->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage();
		return nullptr;
	}

	NextLexem();

	if( it_->type != Lexem::Type::BraceLeft )
	{
		PushErrorMessage();
		return nullptr;
	}

	BlockPtr block= ParseBlock();

	return
		IBlockElementPtr(
			new WhileOperator(
				op_pos,
				std::move( condition ),
				std::move( block ) ) );
}

IBlockElementPtr SyntaxAnalyzer::ParseBreakOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::break_ );

	const FilePos& op_pos= it_->file_pos;

	NextLexem();

	if( it_->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage();
		return nullptr;
	}

	NextLexem();

	return IBlockElementPtr( new BreakOperator( op_pos ) );
}

IBlockElementPtr SyntaxAnalyzer::ParseContinueOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::continue_ );

	const FilePos& op_pos= it_->file_pos;

	NextLexem();

	if( it_->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage();
		return nullptr;
	}

	NextLexem();

	return IBlockElementPtr( new ContinueOperator( op_pos ) );
}

std::unique_ptr<IfOperator> SyntaxAnalyzer::ParseIfOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::if_  || it_->text == Keywords::static_if_ ) );

	const FilePos& op_pos= it_->file_pos;

	NextLexem();

	if( it_->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage();
		return nullptr;
	}

	NextLexem();

	std::vector<IfOperator::Branch> branches;
	branches.emplace_back();

	branches.back().condition= ParseExpression();

	if( it_->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage();
		return nullptr;
	}

	NextLexem();

	if( it_->type != Lexem::Type::BraceLeft )
	{
		PushErrorMessage();
		return nullptr;
	}
	branches.back().block= ParseBlock();

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::else_ )
		{
			branches.emplace_back();

			NextLexem();

			// Optional if.
			if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ )
			{
				NextLexem();

				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage();
					return nullptr;
				}

				NextLexem();

				branches.back().condition= ParseExpression();

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage();
					return nullptr;
				}

				NextLexem();
			}
			// Block - common for "else" and "else if".
			if( it_->type != Lexem::Type::BraceLeft )
			{
				PushErrorMessage();
				return nullptr;
			}

			branches.back().block= ParseBlock();

			if( branches.back().condition == nullptr ) break;

		}
		else
			break;
	}

	return
		std::unique_ptr<IfOperator>(
			new IfOperator(
				op_pos,
				std::prev( it_ )->file_pos,
				std::move( branches ) ) );
}

IBlockElementPtr SyntaxAnalyzer::ParseStaticIfOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_if_ );

	std::unique_ptr<StaticIfOperator> result( new StaticIfOperator( it_->file_pos ) );
	result->if_operator_= ParseIfOperator();
	return std::move(result);
}

std::unique_ptr<StaticAssert> SyntaxAnalyzer::ParseStaticAssert()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ );

	std::unique_ptr<StaticAssert> result( new StaticAssert( it_->file_pos ) );

	NextLexem();

	if( it_->type == Lexem::Type::BracketLeft )
		NextLexem();
	else
		PushErrorMessage();

	result->expression= ParseExpression();

	if( it_->type == Lexem::Type::BracketRight )
		NextLexem();
	else
		PushErrorMessage();

	if( it_->type == Lexem::Type::Semicolon )
		NextLexem();
	else
	{
		PushErrorMessage();
		return nullptr;
	}

	return result;
}

std::unique_ptr<Enum> SyntaxAnalyzer::ParseEnum()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::enum_ );

	std::unique_ptr<Enum> result( new Enum( it_->file_pos ) );

	NextLexem();

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return nullptr;
	}
	result->name= it_->text;
	NextLexem();

	if( it_->type == Lexem::Type::Colon )
	{
		NextLexem();
		result->underlaying_type_name= ParseComplexName();
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

		result->members.emplace_back();
		result->members.back().file_pos= it_->file_pos;
		result->members.back().name= it_->text;
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

IBlockElementPtr SyntaxAnalyzer::ParseHalt()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::halt_ );

	const FilePos& file_pos= it_->file_pos;
	NextLexem();

	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ )
	{
		NextLexem();
		std::unique_ptr<HaltIf> result( new HaltIf( file_pos ) );

		if( it_->type != Lexem::Type::BracketLeft )
		{
			PushErrorMessage();
			return nullptr;
		}
		NextLexem();

		result->condition= ParseExpression();

		if( it_->type != Lexem::Type::BracketRight )
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

		return std::move(result);
	}
	else if( it_->type == Lexem::Type::Semicolon )
	{
		NextLexem();
		std::unique_ptr<Halt> result( new Halt( file_pos ) );
		return std::move(result);
	}
	else
	{
		PushErrorMessage();
		return nullptr;
	}
}

std::vector<IBlockElementPtr> SyntaxAnalyzer::ParseBlockElements()
{
	BlockElements elements;

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
			if( BlockPtr block= ParseBlock() )
			{
				block->safety_= Block::Safety::Safe;
				elements.emplace_back( std::move( block ) );
			}
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::unsafe_ )
		{
			NextLexem();
			if( BlockPtr block= ParseBlock() )
			{
				block->safety_= Block::Safety::Unsafe;
				elements.emplace_back( std::move( block ) );
			}
		}
		else if( it_->type == Lexem::Type::Increment )
		{
			std::unique_ptr<IncrementOperator> op( new IncrementOperator( it_->file_pos ) );
			NextLexem();

			op->expression= ParseExpression();
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
			std::unique_ptr<DecrementOperator> op( new DecrementOperator( it_->file_pos ) );
			NextLexem();

			op->expression= ParseExpression();
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
					BlockElements macro_elements= ExpandMacro( *macro, &SyntaxAnalyzer::ParseBlockElements );
					for( auto& element : macro_elements )
						elements.push_back( std::move(element) );
					continue;
				}
			}

			IExpressionComponentPtr l_expression= ParseExpression();

			if( it_->type == Lexem::Type::Assignment )
			{
				NextLexem();
				IExpressionComponentPtr r_expression= ParseExpression();

				if( it_->type != Lexem::Type::Semicolon )
				{
					PushErrorMessage();
					TryRecoverAfterError( g_block_body_elements_control_lexems );
					continue;
				}
				NextLexem();

				elements.emplace_back(
					new AssignmentOperator(
						(it_-2)->file_pos,
						std::move( l_expression ),
						std::move( r_expression ) ) );
			}
			else if( IsAdditiveAssignmentOperator( *it_ ) )
			{
				std::unique_ptr<AdditiveAssignmentOperator> op( new AdditiveAssignmentOperator( it_->file_pos ) );

				op->additive_operation_= GetAdditiveAssignmentOperator( *it_ );
				NextLexem();

				op->l_value_= std::move(l_expression);
				op->r_value_= ParseExpression();

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
				NextLexem();

				elements.emplace_back(
					new SingleExpressionOperator(
						(it_-1)->file_pos,
						std::move( l_expression ) ) );
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

BlockPtr SyntaxAnalyzer::ParseBlock()
{
	U_ASSERT( it_->type == Lexem::Type::BraceLeft );

	const FilePos& block_pos= it_->file_pos;
	FilePos block_end_file_pos= block_pos;

	NextLexem();

	BlockElements elements= ParseBlockElements();

	if( it_->type == Lexem::Type::BraceRight )
	{
		block_end_file_pos= it_->file_pos;
		NextLexem();
	}
	else
	{
		PushErrorMessage();
		return nullptr;
	}

	return BlockPtr(
		new Block(
			block_pos,
			block_end_file_pos,
			std::move( elements ) ) );
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

std::unique_ptr<Typedef> SyntaxAnalyzer::ParseTypedef()
{
	U_ASSERT( it_->text == Keywords::type_ );

	NextLexem();

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return nullptr;
	}

	const ProgramString& name= it_->text;
	NextLexem();

	std::unique_ptr<Typedef> result= ParseTypedefBody();
	if( result != nullptr )
		result->name= name;
	return result;
}

std::unique_ptr<Typedef> SyntaxAnalyzer::ParseTypedefBody()
{
	// Parse something like "- i32;"

	std::unique_ptr<Typedef> result( new Typedef( it_->file_pos ) );

	if( it_->type != Lexem::Type::Assignment )
	{
		PushErrorMessage();
		return result;
	}
	NextLexem();

	result->value= ParseTypeName();

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

	std::unique_ptr<Function> result( new Function( it_->file_pos ) );

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

	std::vector<FunctionArgumentPtr>& arguments= result->type_.arguments_;

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

			arguments.emplace_back(
				new FunctionArgument(
					file_pos,
					Keyword( Keywords::this_ ),
					ITypeNamePtr(),
					mutability_modifier,
					ReferenceModifier::Reference,
					Keyword( Keywords::this_ ), // Implicit set name for tag of "this" to "this".
					tags_list ) );
		}
	}

	while( NotEndOfFile() && it_->type != Lexem::Type::EndOfFile )
	{
		if( it_->type == Lexem::Type::BracketRight )
		{
			NextLexem();
			break;
		}

		auto arg= ParseFunctionArgument();
		if( arg != nullptr )
			arguments.push_back( std::move(arg) );

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
			result->constructor_initialization_list_.reset( new StructNamedInitializer( it_->file_pos ) );
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
				IInitializerPtr& initializer= result->constructor_initialization_list_->members_initializers.back().initializer;

				NextLexem();
				if( it_->type == Lexem::Type::Assignment )
				{
					NextLexem();
					if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::zero_init_ )
					{
						initializer.reset( new ZeroInitializer( it_->file_pos ) );
						NextLexem();
					}
					else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::uninitialized_ )
					{
						initializer.reset( new UninitializedInitializer( it_->file_pos ) );
						NextLexem();
					}
					else
						initializer.reset( new ExpressionInitializer( it_->file_pos, ParseExpression() ) );
				}
				else
					initializer= ParseInitializer( false );

				if( it_->type == Lexem::Type::Comma )
					NextLexem();
			}
		}

		if( it_->type == Lexem::Type::BraceLeft )
			result->block_= ParseBlock();
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

	std::unique_ptr<Class> result= ParseClassBody();
	if( result != nullptr )
	{
		result->file_pos_= class_file_pos;
		result->name_= std::move(name);
		result->kind_attribute_= class_kind_attribute;
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
			if( IClassElementPtr class_element= ParseVariablesDeclaration() )
				result.emplace_back( std::move(class_element) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ )
		{
			if( IClassElementPtr class_element= ParseAutoVariableDeclaration() )
				result.emplace_back( std::move(class_element) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ )
		{
			if( IClassElementPtr class_element= ParseStaticAssert() )
				result.emplace_back( std::move(class_element) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::enum_ )
		{
			if( IClassElementPtr class_element= ParseEnum() )
				result.emplace_back( std::move(class_element) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::type_ )
		{
			if( IClassElementPtr class_element= ParseTypedef() )
				result.emplace_back( std::move(class_element) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ )
		{
			if( TemplateBasePtr template_= ParseTemplate() )
			{
				if( IClassElement* const class_element= dynamic_cast<IClassElement*>(template_.get()) )
				{
					template_.release();
					result.emplace_back( class_element );
				}
				else
				{
					// TODO - push more relevant message.
					PushErrorMessage();
				}
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

			result.emplace_back( new ClassVisibilityLabel( it_->file_pos, visibility ) );

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

			std::unique_ptr<ClassField> field( new ClassField( it_->file_pos ) );

			field->type= ParseTypeName();

			bool is_reference= false;
			if( it_->type == Lexem::Type::And )
			{
				is_reference= true;
				NextLexem();
				field->reference_modifier= ReferenceModifier::Reference;
			}

			if( it_->type == Lexem::Type::Identifier )
			{
				if( it_->text == Keywords::mut_ )
				{
					NextLexem();
					field->mutability_modifier= MutabilityModifier::Mutable;
				}
				if( it_->text == Keywords::imut_ )
				{
					NextLexem();
					field->mutability_modifier= MutabilityModifier::Immutable;
				}
				if( is_reference && it_->text == Keywords::constexpr_ ) // Allow "constexpr" modifier only for reference fields.
				{
					NextLexem();
					field->mutability_modifier= MutabilityModifier::Constexpr;
				}
			}

			if( it_->type == Lexem::Type::Identifier )
			{
				field->name= it_->text;
				NextLexem();
			}
			else
			{
				PushErrorMessage();
				TryRecoverAfterError( g_class_body_elements_control_lexems );
				continue;
			}

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
	std::unique_ptr<Class> result( new Class( it_->file_pos ) );

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

TemplateBasePtr SyntaxAnalyzer::ParseTemplate()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ );
	const FilePos& template_file_pos= it_->file_pos;
	NextLexem();

	// TemplateBase parameters
	std::vector<TemplateBase::Arg> args;

	if( it_->type != Lexem::Type::TemplateBracketLeft )
	{
		PushErrorMessage();
		return nullptr;
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
			std::unique_ptr<NamedOperand> arg_type( new NamedOperand( it_->file_pos, ParseComplexName() ) );
			args.back().arg_type= &arg_type->name_;
			args.back().arg_type_expr= std::move(arg_type);
		}

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return nullptr;
		}

		ComplexName name;
		name.components.emplace_back();
		name.components.back().name= it_->text;
		std::unique_ptr<NamedOperand> name_ptr( new NamedOperand( it_->file_pos, std::move(name) ) );
		args.back().name= &name_ptr->name_;
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
		return nullptr;
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
			return nullptr;
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
			return nullptr;
		}
		name= it_->text;
		NextLexem();
		template_kind= TemplateKind::Typedef;
	}
	else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::fn_ || it_->text == Keywords::op_ ) )
	{
		std::unique_ptr<FunctionTemplate> function_template( new FunctionTemplate( template_file_pos ) );
		function_template->args_= std::move(args);
		function_template->function_= ParseFunction();
		return std::move(function_template);
	}
	else
	{
		PushErrorMessage();
		return nullptr;
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
			std::unique_ptr<ClassTemplate> class_template( new ClassTemplate( template_file_pos ) );
			class_template->args_= std::move(args);
			class_template->signature_args_= std::move(signature_args);
			class_template->name_= name;
			class_template->is_short_form_= is_short_form;

			ClassKindAttribute class_kind_attribute= ClassKindAttribute::Struct;
			std::vector<ComplexName> class_parents_list;
			if( template_kind == TemplateKind::Class )
			{
				class_kind_attribute= TryParseClassKindAttribute();
				class_parents_list= TryParseClassParentsList();
			}
			class_template->class_= ParseClassBody();
			if( class_template->class_ != nullptr )
			{
				class_template->class_->file_pos_= template_thing_file_pos;
				class_template->class_->name_= std::move(name);
				class_template->class_->kind_attribute_= class_kind_attribute;
				class_template->class_->parents_= std::move(class_parents_list);
			}
			return std::move(class_template);
		}

	case TemplateKind::Typedef:
		{
			std::unique_ptr<TypedefTemplate> typedef_template( new TypedefTemplate( template_file_pos ) );
			typedef_template->args_= std::move(args);
			typedef_template->signature_args_= std::move(signature_args);
			typedef_template->name_= name;
			typedef_template->is_short_form_= is_short_form;

			typedef_template->typedef_= ParseTypedefBody();
			if( typedef_template->typedef_ != nullptr )
			{
				typedef_template->typedef_->name= std::move(name);
			}
			return std::move(typedef_template);
		}

	case TemplateKind::Invalid:
		break;
	};

	U_ASSERT(false);
	return nullptr;
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

	Lexems result_lexems= DoExpandMacro( names_map, macro.result_template_elements );

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
		msg.text= "Unexpected lexem - \""_SpC + it_->text + "\". (In expansion of macro \""_SpC + macro_name + "\")"_SpC;
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
				if( ParseTypeName() == nullptr )
				{
					push_macro_error();
					return false;
				}
				element.end= it_;
				element.kind= match_element.kind;
				out_elements[match_element.name]= std::move(element);
			}
			break;

		case Macro::MatchElementKind::Expression:
			{
				ParsedMacroElement element;
				element.begin= it_;
				if( ParseExpression() == nullptr )
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
				if( ParseBlock() == nullptr )
				{
					push_macro_error();
					return false;
				}
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
	const std::vector<Macro::ResultElement>& result_elements )
{
	Lexems result_lexems;
	for( const Macro::ResultElement& result_element : result_elements )
	{
		switch( result_element.kind )
		{
		case Macro::ResultElementKind::Lexem:
			result_lexems.push_back( result_element.lexem );
			break;

		case Macro::ResultElementKind::VariableElement:
			{
				const auto element= parsed_elements.GetElement( result_element.name );
				if( element == nullptr )
				{
					SyntaxErrorMessage msg;
					msg.file_pos= result_element.lexem.file_pos;
					msg.text= result_element.name + " not found"_SpC;
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
					msg.text= result_element.name + " not found"_SpC;
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

							Lexems element_lexems= DoExpandMacro( sub_elements_map, result_element.sub_elements );
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
						msg.text= "Expected optional or loop."_SpC;
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
		error_message.text= "Syntax error - unexpected lexem \""_SpC + it_->text + "\""_SpC;
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
