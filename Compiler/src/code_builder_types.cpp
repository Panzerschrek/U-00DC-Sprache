#include "boost/functional/hash.hpp"

#include "assert.hpp"
#include "keywords.hpp"

#include "code_builder_types.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

static SizeType GetFundamentalTypeSize( const U_FundamentalType type )
{
	switch(type)
	{
	case U_FundamentalType::InvalidType: return 0u;
	case U_FundamentalType::Void: return 0u;
	case U_FundamentalType::Bool: return 1u;
	case U_FundamentalType::i8 : return 1u;
	case U_FundamentalType::u8 : return 1u;
	case U_FundamentalType::i16: return 2u;
	case U_FundamentalType::u16: return 2u;
	case U_FundamentalType::i32: return 4u;
	case U_FundamentalType::u32: return 4u;
	case U_FundamentalType::i64: return 8u;
	case U_FundamentalType::u64: return 8u;
	case U_FundamentalType::f32: return 4u;
	case U_FundamentalType::f64: return 8u;
	case U_FundamentalType::LastType: break;
	};

	U_ASSERT( false );
	return 0u;
}

//
// Fundamental type
//

FundamentalType::FundamentalType(
	const U_FundamentalType in_fundamental_type,
	llvm::Type* const in_llvm_type )
	: fundamental_type(in_fundamental_type)
	, llvm_type(in_llvm_type)
{}


bool operator==( const FundamentalType& r, const FundamentalType& l )
{
	return r.fundamental_type == l.fundamental_type;
}

bool operator!=( const FundamentalType& r, const FundamentalType& l )
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

