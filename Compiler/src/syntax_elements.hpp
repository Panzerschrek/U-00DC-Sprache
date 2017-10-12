#pragma once
#include <limits>
#include <memory>
#include <vector>

#include <boost/optional.hpp>

#include "lexical_analyzer.hpp"

namespace U
{

class SyntaxElementBase
{
public:
	explicit SyntaxElementBase( const FilePos& file_pos );
	virtual ~SyntaxElementBase()= default;

	FilePos file_pos_;
};

class IExpressionComponent;
typedef std::unique_ptr<IExpressionComponent> IExpressionComponentPtr;
class ExpressionComponentWithUnaryOperators;
typedef std::unique_ptr<ExpressionComponentWithUnaryOperators> ExpressionComponentWithUnaryOperatorsPtr;

class IUnaryPrefixOperator ;
class IUnaryPostfixOperator;

typedef std::unique_ptr<IUnaryPrefixOperator > IUnaryPrefixOperatorPtr ;
typedef std::unique_ptr<IUnaryPostfixOperator> IUnaryPostfixOperatorPtr;

typedef std::vector<IUnaryPrefixOperatorPtr > PrefixOperators ;
typedef std::vector<IUnaryPostfixOperatorPtr> PostfixOperators;

class IInitializer;
typedef std::unique_ptr<IInitializer> IInitializerPtr;

struct ComplexName final
{
	// A
	// A::b
	// TheClass::Method
	// ::Absolute::Name
	// ::C_Function
	// std::vector</i32/>
	// std::map</f32, T/>::value_type

	// If first component name is empty, name starts with "::".
	struct Component
	{
		ProgramString name;
		std::vector<IExpressionComponentPtr> template_parameters;
		bool have_template_parameters= false;

		// True, if complex name generated by CodeBuilder, not SyntaxAnalyzer.
		bool is_generated= false;
	};
	std::vector<Component> components;
};

struct TypeName
{
	// [ [i32, 5] 7 ]
	ComplexName name; // Can be empty in some cases.
	std::vector< IExpressionComponentPtr > array_sizes;

	// Compiler so stupid - can not generate move constructors without noexcept. Make help for it.
	TypeName() = default;
	TypeName( TypeName&& ) noexcept = default;
	TypeName( const TypeName& )= default;

	TypeName& operator=( const TypeName& )= default;
	TypeName& operator=( TypeName&& )= default;
};

class IUnaryPrefixOperator
{
public:
	virtual ~IUnaryPrefixOperator()= default;
};

class IUnaryPostfixOperator
{
public:
	virtual ~IUnaryPostfixOperator()= default;
};

class UnaryPlus final : public SyntaxElementBase, public IUnaryPrefixOperator
{
public:
	explicit UnaryPlus( const FilePos& file_pos );
};

class UnaryMinus final : public SyntaxElementBase, public IUnaryPrefixOperator
{
public:
	explicit UnaryMinus( const FilePos& file_pos );
};

class LogicalNot final : public SyntaxElementBase, public IUnaryPrefixOperator
{
public:
	explicit LogicalNot( const FilePos& file_pos );
};

class BitwiseNot final : public SyntaxElementBase, public IUnaryPrefixOperator
{
public:
	explicit BitwiseNot( const FilePos& file_pos );
};

class CallOperator final : public SyntaxElementBase, public IUnaryPostfixOperator
{
public:
	CallOperator(
		const FilePos& file_pos,
		std::vector<IExpressionComponentPtr> arguments );

	const std::vector<IExpressionComponentPtr> arguments_;
};

class IndexationOperator final : public SyntaxElementBase, public IUnaryPostfixOperator
{
public:
	explicit IndexationOperator( const FilePos& file_pos, IExpressionComponentPtr index );

	const IExpressionComponentPtr index_;
};

class MemberAccessOperator final : public SyntaxElementBase, public IUnaryPostfixOperator
{
public:
	MemberAccessOperator( const FilePos& file_pos, ProgramString member_name );

	const ProgramString member_name_;
};

enum class BinaryOperatorType
{
	Add,
	Sub,
	Mul,
	Div,
	Rem,

