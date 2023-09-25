#include <cctype>
#include <cstring>

#include "../../lex_synt_lib_common/assert.hpp"
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
	std::string text;
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

// See https://en.cppreference.com/w/cpp/language/operator_precedence.
// Use C++ priorities.
static const std::vector< std::pair< Lexem::Type, BinaryOperatorType> > g_operators_by_priority_table[]
{
	{
		{ Lexem::Type::Disjunction, BinaryOperatorType::LazyLogicalOr },
	},
	{
		{ Lexem::Type::Conjunction, BinaryOperatorType::LazyLogicalAnd },
	},
	{
		{ Lexem::Type::Or, BinaryOperatorType::Or },
	},
	{
		{ Lexem::Type::Xor, BinaryOperatorType::Xor },
	},
	{
		{ Lexem::Type::And, BinaryOperatorType::And },
	},
	{
		{ Lexem::Type::CompareEqual, BinaryOperatorType::Equal },
		{ Lexem::Type::CompareNotEqual, BinaryOperatorType::NotEqual },
	},
	{
		{ Lexem::Type::CompareLess, BinaryOperatorType::Less },
		{ Lexem::Type::CompareLessOrEqual, BinaryOperatorType::LessEqual },
		{ Lexem::Type::CompareGreater, BinaryOperatorType::Greater },
		{ Lexem::Type::CompareGreaterOrEqual, BinaryOperatorType::GreaterEqual },
	},
	{
		{ Lexem::Type::CompareOrder, BinaryOperatorType::CompareOrder },
	},
	{
		{ Lexem::Type::ShiftLeft, BinaryOperatorType::ShiftLeft },
		{ Lexem::Type::ShiftRight, BinaryOperatorType::ShiftRight },
	},
	{
		{ Lexem::Type::Plus, BinaryOperatorType::Add },
		{ Lexem::Type::Minus, BinaryOperatorType::Sub },
	},
	{
		{ Lexem::Type::Star, BinaryOperatorType::Mul },
		{ Lexem::Type::Slash, BinaryOperatorType::Div },
		{ Lexem::Type::Percent, BinaryOperatorType::Rem },
	},
};

std::optional<BinaryOperatorType> GetAdditiveAssignmentOperator( const Lexem& lexem )
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
			return std::nullopt;
	};
}

// By declaring this class and all its methods locally( using anonymous namespace ) we allow complier to optimize methods of this class aggressively (using inlining where it is possible).
class SyntaxAnalyzer final
{
public:
	SyntaxAnalyzer();
	SyntaxAnalyzer( const MacrosPtr& macros, const MacroExpansionContextsPtr& macro_expansion_contexts );

	SyntaxAnalysisResult DoAnalyzis( const Lexems& lexems, std::string macro_unique_identifiers_base_name );
	std::vector<Import> ParseImportsOnly( const Lexems& lexems );

private:
	struct ParsedMacroElement;
	using MacroVariablesMap= std::unordered_map<std::string, ParsedMacroElement>;

	struct ParsedMacroElement
	{
		Lexems::const_iterator begin;
		Lexems::const_iterator end;
		std::vector< MacroVariablesMap > sub_elements;
		Macro::MatchElementKind kind= Macro::MatchElementKind::Lexem;
	};

	struct MacroNamesMap
	{
		const MacroNamesMap* prev= nullptr;
		const MacroVariablesMap* names= nullptr;

		const ParsedMacroElement* GetElement( const std::string& name ) const
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
	Macro::MatchElements ParseMacroMatchBlock();
	Macro::ResultElements ParseMacroResultBlock();

	ProgramElementsList ParseNamespaceBody() { return ParseNamespaceBody( Lexem::Type::BraceRight ); }
	ProgramElementsList ParseNamespaceBody( Lexem::Type end_lexem );

	NumericConstant ParseNumericConstant();

	Expression ParseExpression();
	template<size_t priority> Expression TryParseBinaryOperator();

	Expression ParseExpressionInBrackets();
	Expression ParseBinaryOperatorComponent();
	Expression TryParseBinaryOperatorComponentPostfixOperator( Expression expr );
	Expression ParseBinaryOperatorComponentCore();

	TypeName ParseTypeNameInTemplateBrackets();

	FunctionParam ParseFunctionParam();
	void ParseFunctionTypeEnding( FunctionType& result );
	FunctionType ParseFunctionType();

	TypeName ParseTypeName();
	std::vector<Expression> ParseTemplateParameters();
	ComplexName ParseComplexName();
	ComplexName ParseComplexNameTail( ComplexName base );
	ComplexName TryParseComplexNameTailWithTemplateArgs( ComplexName base );
	std::string ParseInnerReferenceTag();
	FunctionReferencesPollutionList ParseFunctionReferencesPollutionList();

	struct SignatureHelpTag{};
	std::variant<std::vector<Expression>, SignatureHelpTag> ParseCall();

	Initializer ParseInitializer( bool parse_expression_initializer );
	Initializer ParseVariableInitializer();
	Initializer ParseArrayInitializer();
	Initializer ParseStructNamedInitializer();
	Initializer ParseConstructorInitializer();

	VariablesDeclaration ParseVariablesDeclaration();
	AutoVariableDeclaration ParseAutoVariableDeclaration();

	ReturnOperator ParseReturnOperator();
	YieldOperator ParseYieldOperator();
	std::optional<Label> TryParseLabel();
	WhileOperator ParseWhileOperator();
	LoopOperator ParseLoopOperator();
	std::variant<RangeForOperator, CStyleForOperator> ParseForOperator();
	RangeForOperator ParseRangeForOperator();
	CStyleForOperator ParseCStyleForOperator();
	BreakOperator ParseBreakOperator();
	ContinueOperator ParseContinueOperator();
	WithOperator ParseWithOperator();
	IfOperator ParseIfOperator();
	StaticIfOperator ParseStaticIfOperator();
	IfCoroAdvanceOperator ParseIfCoroAdvanceOperator();
	SwitchOperator ParseSwitchOperator();
	StaticAssert ParseStaticAssert();
	Enum ParseEnum();
	std::variant<Halt, HaltIf> ParseHalt();

	BlockElementsList ParseBlockElements();
	Block ParseBlock();

	IfAlternativePtr TryParseIfAlternative();
	IfAlternativePtr ParseIfAlternative();

	ClassKindAttribute TryParseClassKindAttribute();
	std::vector<ComplexName> TryParseClassParentsList();
	NonSyncTag TryParseNonSyncTag();
	bool TryParseClassFieldsOrdered();

	TypeAlias ParseTypeAlias();
	TypeAlias ParseTypeAliasBody();
	Function ParseFunction();
	Class ParseClass();
	ClassElementsList ParseClassBodyElements();
	Class ParseClassBody();

	using TemplateVar=
		std::variant<
			EmptyVariant,
			TypeTemplate,
			FunctionTemplate >;
	TemplateVar ParseTemplate();

	const Macro* FetchMacro( const std::string& macro_name, const Macro::Context context );

	template<typename ParseFnResult>
	ParseFnResult ExpandMacro( const Macro& macro, ParseFnResult (SyntaxAnalyzer::*parse_fn)() );

	std::optional<MacroVariablesMap> MatchMacroBlock(
		const Macro::MatchElements& match_elements,
		const std::string& macro_name );

	Lexems DoExpandMacro(
		const MacroNamesMap& parsed_elements,
		const Macro::ResultElements& result_elements,
		ProgramStringMap<std::string>& unique_macro_identifier_map,
		const std::string& macro_unique_identifiers_base_name );

	void ExpectSemicolon();
	void ExpectLexem( Lexem::Type lexem_type );
	void NextLexem();
	bool NotEndOfFile();

	void TryRecoverAfterError( const std::vector<ExpectedLexem>& expected_lexems );
	void TryRecoverAfterError( const std::vector<ExpectedLexem>& expected_lexems0, const std::vector<ExpectedLexem>& expected_lexems1 );
	void TryRecoverAfterError( const std::vector<ExpectedLexem>& expected_lexems0, const std::vector<ExpectedLexem>& expected_lexems1, const std::vector<ExpectedLexem>& expected_lexems2 );
	void TrySkipBrackets( Lexem::Type bracket_type );
	void PushErrorMessage();

private:
	LexSyntErrors error_messages_;
	Lexems::const_iterator it_;
	Lexems::const_iterator it_end_;
	std::string macro_unique_identifiers_base_name_;

	Lexems::const_iterator last_error_it_;
	size_t last_error_repeats_= 0;

	const MacrosPtr macros_;
	const MacroExpansionContextsPtr macro_expansion_contexts_;
};

SyntaxAnalyzer::SyntaxAnalyzer()
	: macros_(std::make_shared<MacrosByContextMap>())
	, macro_expansion_contexts_(std::make_shared<MacroExpansionContexts>())
{}

SyntaxAnalyzer::SyntaxAnalyzer( const MacrosPtr& macros, const MacroExpansionContextsPtr& macro_expansion_contexts )
	: macros_(macros), macro_expansion_contexts_(macro_expansion_contexts)
{}

SyntaxAnalysisResult SyntaxAnalyzer::DoAnalyzis(
	const Lexems& lexems,
	std::string macro_unique_identifiers_base_name )
{
	SyntaxAnalysisResult result;

	it_= lexems.begin();
	it_end_= lexems.end();
	last_error_it_= lexems.end();
	last_error_repeats_= 0u;
	macro_unique_identifiers_base_name_= std::move(macro_unique_identifiers_base_name);

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
			imports.emplace_back( it_->src_loc );
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
	ExpectLexem( Lexem::Type::MacroBracketLeft );

	// MacroName::context
	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return;
	}
	macro.name= it_->text;
	macro.src_loc= it_->src_loc;
	NextLexem();

	if( IsKeyword( macro.name ) )
	{
		LexSyntError msg;
		msg.src_loc= macro.src_loc;
		msg.text= "Using keyword as macro name";
		error_messages_.push_back(std::move(msg));
	}

	ExpectLexem( Lexem::Type::Colon );

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return;
	}
	const std::string& context_str= it_->text;
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
		LexSyntError msg;
		msg.src_loc= macro.src_loc;
		msg.text= "\"" + context_str + "\" unknown macro context";
		error_messages_.push_back(std::move(msg));
	}

	macro.match_template_elements= ParseMacroMatchBlock();

	ExpectLexem( Lexem::Type::MacroBracketRight );
	ExpectLexem( Lexem::Type::RightArrow );

	// Result body.
	ExpectLexem( Lexem::Type::MacroBracketLeft );

	macro.result_template_elements= ParseMacroResultBlock();

	ExpectLexem( Lexem::Type::MacroBracketRight );

	MacroMap& macro_map= (*macros_)[macro_context];
	if( macro_map.find( macro.name ) != macro_map.end() )
	{
		LexSyntError msg;
		msg.src_loc= macro.src_loc;
		msg.text= "\"" + macro.name + "\" macro redefinition.";
		error_messages_.push_back(std::move(msg));
	}
	else
		macro_map[macro.name]= std::move(macro);
}