Type::Type( const FunctionPointer& function_pointer_type )
{
	something_= FunctionPointerPtr( new FunctionPointer( function_pointer_type ) );
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

Type::Type( ClassProxyPtr class_type )
{
	something_= std::move( class_type );
}

Type::Type( EnumPtr enum_type )
{
	something_= std::move( enum_type );
}

Type::Type( const NontypeStub nontype_strub )
{
	something_= nontype_strub;
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

		void operator()( const ClassProxyPtr& class_ )
		{
			this_.something_= class_;
		}

		void operator()( const EnumPtr& enum_ )
		{
			this_.something_= enum_;
		}

		void operator()( const NontypeStub& stub )
		{
			this_.something_= stub;
		}

		void operator()( const FunctionPointerPtr& function_pointer )
		{
			U_ASSERT( function_pointer != nullptr );
			this_.something_= FunctionPointerPtr( new FunctionPointer( *function_pointer ) );
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

FunctionPointer* Type::GetFunctionPointerType()
{
	FunctionPointerPtr* const function_pointer_type= boost::get<FunctionPointerPtr>( &something_ );
	if( function_pointer_type == nullptr )
		return nullptr;
	return function_pointer_type->get();
}

const FunctionPointer* Type::GetFunctionPointerType() const
{
	const FunctionPointerPtr* const function_pointer_type= boost::get<FunctionPointerPtr>( &something_ );
	if( function_pointer_type == nullptr )
		return nullptr;
	return function_pointer_type->get();
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

ClassProxyPtr Type::GetClassTypeProxy() const
{
	const ClassProxyPtr* const class_type= boost::get<ClassProxyPtr>( &something_ );
	if( class_type == nullptr )
		return nullptr;
	return *class_type;
}

Class* Type::GetClassType() const
{
	const ClassProxyPtr class_proxy= GetClassTypeProxy();
	if( class_proxy == nullptr )
		return nullptr;
	return class_proxy->class_.get();
}

Enum* Type::GetEnumType() const
{
	const EnumPtr* enum_ptr= boost::get<EnumPtr>( &something_ );
	if( enum_ptr == nullptr )
		return nullptr;
	return enum_ptr->get();
}

bool Type::ReferenceIsConvertibleTo( const Type& other ) const
{
	if( *this == other )
		return true;

	// SPRACHE_TODO - support other reference casting - derived to base, etc.
	const FundamentalType* const other_fundamental= other.GetFundamentalType();
	if( other_fundamental != nullptr && other_fundamental->fundamental_type == U_FundamentalType::Void )
		return true;

	const Class* const class_type= GetClassType();
	const Class* const other_class_type= other.GetClassType();
	if( class_type != nullptr && other_class_type != nullptr )
	{
		for( const ClassProxyPtr& parent : class_type->parents )
		{
			if( parent->class_.get() == other_class_type )
				return true;
			if( Type(parent).ReferenceIsConvertibleTo( other ) )
				return true;
		}
	}

	return false;
}

SizeType Type::SizeOf() const
{
	struct Visitor final : public boost::static_visitor<SizeType>
	{
		SizeType operator()( const FundamentalType& fundamental ) const
		{
			return GetFundamentalTypeSize( fundamental.fundamental_type );
		}

		SizeType operator()( const FunctionPtr& ) const
		{
			U_ASSERT( false && "SizeOf method not supported for functions." );
			return 1u;
		}

		SizeType operator()( const ArrayPtr& array ) const
		{
			return array->type.SizeOf() * array->size;
		}

		SizeType operator()( const ClassProxyPtr& ) const
		{
			U_ASSERT( false && "SizeOf method not supported for classes." );
			return 1u;
		}

		SizeType operator()( const EnumPtr& enum_type ) const
		{
			return GetFundamentalTypeSize( enum_type->underlaying_type.fundamental_type );
		}

		SizeType operator()( const NontypeStub& ) const
		{
			U_ASSERT( false && "SizeOf method not supported for stub types." );
			return 1u;
		}

		SizeType operator()( const FunctionPointerPtr& ) const
		{
			U_ASSERT( false && "SizeOf method not supported for function-pointer types." );
			return 1u;
		}
	};

	return boost::apply_visitor( Visitor(), something_ );
}

bool Type::IsIncomplete() const
{
	if( const FundamentalType* const fundamental= boost::get<FundamentalType>( &something_ ) )
		return fundamental->fundamental_type == U_FundamentalType::Void;
	else if( const ClassProxyPtr* const class_= boost::get<ClassProxyPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr && (*class_)->class_ != nullptr );
		return (*class_)->class_->completeness != Class::Completeness::Complete;
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
	if( const ClassProxyPtr* const class_= boost::get<ClassProxyPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr && (*class_)->class_ != nullptr );
		return (*class_)->class_->is_default_constructible;
	}
	else if( const ArrayPtr* const array= boost::get<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->ArraySizeOrZero() == 0u || (*array)->type.IsDefaultConstructible();
	}

	return false;
}

bool Type::IsCopyConstructible() const
{
	if( boost::get<FundamentalType>( &something_ ) != nullptr ||
		boost::get<EnumPtr>( &something_ ) != nullptr ||
		boost::get<FunctionPointerPtr>( &something_ ) != nullptr )
	{
		return true;
	}
	else if( const ClassProxyPtr* const class_= boost::get<ClassProxyPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr && (*class_)->class_ != nullptr );
		return (*class_)->class_->is_copy_constructible;
	}
	else if( const ArrayPtr* const array= boost::get<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->ArraySizeOrZero() == 0u || (*array)->type.IsCopyConstructible();
	}

	return false;
}

bool Type::IsCopyAssignable() const
{
	if( GetFundamentalType() != nullptr || GetEnumType() != nullptr || GetFunctionPointerType() != nullptr )
		return true;
	else if( const ClassProxyPtr* const class_= boost::get<ClassProxyPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr && (*class_)->class_ != nullptr );
		return (*class_)->class_->is_copy_assignable;
	}
	else if( const ArrayPtr* const array= boost::get<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->ArraySizeOrZero() == 0u || (*array)->type.IsCopyAssignable();
	}

	return false;
}

bool Type::HaveDestructor() const
{
	if( const ClassProxyPtr* const class_= boost::get<ClassProxyPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr && (*class_)->class_ != nullptr );
		return (*class_)->class_->have_destructor;
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
	if( boost::get<FundamentalType>( &something_ ) != nullptr ||
		boost::get<EnumPtr>( &something_ ) != nullptr ||
		boost::get<FunctionPointerPtr>( &something_ ) != nullptr )
	{
		return true;
	}
	else if( const ArrayPtr* const array= boost::get<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->type.CanBeConstexpr();
	}
	else if( const Class* const class_= GetClassType() )
		return class_->can_be_constexpr;

	return false;
}

size_t Type::ReferencesTagsCount() const
{
	if( const Class* const class_type= GetClassType() )
	{
		return class_type->references_tags_count;
	}
	else if( const ArrayPtr* const array= boost::get<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->type.ReferencesTagsCount();
	}

	return 0u;
}

llvm::Type* Type::GetLLVMType() const
{
	struct Visitor final : public boost::static_visitor<llvm::Type*>
	{
		llvm::Type* operator()( const FundamentalType& fundamental ) const
		{
			return fundamental.llvm_type;
		}

		llvm::Type* operator()( const FunctionPtr& function ) const
		{
			U_ASSERT( function != nullptr );
			return function->llvm_function_type;
		}

		llvm::Type* operator()( const ArrayPtr& array ) const
		{
			U_ASSERT( array != nullptr );
			return array->llvm_type;
		}

		llvm::Type* operator()( const ClassProxyPtr& class_ ) const
		{
			U_ASSERT( class_ != nullptr && class_->class_ != nullptr );
			return class_->class_->llvm_type;
		}

		llvm::Type* operator()( const EnumPtr& enum_ ) const
		{
			return enum_->underlaying_type.llvm_type;
		}

		llvm::Type* operator()( const NontypeStub& ) const
		{
			return nullptr;
		}

		llvm::Type* operator()( const FunctionPointerPtr& function_pointer_type ) const
		{
			return function_pointer_type->llvm_function_pointer_type;
		}
	};

	return boost::apply_visitor( Visitor(), something_ );
}

ProgramString Type::ToString() const
{
	struct Visitor final : public boost::static_visitor<ProgramString>
	{
		ProgramString operator()( const FundamentalType& fundamental ) const
		{
			return GetFundamentalTypeName( fundamental.fundamental_type );
		}

		ProgramString operator()( const FunctionPtr& function ) const
		{
			// TODO - actualize this
			ProgramString result;
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
			return result;
		}

		ProgramString  operator()( const ArrayPtr& array ) const
		{
			return
				"[ "_SpC + array->type.ToString() + ", "_SpC +
				ToProgramString( std::to_string( array->size ).c_str() ) + " ]"_SpC;
		}

		ProgramString operator()( const ClassProxyPtr& class_ ) const
		{
			return "class "_SpC + class_->class_->members.GetThisNamespaceName();
		}

		ProgramString operator()( const EnumPtr& enum_ ) const
		{
			return "enum "_SpC + enum_->members.GetThisNamespaceName();
		}

		ProgramString operator()( const NontypeStub& stub ) const
		{
			switch(stub)
			{
			case NontypeStub::OverloadedFunctionsSet:
				return "overloaded functions set"_SpC;
			case NontypeStub::ThisOverloadedMethodsSet:
				return "this + overloaded methods set"_SpC;
			case NontypeStub::TypeName:
				return "class name"_SpC;
			case NontypeStub::Namespace:
				return "namespace"_SpC;
			case NontypeStub::TypeTemplate:
				return "type template"_SpC;
			case NontypeStub::YetNotDeducedTemplateArg:
				return "yet not deduced template arg"_SpC;
			case NontypeStub::ErrorValue:
				return "error value"_SpC;
			case NontypeStub::VariableStorage:
				return "variable storage"_SpC;
			};
			U_ASSERT(false);
			return ProgramString();
		}

		ProgramString operator()( const FunctionPointerPtr& function_pointer ) const
		{
			U_UNUSED(function_pointer);
			return "ptr to "_SpC; // TODO
		}
	};

	return boost::apply_visitor( Visitor(), something_ );
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
		return r.GetClassTypeProxy() == l.GetClassTypeProxy();
	}
	else if( r.something_.which() == 4 )
	{
		return r.GetEnumType() == l.GetEnumType();
	}
	else if( r.something_.which() == 5 )
	{
		return boost::get<NontypeStub>(r.something_) == boost::get<NontypeStub>(l.something_);
	}
	else if( r.something_.which() == 6 )
	{
		return *r.GetFunctionPointerType() == *l.GetFunctionPointerType();
	}

	U_ASSERT(false);
	return false;
}