	Equal,
	NotEqual,
	Less,
	LessEqual,
	Greater,
	GreaterEqual,

	And,
	Or,
	Xor,

	ShiftLeft ,
	ShiftRight,

	LazyLogicalAnd,
	LazyLogicalOr,

	Last,
};

ProgramString BinaryOperatorToString( BinaryOperatorType op );

class IExpressionComponent
{
public:
	virtual ~IExpressionComponent()= default;

	const FilePos& GetFilePos() const;
};

class IInitializer
{
public:
	virtual ~IInitializer()= default;

	const FilePos& GetFilePos() const;
};

class ArrayInitializer final : public SyntaxElementBase, public IInitializer
{
public:
	explicit ArrayInitializer( const FilePos& file_pos );

	std::vector<IInitializerPtr> initializers;
	bool has_continious_initializer= false; // ... after last initializator.
};

class StructNamedInitializer final : public SyntaxElementBase, public IInitializer
{
public:
	explicit StructNamedInitializer( const FilePos& file_pos );

	struct MemberInitializer
	{
		ProgramString name;
		IInitializerPtr initializer;
	};

	std::vector<MemberInitializer> members_initializers;
};

class ConstructorInitializer final : public SyntaxElementBase, public IInitializer
{
public:
	ConstructorInitializer(
		const FilePos& file_pos,
		std::vector<IExpressionComponentPtr> arguments );

	const CallOperator call_operator;
};

class ExpressionInitializer final : public SyntaxElementBase, public IInitializer
{
public:
	ExpressionInitializer( const FilePos& file_pos, IExpressionComponentPtr expression );

	IExpressionComponentPtr expression;
};

class ZeroInitializer final : public SyntaxElementBase, public IInitializer
{
public:
	explicit ZeroInitializer( const FilePos& file_pos );
};

class BinaryOperator final : public SyntaxElementBase, public IExpressionComponent
{
public:
	explicit BinaryOperator( const FilePos& file_pos );

	BinaryOperatorType operator_type_;
	IExpressionComponentPtr left_;
	IExpressionComponentPtr right_;
};

class ExpressionComponentWithUnaryOperators : public SyntaxElementBase, public IExpressionComponent
{
public:
	explicit ExpressionComponentWithUnaryOperators( const FilePos& file_pos );

	std::vector<IUnaryPrefixOperatorPtr > prefix_operators_ ;
	std::vector<IUnaryPostfixOperatorPtr> postfix_operators_;
};

class NamedOperand final : public ExpressionComponentWithUnaryOperators
{
public:
	NamedOperand( const FilePos& file_pos, ComplexName name );

	const ComplexName name_;
};

class BooleanConstant final : public ExpressionComponentWithUnaryOperators
{
public:
	BooleanConstant( const FilePos& file_pos, bool value );

	const bool value_;
};

class NumericConstant final : public ExpressionComponentWithUnaryOperators
{
public:
	typedef long double LongFloat;
	static_assert(
		std::numeric_limits<LongFloat>::digits >= 64,
		"Too short \"LongFloat\". LongFloat must store all uint64_t and int64_t values exactly." );

	NumericConstant(
		const FilePos& file_pos,
		LongFloat value,
		ProgramString type_suffix,
		bool has_fractional_point );

	const LongFloat value_;
	const ProgramString type_suffix_;
	const bool has_fractional_point_;
};

class BracketExpression final : public ExpressionComponentWithUnaryOperators
{
public:
	BracketExpression( const FilePos& file_pos, IExpressionComponentPtr expression );

	const IExpressionComponentPtr expression_;
};

class TypeNameInExpression final : public ExpressionComponentWithUnaryOperators
{
public:
	explicit TypeNameInExpression( const FilePos& file_pos );

	TypeName type_name;
};

class IProgramElement
{
public:
	virtual ~IProgramElement(){}
};

typedef std::unique_ptr<IProgramElement> IProgramElementPtr;
typedef std::vector<IProgramElementPtr> ProgramElements;

class IClassElement
{
public:
	virtual ~IClassElement(){}
};

typedef std::unique_ptr<IClassElement> IClassElementPtr;
typedef std::vector<IClassElementPtr> ClassElements;

class IBlockElement
{
public:
	virtual ~IBlockElement()= default;

