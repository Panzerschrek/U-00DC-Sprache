#include "../lex_synt_lib/assert.hpp"
#include "code_builder_types.hpp"
#include "value.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

Variable::Variable(
	Type in_type,
	const Location in_location, const ValueType in_value_type,
	llvm::Value* const in_llvm_value, llvm::Constant* const in_constexpr_value )
	: type(std::move(in_type)), location(in_location), value_type(in_value_type)
	, llvm_value(in_llvm_value), constexpr_value(in_constexpr_value)
{}

//
// ThisOverloadedMethodsSet
//

ThisOverloadedMethodsSet::ThisOverloadedMethodsSet()
	: overloaded_methods_set_(new OverloadedFunctionsSet() )
{}

ThisOverloadedMethodsSet::ThisOverloadedMethodsSet( const ThisOverloadedMethodsSet& other )
	: this_(other.this_), overloaded_methods_set_( new OverloadedFunctionsSet( *other.overloaded_methods_set_ ) )
{}

ThisOverloadedMethodsSet& ThisOverloadedMethodsSet::operator=( const ThisOverloadedMethodsSet& other )
{
	this->this_= other.this_;
	*this->overloaded_methods_set_= *other.overloaded_methods_set_;
	return *this;
}

OverloadedFunctionsSet& ThisOverloadedMethodsSet::GetOverloadedFunctionsSet()
{
	return *overloaded_methods_set_;
}

const OverloadedFunctionsSet& ThisOverloadedMethodsSet::GetOverloadedFunctionsSet() const
{
	return *overloaded_methods_set_;
}

//
// Value
//

static_assert( sizeof(Value) <= 160u, "Value is too heavy!" );

Value::Value()
{}

Value::Value( Variable variable, const FilePos& file_pos )
	: file_pos_(file_pos)
{
	something_= std::move(variable);
}

Value::Value( FunctionVariable function_variable )
{
	something_= std::move(function_variable);
}

Value::Value( OverloadedFunctionsSet functions_set )
{
	something_= std::move(functions_set);
}

Value::Value( Type type, const FilePos& file_pos )
	: file_pos_(file_pos)
{
	something_= std::move(type);
}

Value::Value( ClassField class_field, const FilePos& file_pos )
	: file_pos_(file_pos)
{
	something_= std::move( class_field );
}

Value::Value( ThisOverloadedMethodsSet this_overloaded_methods_set )
{
	something_= std::move( this_overloaded_methods_set );
}

Value::Value( const NamesScopePtr& namespace_, const FilePos& file_pos )
	: file_pos_(file_pos)
{
	U_ASSERT( namespace_ != nullptr );
	something_= namespace_;
}

Value::Value( TypeTemplatesSet type_templates, const FilePos& file_pos )
	: file_pos_(file_pos)
{
	something_= std::move(type_templates);
}


Value::Value( StaticAssert static_assert_, const FilePos& file_pos )
	: file_pos_(file_pos)
{
	something_= std::move(static_assert_);
}

Value::Value( Typedef typedef_, const FilePos& file_pos )
	: file_pos_(file_pos)
{
	something_= std::move(typedef_);
}

Value::Value( IncompleteGlobalVariable incomplete_global_variable, const FilePos& file_pos )
	: file_pos_(file_pos)
{
	something_= std::move(incomplete_global_variable);
}

Value::Value( YetNotDeducedTemplateArg yet_not_deduced_template_arg )
{
	something_= std::move(yet_not_deduced_template_arg);
}

Value::Value( ErrorValue error_value )
{
	something_= std::move(error_value);
}

size_t Value::GetKindIndex() const
{
	return something_.index();
}

ProgramString Value::GetKindName() const
{
	struct Visitor final
	{
		ProgramString operator()( const Variable& ) const { return "variable"_SpC; }
		ProgramString operator()( const FunctionVariable& ) const { return "function variable"_SpC; }
		ProgramString operator()( const OverloadedFunctionsSet& ) const { return "functions set"_SpC; }
		ProgramString operator()( const Type& ) const { return "typename"_SpC; }
		ProgramString operator()( const ClassField& ) const { return "class field"_SpC; }
		ProgramString operator()( const ThisOverloadedMethodsSet& ) const { return "this + functions set"_SpC; }
		ProgramString operator()( const NamesScopePtr& ) const { return "namespace"_SpC; }
		ProgramString operator()( const TypeTemplatesSet& ) const { return "type templates set"_SpC; }
		ProgramString operator()( const StaticAssert& ) const { return "static assert"_SpC; }
		ProgramString operator()( const Typedef& ) const { return "incomplete typedef"_SpC; }
		ProgramString operator()( const IncompleteGlobalVariable& ) const { return "incomplete global variable"_SpC; }
		ProgramString operator()( const YetNotDeducedTemplateArg& ) const { return "yet not deduced template arg"_SpC; }
		ProgramString operator()( const ErrorValue& ) const { return "error value"_SpC; }
	};

	return std::visit( Visitor(), something_ );
}