Macro::MatchElements SyntaxAnalyzer::ParseMacroMatchBlock()
{
	Macro::MatchElements result;
	std::unordered_set<std::string> elements_set;

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::MacroBracketRight )
			break;
		else if( it_->type == Lexem::Type::MacroBracketLeft || it_->type == Lexem::Type::MacroUniqueIdentifier )
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
				LexSyntError msg;
				msg.src_loc= it_->src_loc;
				msg.text= "\"" + element.name + "\" macro parameter redefinition.";
				error_messages_.push_back(std::move(msg));
			}
			if( IsKeyword( element.name ) )
			{
				LexSyntError msg;
				msg.src_loc= it_->src_loc;
				msg.text= "Using keyword as macro element name";
				error_messages_.push_back(std::move(msg));
			}
			NextLexem();

			ExpectLexem( Lexem::Type::Colon );

			if( it_->type != Lexem::Type::Identifier )
			{
				PushErrorMessage();
				return result;
			}
			const std::string& element_type_str= it_->text;
			NextLexem();

			if( element_type_str == "ident" )
				element.kind= Macro::MatchElementKind::Identifier;
			else if( element_type_str == "ty" )
				element.kind= Macro::MatchElementKind::Typename;
			else if( element_type_str == "expr" )
				element.kind= Macro::MatchElementKind::Expression;
			else if( element_type_str == "block" )
				element.kind= Macro::MatchElementKind::Block;
			else if( element_type_str == "if_alternative" )
				element.kind= Macro::MatchElementKind::IfAlternative;
			else if( element_type_str == "opt" )
				element.kind= Macro::MatchElementKind::Optional;
			else if( element_type_str == "rep" )
				element.kind= Macro::MatchElementKind::Repeated;
			else
			{
				LexSyntError msg;
				msg.src_loc= it_->src_loc;
				msg.text= "\"" + element_type_str + "\" unknown macro variable type";
				error_messages_.push_back(std::move(msg));
			}

			if( element.kind == Macro::MatchElementKind::Optional || element.kind == Macro::MatchElementKind::Repeated )
			{
				ExpectLexem( Lexem::Type::MacroBracketLeft );
				element.sub_elements= ParseMacroMatchBlock();
				ExpectLexem( Lexem::Type::MacroBracketRight );
			}
			// Loop separator.
			if( element.kind == Macro::MatchElementKind::Repeated && it_->type == Lexem::Type::MacroBracketLeft )
			{
				NextLexem();
				if( it_->type == Lexem::Type::MacroIdentifier ||
					it_->type == Lexem::Type::MacroUniqueIdentifier ||
					it_->type == Lexem::Type::MacroBracketLeft ||
					it_->type == Lexem::Type::MacroBracketRight )
				{
					PushErrorMessage();
					return result;
				}

				element.lexem= *it_;
				NextLexem();

				ExpectLexem( Lexem::Type::MacroBracketRight );
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
		Macro::MatchElement& element= result[i];
		const Macro::MatchElement* const next_element= (i + 1u < result.size()) ? &result[i+1u] : nullptr;

		if( element.kind == Macro::MatchElementKind::Optional || element.kind == Macro::MatchElementKind::Repeated )
		{
			if( !element.sub_elements.empty() && element.sub_elements.front().kind == Macro::MatchElementKind::Lexem )
				element.block_check_lexem_kind= Macro::BlockCheckLexemKind::LexemAtBlockStart;
			else if( next_element != nullptr && next_element->kind == Macro::MatchElementKind::Lexem )
				element.block_check_lexem_kind= Macro::BlockCheckLexemKind::LexemAfterBlockEnd;
			else
			{
				LexSyntError msg;
				msg.src_loc= it_->src_loc;
				msg.text= "Expected lexem at start or after \"" + element.name + "\" element.";
				error_messages_.push_back(std::move(msg));
			}

			if( next_element != nullptr && next_element->kind == Macro::MatchElementKind::Lexem )
			{
				if( element.kind == Macro::MatchElementKind::Optional &&
					!element.sub_elements.empty() && element.sub_elements.front().kind == Macro::MatchElementKind::Lexem &&
					element.sub_elements.front().lexem.type == next_element->lexem.type && element.sub_elements.front().lexem.text == result[i+1u].lexem.text )
				{
					LexSyntError msg;
					msg.src_loc= it_->src_loc;
					msg.text= "Start lexem of optional macro block must be different from first lexem after optional block.";
					error_messages_.push_back(std::move(msg));
				}

				if( element.kind == Macro::MatchElementKind::Repeated &&
					element.lexem.type != Lexem::Type::EndOfFile &&
					element.lexem.type == next_element->lexem.type && element.lexem.text == next_element->lexem.text )
				{
					LexSyntError msg;
					msg.src_loc= it_->src_loc;
					msg.text= "Separator lexem of repeated macro block must be different from first lexem after repeated block.";
					error_messages_.push_back(std::move(msg));
				}

				if( element.kind == Macro::MatchElementKind::Repeated &&
					element.lexem.type == Lexem::Type::EndOfFile &&
					!element.sub_elements.empty() && element.sub_elements.front().kind == Macro::MatchElementKind::Lexem &&
					element.sub_elements.front().lexem.type == next_element->lexem.type && element.sub_elements.front().lexem.text == next_element->lexem.text )
				{
					LexSyntError msg;
					msg.src_loc= it_->src_loc;
					msg.text= "Start lexem of repeated macro block without separator must be different from first lexem after repeated block.";
					error_messages_.push_back(std::move(msg));
				}
			}
		}
	}

	return result;
}

