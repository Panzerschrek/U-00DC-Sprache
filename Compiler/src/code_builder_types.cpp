#include "boost/functional/hash.hpp"

#include "assert.hpp"
#include "keywords.hpp"

#include "code_builder_types.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

namespace
{

const size_t g_fundamental_types_size[ size_t(U_FundamentalType::LastType) ]=
{
	U_DESIGNATED_INITIALIZER( U_FundamentalType::InvalidType, 0u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Void, 0u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Bool, 1u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i8 , 1u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u8 , 1u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i16, 2u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u16, 2u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i32, 4u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u32, 4u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i64, 8u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u64, 8u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f32, 4u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f64, 8u ),
};

const char g_invalid_type_name_ascii[]= "InvalidType";
const ProgramString g_invalid_type_name= ToProgramString( g_invalid_type_name_ascii );

const char* const g_fundamental_types_names_ascii[ size_t(U_FundamentalType::LastType) ]=
{
	U_DESIGNATED_INITIALIZER( U_FundamentalType::InvalidType, g_invalid_type_name_ascii ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Void,  KeywordAscii( Keywords::void_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Bool, KeywordAscii( Keywords::bool_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i8 , KeywordAscii( Keywords::i8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u8 , KeywordAscii( Keywords::u8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i16, KeywordAscii( Keywords::i16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u16, KeywordAscii( Keywords::u16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i32, KeywordAscii( Keywords::i32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u32, KeywordAscii( Keywords::u32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i64, KeywordAscii( Keywords::i64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u64, KeywordAscii( Keywords::u64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f32, KeywordAscii( Keywords::f32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f64, KeywordAscii( Keywords::f64_ ) ),
};

const ProgramString (&g_fundamental_types_names)[ size_t(U_FundamentalType::LastType) ]=
{
	U_DESIGNATED_INITIALIZER( U_FundamentalType::InvalidType, g_invalid_type_name ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Void,  Keyword( Keywords::void_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Bool, Keyword( Keywords::bool_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i8 , Keyword( Keywords::i8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u8 , Keyword( Keywords::u8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i16, Keyword( Keywords::i16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u16, Keyword( Keywords::u16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i32, Keyword( Keywords::i32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u32, Keyword( Keywords::u32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i64, Keyword( Keywords::i64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u64, Keyword( Keywords::u64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f32, Keyword( Keywords::f32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f64, Keyword( Keywords::f64_ ) ),
};

} // namespace

//
// Fundamental type
//

FundamentalType::FundamentalType(
	const U_FundamentalType in_fundamental_type,
	llvm::Type* const in_llvm_type )
	: fundamental_type(in_fundamental_type)
	, llvm_type(in_llvm_type)
{}

TemplateDependentType::TemplateDependentType( const size_t in_index, llvm::Type* const in_llvm_type )
	: index(in_index)
	, llvm_type( in_llvm_type )
{}

bool operator==( const FundamentalType& r, const FundamentalType& l )
{
	return r.fundamental_type == l.fundamental_type;
}

bool operator!=( const FundamentalType& r, const FundamentalType& l )
{
	return !( r == l );
}

bool operator==( const TemplateDependentType& r, const TemplateDependentType& l )
{
	return r.index == l.index;
}

bool operator!=( const TemplateDependentType& r, const TemplateDependentType& l )
{
	return !( r == l );
}

Type::Type( const Type& other )
{
	*this= other;
}

Type::Type( FundamentalType fundamental_type )
{
	something_= std::move( fundamental_type );
}

Type::Type( const Function& function_type )
{
	something_= FunctionPtr( new Function( function_type ) );
}

Type::Type( Function&& function_type )
{
	something_= FunctionPtr( new Function( std::move( function_type ) ) );
}

Type::Type( const Array& array_type )
{
	something_= ArrayPtr( new Array( array_type ) );
}

Type::Type( Array&& array_type )
{
	something_= ArrayPtr( new Array( std::move( array_type ) ) );
}

Type::Type( ClassPtr class_type )
{
	something_= std::move( class_type );
}

Type::Type( const NontypeStub nontype_strub )
{
	something_= nontype_strub;
}


Type::Type( TemplateDependentType template_dependent_type )
{
	something_= std::move( template_dependent_type );
}

Type& Type::operator=( const Type& other )
{
	struct Visitor final : public boost::static_visitor<>
	{
		Type& this_;

		explicit Visitor( Type& in_this_ )
			: this_(in_this_)
		{}

		void operator()( const FundamentalType& fundamental )
		{
			this_.something_= fundamental;
		}

		void operator()( const FunctionPtr& function )
		{
			U_ASSERT( function != nullptr );
			this_.something_= FunctionPtr( new Function( *function ) );
		}

		void operator()( const ArrayPtr& array )
		{
			U_ASSERT( array != nullptr );
			this_.something_= ArrayPtr( new Array( *array ) );
		}

		void operator()( const ClassPtr& class_ )
		{
			this_.something_= class_;
		}

		void operator()( const NontypeStub& stub )
		{
			this_.something_= stub;
		}

		void operator()( const TemplateDependentType& template_dependent_type )
		{
			this_.something_= template_dependent_type;
		}
	};

	Visitor visitor( *this );
	boost::apply_visitor( visitor, other.something_ );
	return *this;
}

FundamentalType* Type::GetFundamentalType()
{
	return boost::get<FundamentalType>( &something_ );
}

const FundamentalType* Type::GetFundamentalType() const
{
	return boost::get<FundamentalType>( &something_ );
}

Function* Type::GetFunctionType()
{
	FunctionPtr* const function_type= boost::get<FunctionPtr>( &something_ );
	if( function_type == nullptr )
		return nullptr;
	return function_type->get();
}

const Function* Type::GetFunctionType() const
{
	const FunctionPtr* const function_type= boost::get<FunctionPtr>( &something_ );
	if( function_type == nullptr )
		return nullptr;
	return function_type->get();
}

Array* Type::GetArrayType()
{
	ArrayPtr* const array_type= boost::get<ArrayPtr>( &something_ );
	if( array_type == nullptr )
		return nullptr;
	return array_type->get();
}

const Array* Type::GetArrayType() const
{
	const ArrayPtr* const array_type= boost::get<ArrayPtr>( &something_ );
	if( array_type == nullptr )
		return nullptr;
	return array_type->get();
}

ClassPtr Type::GetClassType() const
{
	const ClassPtr* const class_type= boost::get<ClassPtr>( &something_ );
	if( class_type == nullptr )
		return nullptr;
	return *class_type;
}

TemplateDependentType* Type::GetTemplateDependentType()
{
	return boost::get<TemplateDependentType>( &something_ );
}

const TemplateDependentType* Type::GetTemplateDependentType() const
{
	return boost::get<TemplateDependentType>( &something_ );
}

size_t Type::SizeOf() const
{
	struct Visitor final : public boost::static_visitor<>
	{
		size_t size= 1u;

		void operator()( const FundamentalType& fundamental )
		{
			size= g_fundamental_types_size[ size_t( fundamental.fundamental_type ) ];
		}

		void operator()( const FunctionPtr& function )
		{
			if( function == nullptr ) return;
			U_ASSERT( false && "SizeOf method not supported for functions." );
		}

		void operator()( const ArrayPtr& array )
		{
			if( array == nullptr ) return;
			size= array->type.SizeOf() * array->size;
		}

		void operator()( const ClassPtr& class_ )
		{
			if( class_ == nullptr ) return;
			U_ASSERT( false && "SizeOf method not supported for classes." );
		}

		void operator()( const NontypeStub& stub )
		{
			U_UNUSED(stub);
			U_ASSERT( false && "SizeOf method not supported for stub types." );
		}

		void operator()( const TemplateDependentType& template_dependent_type )
		{
			U_UNUSED(template_dependent_type);
			U_ASSERT( false && "SizeOf method not supported for template-dependent types." );
		}
	};

	Visitor visitor;
	boost::apply_visitor( visitor, something_ );
	return visitor.size;
}

bool Type::IsIncomplete() const
{
	if( const ClassPtr* const class_= boost::get<ClassPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr );
		return (*class_)->is_incomplete;
	}
	else if( const ArrayPtr* const array= boost::get<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->type.IsIncomplete();
	}

	return false;
}

bool Type::IsDefaultConstructible() const
{
	if( const ClassPtr* const class_= boost::get<ClassPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr );
		return (*class_)->is_default_constructible;
	}
	else if( const ArrayPtr* const array= boost::get<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->type.IsDefaultConstructible();
	}

	return false;
}

bool Type::IsCopyConstructible() const
{
	if( const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &something_ ) )
	{
		U_UNUSED(fundamental_type);
		return true;
	}
	else if( const ClassPtr* const class_= boost::get<ClassPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr );
		return (*class_)->is_copy_constructible;
	}
	else if( const ArrayPtr* const array= boost::get<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->type.IsCopyConstructible();
	}

	return false;
}

bool Type::HaveDestructor() const
{
	if( const ClassPtr* const class_= boost::get<ClassPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr );
		return (*class_)->have_destructor;
	}
	else if( const ArrayPtr* const array= boost::get<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->type.HaveDestructor();
	}

	return false;
}

bool Type::CanBeConstexpr() const
{
	if( boost::get<FundamentalType>( &something_ ) != nullptr )
	{
		return true;
	}
	else if( const ArrayPtr* const array= boost::get<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->type.CanBeConstexpr();
	}

	return false;
}

llvm::Type* Type::GetLLVMType() const
{
	struct Visitor final : public boost::static_visitor<>
	{
		llvm::Type* llvm_type= nullptr;

		void operator()( const FundamentalType& fundamental )
		{
			llvm_type= fundamental.llvm_type;
		}

		void operator()( const FunctionPtr& function )
		{
			U_ASSERT( function != nullptr );
			llvm_type= function->llvm_function_type;
		}

		void operator()( const ArrayPtr& array )
		{
			if( array == nullptr ) return;
			llvm_type= array->llvm_type;
		}

		void operator()( const ClassPtr& class_ )
		{
			if( class_ == nullptr ) return;
			llvm_type= class_->llvm_type;
		}

		void operator()( const NontypeStub& stub )
		{
			U_UNUSED(stub);
		}

		void operator()( const TemplateDependentType& template_dependent_type )
		{
			llvm_type= template_dependent_type.llvm_type;
		}
	};

	Visitor visitor;
	boost::apply_visitor( visitor, something_ );
	return visitor.llvm_type;
}

ProgramString Type::ToString() const
{
	struct Visitor final : public boost::static_visitor<>
	{
		ProgramString result;

		void operator()( const FundamentalType& fundamental )
		{
			result= GetFundamentalTypeName( fundamental.fundamental_type );
		}

		void operator()( const FunctionPtr& function )
		{
			if( function == nullptr ) return;

			result+= "fn "_SpC;
			result+= function->return_type.ToString();
			result+= " ( "_SpC;
			for( const Function::Arg& arg : function->args )
			{
				if( arg.is_reference )
					result+= "&"_SpC;
				if( arg.is_mutable )
					result+= "mut "_SpC;
				else
					result+= "imut "_SpC;

				result+= arg.type.ToString();
				if( &arg != &function->args.back() )
					result+= ", "_SpC;
			}
			result+= " )"_SpC;
		}

		void operator()( const ArrayPtr& array )
		{
			if( array == nullptr ) return;

			result=
				"[ "_SpC + array->type.ToString() + ", "_SpC +
				ToProgramString( std::to_string( array->size ).c_str() ) + " ]"_SpC;
		}

		void operator()( const ClassPtr& class_ )
		{
			if( class_ == nullptr ) return;

			result= "class "_SpC + class_->members.GetThisNamespaceName();
		}

		void operator()( const NontypeStub& stub )
		{
			switch(stub)
			{
			case NontypeStub::OverloadedFunctionsSet:
				result= "overloaded functions set"_SpC;
				break;
			case NontypeStub::ThisOverloadedMethodsSet:
				result= "this + overloaded methods set"_SpC;
				break;
			case NontypeStub::TypeName:
				result= "class name"_SpC;
				break;
			case NontypeStub::Namespace:
				result= "namespace"_SpC;
				break;
			case NontypeStub::ClassTemplate:
				result= "class template"_SpC;
			case NontypeStub::TemplateDependentValue:
				result= "template-dependent value"_SpC;
				break;
			case NontypeStub::YetNotDeducedTemplateArg:
				result= "yet not deduced template arg"_SpC;
				break;
			};
			U_ASSERT(!result.empty());
		}

		void operator()( const TemplateDependentType& )
		{
			result= "template dependent type"_SpC;
		}
	};

	Visitor visitor;
	boost::apply_visitor( visitor, something_ );
	return std::move( visitor.result );
}

bool operator==( const Type& r, const Type& l )
{
	if( r.something_.which() != l.something_.which() )
		return false;

	if( r.something_.which() == 0 )
	{
		return *r.GetFundamentalType() == *l.GetFundamentalType();
	}
	else if( r.something_.which() == 1 )
	{
		return *r.GetFunctionType() == *l.GetFunctionType();
	}
	else if( r.something_.which() == 2 )
	{
		return *r.GetArrayType() == *l.GetArrayType();
	}
	else if( r.something_.which() == 3 )
	{
		return r.GetClassType() == l.GetClassType();
	}
	else if( r.something_.which() == 4 )
	{
		return boost::get<NontypeStub>(r.something_) == boost::get<NontypeStub>(l.something_);
	}
	else if( r.something_.which() == 5 )
	{
		return boost::get<TemplateDependentType>(r.something_) == boost::get<TemplateDependentType>(l.something_);
	}

	U_ASSERT(false);
	return false;
}

bool operator!=( const Type& r, const Type& l )
{
	return !( r == l );
}

bool operator==( const Function::Arg& r, const Function::Arg& l )
{
	return r.type == l.type && r.is_mutable == l.is_mutable && r.is_reference == l.is_reference;
}

bool operator!=( const Function::Arg& r, const Function::Arg& l )
{
	return !( r == l );
}

bool operator==( const Function& r, const Function& l )
{
	return
		r.return_type == l.return_type &&
		r.return_value_is_mutable == l.return_value_is_mutable &&
		r.return_value_is_reference == l.return_value_is_reference &&
		r.args == l.args;
}

bool operator!=( const Function& r, const Function& l )
{
	return !( r == l );
}

bool operator==( const Array& r, const Array& l )
{
	return r.type == l.type && r.size == l.size;
}

bool operator!=( const Array& r, const Array& l )
{
	return !( r == l );
}

size_t NameResolvingKeyHasher::operator()( const NameResolvingKey& key ) const
{
	size_t result= 0u;
	boost::hash_combine( result, key.components );
	boost::hash_combine( result, key.component_count );

	return result;
}

bool NameResolvingKeyHasher::operator()( const NameResolvingKey& a, const NameResolvingKey& b ) const
{
	return a.components == b.components && a.component_count == b.component_count;
}

Class::Class( const ProgramString& in_name, const NamesScope* const parent_scope )
	: members( in_name, parent_scope )
{}

Class::~Class()
{}


//
// Value
//

static const Type g_overloaded_functions_set_stub_type= NontypeStub::OverloadedFunctionsSet;
static const Type g_this_overloaded_methods_set_stub_type=NontypeStub::ThisOverloadedMethodsSet;
static const Type g_typename_type_stub= NontypeStub::TypeName;
static const Type g_namespace_type_stub= NontypeStub::Namespace;
static const Type g_class_template_type_stub= NontypeStub::ClassTemplate;
static const Type g_template_dependent_type_stub= NontypeStub::TemplateDependentValue;
static const Type g_yet_not_deduced_template_arg_type_stub= NontypeStub::YetNotDeducedTemplateArg;

Value::Value()
{}

Value::Value( Variable variable )
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

Value::Value( Type type )
{
	something_= std::move(type);
}

Value::Value( ClassField class_field )
{
	something_= std::move( class_field );
}

Value::Value( ThisOverloadedMethodsSet this_overloaded_methods_set )
{
	something_= std::move( this_overloaded_methods_set );
}

Value::Value( const NamesScopePtr& namespace_ )
{
	U_ASSERT( namespace_ != nullptr );
	something_= namespace_;
}

Value::Value( const ClassTemplatePtr& class_template )
{
	U_ASSERT( class_template != nullptr );
	something_= class_template;
}

Value::Value( TemplateDependentValue template_dependent_value )
{
	something_= std::move(template_dependent_value);
}

Value::Value( YetNotDeducedTemplateArg yet_not_deduced_template_arg )
{
	something_= std::move(yet_not_deduced_template_arg);
}

const Type& Value::GetType() const
{
	struct Visitor final : public boost::static_visitor<>
	{
		const Type* type;

		void operator()( const Variable& variable )
		{ type= &variable.type; }

		void operator()( const FunctionVariable& function_variable )
		{ type= &function_variable.type; }

		void operator()( const OverloadedFunctionsSet& )
		{ type= &g_overloaded_functions_set_stub_type; }

		void operator()( const Type& )
		{ type= &g_typename_type_stub; }

		void operator()( const ClassField& class_field )
		{ type= &class_field.type; }

		void operator()( const ThisOverloadedMethodsSet& )
		{ type= &g_this_overloaded_methods_set_stub_type; }

		void operator()( const NamesScopePtr& )
		{ type= &g_namespace_type_stub; }

		void operator()( const ClassTemplatePtr& )
		{ type= &g_class_template_type_stub; }

		void operator()( const TemplateDependentValue& )
		{ type= &g_template_dependent_type_stub; }

		void operator()( const YetNotDeducedTemplateArg& )
		{ type= &g_yet_not_deduced_template_arg_type_stub; }
	};

	Visitor visitor;
	boost::apply_visitor( visitor, something_ );
	return *visitor.type;
}

Variable* Value::GetVariable()
{
	return boost::get<Variable>( &something_ );
}

const Variable* Value::GetVariable() const
{
	return boost::get<Variable>( &something_ );
}

FunctionVariable* Value::GetFunctionVariable()
{
	return boost::get<FunctionVariable>( &something_ );
}

const FunctionVariable* Value::GetFunctionVariable() const
{
	return boost::get<FunctionVariable>( &something_ );
}

OverloadedFunctionsSet* Value::GetFunctionsSet()
{
	return boost::get<OverloadedFunctionsSet>( &something_ );
}

const OverloadedFunctionsSet* Value::GetFunctionsSet() const
{
	return boost::get<OverloadedFunctionsSet>( &something_ );
}

Type* Value::GetTypeName()
{
	return boost::get<Type>( &something_ );
}

const Type* Value::GetTypeName() const
{
	return boost::get<Type>( &something_ );
}

const ClassField* Value::GetClassField() const
{
	return boost::get<ClassField>( &something_ );
}

ThisOverloadedMethodsSet* Value::GetThisOverloadedMethodsSet()
{
	return boost::get<ThisOverloadedMethodsSet>( &something_ );
}

const ThisOverloadedMethodsSet* Value::GetThisOverloadedMethodsSet() const
{
	return boost::get<ThisOverloadedMethodsSet>( &something_ );
}

NamesScopePtr Value::GetNamespace() const
{
	const NamesScopePtr* const namespace_= boost::get<NamesScopePtr>( &something_ );
	if( namespace_ == nullptr )
		return nullptr;
	return *namespace_;
}

ClassTemplatePtr Value::GetClassTemplate() const
{
	const ClassTemplatePtr* const class_template= boost::get<ClassTemplatePtr>( &something_ );
	if( class_template == nullptr )
		return nullptr;
	return *class_template;
}

TemplateDependentValue* Value::GetTemplateDependentValue()
{
	return boost::get<TemplateDependentValue>( &something_ );
}

const TemplateDependentValue* Value::GetTemplateDependentValue() const
{
	return boost::get<TemplateDependentValue>( &something_ );
}

YetNotDeducedTemplateArg* Value::GetYetNotDeducedTemplateArg()
{
	return boost::get<YetNotDeducedTemplateArg>( &something_ );
}

const YetNotDeducedTemplateArg* Value::GetYetNotDeducedTemplateArg() const
{
	return boost::get<YetNotDeducedTemplateArg>( &something_ );
}

ArgOverloadingClass GetArgOverloadingClass( const bool is_reference, const bool is_mutable )
{
	if( is_reference && is_mutable )
		return ArgOverloadingClass::MutalbeReference;
	return ArgOverloadingClass::ImmutableReference;
}

ArgOverloadingClass GetArgOverloadingClass( const ValueType value_type )
{
	switch( value_type )
	{
	case ValueType::Value:
	case ValueType::ConstReference:
		return ArgOverloadingClass::ImmutableReference;

	case ValueType::Reference:
		return ArgOverloadingClass::MutalbeReference;
	};

	U_ASSERT(false);
	return ArgOverloadingClass::ImmutableReference;
}

ArgOverloadingClass GetArgOverloadingClass( const Function::Arg& arg )
{
	return GetArgOverloadingClass( arg.is_mutable, arg.is_reference );
}

NamesScope::NamesScope(
	ProgramString name,
	const NamesScope* const parent )
	: name_(std::move(name) )
	, parent_(parent)
{}

bool NamesScope::IsAncestorFor( const NamesScope& other ) const
{
	const NamesScope* n= other.parent_;
	while( n != nullptr )
	{
		if( this == n )
			return true;
		n= n->parent_;
	}

	return false;
}

const ProgramString& NamesScope::GetThisNamespaceName() const
{
	return name_;
}

void NamesScope::SetThisNamespaceName( ProgramString name )
{
	name_= std::move(name);
}

NamesScope::InsertedName* NamesScope::AddName(
	const ProgramString& name,
	Value value )
{
	auto it_bool_pair = names_map_.emplace( name, std::move( value ) );
	if( it_bool_pair.second )
		return &*it_bool_pair.first;

	return nullptr;
}

NamesScope::InsertedName* NamesScope::GetThisScopeName( const ProgramString& name ) const
{
	const auto it= names_map_.find( name );
	if( it != names_map_.end() )
		return const_cast<InsertedName*>(&*it);
	return nullptr;
}

NamesScope::InsertedName& NamesScope::GetTemplateDependentValue()
{
	const ProgramString name= "0_tdv"_SpC; // use identifier with number start - that can not exists in real program.
	const auto it= names_map_.find(name);
	if( it != names_map_.end() )
		return *it;

	return *names_map_.emplace( name, TemplateDependentValue() ).first;
}

const NamesScope* NamesScope::GetParent() const
{
	return parent_;
}

const NamesScope* NamesScope::GetRoot() const
{
	const NamesScope* root= this;
	while( root->parent_ != nullptr )
		root= root->parent_;
	return root;
}

void NamesScope::SetParent( const NamesScope* const parent )
{
	parent_= parent;
}

const ProgramString& GetFundamentalTypeName( const U_FundamentalType fundamental_type )
{
	if( fundamental_type >= U_FundamentalType::LastType )
		return g_invalid_type_name;

	return g_fundamental_types_names[ size_t( fundamental_type ) ];
}

const char* GetFundamentalTypeNameASCII( const U_FundamentalType fundamental_type )
{
	if( fundamental_type >= U_FundamentalType::LastType )
		return g_invalid_type_name_ascii;

	return g_fundamental_types_names_ascii[ size_t( fundamental_type ) ];
}

} //namespace CodeBuilderLLVMPrivate

} // namespace U