bool operator!=( const Type& r, const Type& l )
{
	return !( r == l );
}

bool Function::PointerCanBeConvertedTo( const Function& other ) const
{
	const Function&  src_function_type= *this;
	const Function& dst_function_type= other;
	if( src_function_type.return_type != dst_function_type.return_type ||
		src_function_type.return_value_is_reference != dst_function_type.return_value_is_reference )
		return false;

	if( !src_function_type.return_value_is_mutable && dst_function_type.return_value_is_mutable )
		return false; // Allow mutability conversions, except mut->imut

	if( src_function_type.args.size() != dst_function_type.args.size() )
		return false;
	for( size_t i= 0u; i < src_function_type.args.size(); ++i )
	{
		if( src_function_type.args[i].type != dst_function_type.args[i].type ||
			src_function_type.args[i].is_reference != dst_function_type.args[i].is_reference )
			return false;

		if( src_function_type.args[i].is_mutable && !dst_function_type.args[i].is_mutable )
			return false; // Allow mutability conversions, except mut->imut
	}

	// We can convert function, returning less references to function, returning more referenes.
	for( const size_t src_arg_reference : src_function_type.return_references.args_references )
	{
		bool found= false;
		for( const size_t dst_arg_reference : dst_function_type.return_references.args_references )
		{
			if( dst_arg_reference == src_arg_reference )
			{
				found= true;
				break;
			}
		}
		if( !found )
			return false;
	}
	for( const Function::ArgReference& src_inner_arg_reference : src_function_type.return_references.inner_args_references )
	{
		bool found= false;
		for( const Function::ArgReference& dst_inner_arg_reference : dst_function_type.return_references.inner_args_references )
		{
			if( dst_inner_arg_reference == src_inner_arg_reference )
			{
				found= true;
				break;
			}
		}
		if( !found )
			return false;
	}

	// We can convert function, linkink less references to function, linking more references
	for( const Function::ReferencePollution& src_pollution : src_function_type.references_pollution )
	{
		 // TODO - maybe compare with mutability conversion possibility?
		if( dst_function_type.references_pollution.count(src_pollution) == 0u )
			return false;
	}

	if( src_function_type.unsafe && !dst_function_type.unsafe )
		return false; // Conversion from unsafe to safe function is forbidden.

	// Finally, we check all conditions
	return true;
}