Macro::ResultElements SyntaxAnalyzer::ParseMacroResultBlock()
{
	Macro::ResultElements result;

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

				ExpectLexem( Lexem::Type::MacroBracketRight );

				if( it_->type == Lexem::Type::MacroBracketLeft )
				{
					NextLexem();
					if( it_->type == Lexem::Type::MacroIdentifier ||
						it_->type == Lexem::Type::MacroUniqueIdentifier ||
						it_->type == Lexem::Type::MacroBracketLeft ||
						it_->type == Lexem::Type::MacroBracketRight )
					{
						PushErrorMessage();
						return result;
					}

					element.lexem= *it_;
					NextLexem();

					ExpectLexem(Lexem::Type::MacroBracketRight );
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

ProgramElementsList SyntaxAnalyzer::ParseNamespaceBody( const Lexem::Type end_lexem )
{
	ProgramElementsList::Builder result_builder;

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::fn_ || it_->text == Keywords::op_ ) )
		{
			result_builder.Append( ParseFunction() );
		}
		else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::struct_ || it_->text == Keywords::class_ ) )
		{
			result_builder.Append( ParseClass() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ )
		{
			TemplateVar template_= ParseTemplate();
			if( auto* const type_template= std::get_if<TypeTemplate>(&template_) )
				result_builder.Append( std::move( *type_template ) );
			else if( auto* const function_template= std::get_if<FunctionTemplate>(&template_) )
				result_builder.Append( std::move( *function_template ) );
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
			result_builder.Append( ParseVariablesDeclaration() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ )
		{
			result_builder.Append( ParseAutoVariableDeclaration() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ )
		{
			result_builder.Append( ParseStaticAssert() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::enum_ )
		{
			result_builder.Append( ParseEnum() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::type_ )
		{
			result_builder.Append(ParseTypeAlias() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::namespace_ )
		{
			Namespace namespace_( it_->src_loc );
			NextLexem();

			std::string name;
			if( it_->type == Lexem::Type::Identifier )
			{
				name= it_->text;
				namespace_.src_loc= it_->src_loc;
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

			namespace_.name= std::move(name);
			namespace_.elements= ParseNamespaceBody( Lexem::Type::BraceRight );

			ExpectLexem( Lexem::Type::BraceRight );

			result_builder.Append( std::move( namespace_ ) );
		}
		else if( it_->type == end_lexem )
		{
			// End of namespace
			break;
		}
		else
		{
			if( it_->type == Lexem::Type::Identifier )
			{
				if( const Macro* const macro= FetchMacro( it_->text, Macro::Context::Namespace ) )
				{
					result_builder.AppendList( ExpandMacro( *macro, &SyntaxAnalyzer::ParseNamespaceBody ) );
					continue;
				}
			}

			PushErrorMessage();
			TryRecoverAfterError( g_namespace_body_elements_start_lexems );
		}
	} // while not end of file

	return result_builder.Build();
}

NumericConstant SyntaxAnalyzer::ParseNumericConstant()
{
	U_ASSERT( it_->type == Lexem::Type::Number );
	U_ASSERT( it_->text.size() == sizeof(NumberLexemData) );
	
	NumericConstant result( it_->src_loc );
	
	std::memcpy( static_cast<NumberLexemData*>(&result), it_->text.data(), sizeof(NumberLexemData) );
	
	NextLexem();
	return result;
}

Expression SyntaxAnalyzer::ParseExpression()
{
	return TryParseBinaryOperator<0>();
}

template<size_t priority> Expression SyntaxAnalyzer::TryParseBinaryOperator()
{
	// Parse chain of binary operators with same priority and combine chain components together (last is root).
	// Use binary operators with greater priority as chain components.

	constexpr size_t max_priority= std::size(g_operators_by_priority_table);
	constexpr size_t next_priority= priority >= max_priority ? priority : priority + 1;

	if( priority >= max_priority )
		return ParseBinaryOperatorComponent();

	Expression expr= TryParseBinaryOperator<next_priority>();
	while( NotEndOfFile() )
	{
		bool binary_op_parsed= false;
		for( const auto& op_pair : g_operators_by_priority_table[priority] )
		{
			if( it_->type == op_pair.first )
			{
				auto binary_operator= std::make_unique<BinaryOperator>( it_->src_loc );
				NextLexem();

				binary_operator->left= std::move(expr);
				binary_operator->operator_type= op_pair.second;
				binary_operator->right= TryParseBinaryOperator<next_priority>();

				expr= std::move(binary_operator);
				binary_op_parsed= true;
				break;
			}
		}

		if( !binary_op_parsed )
			break;
	}

	return expr;
}

Expression SyntaxAnalyzer::ParseExpressionInBrackets()
{
	ExpectLexem( Lexem::Type::BracketLeft );
	Expression expr= ParseExpression();
	ExpectLexem( Lexem::Type::BracketRight );
	return expr;
}

Expression SyntaxAnalyzer::ParseBinaryOperatorComponent()
{
	return TryParseBinaryOperatorComponentPostfixOperator(ParseBinaryOperatorComponentCore());
}

Expression SyntaxAnalyzer::TryParseBinaryOperatorComponentPostfixOperator( Expression expr )
{
	switch( it_->type )
	{
	case Lexem::Type::SquareBracketLeft:
		{
			auto indexation_opearator= std::make_unique<IndexationOperator>( it_->src_loc );
			NextLexem();

			indexation_opearator->expression= std::move(expr);
			indexation_opearator->index= ParseExpression();

			ExpectLexem( Lexem::Type::SquareBracketRight );

			return TryParseBinaryOperatorComponentPostfixOperator(std::move(indexation_opearator));
		}

	case Lexem::Type::BracketLeft:
		{
			auto call_operator= std::make_unique<CallOperator>( it_->src_loc );

			call_operator->expression= std::move(expr);

			auto call_result= ParseCall();
			if( const auto args= std::get_if< std::vector<Expression> >( &call_result ) )
				call_operator->arguments= std::move(*args);
			else if( std::get_if< SignatureHelpTag >( &call_result ) != nullptr )
			{
				auto signature_help_result= std::make_unique<CallOperatorSignatureHelp>( it_->src_loc );
				NextLexem();
				signature_help_result->expression= std::move(call_operator->expression);
				return std::move(signature_help_result);
			}
			else U_ASSERT(false);

			return TryParseBinaryOperatorComponentPostfixOperator(std::move(call_operator));
		}

	case Lexem::Type::SignatureHelpBracketLeft:
		{
			auto call_operator_signature_help= std::make_unique<CallOperatorSignatureHelp>( it_->src_loc );
			NextLexem();

			call_operator_signature_help->expression= std::move(expr);

			return TryParseBinaryOperatorComponentPostfixOperator(std::move(call_operator_signature_help));
		}

	case Lexem::Type::Dot:
		{
			NextLexem();

			if( it_->type == Lexem::Type::Identifier )
			{
				MemberAccessOperator member_access_operator( it_->src_loc );
				member_access_operator.member_name= it_->text;
				NextLexem();

				member_access_operator.expression= std::move(expr);

				if( it_->type == Lexem::Type::TemplateBracketLeft )
					member_access_operator.template_parameters= ParseTemplateParameters();

				return TryParseBinaryOperatorComponentPostfixOperator( std::make_unique<MemberAccessOperator>( std::move(member_access_operator) ) );
			}
			else if( it_->type == Lexem::Type::CompletionIdentifier )
			{
				MemberAccessOperatorCompletion member_access_operator_completion( it_->src_loc );
				member_access_operator_completion.member_name= it_->text;
				NextLexem();

				member_access_operator_completion.expression= std::move(expr);

				return std::make_unique<MemberAccessOperatorCompletion>( std::move(member_access_operator_completion) );
			}
			else
			{
				PushErrorMessage();
				return EmptyVariant();
			}
		}

	case Lexem::Type::CompletionDot:
		{
			MemberAccessOperatorCompletion member_access_operator_completion( it_->src_loc );
			member_access_operator_completion.member_name= "";
			NextLexem();

			member_access_operator_completion.expression= std::move(expr);

			return std::make_unique<MemberAccessOperatorCompletion>( std::move(member_access_operator_completion) );
		}

	default:
		return expr;
	};
}

Expression SyntaxAnalyzer::ParseBinaryOperatorComponentCore()
{
	switch( it_->type )
	{
	case Lexem::Type::Plus:
		{
			auto unary_plus= std::make_unique<UnaryPlus>( it_->src_loc );
			NextLexem();

			unary_plus->expression= ParseBinaryOperatorComponent();
			return std::move(unary_plus);
		}
	case Lexem::Type::Minus:
		{
			auto unary_minus= std::make_unique<UnaryMinus>( it_->src_loc );
			NextLexem();

			unary_minus->expression= ParseBinaryOperatorComponent();
			return std::move(unary_minus);
		}
	case Lexem::Type::Not:
		{
			auto logical_not= std::make_unique<LogicalNot>( it_->src_loc );
			NextLexem();

			logical_not->expression= ParseBinaryOperatorComponent();
			return std::move(logical_not);
		}
	case Lexem::Type::Tilda:
		{
			auto bitwise_not= std::make_unique<BitwiseNot>( it_->src_loc );
			NextLexem();

			bitwise_not->expression= ParseBinaryOperatorComponent();
			return std::move(bitwise_not);
		}
	case Lexem::Type::Scope:
	case Lexem::Type::CompletionScope:
			return ParseComplexName();
	case Lexem::Type::Number:
			return ParseNumericConstant();
	case Lexem::Type::String:
		{
			StringLiteral string_literal( it_->src_loc );
			string_literal.value= it_->text;
			NextLexem();

			if( it_->type == Lexem::Type::LiteralSuffix )
			{
				if( it_->text.size() > sizeof(TypeSuffix) / sizeof(TypeSuffix::value_type) - 1 )
				{
					LexSyntError msg;
					msg.src_loc= it_->src_loc;
					msg.text= "String literal is too long";
					error_messages_.push_back( msg );
					return EmptyVariant();
				}
				std::copy( it_->text.begin(), it_->text.end(), string_literal.type_suffix.begin() );
				NextLexem();
			}

			return std::move(string_literal);
		}
	case Lexem::Type::BracketLeft:
		return ParseExpressionInBrackets();
	case Lexem::Type::SquareBracketLeft:
	case Lexem::Type::PointerTypeMark:
			return std::visit( [&](auto&& t) -> Expression { return std::move(t); }, ParseTypeName() );
	case Lexem::Type::ReferenceToPointer:
		{
			auto reference_to_raw_pointer_operator= std::make_unique<ReferenceToRawPointerOperator>( it_->src_loc );
			NextLexem();

			reference_to_raw_pointer_operator->expression= ParseExpressionInBrackets();

			return std::move(reference_to_raw_pointer_operator);
		}
	case Lexem::Type::PointerToReference:
		{
			auto raw_pointer_to_reference_operator= std::make_unique<RawPointerToReferenceOperator>( it_->src_loc );
			NextLexem();

			raw_pointer_to_reference_operator->expression= ParseExpressionInBrackets();

			return std::move(raw_pointer_to_reference_operator);
		}
	case Lexem::Type::Identifier:
		if( it_->text == Keywords::true_ )
		{
			BooleanConstant boolean_constant( it_->src_loc, true );
			NextLexem();
			return std::move(boolean_constant);
		}
		if( it_->text == Keywords::false_ )
		{
			BooleanConstant boolean_constant( it_->src_loc, false );
			NextLexem();
			return std::move(boolean_constant);
		}
		if( it_->text == Keywords::move_ )
		{
			NextLexem();
			ExpectLexem( Lexem::Type::BracketLeft );

			if( it_->type == Lexem::Type::CompletionIdentifier )
			{
				MoveOperatorCompletion move_operator_completion( it_->src_loc );
				move_operator_completion.var_name= it_->text;
				NextLexem();
				return std::move(move_operator_completion);
			}

			MoveOperator move_operator( it_->src_loc );

			if( it_->type == Lexem::Type::Identifier )
			{
				move_operator.var_name= it_->text;
				NextLexem();
			}
			else
			{
				PushErrorMessage();
				return EmptyVariant();
			}

			ExpectLexem( Lexem::Type::BracketRight );

			return std::move(move_operator);
		}
		if( it_->text == Keywords::take_ )
		{
			auto take_operator= std::make_unique<TakeOperator>( it_->src_loc );
			NextLexem();

			take_operator->expression= ParseExpressionInBrackets();

			return std::move(take_operator);
		}
		if( it_->text == Keywords::select_ )
		{
			auto ternary_operator= std::make_unique<TernaryOperator>( it_->src_loc );
			NextLexem();

			ExpectLexem( Lexem::Type::BracketLeft );
			ternary_operator->condition= ParseExpression();
			ExpectLexem( Lexem::Type::Question );
			ternary_operator->true_branch= ParseExpression();
			ExpectLexem( Lexem::Type::Colon );
			ternary_operator->false_branch= ParseExpression();
			ExpectLexem( Lexem::Type::BracketRight );

			return std::move(ternary_operator);
		}
		if( it_->text == Keywords::cast_ref_ )
		{
			auto cast= std::make_unique<CastRef>( it_->src_loc );
			NextLexem();
			cast->type= ParseTypeNameInTemplateBrackets();
			cast->expression= ParseExpressionInBrackets();

			return std::move(cast);
		}
		if( it_->text == Keywords::cast_ref_unsafe_ )
		{
			auto cast= std::make_unique<CastRefUnsafe>( it_->src_loc );
			NextLexem();
			cast->type= ParseTypeNameInTemplateBrackets();
			cast->expression= ParseExpressionInBrackets();

			return std::move(cast);
		}
		if( it_->text == Keywords::cast_imut_ )
		{
			auto cast= std::make_unique<CastImut>( it_->src_loc );
			NextLexem();
			cast->expression= ParseExpressionInBrackets();

			return std::move(cast);
		}
		if( it_->text == Keywords::cast_mut_ )
		{
			auto cast= std::make_unique<CastMut>( it_->src_loc );
			NextLexem();
			cast->expression= ParseExpressionInBrackets();

			return std::move(cast);
		}
		if( it_->text == Keywords::typeinfo_ )
		{
			auto typeinfo_= std::make_unique<TypeInfo>(it_->src_loc );
			NextLexem();
			typeinfo_->type= ParseTypeNameInTemplateBrackets();

			return std::move(typeinfo_);
		}
		if( it_->text == Keywords::same_type_ )
		{
			auto same_type= std::make_unique<SameType>(it_->src_loc );

			NextLexem();
			ExpectLexem( Lexem::Type::TemplateBracketLeft );
			same_type->l= ParseTypeName();
			ExpectLexem( Lexem::Type::Comma );
			same_type->r= ParseTypeName();
			ExpectLexem( Lexem::Type::TemplateBracketRight );

			return std::move(same_type);
		}
		if( it_->text == Keywords::non_sync_ )
		{
			auto non_sync_expression= std::make_unique<NonSyncExpression>(it_->src_loc );
			NextLexem();
			non_sync_expression->type= ParseTypeNameInTemplateBrackets();

			return std::move(non_sync_expression);
		}
		if( it_->text == Keywords::safe_ )
		{
			auto expr= std::make_unique<SafeExpression>( it_->src_loc );
			NextLexem();
			expr->expression= ParseExpressionInBrackets();

			return std::move(expr);
		}
		if( it_->text == Keywords::unsafe_ )
		{
			auto expr= std::make_unique<UnsafeExpression>( it_->src_loc );
			NextLexem();
			expr->expression= ParseExpressionInBrackets();

			return std::move(expr);
		}
		if( it_->text == Keywords::fn_ || it_->text == Keywords::typeof_ || it_->text == Keywords::tup_ || it_->text == Keywords::generator_ )
			return std::visit( [&](auto&& t) -> Expression { return std::move(t); }, ParseTypeName() );
		if( auto macro= FetchMacro( it_->text, Macro::Context::Expression ) )
		{
			Expression macro_expression= ExpandMacro( *macro, &SyntaxAnalyzer::ParseExpression );
			if( std::get_if<EmptyVariant>( &macro_expression ) != nullptr )
				return EmptyVariant();

			return macro_expression;
		}

		return ParseComplexName();

	case Lexem::Type::CompletionIdentifier:
		return ParseComplexName();

	default:
		PushErrorMessage();
		return Expression();
	};
}

TypeName SyntaxAnalyzer::ParseTypeNameInTemplateBrackets()
{
	ExpectLexem( Lexem::Type::TemplateBracketLeft );
	TypeName result= ParseTypeName();
	ExpectLexem( Lexem::Type::TemplateBracketRight );
	return result;
}

FunctionParam SyntaxAnalyzer::ParseFunctionParam()
{
	FunctionParam result( it_->src_loc );
	result.type= ParseTypeName();

	result.reference_modifier= ReferenceModifier::None;
	result.mutability_modifier= MutabilityModifier::None;

	if( it_->type == Lexem::Type::And )
	{
		result.reference_modifier= ReferenceModifier::Reference;
		NextLexem();

		if( it_->type == Lexem::Type::Apostrophe )
		{
			NextLexem();

			if( it_->type == Lexem::Type::Identifier )
			{
				result.reference_tag = it_->text;
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
		result.mutability_modifier= MutabilityModifier::Mutable;
		NextLexem();
	}
	else if( it_->text == Keywords::imut_ )
	{
		result.mutability_modifier= MutabilityModifier::Immutable;
		NextLexem();
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		TryRecoverAfterError( g_function_arguments_list_control_lexems );
		return result;
	}

	result.name= it_->text;
	result.src_loc= it_->src_loc;
	NextLexem();

	if( it_->type == Lexem::Type::Apostrophe )
		result.inner_arg_reference_tag= ParseInnerReferenceTag();

	return result;
}

void SyntaxAnalyzer::ParseFunctionTypeEnding( FunctionType& result )
{
	if( it_->type == Lexem::Type::Apostrophe )
		result.references_pollution_list= ParseFunctionReferencesPollutionList();

	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::unsafe_ )
	{
		result.unsafe= true;
		NextLexem();
	}

	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::call_conv_ )
	{
		NextLexem();
		ExpectLexem( Lexem::Type::BracketLeft );

		if( it_->type != Lexem::Type::String )
			PushErrorMessage();

		result.calling_convention= it_->text;
		NextLexem();

		ExpectLexem( Lexem::Type::BracketRight );
	}

	if( it_->type == Lexem::Type::Colon )
	{
		NextLexem();

		result.return_type= std::make_unique<TypeName>( ParseTypeName() );

		if( it_->type == Lexem::Type::And )
		{
			result.return_value_reference_modifier= ReferenceModifier::Reference;
			NextLexem();

			if( it_->type == Lexem::Type::Apostrophe )
			{
				NextLexem();

				if( it_->type == Lexem::Type::Identifier )
				{
					result.return_value_reference_tag = it_->text;
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
					result.return_value_mutability_modifier= MutabilityModifier::Mutable;
				else if( it_->text == Keywords::imut_ )
					result.return_value_mutability_modifier= MutabilityModifier::Immutable;
				else
					PushErrorMessage();
				NextLexem();
			}
		}
		else if( it_->type == Lexem::Type::Apostrophe )
			result.return_value_inner_reference_tag= ParseInnerReferenceTag();
	}
}

FunctionType SyntaxAnalyzer::ParseFunctionType()
{
	FunctionType result( it_->src_loc );

	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::fn_ );
	NextLexem();

	ExpectLexem( Lexem::Type::BracketLeft );

	while( NotEndOfFile() && it_->type != Lexem::Type::EndOfFile )
	{
		if( it_->type == Lexem::Type::BracketRight )
		{
			NextLexem();
			break;
		}

		result.params.push_back( ParseFunctionParam() );

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

	ParseFunctionTypeEnding( result );

	return result;
}

TypeName SyntaxAnalyzer::ParseTypeName()
{
	if( it_->type == Lexem::Type::SquareBracketLeft )
	{
		auto array_type_name= std::make_unique<ArrayTypeName>(it_->src_loc);

		NextLexem();
		array_type_name->element_type= ParseTypeName();
		ExpectLexem( Lexem::Type::Comma );
		array_type_name->size= ParseExpression();
		ExpectLexem( Lexem::Type::SquareBracketRight );

		return std::move(array_type_name);
	}
	else if( it_->type == Lexem::Type::BracketLeft )
	{
		// Type name inside (). We needs this for better parsing of function pointer types, for example.
		NextLexem();

		TypeName type_name= ParseTypeName();

		ExpectLexem( Lexem::Type::BracketRight );

		return type_name;
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::tup_ )
	{
		TupleType tuple_type( it_->src_loc );
		NextLexem();

		ExpectLexem(Lexem::Type::SquareBracketLeft );

		while( NotEndOfFile() )
		{
			if( it_->type == Lexem::Type::SquareBracketRight )
			{
				NextLexem();
				break;
			}

			tuple_type.element_types.push_back( ParseTypeName() );
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
	else if( it_->type == Lexem::Type::PointerTypeMark )
	{
		auto raw_pointer_type= std::make_unique<RawPointerType>( it_->src_loc );
		NextLexem();

		ExpectLexem( Lexem::Type::BracketLeft );
		raw_pointer_type->element_type= ParseTypeName();
		ExpectLexem(Lexem::Type::BracketRight );

		return std::move(raw_pointer_type);
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::generator_ )
	{
		GeneratorType generator_type( it_->src_loc );
		NextLexem();

		if( it_->type == Lexem::Type::Apostrophe )
		{
			NextLexem();

			GeneratorType::InnerReferenceTag inner_reference_tag;

			if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::mut_ )
			{
				NextLexem();
				inner_reference_tag.mutability_modifier= MutabilityModifier::Mutable;
			}
			else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::imut_ )
			{
				NextLexem();
				inner_reference_tag.mutability_modifier= MutabilityModifier::Immutable;
			}

			if( it_->type != Lexem::Type::Identifier )
				PushErrorMessage();
			inner_reference_tag.name= it_->text;
			NextLexem();

			ExpectLexem( Lexem::Type::Apostrophe );

			generator_type.inner_reference_tag= std::make_unique<GeneratorType::InnerReferenceTag>( std::move(inner_reference_tag) );
		}

		generator_type.non_sync_tag= TryParseNonSyncTag();

		ExpectLexem( Lexem::Type::Colon );
		generator_type.return_type= ParseTypeName();

		if( it_->type == Lexem::Type::And )
		{
			NextLexem();
			generator_type.return_value_reference_modifier= ReferenceModifier::Reference;

			if( it_->type == Lexem::Type::Apostrophe )
			{
				NextLexem();

				if( it_->type == Lexem::Type::Identifier )
				{
					generator_type.return_value_reference_tag= it_->text;
					NextLexem();
				}
				else
					PushErrorMessage();
			}

			if( it_->type == Lexem::Type::Identifier )
			{
				if( it_->text == Keywords::mut_ )
				{
					generator_type.return_value_mutability_modifier= MutabilityModifier::Mutable;
					NextLexem();
				}
				else if( it_->text == Keywords::imut_ )
				{
					generator_type.return_value_mutability_modifier= MutabilityModifier::Immutable;
					NextLexem();
				}
			}
		}
		else if( it_->type == Lexem::Type::Apostrophe )
			generator_type.return_value_reference_tag= ParseInnerReferenceTag();

		return std::make_unique<GeneratorType>(std::move(generator_type));
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::fn_ )
		return std::make_unique<FunctionType>( ParseFunctionType() );
	else
		return std::visit( []( auto&& el ) { return TypeName( std::move(el) ); }, ParseComplexName() );
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
	if( it_->type == Lexem::Type::CompletionScope )
	{
		RootNamespaceNameLookupCompletion root_namespace_lookup_completion( it_->src_loc );
		NextLexem(); // Skip ::

		root_namespace_lookup_completion.name= ""; // Complete with empty string.

		return std::move(root_namespace_lookup_completion);
	}
	else if( it_->type == Lexem::Type::Scope )
	{
		NextLexem(); // Skip ::

		if( it_->type == Lexem::Type::Identifier )
		{
			RootNamespaceNameLookup root_namespace_lookup( it_->src_loc );
			root_namespace_lookup.name= it_->text;
			NextLexem();

			return TryParseComplexNameTailWithTemplateArgs( std::move(root_namespace_lookup) );
		}
		else if( it_->type == Lexem::Type::CompletionIdentifier )
		{
			RootNamespaceNameLookupCompletion root_namespace_lookup_completion( it_->src_loc );
			root_namespace_lookup_completion.name= it_->text;
			NextLexem();

			return std::move(root_namespace_lookup_completion);
		}
		else
		{
			PushErrorMessage();
			return RootNamespaceNameLookupCompletion( it_->src_loc );
		}
	}
	else if( it_->type == Lexem::Type::Identifier )
	{
		if( it_->text == Keywords::typeof_ )
		{
			auto typeof_type_name= std::make_unique<TypeofTypeName>( it_->src_loc );
			NextLexem();
			typeof_type_name->expression=  ParseExpressionInBrackets();
			return ParseComplexNameTail( std::move( typeof_type_name ) );
		}
		else
		{
			NameLookup name_lookup( it_->src_loc );

			if( it_->type != Lexem::Type::Identifier )
				PushErrorMessage();
			name_lookup.name= it_->text;
			NextLexem();

			return TryParseComplexNameTailWithTemplateArgs( std::move(name_lookup) );
		}
	}
	else if( it_->type == Lexem::Type::CompletionIdentifier )
	{
		NameLookupCompletion name_lookup_completion( it_->src_loc );
		name_lookup_completion.name= it_->text;
		NextLexem();

		return std::move(name_lookup_completion);
	}
	else
	{
		PushErrorMessage();
		return RootNamespaceNameLookup( it_->src_loc );
	}
}

ComplexName SyntaxAnalyzer::ParseComplexNameTail( ComplexName base )
{
	if( it_->type == Lexem::Type::Scope )
	{
		NextLexem(); // Skip ::

		if( it_->type == Lexem::Type::Identifier )
		{
			const SrcLoc src_loc= it_->src_loc;
			const std::string& name= it_->text;
			NextLexem();

			return TryParseComplexNameTailWithTemplateArgs( std::make_unique<NamesScopeNameFetch>( NamesScopeNameFetch{ src_loc, name, std::move(base) } ) );
		}
		else if( it_->type == Lexem::Type::CompletionIdentifier )
		{
			const SrcLoc src_loc= it_->src_loc;
			const std::string& name= it_->text;
			NextLexem();

			return std::make_unique<NamesScopeNameFetchCompletion>( NamesScopeNameFetchCompletion{ src_loc, name, std::move(base) } );
		}
		else
		{
			PushErrorMessage();
			return base;
		}
	}
	else if( it_->type == Lexem::Type::CompletionScope )
	{
		const SrcLoc src_loc= it_->src_loc;  // Use "src_loc" of completion scope lexem, since there is no identifier lexem here.
		const std::string name= ""; // Complete with empty string.
		NextLexem(); // Skip ::

		return std::make_unique<NamesScopeNameFetchCompletion>( NamesScopeNameFetchCompletion{ src_loc, name, std::move(base) } );
	}
	else
		return base;
}

ComplexName SyntaxAnalyzer::TryParseComplexNameTailWithTemplateArgs( ComplexName base )
{
	if( it_->type == Lexem::Type::TemplateBracketLeft )
	{
		const SrcLoc src_loc= it_->src_loc;
		return ParseComplexNameTail( std::make_unique<TemplateParametrization>( TemplateParametrization{ src_loc, ParseTemplateParameters(), std::move(base) } ) );
	}
	else
		return ParseComplexNameTail( std::move(base) );
}

std::string SyntaxAnalyzer::ParseInnerReferenceTag()
{
	U_ASSERT( it_->type == Lexem::Type::Apostrophe );
	NextLexem();

	std::string result;

	if( it_->type == Lexem::Type::Apostrophe )
	{
		// Empty list
		// TODO - remove it
		NextLexem();
		return result;
	}

	if( it_->type == Lexem::Type::Identifier )
	{
		result= it_->text;
		NextLexem();
	}
	else
	{
		PushErrorMessage();
		return result;
	}

	if( it_->type == Lexem::Type::Apostrophe )
	{
		NextLexem();
	}
	else
	{
		PushErrorMessage();
		return result;
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

		ExpectLexem( Lexem::Type::LeftArrow );

		if( it_->type == Lexem::Type::Identifier )
		{
			result.back().second= it_->text;
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

std::variant<std::vector<Expression>, SyntaxAnalyzer::SignatureHelpTag> SyntaxAnalyzer::ParseCall()
{
	ExpectLexem( Lexem::Type::BracketLeft );

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
				break;
			}
		}
		else if( it_->type == Lexem::Type::SignatureHelpComma )
			return SignatureHelpTag{};
		else
			break;
	}

	ExpectLexem( Lexem::Type::BracketRight );

	return std::move(args);
}

Initializer SyntaxAnalyzer::ParseInitializer( const bool parse_expression_initializer )
{
	if( it_->type == Lexem::Type::SquareBracketLeft )
	{
		return ParseArrayInitializer();
	}
	else if( it_->type == Lexem::Type::BracketLeft )
	{
		return ParseConstructorInitializer(); // TODO - fix case, like :    var [ i32, 1] x[ (1 + 2) * 3 ];
	}
	else if( it_->type == Lexem::Type::SignatureHelpBracketLeft )
	{
		ConstructorInitializerSignatureHelp result( it_->src_loc );
		NextLexem();
		return std::move(result);
	}
	else if( it_->type == Lexem::Type::BraceLeft )
	{
		return ParseStructNamedInitializer();
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::zero_init_ )
	{
		const auto prev_it= it_;
		NextLexem();
		return ZeroInitializer( prev_it->src_loc );
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::uninitialized_ )
	{
		const auto prev_it= it_;
		NextLexem();
		return UninitializedInitializer( prev_it->src_loc );
	}
	else if( parse_expression_initializer )
	{
		// In some cases usage of expression in initializer is forbidden.
		return ParseExpression();
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
			initializer= ZeroInitializer( it_->src_loc );
			NextLexem();
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::uninitialized_ )
		{
			initializer= UninitializedInitializer( it_->src_loc );
			NextLexem();
		}
		else
			initializer= ParseExpression();
	}
	else if(
		it_->type == Lexem::Type::BracketLeft ||
		it_->type == Lexem::Type::SignatureHelpBracketLeft ||
		it_->type == Lexem::Type::SquareBracketLeft ||
		it_->type == Lexem::Type::BraceLeft )
		return ParseInitializer( false );

	return initializer;
}

Initializer SyntaxAnalyzer::ParseArrayInitializer()
{
	U_ASSERT( it_->type == Lexem::Type::SquareBracketLeft );

	SequenceInitializer result( it_->src_loc );
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
	ExpectLexem( Lexem::Type::SquareBracketRight );

	return std::move(result);
}

Initializer SyntaxAnalyzer::ParseStructNamedInitializer()
{
	U_ASSERT( it_->type == Lexem::Type::BraceLeft );

	StructNamedInitializer result( it_->src_loc );
	NextLexem();

	while( NotEndOfFile() && it_->type != Lexem::Type::BraceRight )
	{
		result.members_initializers.emplace_back();
		StructNamedInitializer::MemberInitializer& out_initializer= result.members_initializers.back();

		if( it_->type == Lexem::Type::Dot )
		{
			NextLexem();

			if( it_->type == Lexem::Type::Identifier )
			{
				out_initializer.name= it_->text;
				out_initializer.src_loc= it_->src_loc;
				NextLexem();

				Initializer initializer= ParseVariableInitializer();
				if( std::get_if<EmptyVariant>(&initializer) != nullptr )
					PushErrorMessage();
				out_initializer.initializer= std::move(initializer);
			}
			else if( it_->type == Lexem::Type::CompletionIdentifier )
			{
				out_initializer.name= it_->text;
				out_initializer.src_loc= it_->src_loc;
				NextLexem();
				out_initializer.completion_requested= true;
			}
			else
			{
				PushErrorMessage();
				return std::move(result);
			}
		}
		else if( it_->type == Lexem::Type::CompletionDot )
		{
			out_initializer.completion_requested= true;
			out_initializer.src_loc= it_->src_loc;
			out_initializer.name= ""; // Complete with empty string.
			NextLexem();
		}
		else
		{
			PushErrorMessage();
			return std::move(result);
		}

		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();
			if( it_->type == Lexem::Type::BraceRight )
				break;
		}
		else
			break;
	}
	ExpectLexem( Lexem::Type::BraceRight );

	return std::move(result);
}

Initializer SyntaxAnalyzer::ParseConstructorInitializer()
{
	ConstructorInitializer result( it_->src_loc );

	auto call_result= ParseCall();
	if( const auto args= std::get_if< std::vector<Expression> >( &call_result ) )
		result.arguments= std::move(*args);
	else if( std::get_if< SignatureHelpTag >( &call_result ) != nullptr )
	{
		ConstructorInitializerSignatureHelp signature_help_result( it_->src_loc );
		NextLexem();
		return std::move(signature_help_result);
	}
	else U_ASSERT(false);

	return std::move(result);
}

VariablesDeclaration SyntaxAnalyzer::ParseVariablesDeclaration()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ );
	VariablesDeclaration decl( it_->src_loc );
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
		variable_entry.src_loc= it_->src_loc;
		NextLexem();

		Initializer variable_initializer= ParseVariableInitializer();
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
	AutoVariableDeclaration result( it_->src_loc );
	NextLexem();

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
	result.src_loc= it_->src_loc;
	NextLexem();

	ExpectLexem( Lexem::Type::Assignment );

	result.initializer_expression = ParseExpression();

	ExpectSemicolon();

	return result;
}

ReturnOperator SyntaxAnalyzer::ParseReturnOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::return_ );

	ReturnOperator result( it_->src_loc );
	NextLexem();

	if( it_->type == Lexem::Type::Semicolon )
	{
		NextLexem();
		return result;
	}

	result.expression= ParseExpression();

	ExpectSemicolon();

	return result;
}