const FilePos& Value::GetFilePos() const
{
	return file_pos_;
}

bool Value::IsTemplateParameter() const
{
	return is_template_parameter_;
}

void Value::SetIsTemplateParameter( const bool is_template_parameter )
{
	is_template_parameter_= is_template_parameter;
}

Variable* Value::GetVariable()
{
	return std::get_if<Variable>( &something_ );
}

const Variable* Value::GetVariable() const
{
	return std::get_if<Variable>( &something_ );
}

FunctionVariable* Value::GetFunctionVariable()
{
	return std::get_if<FunctionVariable>( &something_ );
}

const FunctionVariable* Value::GetFunctionVariable() const
{
	return std::get_if<FunctionVariable>( &something_ );
}

OverloadedFunctionsSet* Value::GetFunctionsSet()
{
	return std::get_if<OverloadedFunctionsSet>( &something_ );
}

const OverloadedFunctionsSet* Value::GetFunctionsSet() const
{
	return std::get_if<OverloadedFunctionsSet>( &something_ );
}

Type* Value::GetTypeName()
{
	return std::get_if<Type>( &something_ );
}

const Type* Value::GetTypeName() const
{
	return std::get_if<Type>( &something_ );
}

ClassField* Value::GetClassField()
{
	return std::get_if<ClassField>( &something_ );
}

const ClassField* Value::GetClassField() const
{
	return std::get_if<ClassField>( &something_ );
}

ThisOverloadedMethodsSet* Value::GetThisOverloadedMethodsSet()
{
	return std::get_if<ThisOverloadedMethodsSet>( &something_ );
}

const ThisOverloadedMethodsSet* Value::GetThisOverloadedMethodsSet() const
{
	return std::get_if<ThisOverloadedMethodsSet>( &something_ );
}

NamesScopePtr Value::GetNamespace() const
{
	const NamesScopePtr* const namespace_= std::get_if<NamesScopePtr>( &something_ );
	if( namespace_ == nullptr )
		return nullptr;
	return *namespace_;
}

TypeTemplatesSet* Value::GetTypeTemplatesSet()
{
	return std::get_if<TypeTemplatesSet>( &something_ );
}

const TypeTemplatesSet* Value::GetTypeTemplatesSet() const
{
	return std::get_if<TypeTemplatesSet>( &something_ );
}

StaticAssert* Value::GetStaticAssert()
{
	return std::get_if<StaticAssert>( &something_ );
}

const StaticAssert* Value::GetStaticAssert() const
{
	return std::get_if<StaticAssert>( &something_ );
}

Typedef* Value::GetTypedef()
{
	return std::get_if<Typedef>( &something_ );
}

const Typedef* Value::GetTypedef() const
{
	return std::get_if<Typedef>( &something_ );
}

IncompleteGlobalVariable* Value::GetIncompleteGlobalVariable()
{
	return std::get_if<IncompleteGlobalVariable>( &something_ );
}

const IncompleteGlobalVariable* Value::GetIncompleteGlobalVariable() const
{
	return std::get_if<IncompleteGlobalVariable>( &something_ );
}

YetNotDeducedTemplateArg* Value::GetYetNotDeducedTemplateArg()
{
	return std::get_if<YetNotDeducedTemplateArg>( &something_ );
}

const YetNotDeducedTemplateArg* Value::GetYetNotDeducedTemplateArg() const
{
	return std::get_if<YetNotDeducedTemplateArg>( &something_ );
}

ErrorValue* Value::GetErrorValue()
{
	return std::get_if<ErrorValue>( &something_ );
}

const ErrorValue* Value::GetErrorValue() const
{
	return std::get_if<ErrorValue>( &something_ );
}

} //namespace CodeBuilderPrivate

} // namespace U