bool operator==( const Function::InToOutReferences& l, const Function::InToOutReferences& r )
{
	return l.args_references == r.args_references && l.inner_args_references == r.inner_args_references;
}

bool operator!=( const Function::InToOutReferences& l, const Function::InToOutReferences& r )
{
	return !( l == r );
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
		r.args == l.args &&
		r.return_references == l.return_references &&
		r.references_pollution == l.references_pollution &&
		r.unsafe == l.unsafe;
}

bool operator!=( const Function& r, const Function& l )
{
	return !( r == l );
}

bool operator==( const FunctionPointer& r, const FunctionPointer& l )
{
	return r.function == l.function;
}
bool operator!=( const FunctionPointer& r, const FunctionPointer& l )
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

constexpr size_t Function::c_arg_reference_tag_number;

bool Function::ReferencePollution::operator==( const ReferencePollution& other ) const
{
	return this->dst == other.dst && this->src == other.src && this->src_is_mutable == other.src_is_mutable;
}

size_t Function::ReferencePollutionHasher::operator()( const ReferencePollution& r ) const
{
	size_t result= 0u;
	boost::hash_combine( result, r.dst.first );
	boost::hash_combine( result, r.dst.second );
	boost::hash_combine( result, r.src.first );
	boost::hash_combine( result, r.src.second );
	return result;
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

size_t TemplateClassKeyHasher::operator()( const TemplateClassKey& key ) const
{
	size_t result= 0u;
	boost::hash_combine( result, key.template_ );
	boost::hash_combine( result, key.class_name_encoded );
	return result;
}
bool TemplateClassKeyHasher::operator()( const TemplateClassKey& a, const TemplateClassKey& b ) const
{
	return a.template_ == b.template_ && a.class_name_encoded == b.class_name_encoded;
}


bool FunctionVariable::VirtuallyEquals( const FunctionVariable& other ) const
{
	U_ASSERT( this->is_this_call && other.is_this_call );

	const Function& l_type= *this->type.GetFunctionType();
	const Function& r_type= *other.type.GetFunctionType();

	return
		l_type.return_type == r_type.return_type &&
		l_type.return_value_is_reference == r_type.return_value_is_reference &&
		l_type.return_value_is_mutable == r_type.return_value_is_mutable &&
		l_type.return_references == r_type.return_references &&
		l_type.references_pollution == r_type.references_pollution &&
		l_type.unsafe == r_type.unsafe &&
		l_type.args.size() == r_type.args.size() &&
		std::equal( l_type.args.begin() + 1, l_type.args.end(), r_type.args.begin() + 1 );  // Compare args, except first.
}

//
// Class
//

Class::Class( const ProgramString& in_name, const NamesScope* const parent_scope )
	: members( in_name, parent_scope )
{}

Class::~Class()
{}

ClassMemberVisibility Class::GetMemberVisibility( const ProgramString& member_name ) const
{
	const auto it= members_visibility.find( member_name );
	if( it == members_visibility.end() )
		return ClassMemberVisibility::Public;
	return it->second;
}

void Class::SetMemberVisibility( const ProgramString& member_name, const ClassMemberVisibility visibility )
{
	if( visibility == ClassMemberVisibility::Public )
		return;
	members_visibility[member_name]= visibility;
}

Enum::Enum( const ProgramString& in_name, const NamesScope* parent_scope )
	: members( in_name, parent_scope )
{}

//
// StoredVariable
//

StoredVariable::StoredVariable(
	ProgramString in_name,
	Variable in_content,
	Kind in_kind,
	bool in_is_global_constant )
	: name(std::move(in_name) ), content(std::move(in_content))
	, kind(in_kind), is_global_constant(in_is_global_constant)
{}

//
// VariablesState
//

VariablesState::VariablesState( VariablesContainer variables )
	: variables_(std::move(variables))
{}

void VariablesState::AddVariable( const StoredVariablePtr& var )
{
	U_ASSERT( variables_.find(var) == variables_.end() );
	variables_[var]= VariableEntry();
}

void VariablesState::RemoveVariable( const StoredVariablePtr& var )
{
	U_ASSERT( variables_.find(var) != variables_.end() );
	variables_.erase(var);
}

bool VariablesState::AddPollution( const StoredVariablePtr& dst, const StoredVariablePtr& src, bool is_mutable )
{
	U_ASSERT( variables_.find(dst) != variables_.end() );

	VariableEntry& variable_entry= variables_.find(dst)->second;

	const auto it= variable_entry.inner_references.find(src);
	if( it == variable_entry.inner_references.end() )
	{
		Reference ref;
		ref.use_counter= is_mutable ? src->mut_use_counter : src->imut_use_counter;
		ref.is_mutable= is_mutable;
		variable_entry.inner_references[src]= ref;
	}
	else
	{
		if( it->second.IsMutable() || is_mutable ) // Error - link mutable not once.
			return false;
	}

	return true;
}

void VariablesState::AddPollutionForArgInnerVariable( const StoredVariablePtr& arg, const StoredVariablePtr& inner_variable )
{
	U_ASSERT( variables_.find(arg) != variables_.end() );

	VariableEntry& variable_entry= variables_.find(arg)->second;
	U_ASSERT( variable_entry.inner_references.find(inner_variable) == variable_entry.inner_references.end() );

	Reference ref;
	ref.use_counter= nullptr;
	ref.is_arg_inner_variable= true;
	variable_entry.inner_references[inner_variable]= ref;
}

void VariablesState::Move( const StoredVariablePtr& var )
{
	const auto it= variables_.find(var);
	U_ASSERT( it != variables_.end() );
	U_ASSERT( !it->second.is_moved );

	it->second.is_moved= true;
	it->second.inner_references.clear();
}

bool VariablesState::VariableIsMoved( const StoredVariablePtr& var ) const
{
	const auto it= variables_.find(var);
	if( it == variables_.end() ) // Can be for global constants, for example.
		return false;
	return it->second.is_moved;
}

const VariablesState::VariableReferences& VariablesState::GetVariableReferences( const StoredVariablePtr& var ) const
{
	const auto it= variables_.find(var);

	if( it == variables_.end() ) // May be for globals.
	{
		static const VariableReferences empty_references;
		return empty_references;
	}

	return it->second.inner_references;
}

const VariablesState::VariablesContainer& VariablesState::GetVariables() const
{
	return variables_;
}

VariablesState::AchievableVariables VariablesState::RecursiveGetAllReferencedVariables( const StoredVariablePtr& var ) const
{
	U_ASSERT( var->kind == StoredVariable::Kind::Variable || var->kind == StoredVariable::Kind::ReferenceArg );
	U_ASSERT( variables_.find(var) != variables_.end() );
	const VariableEntry& var_entry= variables_.find(var)->second;

	AchievableVariables result;

	for( const auto& referenced_variable_pair : var_entry.inner_references )
	{
		result.variables.insert( referenced_variable_pair.first );
		const AchievableVariables achievable_variables=
			RecursiveGetAllReferencedVariables(referenced_variable_pair.first);
		result.variables.insert( achievable_variables.variables.begin(), achievable_variables.variables.end() );

		if( referenced_variable_pair.second.IsMutable() ||
			achievable_variables.any_variable_is_mutable )
			result.any_variable_is_mutable= true;
	}

	return result;
}

void VariablesState::ActivateLocks()
{
	for( auto& variable_pair : variables_ )
		for( auto& ref_pair : variable_pair.second.inner_references )
		{
			if( !ref_pair.second.is_arg_inner_variable )
				ref_pair.second.use_counter= ref_pair.second.is_mutable ? ref_pair.first->mut_use_counter : ref_pair.first->imut_use_counter;
		}
}

void VariablesState::DeactivateLocks()
{
	for( auto& variable_pair : variables_ )
		for( auto& ref_pair : variable_pair.second.inner_references )
			ref_pair.second.use_counter= nullptr;
}

//
// Value
//

static const Type g_overloaded_functions_set_stub_type= NontypeStub::OverloadedFunctionsSet;
static const Type g_this_overloaded_methods_set_stub_type=NontypeStub::ThisOverloadedMethodsSet;
static const Type g_typename_type_stub= NontypeStub::TypeName;
static const Type g_namespace_type_stub= NontypeStub::Namespace;
static const Type g_type_template_type_stub= NontypeStub::TypeTemplate;
static const Type g_yet_not_deduced_template_arg_type_stub= NontypeStub::YetNotDeducedTemplateArg;
static const Type g_error_value_type_stub= NontypeStub::ErrorValue;
static const Type g_variable_storage_type_stub= NontypeStub::VariableStorage;

Value::Value()
{}

Value::Value( Variable variable, const FilePos& file_pos )
	: file_pos_(file_pos)
{
	something_= std::move(variable);
}

Value::Value( StoredVariablePtr stored_variable, const FilePos& file_pos )
	: file_pos_(file_pos)
{
	U_ASSERT( stored_variable != nullptr );
	something_= std::move(stored_variable);
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

Value::Value( YetNotDeducedTemplateArg yet_not_deduced_template_arg )
{
	something_= std::move(yet_not_deduced_template_arg);
}

Value::Value( ErrorValue error_value )
{
	something_= std::move(error_value);
}

const Type& Value::GetType() const
{
	struct Visitor final : public boost::static_visitor< const Type& >
	{
		const Type& operator()( const Variable& variable ) const
		{ return variable.type; }

		const Type& operator()( const StoredVariablePtr& ) const
		{ return g_variable_storage_type_stub; }

		const Type& operator()( const FunctionVariable& function_variable ) const
		{ return function_variable.type; }

		const Type& operator()( const OverloadedFunctionsSet& ) const
		{ return g_overloaded_functions_set_stub_type; }

		const Type& operator()( const Type& ) const
		{ return g_typename_type_stub; }

		const Type& operator()( const ClassField& class_field ) const
		{ return class_field.type; }

		const Type& operator()( const ThisOverloadedMethodsSet& ) const
		{ return g_this_overloaded_methods_set_stub_type; }

		const Type& operator()( const NamesScopePtr& ) const
		{ return g_namespace_type_stub; }

		const Type& operator()( const TypeTemplatesSet& ) const
		{ return g_type_template_type_stub; }

		const Type& operator()( const YetNotDeducedTemplateArg& ) const
		{ return g_yet_not_deduced_template_arg_type_stub; }

		const Type& operator()( const ErrorValue& ) const
		{ return g_error_value_type_stub; }
	};

	return boost::apply_visitor( Visitor(), something_ );
}

int Value::GetKindIndex() const
{
	return something_.which();
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
	return boost::get<Variable>( &something_ );
}

const Variable* Value::GetVariable() const
{
	return boost::get<Variable>( &something_ );
}

StoredVariablePtr Value::GetStoredVariable() const
{
	const StoredVariablePtr* const stored_variable= boost::get<StoredVariablePtr>( &something_ );
	if( stored_variable == nullptr )
		return nullptr;
	return *stored_variable;
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

TypeTemplatesSet* Value::GetTypeTemplatesSet()
{
	return boost::get<TypeTemplatesSet>( &something_ );
}

const TypeTemplatesSet* Value::GetTypeTemplatesSet() const
{
	return boost::get<TypeTemplatesSet>( &something_ );
}

YetNotDeducedTemplateArg* Value::GetYetNotDeducedTemplateArg()
{
	return boost::get<YetNotDeducedTemplateArg>( &something_ );
}

const YetNotDeducedTemplateArg* Value::GetYetNotDeducedTemplateArg() const
{
	return boost::get<YetNotDeducedTemplateArg>( &something_ );
}

ErrorValue* Value::GetErrorValue()
{
	return boost::get<ErrorValue>( &something_ );
}

const ErrorValue* Value::GetErrorValue() const
{
	return boost::get<ErrorValue>( &something_ );
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

void NamesScope::AddAccessRightsFor( const ClassProxyPtr& class_, const ClassMemberVisibility visibility )
{
	access_rights_[class_]= visibility;
}

ClassMemberVisibility NamesScope::GetAccessFor( const ClassProxyPtr& class_ ) const
{
	const auto it= access_rights_.find(class_);
	const auto this_rights= it == access_rights_.end() ? ClassMemberVisibility::Public : it->second;
	const auto parent_rights= parent_ == nullptr ? ClassMemberVisibility::Public : parent_->GetAccessFor( class_ );
	return std::max( this_rights, parent_rights );
}

//
// DeducedTemplateParameter
//

DeducedTemplateParameter::Array::Array( const Array& other )
{
	*this= other;
}

DeducedTemplateParameter::Array& DeducedTemplateParameter::Array::operator=( const Array& other )
{
	size.reset( new DeducedTemplateParameter( *other.size ) );
	type.reset( new DeducedTemplateParameter( *other.type ) );
	return *this;
}

DeducedTemplateParameter::Function::Function( const Function& other )
{
	*this= other;
}

DeducedTemplateParameter::Function& DeducedTemplateParameter::Function::operator=( const Function& other )
{
	return_type.reset( new DeducedTemplateParameter( *other.return_type ) );
	argument_types= other.argument_types;
	return *this;
}

DeducedTemplateParameter::DeducedTemplateParameter( Invalid invalid )
{
	something_= std::move(invalid);
}

DeducedTemplateParameter::DeducedTemplateParameter( Type type )
{
	something_= std::move(type);
}

DeducedTemplateParameter::DeducedTemplateParameter( Variable variable )
{
	something_= std::move(variable);
}

DeducedTemplateParameter::DeducedTemplateParameter( TemplateParameter template_parameter )
{
	something_= std::move(template_parameter);
}

DeducedTemplateParameter::DeducedTemplateParameter( Array array )
{
	something_= std::move(array);
}

DeducedTemplateParameter::DeducedTemplateParameter( Function function )
{
	something_= std::move(function);
}

DeducedTemplateParameter::DeducedTemplateParameter( Template template_ )
{
	something_= std::move(template_);
}

bool DeducedTemplateParameter::IsInvalid() const
{
	return boost::get<Invalid>( &something_ ) != nullptr;
}

bool DeducedTemplateParameter::IsType() const
{
	return boost::get<Type>( &something_ ) != nullptr;
}

bool DeducedTemplateParameter::IsVariable() const
{
	return boost::get<Variable>( &something_ ) != nullptr;
}

bool DeducedTemplateParameter::IsTemplateParameter() const
{
	return boost::get<TemplateParameter>( &something_ ) != nullptr;
}

const DeducedTemplateParameter::Array* DeducedTemplateParameter::GetArray() const
{
	return boost::get<Array>( &something_ );
}

const DeducedTemplateParameter::Function* DeducedTemplateParameter::GetFunction() const
{
	return boost::get<Function>( &something_ );
}

const DeducedTemplateParameter::Template* DeducedTemplateParameter::GetTemplate() const
{
	return boost::get<Template>( &something_ );
}

//
//
//

const ProgramString g_invalid_type_name= "InvalidType"_SpC;

const ProgramString& GetFundamentalTypeName( const U_FundamentalType type )
{
	switch(type)
	{
	case U_FundamentalType::InvalidType: return g_invalid_type_name;
	case U_FundamentalType::Void: return Keyword( Keywords::void_ );
	case U_FundamentalType::Bool: return Keyword( Keywords::bool_ );
	case U_FundamentalType::i8 : return Keyword( Keywords::i8_ );
	case U_FundamentalType::u8 : return Keyword( Keywords::u8_ );
	case U_FundamentalType::i16: return Keyword( Keywords::i16_ );
	case U_FundamentalType::u16: return Keyword( Keywords::u16_ );
	case U_FundamentalType::i32: return Keyword( Keywords::i32_ );
	case U_FundamentalType::u32: return Keyword( Keywords::u32_ );
	case U_FundamentalType::i64: return Keyword( Keywords::i64_ );
	case U_FundamentalType::u64: return Keyword( Keywords::u64_ );
	case U_FundamentalType::f32: return Keyword( Keywords::f32_ );
	case U_FundamentalType::f64: return Keyword( Keywords::f64_ );
	case U_FundamentalType::LastType: break;
	};

	U_ASSERT( false );
	return g_invalid_type_name;
}

} //namespace CodeBuilderLLVMPrivate

} // namespace U