YieldOperator SyntaxAnalyzer::ParseYieldOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::yield_ );

	YieldOperator result( it_->src_loc );
	NextLexem();

	if( it_->type != Lexem::Type::Semicolon )
		result.expression= ParseExpression();

	ExpectSemicolon();

	return result;
}

std::optional<Label> SyntaxAnalyzer::TryParseLabel()
{
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::label_ )
	{
		Label label( it_->src_loc );

		NextLexem();
		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return std::move(label);
		}

		label.name= it_->text;
		NextLexem();

		return std::move(label);
	}

	return std::nullopt;
}

WhileOperator SyntaxAnalyzer::ParseWhileOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::while_ );
	WhileOperator result( it_->src_loc );
	NextLexem();

	result.condition= ParseExpressionInBrackets();
	result.label= TryParseLabel();
	result.block= ParseBlock();
	return result;
}

LoopOperator SyntaxAnalyzer::ParseLoopOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::loop_ );
	LoopOperator result( it_->src_loc );
	NextLexem();

	result.label= TryParseLabel();
	result.block= ParseBlock();

	return result;
}

std::variant<RangeForOperator, CStyleForOperator> SyntaxAnalyzer::ParseForOperator()
{
	if( it_end_ - it_ >= 3 )
	{
		const Lexem& next_lexem= *(it_ + 2);
		if( next_lexem.type == Lexem::Type::Identifier )
		{
			if( next_lexem.text == Keywords::var_ || next_lexem.text == Keywords::auto_ )
				return ParseCStyleForOperator();
		}
		if( next_lexem.type == Lexem::Type::Semicolon )
			return ParseCStyleForOperator();
	}

	return ParseRangeForOperator();
}