	const FilePos& GetFilePos() const;
};

typedef std::unique_ptr<IBlockElement> IBlockElementPtr;
typedef std::vector<IBlockElementPtr> BlockElements;

class Block final : public SyntaxElementBase, public IBlockElement
{
public:
	Block( const FilePos& file_pos, BlockElements elements );

public:
	const BlockElements elements_;
};

typedef std::unique_ptr<Block> BlockPtr;

enum class MutabilityModifier
{
	None,
	Mutable,
	Immutable,
	Constexpr,
};

enum class ReferenceModifier
{
	None,
	Reference,
	// SPRACE_TODO - add "move" references here
};

struct VariablesDeclaration final
	: public SyntaxElementBase
	, public IBlockElement
	, public IProgramElement
	, public IClassElement
{
	VariablesDeclaration( const FilePos& file_pos );
	VariablesDeclaration( const VariablesDeclaration& )= delete;
	VariablesDeclaration( VariablesDeclaration&& other );

	VariablesDeclaration operator=( const VariablesDeclaration& )= delete;
	VariablesDeclaration& operator=( VariablesDeclaration&& other );

	struct VariableEntry
	{
		ProgramString name;
		IInitializerPtr initializer; // May be null for types with default constructor.
		MutabilityModifier mutability_modifier= MutabilityModifier::None;
		ReferenceModifier reference_modifier= ReferenceModifier::None;
	};

	std::vector<VariableEntry> variables;
	TypeName type;
};

typedef std::unique_ptr<VariablesDeclaration> VariablesDeclarationPtr;

struct AutoVariableDeclaration final
	: public SyntaxElementBase
	, public IBlockElement
	, public IProgramElement
	, public IClassElement
{
	explicit AutoVariableDeclaration( const FilePos& file_pos );

	ProgramString name;
	IExpressionComponentPtr initializer_expression;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
};

class ReturnOperator final : public SyntaxElementBase, public IBlockElement
{
public:
	ReturnOperator( const FilePos& file_pos, IExpressionComponentPtr expression );

	const IExpressionComponentPtr expression_;
};

class WhileOperator final : public SyntaxElementBase, public IBlockElement
{
public:
	WhileOperator( const FilePos& file_pos, IExpressionComponentPtr condition, BlockPtr block );

	const IExpressionComponentPtr condition_;
	const BlockPtr block_;
};

class BreakOperator final : public SyntaxElementBase, public IBlockElement
{
public:
	explicit BreakOperator( const FilePos& file_pos );
};

class ContinueOperator final : public SyntaxElementBase, public IBlockElement
{
public:
	explicit ContinueOperator( const FilePos& file_pos );
};

class IfOperator final : public SyntaxElementBase, public IBlockElement
{
public:
	struct Branch
	{
		// Condition - nullptr for last if.
		IExpressionComponentPtr condition;
		BlockPtr block;
	};

	IfOperator( const FilePos& file_pos, std::vector<Branch> branches );

	std::vector<Branch> branches_; // else if()
};

class SingleExpressionOperator final : public SyntaxElementBase, public IBlockElement
{
public:
	SingleExpressionOperator( const FilePos& file_pos, IExpressionComponentPtr expression );

	const IExpressionComponentPtr expression_;
};

class AssignmentOperator final : public SyntaxElementBase, public IBlockElement
{
public:
	AssignmentOperator( const FilePos& file_pos, IExpressionComponentPtr l_value, IExpressionComponentPtr r_value );

	IExpressionComponentPtr l_value_;
	IExpressionComponentPtr r_value_;
};

class AdditiveAssignmentOperator final : public SyntaxElementBase, public IBlockElement
{
public:
	explicit AdditiveAssignmentOperator( const FilePos& file_pos );

	IExpressionComponentPtr l_value_;
	IExpressionComponentPtr r_value_;
	BinaryOperatorType additive_operation_;
};

class IncrementOperator final : public SyntaxElementBase, public IBlockElement
{
public:
	explicit IncrementOperator( const FilePos& file_pos );

	IExpressionComponentPtr expression;
};

class DecrementOperator final : public SyntaxElementBase, public IBlockElement
{
public:
	explicit DecrementOperator( const FilePos& file_pos );