RangeForOperator SyntaxAnalyzer::ParseRangeForOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::for_ );
	RangeForOperator result( it_->src_loc );
	NextLexem();

	ExpectLexem( Lexem::Type::BracketLeft );

	if( it_->type == Lexem::Type::And )
	{
		result.reference_modifier= ReferenceModifier::Reference;
		NextLexem();
	}
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::mut_ )
	{
		result.mutability_modifier= MutabilityModifier::Mutable;
		NextLexem();
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::imut_ )
	{
		result.mutability_modifier= MutabilityModifier::Immutable;
		NextLexem();
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return result;
	}
	result.loop_variable_name= it_->text;
	NextLexem();

	ExpectLexem( Lexem::Type::Colon );

	result.sequence= ParseExpression();

	ExpectLexem( Lexem::Type::BracketRight );

	result.label= TryParseLabel();
	result.block= ParseBlock();

	return result;
}

CStyleForOperator SyntaxAnalyzer::ParseCStyleForOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::for_ );
	CStyleForOperator result( it_->src_loc );
	NextLexem();

	ExpectLexem( Lexem::Type::BracketLeft );

	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ )
	{
		result.variable_declaration_part=
			std::make_unique< std::variant<VariablesDeclaration, AutoVariableDeclaration> >( ParseVariablesDeclaration() );
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ )
	{
		result.variable_declaration_part=
			std::make_unique< std::variant<VariablesDeclaration, AutoVariableDeclaration> >( ParseAutoVariableDeclaration() );
	}
	else if( it_->type == Lexem::Type::Semicolon )
		NextLexem();
	else
	{
		PushErrorMessage();
		return result;
	}

	if( it_->type != Lexem::Type::Semicolon )
		result.loop_condition= ParseExpression();

	ExpectSemicolon();

	while( NotEndOfFile() && it_->type != Lexem::Type::BracketRight )
	{
		if( it_->type == Lexem::Type::Increment )
		{
			IncrementOperator increment_operator( it_->src_loc );
			NextLexem();
			increment_operator.expression= ParseExpression();

			result.iteration_part_elements.push_back( std::move(increment_operator) );
		}
		else if( it_->type == Lexem::Type::Decrement )
		{
			DecrementOperator decrement_operator( it_->src_loc );
			NextLexem();
			decrement_operator.expression= ParseExpression();

			result.iteration_part_elements.push_back( std::move(decrement_operator) );
		}
		else
		{
			Expression expression_l= ParseExpression();

			if( it_->type == Lexem::Type::Assignment )
			{
				AssignmentOperator assignment_operator( it_->src_loc );
				NextLexem();
				assignment_operator.l_value= std::move(expression_l);
				assignment_operator.r_value= ParseExpression();

				result.iteration_part_elements.push_back( std::move(assignment_operator) );
			}
			else if( const auto additive_operation= GetAdditiveAssignmentOperator( *it_ ) )
			{
				AdditiveAssignmentOperator additive_assignment_operator( it_->src_loc );
				additive_assignment_operator.additive_operation= *additive_operation;
				NextLexem();
				additive_assignment_operator.l_value= std::move(expression_l);
				additive_assignment_operator.r_value= ParseExpression();

				result.iteration_part_elements.push_back( std::move(additive_assignment_operator) );
			}
			else
			{
				SingleExpressionOperator single_expression_operator( GetExpressionSrcLoc( expression_l ) );
				single_expression_operator.expression= std::move(expression_l);

				result.iteration_part_elements.push_back( std::move(single_expression_operator) );
			}
		}

		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();

			if( it_->type == Lexem::Type::BracketRight ) // forbid ) after ,
			{
				PushErrorMessage();
				return result;
			}
			continue;
		}
		else
			break;
	}

	ExpectLexem( Lexem::Type::BracketRight );

	result.label= TryParseLabel();
	result.block= ParseBlock();

	return result;
}

BreakOperator SyntaxAnalyzer::ParseBreakOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::break_ );
	BreakOperator result( it_->src_loc );
	NextLexem();

	result.label= TryParseLabel();

	ExpectSemicolon();

	return result;
}

ContinueOperator SyntaxAnalyzer::ParseContinueOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::continue_ );
	ContinueOperator result( it_->src_loc );
	NextLexem();

	result.label= TryParseLabel();

	ExpectSemicolon();

	return result;
}

WithOperator SyntaxAnalyzer::ParseWithOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::with_ );
	WithOperator result( it_->src_loc );
	NextLexem();

	ExpectLexem( Lexem::Type::BracketLeft );

	if( it_->type == Lexem::Type::And )
	{
		result.reference_modifier= ReferenceModifier::Reference;
		NextLexem();
	}
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::mut_ )
	{
		result.mutability_modifier= MutabilityModifier::Mutable;
		NextLexem();
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::imut_ )
	{
		result.mutability_modifier= MutabilityModifier::Immutable;
		NextLexem();
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return result;
	}
	result.variable_name= it_->text;
	result.src_loc= it_->src_loc; // TODO - maybe create second src_loc for variable name itslef?
	NextLexem();

	ExpectLexem( Lexem::Type::Colon );

	result.expression= ParseExpression();

	ExpectLexem( Lexem::Type::BracketRight );

	result.block= ParseBlock();
	return result;
}

IfOperator SyntaxAnalyzer::ParseIfOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ );
	IfOperator result( it_->src_loc );
	NextLexem();

	result.condition= ParseExpressionInBrackets();
	result.block= ParseBlock();
	result.alternative= TryParseIfAlternative();
	result.end_src_loc= std::prev( it_ )->src_loc;

	return result;
}

StaticIfOperator SyntaxAnalyzer::ParseStaticIfOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_if_ );

	StaticIfOperator result( it_->src_loc  );
	NextLexem();

	result.condition= ParseExpressionInBrackets();
	result.block= ParseBlock();
	result.alternative= TryParseIfAlternative();

	return result;
}

IfCoroAdvanceOperator SyntaxAnalyzer::ParseIfCoroAdvanceOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_coro_advance_ );
	IfCoroAdvanceOperator result( it_->src_loc );
	NextLexem();

	ExpectLexem( Lexem::Type::BracketLeft );

	if( it_->type == Lexem::Type::And )
	{
		result.reference_modifier= ReferenceModifier::Reference;
		NextLexem();
	}
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::mut_ )
	{
		result.mutability_modifier= MutabilityModifier::Mutable;
		NextLexem();
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::imut_ )
	{
		result.mutability_modifier= MutabilityModifier::Immutable;
		NextLexem();
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return result;
	}
	result.variable_name= it_->text;
	NextLexem();

	ExpectLexem( Lexem::Type::Colon );

	result.expression= ParseExpression();

	ExpectLexem( Lexem::Type::BracketRight );

	result.block= ParseBlock();
	result.alternative= TryParseIfAlternative();
	result.end_src_loc= std::prev( it_ )->src_loc;

	return result;
}

SwitchOperator SyntaxAnalyzer::ParseSwitchOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::switch_ );
	SwitchOperator result( it_->src_loc );
	NextLexem();

	result.value= ParseExpressionInBrackets();

	ExpectLexem( Lexem::Type::BraceLeft );

	while( it_->type != Lexem::Type::BraceRight && NotEndOfFile() )
	{
		const auto values_end_lexem= Lexem::Type::RightArrow; // TODO - maybe use another lexem?

		SwitchOperator::CaseValues case_values;
		if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::default_ )
		{
			NextLexem();
			case_values= SwitchOperator::DefaultPlaceholder{};
		}
		else
		{
			std::vector<SwitchOperator::CaseValue> values;
			while(true)
			{
				if( it_->type == Lexem::Type::Ellipsis )
				{
					NextLexem();
					SwitchOperator::CaseRange range;
					if( it_->type != Lexem::Type::Comma && it_->type != values_end_lexem )
						range.high= ParseExpression();
					values.push_back( std::move(range) );
				}
				else
				{
					Expression expression= ParseExpression();
					if( it_->type== Lexem::Type::Ellipsis )
					{
						NextLexem();
						SwitchOperator::CaseRange range;
						range.low= std::move(expression);

						if( it_->type != Lexem::Type::Comma && it_->type != values_end_lexem )
							range.high= ParseExpression();
						values.push_back( std::move(range) );
					}
					else
						values.push_back( std::move(expression) );
				}

				if( it_->type == Lexem::Type::Comma )
				{
					NextLexem();
					continue;
				}
				else
					break;
			}
			case_values= std::move(values);
		}

		ExpectLexem( values_end_lexem );
		Block block= ParseBlock();

		SwitchOperator::Case case_{ std::move(case_values), std::move(block) };
		result.cases.push_back( std::move(case_) );

		if( it_->type == Lexem::Type::Comma )
		{
			NextLexem();
			continue;
		}
		else
			break;
	}

	result.end_src_loc= it_->src_loc;
	ExpectLexem( Lexem::Type::BraceRight );

	return result;
}

StaticAssert SyntaxAnalyzer::ParseStaticAssert()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ );
	StaticAssert result( it_->src_loc );
	NextLexem();

	ExpectLexem( Lexem::Type::BracketLeft );

	result.expression= ParseExpression();

	if( it_->type == Lexem::Type::Comma )
	{
		NextLexem();
		if( it_->type != Lexem::Type::String )
			PushErrorMessage();
		result.message= it_->text;
		NextLexem();
	}

	ExpectLexem( Lexem::Type::BracketRight );

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
	Enum result( it_->src_loc );
	NextLexem();

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return result;
	}
	result.name= it_->text;
	result.src_loc= it_->src_loc;
	NextLexem();

	if( it_->type == Lexem::Type::Colon )
	{
		NextLexem();
		result.underlaying_type_name= ParseComplexName();
	}

	ExpectLexem( Lexem::Type::BraceLeft );

	while( NotEndOfFile() )
	{
		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return result;
		}

		result.members.emplace_back();
		result.members.back().src_loc= it_->src_loc;
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

std::variant<Halt, HaltIf> SyntaxAnalyzer::ParseHalt()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::halt_ );
	const SrcLoc& src_loc= it_->src_loc;
	NextLexem();

	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ )
	{
		NextLexem();
		HaltIf result( src_loc );

		result.condition= ParseExpressionInBrackets();

		ExpectSemicolon();

		return std::move(result);
	}
	else if( it_->type == Lexem::Type::Semicolon )
	{
		NextLexem();
		return Halt( src_loc );
	}
	else
	{
		PushErrorMessage();
		return Halt( src_loc );
	}
}

BlockElementsList SyntaxAnalyzer::ParseBlockElements()
{
	BlockElementsList::Builder result_builder;

	while( NotEndOfFile() && it_->type != Lexem::Type::EndOfFile )
	{
		if( it_->type == Lexem::Type::BraceRight )
			break;
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ )
			result_builder.Append( ParseVariablesDeclaration() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ )
			result_builder.Append( ParseAutoVariableDeclaration() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::return_ )
			result_builder.Append( ParseReturnOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::yield_ )
			result_builder.Append( ParseYieldOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::while_ )
			result_builder.Append( ParseWhileOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::loop_ )
			result_builder.Append( ParseLoopOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::for_ )
			std::visit( [&]( auto el ) { result_builder.Append( std::move(el) ); }, ParseForOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::break_ )
			result_builder.Append( ParseBreakOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::continue_ )
			result_builder.Append( ParseContinueOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::with_ )
			result_builder.Append( ParseWithOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ )
			result_builder.Append( ParseIfOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_if_ )
			result_builder.Append( ParseStaticIfOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_coro_advance_ )
			result_builder.Append( ParseIfCoroAdvanceOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::switch_ )
			result_builder.Append( ParseSwitchOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ )
			result_builder.Append( ParseStaticAssert() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::type_ )
			result_builder.Append( ParseTypeAlias() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::halt_ )
			std::visit( [&]( auto el ) { result_builder.Append( std::move(el) ); }, ParseHalt() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::safe_ &&
				std::next(it_)->type == Lexem::Type::BraceLeft )
		{
			NextLexem();
			ScopeBlock block= ParseBlock();
			block.safety= ScopeBlock::Safety::Safe;
			block.label= TryParseLabel();
			result_builder.Append( std::move( block ) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::unsafe_ &&
				std::next(it_)->type == Lexem::Type::BraceLeft )
		{
			NextLexem();
			ScopeBlock block= ParseBlock();
			block.safety= ScopeBlock::Safety::Unsafe;
			block.label= TryParseLabel();
			result_builder.Append( std::move( block ) );
		}
		else if( it_->type == Lexem::Type::BraceLeft )
		{
			ScopeBlock block= ParseBlock();
			block.label= TryParseLabel();
			result_builder.Append( std::move(block) );
		}
		else if( it_->type == Lexem::Type::Increment )
		{
			IncrementOperator op( it_->src_loc );
			NextLexem();

			op.expression= ParseExpression();
			result_builder.Append( std::move(op) );

			ExpectSemicolon();
		}
		else if( it_->type == Lexem::Type::Decrement )
		{
			DecrementOperator op( it_->src_loc );
			NextLexem();

			op.expression= ParseExpression();
			result_builder.Append( std::move(op) );

			ExpectSemicolon();
		}
		else
		{
			if( it_->type == Lexem::Type::Identifier )
			{
				if( const auto macro= FetchMacro( it_->text, Macro::Context::Block ) )
				{
					result_builder.AppendList( ExpandMacro( *macro, &SyntaxAnalyzer::ParseBlockElements ) );
					continue;
				}
			}

			Expression l_expression= ParseExpression();

			if( it_->type == Lexem::Type::Assignment )
			{
				AssignmentOperator assignment_operator( it_->src_loc );
				NextLexem();

				assignment_operator.l_value= std::move(l_expression);
				assignment_operator.r_value= ParseExpression();

				result_builder.Append( std::move(assignment_operator) );

				if( it_->type != Lexem::Type::Semicolon )
				{
					PushErrorMessage();
					TryRecoverAfterError( g_block_body_elements_control_lexems );
					continue;
				}
				NextLexem();
			}
			else if( const auto additive_operation= GetAdditiveAssignmentOperator( *it_ ) )
			{
				AdditiveAssignmentOperator op( it_->src_loc );

				op.additive_operation= *additive_operation;
				NextLexem();

				op.l_value= std::move(l_expression);
				op.r_value= ParseExpression();

				result_builder.Append( std::move(op) );

				if( it_->type != Lexem::Type::Semicolon )
				{
					PushErrorMessage();
					TryRecoverAfterError( g_block_body_elements_control_lexems );
					continue;
				}
				NextLexem();
			}
			else
			{
				SingleExpressionOperator expr(it_->src_loc );
				expr.expression= std::move(l_expression);
				result_builder.Append( std::move(expr) );

				if( it_->type == Lexem::Type::Semicolon )
					NextLexem();
				else
				{
					PushErrorMessage();
					TryRecoverAfterError( g_block_body_elements_control_lexems );
				}
			}
		}
	}

	return result_builder.Build();
}

Block SyntaxAnalyzer::ParseBlock()
{
	Block block( it_->src_loc );

	ExpectLexem( Lexem::Type::BraceLeft );

	block.elements= ParseBlockElements();
	block.end_src_loc= it_->src_loc;

	ExpectLexem( Lexem::Type::BraceRight );

	return block;
}

IfAlternativePtr SyntaxAnalyzer::TryParseIfAlternative()
{
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::else_ )
	{
		NextLexem();
		return ParseIfAlternative();
	}

	return nullptr;
}

IfAlternativePtr SyntaxAnalyzer::ParseIfAlternative()
{
	if( it_->type == Lexem::Type::BraceLeft )
		return std::make_unique<IfAlternative>( ParseBlock() );
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ )
		return std::make_unique<IfAlternative>( ParseIfOperator() );
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_if_ )
		return std::make_unique<IfAlternative>( ParseStaticIfOperator() );
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_coro_advance_ )
		return std::make_unique<IfAlternative>( ParseIfCoroAdvanceOperator() );

	// Accept macros, producing single element of if-alternative kind, as if-alternative.
	if( it_->type == Lexem::Type::Identifier )
	{
		if( const auto macro= FetchMacro( it_->text, Macro::Context::Block ) )
		{
			BlockElementsList list= ExpandMacro( *macro, &SyntaxAnalyzer::ParseBlockElements );

			if( list.HasTail() )
			{
				LexSyntError error_message;
				error_message.src_loc= it_->src_loc;
				error_message.text= "Syntax error - expected exactly one element in expansion of macro for \"if\" alternative.";
				error_messages_.push_back( std::move(error_message) );
				return nullptr;
			}


			if( auto scope_block= list.TryTakeStart<ScopeBlock>() )
			{
				ScopeBlock& block= *scope_block;
				if( block.safety == ScopeBlock::Safety::None && block.label == std::nullopt )
				{
					// Accept only pure blocks without safety modifiers and labels.
					return std::make_unique<IfAlternative>( std::move(block) );
				}
				else
				{
					LexSyntError error_message;
					error_message.src_loc= it_->src_loc;
					error_message.text= "Syntax error - expected block without safety modifiers and labels for \"if\" alternative.";
					error_messages_.push_back( std::move(error_message) );
					return nullptr;
				}
			}

			if( auto if_operator= list.TryTakeStart< IfOperator >() )
				return std::make_unique<IfAlternative>( std::move( *if_operator ) );
			if( auto static_if= list.TryTakeStart< StaticIfOperator >() )
				return std::make_unique<IfAlternative>( std::move( *static_if ) );
			if( auto if_coro_advance_operator= list.TryTakeStart< IfCoroAdvanceOperator >() )
				return std::make_unique<IfAlternative>( std::move( *if_coro_advance_operator ) );

			LexSyntError error_message;
			error_message.src_loc= it_->src_loc;
			error_message.text= "Syntax error - unexpected element kind for \"if\" alternative.";
			error_messages_.push_back( std::move(error_message) );
			return nullptr;
		}
	}

	PushErrorMessage();
	return nullptr;
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

NonSyncTag SyntaxAnalyzer::TryParseNonSyncTag()
{
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::non_sync_ )
	{
		NextLexem();

		if( it_->type == Lexem::Type::BracketLeft )
			return std::make_unique<Expression>( ParseExpressionInBrackets() );

		return NonSyncTagTrue();
	}
	return NonSyncTagNone();
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

TypeAlias SyntaxAnalyzer::ParseTypeAlias()
{
	U_ASSERT( it_->text == Keywords::type_ );

	NextLexem();

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return TypeAlias( it_->src_loc );
	}

	const std::string& name= it_->text;
	const SrcLoc src_loc= it_->src_loc;
	NextLexem();

	TypeAlias result= ParseTypeAliasBody();
	result.name= name;
	result.src_loc= src_loc;
	return result;
}

TypeAlias SyntaxAnalyzer::ParseTypeAliasBody()
{
	// Parse something like "= i32;"

	TypeAlias result( it_->src_loc );

	ExpectLexem( Lexem::Type::Assignment );

	result.value= ParseTypeName();

	ExpectSemicolon();

	return result;
}