	IExpressionComponentPtr expression;
};

class StaticAssert final
	: public SyntaxElementBase
	, public IBlockElement
	, public IProgramElement
	, public IClassElement
{
public:
	explicit StaticAssert( const FilePos& file_pos );

	IExpressionComponentPtr expression;
};

class Typedef final
	: public SyntaxElementBase
	, public IProgramElement
	, public IClassElement
{
public:
	explicit Typedef( const FilePos& file_pos );

	ProgramString name;
	TypeName value;
};

class FunctionArgumentDeclaration final : public SyntaxElementBase
{
public:
	FunctionArgumentDeclaration(
		const FilePos& file_pos,
		ProgramString name,
		TypeName type,
		MutabilityModifier mutability_modifier,
		ReferenceModifier reference_modifier );

public:
	const ProgramString name_;
	const TypeName type_;
	const MutabilityModifier mutability_modifier_;
	const ReferenceModifier reference_modifier_;
};

typedef std::unique_ptr<FunctionArgumentDeclaration> FunctionArgumentDeclarationPtr;
typedef std::vector<FunctionArgumentDeclarationPtr> FunctionArgumentsDeclaration;

class FunctionDeclaration final
	: public SyntaxElementBase
	, public IProgramElement
	, public IClassElement
{
public:
	FunctionDeclaration(
		const FilePos& file_pos,
		ComplexName name,
		TypeName return_type,
		MutabilityModifier return_value_mutability_modifier,
		ReferenceModifier return_value_reference_modifier,
		FunctionArgumentsDeclaration arguments,
		std::unique_ptr<StructNamedInitializer> constructor_initialization_list,
		BlockPtr block );

	const ComplexName name_;
	const TypeName return_type_;
	const MutabilityModifier return_value_mutability_modifier_;
	const ReferenceModifier return_value_reference_modifier_;
	const FunctionArgumentsDeclaration arguments_;
	const std::unique_ptr<StructNamedInitializer> constructor_initialization_list_;
	const BlockPtr block_;
};

class ClassTemplateDeclaration;

class ClassFieldDeclaration final
	: public SyntaxElementBase
	, public IClassElement
{
public:
	explicit ClassFieldDeclaration( const FilePos& file_pos );

	TypeName type;
	ProgramString name;
};

class ClassDeclaration final
	: public SyntaxElementBase
	, public IProgramElement
	, public IClassElement
{
public:
	explicit ClassDeclaration( const FilePos& file_pos );

	ClassElements elements_;
	ComplexName name_;
	bool is_forward_declaration_= false;
};

class TemplateBase : public SyntaxElementBase
{
public:
	explicit TemplateBase( const FilePos& file_pos );

	// For type arguments, like template</ type A, type B />, arg_type is empty.
	// For value arguments, like template</ type A, A x, i32 y />, arg_type is comples name of argument.
	struct Arg
	{
		ComplexName arg_type;
		ProgramString name;
	};

	// Argument in template signature.
	struct SignatureArg
	{
		ComplexName name;
		boost::optional<ComplexName> default_value;
	};

	std::vector<Arg> args_;
	std::vector<SignatureArg> signature_args_;
	ProgramString name_;
};

typedef std::unique_ptr<TemplateBase> TemplateBasePtr;

class ClassTemplateDeclaration final
	: public TemplateBase
	, public IProgramElement
	, public IClassElement
{
public:
	explicit ClassTemplateDeclaration( const FilePos& file_pos );

	std::unique_ptr<ClassDeclaration> class_;
};

class TypedefTemplate final
	: public TemplateBase
	, public IProgramElement
	, public IClassElement
{
public:
	explicit TypedefTemplate( const FilePos& file_pos );

	std::unique_ptr<Typedef> typedef_;
};

class Namespace final
	: public SyntaxElementBase
	, public IProgramElement
{
public:
	explicit Namespace( const FilePos& file_pos );

	ProgramString name_;
	ProgramElements elements_;
};

class Import final : public SyntaxElementBase
{
public:
	explicit Import( const FilePos& file_pos );

	ProgramString import_name;
};

} // namespace U