Function SyntaxAnalyzer::ParseFunction()
{
	U_ASSERT( it_->text == Keywords::fn_ || it_->text == Keywords::op_ );

	Function result( it_->src_loc );

	const std::string& function_definition_lexem= it_->text;
	NextLexem();

	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::virtual_ )
	{
		NextLexem();
		result.virtual_function_kind= VirtualFunctionKind::DeclareVirtual;
		if( it_->type == Lexem::Type::Identifier )
		{
			if( it_->text == Keywords::override_ )
			{
				result.virtual_function_kind= VirtualFunctionKind::VirtualOverride;
				NextLexem();
			}
			else if( it_->text == Keywords::final_ )
			{
				result.virtual_function_kind= VirtualFunctionKind::VirtualFinal;
				NextLexem();
			}
			else if( it_->text == Keywords::pure_ )
			{
				result.virtual_function_kind= VirtualFunctionKind::VirtualPure;
				NextLexem();
			}
		}
	}
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::generator_ )
	{
		NextLexem();
		result.kind= Function::Kind::Generator;

		result.coroutine_non_sync_tag= TryParseNonSyncTag();
	}
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::constexpr_ )
	{
		NextLexem();
		result.constexpr_= true;
	}
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::nomangle_ )
	{
		NextLexem();
		result.no_mangle= true;
	}
	// TODO - parse "enalbe_if" prior to other modifiers?
	if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::enable_if_ )
	{
		NextLexem();

		result.condition= ParseExpressionInBrackets();
	}

	// Parse complex name before function name - such "fn MyStruct::A::B"
	if( it_->type == Lexem::Type::Scope )
	{
		result.name.push_back(Function::NameComponent{});
		NextLexem();
	}

	if( it_->type == Lexem::Type::CompletionScope )
	{
		// Complete with empty string.
		result.name.push_back(Function::NameComponent{ "", it_->src_loc });
		result.name.back().completion_requested= true;
		NextLexem();
		result.name.push_back(Function::NameComponent{ "", it_->src_loc });
		return result;
	}
	if( it_->type == Lexem::Type::CompletionIdentifier )
	{
		result.name.push_back(Function::NameComponent{ it_->text, it_->src_loc, true });
		NextLexem();
		return result;
	}

	if( it_->type == Lexem::Type::Identifier )
	{
		while( NotEndOfFile() )
		{
			if( it_->type == Lexem::Type::CompletionIdentifier )
			{
				result.name.push_back(Function::NameComponent{ it_->text, it_->src_loc, true });
				NextLexem();
				return result;
			}

			if( it_->type != Lexem::Type::Identifier )
			{
				PushErrorMessage();
				return result;
			}
			result.name.push_back(Function::NameComponent{ it_->text, it_->src_loc });
			result.src_loc= it_->src_loc;
			NextLexem();

			if( it_->type == Lexem::Type::Scope )
			{
				NextLexem();

				if( it_->type == Lexem::Type::Identifier || it_->type == Lexem::Type::CompletionIdentifier )
					continue;
				else
				{
					if( function_definition_lexem == Keywords::op_ )
						break; // Allow op A::+
					else
					{
						PushErrorMessage();
						return result;
					}
				}
			}
			else if( it_->type == Lexem::Type::CompletionScope )
			{
				result.name.push_back(Function::NameComponent{ "", it_->src_loc, true }); // Complete with empty string.
				NextLexem();
				return result;
			}
			else
				break;
		}
	}

	if( function_definition_lexem == Keywords::fn_ )
	{
		if( result.name.empty() )
		{
			PushErrorMessage();
			result.name.push_back(Function::NameComponent{ "dummy", SrcLoc() });
		}
		if( result.name.back().name == Keywords::conversion_constructor_ )
		{
			result.name.back().name= Keyword( Keywords::constructor_ );
			result.is_conversion_constructor= true;
		}
	}
	else
	{
		result.src_loc= it_->src_loc;
		OverloadedOperator overloaded_operator= OverloadedOperator::None;
		switch( it_->type )
		{
		case Lexem::Type::Plus   : overloaded_operator= OverloadedOperator::Add; break;
		case Lexem::Type::Minus  : overloaded_operator= OverloadedOperator::Sub; break;
		case Lexem::Type::Star   : overloaded_operator= OverloadedOperator::Mul; break;
		case Lexem::Type::Slash  : overloaded_operator= OverloadedOperator::Div; break;
		case Lexem::Type::Percent: overloaded_operator= OverloadedOperator::Rem; break;
		case Lexem::Type::CompareEqual: overloaded_operator= OverloadedOperator::CompareEqual; break;
		case Lexem::Type::CompareOrder: overloaded_operator= OverloadedOperator::CompareOrder; break;
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
				return result;
			}
			overloaded_operator= OverloadedOperator::Indexing;
			break;

		case Lexem::Type::BracketLeft:
			NextLexem();
			if( it_->type != Lexem::Type::BracketRight )
			{
				PushErrorMessage();
				return result;
			}
			overloaded_operator= OverloadedOperator::Call;
			break;

		default:
			PushErrorMessage();
			return result;
		};

		result.name.emplace_back( Function::NameComponent{ std::string( OverloadedOperatorToString( overloaded_operator ) ), result.src_loc } );
		result.overloaded_operator= overloaded_operator;

		NextLexem();
	}

	ExpectLexem( Lexem::Type::BracketLeft );

	FunctionParams& params= result.type.params;

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
				return result;
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
				return result;
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
			const SrcLoc& src_loc= it_->src_loc;

			std::string inner_reference_tag;
			if( it_->type == Lexem::Type::Apostrophe )
				inner_reference_tag= ParseInnerReferenceTag();

			if( it_->type == Lexem::Type::Comma )
			{
				NextLexem();
				// Disallov constructions, like "fn f( mut this, ){}"
				if( it_->type == Lexem::Type::BracketRight )
					PushErrorMessage();
			}

			FunctionParam this_argument( src_loc );
			this_argument.name= Keyword( Keywords::this_ );
			this_argument.mutability_modifier= mutability_modifier;
			this_argument.reference_modifier= ReferenceModifier::Reference;
			this_argument.reference_tag= Keyword( Keywords::this_ ); // Implicit set name for tag of "this" to "this".
			this_argument.inner_arg_reference_tag= std::move(inner_reference_tag);
			params.push_back( std::move( this_argument ) );
		}
	}

	while( NotEndOfFile() && it_->type != Lexem::Type::EndOfFile )
	{
		if( it_->type == Lexem::Type::BracketRight )
		{
			NextLexem();
			break;
		}

		params.push_back( ParseFunctionParam() );

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
	if( ( result.name.back().name == Keywords::constructor_ || result.name.back().name == Keywords::destructor_ ) &&
		( params.empty() || params.front().name != Keywords::this_ ) )
	{
		FunctionParam this_argument( result.src_loc );
		this_argument.name= Keyword( Keywords::this_ );
		this_argument.mutability_modifier= MutabilityModifier::Mutable;
		this_argument.reference_modifier= ReferenceModifier::Reference;
		this_argument.reference_tag= Keyword( Keywords::this_ );
		params.insert( params.begin(), std::move( this_argument ) );
	}

	ParseFunctionTypeEnding( result.type );

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
			result.body_kind= it_->text == Keywords::default_ ? Function::BodyKind::BodyGenerationRequired : Function::BodyKind::BodyGenerationDisabled;
		else
		{
			PushErrorMessage();
			return result;
		}
		NextLexem();

		ExpectSemicolon();
	}
	else
	{
		if( it_->type == Lexem::Type::BracketLeft )
		{
			auto constructor_initialization_list= std::make_unique<StructNamedInitializer>( it_->src_loc );
			NextLexem();

			while( NotEndOfFile() && it_->type != Lexem::Type::BracketRight )
			{
				constructor_initialization_list->members_initializers.emplace_back();
				StructNamedInitializer::MemberInitializer& member_initializer= constructor_initialization_list->members_initializers.back();

				if( it_->type == Lexem::Type::Identifier )
				{
					member_initializer.name= it_->text;
					member_initializer.src_loc= it_->src_loc;
				}
				else if( it_->type == Lexem::Type::CompletionIdentifier )
				{
					member_initializer.name= it_->text;
					member_initializer.src_loc= it_->src_loc;
					member_initializer.completion_requested= true;
				}
				else
					PushErrorMessage();

				Initializer& initializer= member_initializer.initializer;

				NextLexem();
				initializer= ParseVariableInitializer();
				if( std::get_if<EmptyVariant>(&initializer) != nullptr )
					PushErrorMessage();

				if( it_->type == Lexem::Type::Comma )
				{
					NextLexem();
					if( it_->type == Lexem::Type::BracketRight )
						break;
				}
				else
					break;
			}
			ExpectLexem( Lexem::Type::BracketRight );

			result.constructor_initialization_list= std::move(constructor_initialization_list);
		}

		if( it_->type == Lexem::Type::BraceLeft )
			result.block= std::make_unique<Block>( ParseBlock() );
		else
		{
			PushErrorMessage();
			return result;
		}
	}

	return result;
}

Class SyntaxAnalyzer::ParseClass()
{
	U_ASSERT( it_->text == Keywords::struct_ || it_->text == Keywords::class_ );
	const bool is_class= it_->text == Keywords::class_;
	NextLexem();

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage();
		return Class( it_->src_loc );
	}
	std::string name= it_->text;
	const SrcLoc& class_src_loc= it_->src_loc;
	NextLexem();

	ClassKindAttribute class_kind_attribute= ClassKindAttribute::Struct;
	std::vector<ComplexName> parents_list;
	if( is_class )
	{
		class_kind_attribute= TryParseClassKindAttribute();
		parents_list= TryParseClassParentsList();
	}
	NonSyncTag non_sync_tag= TryParseNonSyncTag();
	const bool keep_fields_order= TryParseClassFieldsOrdered();

	Class result= ParseClassBody();
	result.src_loc= class_src_loc;
	result.name= std::move(name);
	result.kind_attribute_= class_kind_attribute;
	result.non_sync_tag= std::move(non_sync_tag);
	result.keep_fields_order= keep_fields_order;
	result.parents= std::move(parents_list);

	return result;
}

ClassElementsList SyntaxAnalyzer::ParseClassBodyElements()
{
	ClassElementsList::Builder result_builder;

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::BraceRight )
			break;
		else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::fn_ || it_->text == Keywords::op_ ) )
		{
			result_builder.Append( ParseFunction() );
		}
		else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::struct_ || it_->text == Keywords::class_ ) )
		{
			result_builder.Append( ParseClass() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ )
		{
			result_builder.Append( ParseVariablesDeclaration() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ )
		{
			result_builder.Append( ParseAutoVariableDeclaration() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ )
		{
			result_builder.Append( ParseStaticAssert() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::enum_ )
		{
			result_builder.Append( ParseEnum() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::type_ )
		{
			result_builder.Append( ParseTypeAlias() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ )
		{
			TemplateVar template_= ParseTemplate();
			
			if( auto* const type_template= std::get_if<TypeTemplate>(&template_) )
				result_builder.Append( std::move( *type_template ) );
			else if( auto* const function_template= std::get_if<FunctionTemplate>(&template_) )
				result_builder.Append( std::move( *function_template ) );
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

			result_builder.Append( ClassVisibilityLabel( it_->src_loc, visibility ) );

			NextLexem();
			ExpectLexem( Lexem::Type::Colon );
		}
		else
		{
			if( it_->type == Lexem::Type::Identifier )
			{
				if( const Macro* const macro= FetchMacro( it_->text, Macro::Context::Class ) )
				{
					result_builder.AppendList( ExpandMacro( *macro, &SyntaxAnalyzer::ParseClassBodyElements ) );
					continue;
				}
			}

			ClassField field( it_->src_loc );

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
				field.src_loc= it_->src_loc;
				NextLexem();
			}
			else
			{
				result_builder.Append( std::move( field ) );
				PushErrorMessage();
				TryRecoverAfterError( g_class_body_elements_control_lexems );
				continue;
			}

			Initializer field_initializer= ParseVariableInitializer();
			if( std::get_if<EmptyVariant>( &field_initializer ) == nullptr )
				field.initializer= std::make_unique<Initializer>( std::move(field_initializer) );

			result_builder.Append( std::move( field ) );

			if( it_->type == Lexem::Type::Semicolon )
				NextLexem();
			else
			{
				PushErrorMessage();
				TryRecoverAfterError( g_class_body_elements_control_lexems );
			}
		}
	}

	return result_builder.Build();
}

Class SyntaxAnalyzer::ParseClassBody()
{
	Class result( it_->src_loc );

	if( it_->type == Lexem::Type::BraceLeft )
	{
		NextLexem();
	}
	else
	{
		PushErrorMessage();
		return result;
	}

	result.elements= ParseClassBodyElements();

	ExpectLexem( Lexem::Type::BraceRight );

	return result;
}

SyntaxAnalyzer::TemplateVar SyntaxAnalyzer::ParseTemplate()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ );
	const SrcLoc& template_src_loc= it_->src_loc;
	NextLexem();

	// TemplateBase parameters
	std::vector<TemplateBase::Param> params;

	ExpectLexem( Lexem::Type::TemplateBracketLeft );

	while( NotEndOfFile() )
	{
		if( it_->type == Lexem::Type::TemplateBracketRight )
		{
			NextLexem();
			break;
		}

		params.emplace_back();

		if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::type_ )
			NextLexem();
		else
			params.back().param_type= ParseComplexName();

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage();
			return EmptyVariant();
		}

		params.back().name= it_->text;
		params.back().src_loc= it_->src_loc;
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
		TypeAlias,
	};
	TemplateKind template_kind= TemplateKind::Invalid;

	std::string name;
	const SrcLoc& template_thing_src_loc= it_->src_loc;
	SrcLoc template_name_src_loc= it_->src_loc;
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
		template_name_src_loc= it_->src_loc;
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
		template_name_src_loc= it_->src_loc;
		NextLexem();
		template_kind= TemplateKind::TypeAlias;
	}
	else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::fn_ || it_->text == Keywords::op_ ) )
	{
		FunctionTemplate function_template( template_src_loc );
		function_template.params= std::move(params);
		function_template.function= std::make_unique<Function>(ParseFunction());
		function_template.src_loc= function_template.function->src_loc;
		return std::move(function_template);
	}
	else
	{
		PushErrorMessage();
		return EmptyVariant();
	}

	// TypeTemplateBase parameters
	std::vector<TypeTemplate::SignatureParam> signature_params;
	bool is_short_form= false;

	if( it_->type == Lexem::Type::TemplateBracketLeft )
	{
		// Parse signature params
		NextLexem();
		while( NotEndOfFile() )
		{
			if( it_->type == Lexem::Type::TemplateBracketRight )
			{
				NextLexem();
				break;
			}

			signature_params.emplace_back();
			signature_params.back().name= ParseExpression();

			if( it_->type == Lexem::Type::Assignment )
			{
				NextLexem();
				signature_params.back().default_value= ParseExpression();
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
			TypeTemplate class_template( template_name_src_loc );
			class_template.params= std::move(params);
			class_template.signature_params= std::move(signature_params);
			class_template.name= name;
			class_template.is_short_form= is_short_form;

			ClassKindAttribute class_kind_attribute= ClassKindAttribute::Struct;
			std::vector<ComplexName> class_parents_list;
			if( template_kind == TemplateKind::Class )
			{
				class_kind_attribute= TryParseClassKindAttribute();
				class_parents_list= TryParseClassParentsList();
			}
			NonSyncTag non_sync_tag= TryParseNonSyncTag();
			const bool keep_fields_order= TryParseClassFieldsOrdered();

			Class class_= ParseClassBody();
			class_.src_loc= template_thing_src_loc;
			class_.name= "_"; // // Give special name for all template classes
			class_.kind_attribute_= class_kind_attribute;
			class_.non_sync_tag= std::move(non_sync_tag);
			class_.keep_fields_order= keep_fields_order;
			class_.parents= std::move(class_parents_list);
			class_template.something= std::make_unique<Class>(std::move(class_));
			return std::move(class_template);
		}

	case TemplateKind::TypeAlias:
		{
			TypeTemplate typedef_template( template_name_src_loc );
			typedef_template.params= std::move(params);
			typedef_template.signature_params= std::move(signature_params);
			typedef_template.name= name;
			typedef_template.is_short_form= is_short_form;

			auto type_alias= std::make_unique<TypeAlias>( ParseTypeAliasBody() );
			type_alias->name= std::move(name);

			typedef_template.something= std::move(type_alias);
			return std::move(typedef_template);
		}

	case TemplateKind::Invalid:
		break;
	};

	U_ASSERT(false);
	return EmptyVariant();
}

const Macro* SyntaxAnalyzer::FetchMacro( const std::string& macro_name, const Macro::Context context )
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
	const SrcLoc& expansion_src_loc = it_->src_loc;
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == macro.name );
	NextLexem();

	auto elements_map= MatchMacroBlock( macro.match_template_elements, macro.name );
	if( !elements_map.has_value() )
		return ParseFnResult();

	MacroNamesMap names_map;
	names_map.names= &*elements_map;

	ProgramStringMap<std::string> unique_macro_identifier_map;

	// Append expansion point line/column in order to make macro unique identifiers unique in different macto expansions.
	// Use only line/column and not use file index/macro expansion index, bacause we need to produce same result for file imported in different files.
	std::string macro_unique_identifiers_base_name= macro_unique_identifiers_base_name_;
	macro_unique_identifiers_base_name+= "_l";
	macro_unique_identifiers_base_name+= std::to_string( expansion_src_loc.GetLine() );
	macro_unique_identifiers_base_name+= "_c";
	macro_unique_identifiers_base_name+= std::to_string( expansion_src_loc.GetColumn() );

	Lexems result_lexems=
		DoExpandMacro(
			names_map,
			macro.result_template_elements,
			unique_macro_identifier_map,
			macro_unique_identifiers_base_name );

	Lexem eof;
	eof.type= Lexem::Type::EndOfFile;
	result_lexems.push_back(eof);

	const uint32_t macro_expansion_index= uint32_t(macro_expansion_contexts_->size());
	MacroExpansionContext macro_expansion_context;
	macro_expansion_context.macro_name= macro.name;
	macro_expansion_context.macro_declaration_src_loc= macro.src_loc;
	macro_expansion_context.src_loc= expansion_src_loc;
	macro_expansion_contexts_->push_back(macro_expansion_context);
	for( Lexem& lexem : result_lexems )
		lexem.src_loc.SetMacroExpansionIndex( macro_expansion_index );

	SyntaxAnalyzer result_analyzer( macros_, macro_expansion_contexts_ );
	result_analyzer.it_= result_lexems.begin();
	result_analyzer.it_end_= result_lexems.end();
	result_analyzer.last_error_it_= result_lexems.end();
	// For subsequent macro expansions use base name that contains also root expansion point (and more - full expansion path).
	result_analyzer.macro_unique_identifiers_base_name_= macro_unique_identifiers_base_name;

	auto element= (result_analyzer.*parse_fn)();
	error_messages_.insert( error_messages_.end(), result_analyzer.error_messages_.begin(), result_analyzer.error_messages_.end() );
	return std::move(element);
}

std::optional<SyntaxAnalyzer::MacroVariablesMap> SyntaxAnalyzer::MatchMacroBlock(
	const Macro::MatchElements& match_elements,
	const std::string& macro_name )
{
	MacroVariablesMap out_elements;

	const auto push_macro_error=
	[&]
	{
		LexSyntError msg;
		msg.src_loc= it_->src_loc;
		msg.text= "Unexpected lexem - \"" + it_->text + "\". (In expansion of macro \"" + macro_name + "\")";
		error_messages_.push_back(std::move(msg));
	};

	for( size_t i= 0u; i < match_elements.size(); ++i )
	{
		const Macro::MatchElement& match_element= match_elements[i];
		const Macro::MatchElement* const next_match_element= (i + 1u < match_elements.size()) ? &match_elements[i + 1] : nullptr;

		ParsedMacroElement element;
		element.kind= match_element.kind;
		element.begin= it_;

		switch( match_element.kind )
		{
		case Macro::MatchElementKind::Lexem:
			if( it_->type == match_element.lexem.type && it_->text == match_element.lexem.text )
				NextLexem();
			else
			{
				push_macro_error();
				return std::nullopt;
			}
			break;

		case Macro::MatchElementKind::Identifier:
			if( it_->type == Lexem::Type::Identifier )
				NextLexem();
			else
			{
				push_macro_error();
				return std::nullopt;
			}
			break;

		case Macro::MatchElementKind::Typename:
			ParseTypeName();
			break;

		case Macro::MatchElementKind::Expression:
			{
				const Expression expression= ParseExpression();
				if( std::get_if<EmptyVariant>( &expression ) != nullptr )
				{
					push_macro_error();
					return std::nullopt;
				}
			}
			break;

		case Macro::MatchElementKind::Block:
			ParseBlock();
			break;

		case Macro::MatchElementKind::IfAlternative:
			ParseIfAlternative();
			break;

		case Macro::MatchElementKind::Optional:
			{
				bool has_value= true;

				if( next_match_element != nullptr && match_element.block_check_lexem_kind == Macro::BlockCheckLexemKind::LexemAfterBlockEnd )
				{
					const Lexem& terminator_lexem= next_match_element->lexem;
					has_value= !(it_->type == terminator_lexem.type && it_->text == terminator_lexem.text);
				}
				else if( match_element.block_check_lexem_kind == Macro::BlockCheckLexemKind::LexemAtBlockStart )
				{
					U_ASSERT( ! match_element.sub_elements.empty() && match_element.sub_elements.front().kind == Macro::MatchElementKind::Lexem );
					const Lexem& check_lexem= match_element.sub_elements.front().lexem;
					has_value= it_->type == check_lexem.type && it_->text == check_lexem.text;
				}
				else U_ASSERT(false);

				if( has_value )
				{
					auto optional_elements= MatchMacroBlock( match_element.sub_elements, macro_name );
					if( optional_elements.has_value() )
						element.sub_elements.push_back( std::move(*optional_elements) );
					else
						return std::nullopt;
				}
			}
			break;

		case Macro::MatchElementKind::Repeated:
			{
				if( next_match_element != nullptr && match_element.block_check_lexem_kind == Macro::BlockCheckLexemKind::LexemAfterBlockEnd )
				{
					const Lexem& terminator_lexem= next_match_element->lexem;
					while(NotEndOfFile())
					{
						if( it_->type == terminator_lexem.type && it_->text == terminator_lexem.text )
							break;

						auto repeated_elements= MatchMacroBlock( match_element.sub_elements, macro_name );
						if( repeated_elements.has_value() )
							element.sub_elements.push_back( std::move(*repeated_elements) );
						else
							return std::nullopt;

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
									return std::nullopt;
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

						auto repeated_elements= MatchMacroBlock( match_element.sub_elements, macro_name );
						if( repeated_elements.has_value() )
							element.sub_elements.push_back( std::move(*repeated_elements) );
						else
							return std::nullopt;

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
									return std::nullopt;
								}
							}
							else
								break;
						}
					}
				}
				else U_ASSERT(false);
			}
			break;
		}

		element.end= it_;
		out_elements[match_element.name]= std::move(element);
	}

	return out_elements;
}

Lexems SyntaxAnalyzer::DoExpandMacro(
	const MacroNamesMap& parsed_elements,
	const Macro::ResultElements& result_elements,
	ProgramStringMap<std::string>& unique_macro_identifier_map,
	const std::string& macro_unique_identifiers_base_name )
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
				l.src_loc= result_element.lexem.src_loc;

				const auto it= unique_macro_identifier_map.find( result_element.lexem.text );
				if( it != unique_macro_identifier_map.end() )
					l.text= it->second;
				else
				{
					U_ASSERT( result_element.lexem.text.size() > 2u && result_element.lexem.text[0] == '?' && result_element.lexem.text[1] == '?' );

					l.text= "_macro_ident_";
					l.text+= result_element.lexem.text.substr(2u);
					l.text+= "_";
					l.text+= macro_unique_identifiers_base_name;
					l.text+= "_";
					l.text+= std::to_string( unique_macro_identifier_map.size() );

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
					LexSyntError msg;
					msg.src_loc= result_element.lexem.src_loc;
					msg.text= result_element.name + " not found";
					error_messages_.push_back( std::move(msg) );
					return result_lexems;
				}

				result_lexems.insert( result_lexems.end(), element->begin, element->end );
			}
			break;

		case Macro::ResultElementKind::VariableElementWithMacroBlock:
			{
				const auto element= parsed_elements.GetElement( result_element.name );
				if( element == nullptr )
				{
					LexSyntError msg;
					msg.src_loc= result_element.lexem.src_loc;
					msg.text= result_element.name + " not found";
					error_messages_.push_back( std::move(msg) );
					return result_lexems;
				}

				if( element->kind == Macro::MatchElementKind::Optional || element->kind == Macro::MatchElementKind::Repeated )
				{
					for( const auto& sub_elements : element->sub_elements )
					{
						MacroNamesMap sub_elements_map;
						sub_elements_map.prev= &parsed_elements;
						sub_elements_map.names= &sub_elements;

						Lexems element_lexems=
							DoExpandMacro(
								sub_elements_map,
								result_element.sub_elements,
								unique_macro_identifier_map,
								macro_unique_identifiers_base_name );
						result_lexems.insert( result_lexems.end(), element_lexems.begin(), element_lexems.end() );

						// Push separator.
						if( &sub_elements != &element->sub_elements.back() && result_element.lexem.type != Lexem::Type::EndOfFile )
							result_lexems.push_back( result_element.lexem );
					}
				}
				else
				{
					LexSyntError msg;
					msg.src_loc= result_element.lexem.src_loc;
					msg.text= "Expected optional or repated.";
					error_messages_.push_back( std::move(msg) );
					return result_lexems;
				}
			}
			break;
		};
	}

	return result_lexems;
}

void SyntaxAnalyzer::ExpectSemicolon()
{
	ExpectLexem( Lexem::Type::Semicolon );
}

void SyntaxAnalyzer::ExpectLexem( const Lexem::Type lexem_type )
{
	if( it_->type == lexem_type )
		NextLexem();
	else
		PushErrorMessage();
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
	if( error_messages_.empty() || error_messages_.back().src_loc != it_->src_loc )
	{
		LexSyntError error_message;
		error_message.src_loc= it_->src_loc;

		if( it_->type == Lexem::Type::Number )
		{
			// Process numbers specially since text of number lexem is not a text but binary struct data.
			error_message.text= "Syntax error - unexpected number lexem";
		}
		else
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

SyntaxAnalysisResult SyntaxAnalysis(
	const Lexems& lexems,
	MacrosByContextMap macros,
	const MacroExpansionContextsPtr& macro_expansion_contexts,
	std::string source_file_contents_hash )
{
	SyntaxAnalyzer syntax_analyzer(
		std::make_shared<MacrosByContextMap>( std::move(macros) ),
		macro_expansion_contexts );

	return syntax_analyzer.DoAnalyzis( lexems, std::move(source_file_contents_hash) );
}

} // namespace Synt

} // namespace U
