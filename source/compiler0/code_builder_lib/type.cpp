#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Hashing.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "class.hpp"
#include "enum.hpp"
#include "template_types.hpp"
#include "type.hpp"


namespace U
{

namespace CodeBuilderPrivate
{

namespace
{

uint64_t GetFundamentalTypeSize( const U_FundamentalType type )
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
	case U_FundamentalType::i128: return 16u;
	case U_FundamentalType::u128: return 16u;
	case U_FundamentalType::f32: return 4u;
	case U_FundamentalType::f64: return 8u;
	case U_FundamentalType::char8 : return 1u;
	case U_FundamentalType::char16: return 2u;
	case U_FundamentalType::char32: return 4u;
	case U_FundamentalType::LastType: break;
	};

	U_ASSERT( false );
	return 0u;
}

template<typename T>
llvm::Type* GetLLVMTypeImpl( const T& el )
{
	return el.llvm_type;
}

template<typename T>
llvm::Type* GetLLVMTypeImpl( const std::unique_ptr<T>& boxed )
{
	U_ASSERT(boxed != nullptr);
	return GetLLVMTypeImpl( *boxed );
}

llvm::Type* GetLLVMTypeImpl( const ClassProxyPtr& class_ )
{
	U_ASSERT( class_ != nullptr && class_->class_ != nullptr );
	return class_->class_->llvm_type;
}

llvm::Type* GetLLVMTypeImpl( const EnumPtr& enum_ )
{
	U_ASSERT( enum_ != nullptr );
	return enum_->underlaying_type.llvm_type;
}

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

uint64_t FundamentalType::GetSize() const
{
	return GetFundamentalTypeSize(fundamental_type);
}

bool operator==( const FundamentalType& l, const FundamentalType& r )
{
	return l.fundamental_type == r.fundamental_type;
}

bool operator!=( const FundamentalType& l, const FundamentalType& r )
{
	return !( l == r );
}

//
// Tuple
//

bool operator==( const Tuple& l, const Tuple& r )
{
	return l.elements == r.elements;
}

bool operator!=( const Tuple& l, const Tuple& r )
{
	return !( l == r );
}

//
// Type
//

static_assert( sizeof(Type) <= 40u, "Type is too heavy!" );

Type::Type( const Type& other )
{
	*this= other;
}

Type::Type( FundamentalType fundamental_type )
	: something_( std::move(fundamental_type) )
{}

Type::Type( Function function_type )
	: something_( std::make_unique<Function>( std::move(function_type) ) )
{}

Type::Type( FunctionPointer function_pointer_type )
	: something_( std::make_unique<FunctionPointer>( std::move(function_pointer_type) ) )
{}

Type::Type( Array array_type )
	: something_( std::make_unique<Array>( std::move(array_type) ) )
{}

Type::Type( RawPointer raw_pointer_type )
	: something_( std::make_unique<RawPointer>( std::move(raw_pointer_type) ) )
{}

Type::Type( Tuple tuple_type )
	: something_( std::move(tuple_type) )
{}

Type::Type( ClassProxyPtr class_type )
	: something_( std::move(class_type) )
{}

Type::Type( EnumPtr enum_type )
	: something_( std::move(enum_type) )
{}

Type& Type::operator=( const Type& other )
{
	something_= std::visit( [&]( const auto& el ) { return CopyVariant(el); }, other.something_ );
	return *this;
}

FundamentalType* Type::GetFundamentalType()
{
	return std::get_if<FundamentalType>( &something_ );
}

const FundamentalType* Type::GetFundamentalType() const
{
	return std::get_if<FundamentalType>( &something_ );
}

Function* Type::GetFunctionType()
{
	const FunctionPtr* const function_type= std::get_if<FunctionPtr>( &something_ );
	if( function_type == nullptr )
		return nullptr;
	return function_type->get();
}

const Function* Type::GetFunctionType() const
{
	const FunctionPtr* const function_type= std::get_if<FunctionPtr>( &something_ );
	if( function_type == nullptr )
		return nullptr;
	return function_type->get();
}

FunctionPointer* Type::GetFunctionPointerType()
{
	const FunctionPointerPtr* const function_pointer_type= std::get_if<FunctionPointerPtr>( &something_ );
	if( function_pointer_type == nullptr )
		return nullptr;
	return function_pointer_type->get();
}

const FunctionPointer* Type::GetFunctionPointerType() const
{
	const FunctionPointerPtr* const function_pointer_type= std::get_if<FunctionPointerPtr>( &something_ );
	if( function_pointer_type == nullptr )
		return nullptr;
	return function_pointer_type->get();
}

Array* Type::GetArrayType()
{
	const ArrayPtr* const array_type= std::get_if<ArrayPtr>( &something_ );
	if( array_type == nullptr )
		return nullptr;
	return array_type->get();
}

const Array* Type::GetArrayType() const
{
	const ArrayPtr* const array_type= std::get_if<ArrayPtr>( &something_ );
	if( array_type == nullptr )
		return nullptr;
	return array_type->get();
}

RawPointer* Type::GetRawPointerType()
{
	const RawPointerPtr* const raw_pointer_type= std::get_if<RawPointerPtr>( &something_ );
	if( raw_pointer_type == nullptr )
		return nullptr;
	return raw_pointer_type->get();
}

const RawPointer* Type::GetRawPointerType() const
{
	const RawPointerPtr* const raw_pointer_type= std::get_if<RawPointerPtr>( &something_ );
	if( raw_pointer_type == nullptr )
		return nullptr;
	return raw_pointer_type->get();
}

Tuple* Type::GetTupleType()
{
	return std::get_if<Tuple>( &something_ );
}

const Tuple* Type::GetTupleType() const
{
	return std::get_if<Tuple>( &something_ );
}

ClassProxyPtr Type::GetClassTypeProxy() const
{
	const ClassProxyPtr* const class_type= std::get_if<ClassProxyPtr>( &something_ );
	if( class_type == nullptr )
		return nullptr;
	return *class_type;
}

Class* Type::GetClassType() const
{
	const ClassProxyPtr class_proxy= GetClassTypeProxy();
	if( class_proxy == nullptr )
		return nullptr;
	return class_proxy->class_;
}

Enum* Type::GetEnumType() const
{
	const EnumPtr* enum_ptr= std::get_if<EnumPtr>( &something_ );
	if( enum_ptr == nullptr )
		return nullptr;
	return *enum_ptr;
}

EnumPtr Type::GetEnumTypePtr() const
{
	const EnumPtr* enum_ptr= std::get_if<EnumPtr>( &something_ );
	if( enum_ptr == nullptr )
		return nullptr;
	return *enum_ptr;
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
		for( const Class::Parent& parent : class_type->parents )
		{
			if( parent.class_->class_ == other_class_type )
				return true;
			if( Type(parent.class_).ReferenceIsConvertibleTo( other ) )
				return true;
		}
	}

	return false;
}

bool Type::IsDefaultConstructible() const
{
	if( const ClassProxyPtr* const class_= std::get_if<ClassProxyPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr && (*class_)->class_ != nullptr );
		return (*class_)->class_->is_default_constructible;
	}
	else if( const ArrayPtr* const array= std::get_if<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->size == 0u || (*array)->type.IsDefaultConstructible();
	}
	else if( const Tuple* const tuple= std::get_if<Tuple>( &something_ ) )
	{
		bool default_constructible= true;
		for( const Type& element : tuple->elements )
			default_constructible= default_constructible && element.IsDefaultConstructible();
		return default_constructible;
	}

	return false;
}

bool Type::IsCopyConstructible() const
{
	if( GetFundamentalType() != nullptr ||
		GetEnumType() != nullptr ||
		GetRawPointerType() != nullptr ||
		GetFunctionPointerType() != nullptr )
		return true;
	else if( const ClassProxyPtr* const class_= std::get_if<ClassProxyPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr && (*class_)->class_ != nullptr );
		return (*class_)->class_->is_copy_constructible;
	}
	else if( const ArrayPtr* const array= std::get_if<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->size == 0u || (*array)->type.IsCopyConstructible();
	}
	else if( const Tuple* const tuple= std::get_if<Tuple>( &something_ ) )
	{
		bool copy_constructible= true;
		for( const Type& element : tuple->elements )
			copy_constructible= copy_constructible && element.IsCopyConstructible();
		return copy_constructible;
	}

	return false;
}

bool Type::IsCopyAssignable() const
{
	if( GetFundamentalType() != nullptr ||
		GetEnumType() != nullptr ||
		GetRawPointerType() != nullptr ||
		GetFunctionPointerType() != nullptr )
		return true;
	else if( const ClassProxyPtr* const class_= std::get_if<ClassProxyPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr && (*class_)->class_ != nullptr );
		return (*class_)->class_->is_copy_assignable;
	}
	else if( const ArrayPtr* const array= std::get_if<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->size == 0u || (*array)->type.IsCopyAssignable();
	}
	else if( const Tuple* const tuple= std::get_if<Tuple>( &something_ ) )
	{
		bool copy_assignable= true;
		for( const Type& element : tuple->elements )
			copy_assignable= copy_assignable && element.IsCopyAssignable();
		return copy_assignable;
	}

	return false;
}

bool Type::HaveDestructor() const
{
	if( const ClassProxyPtr* const class_= std::get_if<ClassProxyPtr>( &something_ ) )
	{
		U_ASSERT( *class_ != nullptr && (*class_)->class_ != nullptr );
		return (*class_)->class_->have_destructor;
	}
	else if( const ArrayPtr* const array= std::get_if<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->type.HaveDestructor();
	}
	else if( const Tuple* const tuple= std::get_if<Tuple>( &something_ ) )
	{
		bool have_destructor= false;
		for( const Type& element : tuple->elements )
			have_destructor= have_destructor || element.HaveDestructor();
		return have_destructor;
	}

	return false;
}

bool Type::CanBeConstexpr() const
{
	if( std::get_if<FundamentalType>( &something_ ) != nullptr ||
		std::get_if<EnumPtr>( &something_ ) != nullptr ||
		std::get_if<FunctionPointerPtr>( &something_ ) != nullptr )
	{
		return true;
	}

	// Raw pointer type is not constexpr.

	else if( const ArrayPtr* const array= std::get_if<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->type.CanBeConstexpr();
	}
	else if( const Class* const class_= GetClassType() )
		return class_->can_be_constexpr;
	else if( const Tuple* const tuple= std::get_if<Tuple>( &something_ ) )
	{
		bool can_be_constexpr= true;
		for( const Type& element : tuple->elements )
			can_be_constexpr= can_be_constexpr && element.CanBeConstexpr();
		return can_be_constexpr;
	}

	return false;
}

bool Type::IsAbstract() const
{
	if( std::get_if<FundamentalType>( &something_ ) != nullptr ||
		std::get_if<EnumPtr>( &something_ ) != nullptr ||
		std::get_if<FunctionPointerPtr>( &something_ ) != nullptr )
	{
		return false;
	}
	else if( const ArrayPtr* const array= std::get_if<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		return (*array)->size > 0u && (*array)->type.IsAbstract();
	}
	else if( const Class* const class_= GetClassType() )
		return class_->kind == Class::Kind::Abstract || class_->kind == Class::Kind::Interface;
	else if( const Tuple* const tuple= std::get_if<Tuple>( &something_ ) )
	{
		bool is_abstract= false;
		for( const Type& element : tuple->elements )
			is_abstract= is_abstract || element.IsAbstract();
		return is_abstract;
	}

	return false;
}

size_t Type::ReferencesTagsCount() const
{
	return GetInnerReferenceType() == InnerReferenceType::None ? 0 : 1;
}

InnerReferenceType Type::GetInnerReferenceType() const
{
	InnerReferenceType result= InnerReferenceType::None;

	if( const Class* const class_type= GetClassType() )
	{
		result= class_type->inner_reference_type;
	}
	else if( const ArrayPtr* const array= std::get_if<ArrayPtr>( &something_ ) )
	{
		U_ASSERT( *array != nullptr );
		result= (*array)->type.GetInnerReferenceType();
	}
	else if( const Tuple* const tuple= std::get_if<Tuple>( &something_ ) )
	{
		for( const Type& element : tuple->elements )
			result= std::max( result, element.GetInnerReferenceType() );
	}

	return result;
}

llvm::Type* Type::GetLLVMType() const
{
	return std::visit( []( const auto& el ){ return GetLLVMTypeImpl( el ); }, something_ );
}

std::string Type::ToString() const
{
	struct Visitor final
	{
		std::string operator()( const FundamentalType& fundamental ) const
		{
			return GetFundamentalTypeName( fundamental.fundamental_type );
		}

		std::string operator()( const FunctionPtr& function ) const
		{
			return ProcessFunctionType( *function );
		}

		std::string operator()( const ArrayPtr& array ) const
		{
			return
				"[ " + array->type.ToString() + ", " +
				std::to_string( array->size ) + " ]";
		}

		std::string operator()( const RawPointerPtr& raw_pointer ) const
		{
			return "$( " + raw_pointer->type.ToString() + " )";
		}

		std::string operator()( const Tuple& tuple ) const
		{
			std::string res= "tup[ ";

			for( const Type& element_type : tuple.elements )
			{
				res+= element_type.ToString();
				if( &element_type != & tuple.elements.back() )
					res+= ", ";
			}
			res+= " ]";
			return res;
		}

		std::string operator()( const ClassProxyPtr& class_ ) const
		{
			std::string result;
			if( class_->class_->typeinfo_type != std::nullopt )
			{
				result= Keyword(Keywords::typeof_);
				result+= "(";
				result+= Keyword(Keywords::typeinfo_);
				result+= "</";
				result+= class_->class_->typeinfo_type->ToString();
				result+= "/>";
				result+=")";
			}
			else if( class_->class_->base_template != std::nullopt )
			{
				// Skip template parameters namespace.
				const std::string template_namespace_name= class_->class_->members.GetParent()->GetParent()->ToString();
				if( !template_namespace_name.empty() )
					result+= template_namespace_name + "::";

				const std::string& class_name= class_->class_->base_template->class_template->syntax_element->name_;
				result+= class_name;
				result+= "</";
				for( const TemplateArg& arg : class_->class_->base_template->signature_args )
				{
					if( const Type* const param_as_type = std::get_if<Type>( &arg ) )
						result+= param_as_type->ToString();
					else if( const Variable* const param_as_variable= std::get_if<Variable>( &arg ) )
						result+= ConstantVariableToString( *param_as_variable );
					else U_ASSERT(false);

					if( &arg != &class_->class_->base_template->signature_args.back() )
						result+= ", ";
				}
				result+= "/>";
			}
			else
				result+= class_->class_->members.ToString();
			return result;
		}

		std::string operator()( const EnumPtr& enum_ ) const
		{
			return "enum " + enum_->members.GetThisNamespaceName();
		}

		std::string operator()( const FunctionPointerPtr& function_pointer ) const
		{
			return ProcessFunctionType( function_pointer->function );
		}

	private:
		std::string ProcessFunctionType( const Function& function ) const
		{
			// TODO - actualize this
			std::string result;
			result+= "fn ";
			result+= function.return_type.ToString();
			result+= " ( ";
			for( const Function::Arg& arg : function.args )
			{
				if( arg.is_reference )
					result+= "&";
				if( arg.is_mutable )
					result+= "mut ";
				else
					result+= "imut ";

				result+= arg.type.ToString();
				if( &arg != &function.args.back() )
					result+= ", ";
			}
			result+= " )";
			if( function.unsafe )
				result+= " unsafe";
			return result;
		}
	};

	return std::visit( Visitor(), something_ );
}

size_t Type::Hash() const
{
	struct Visitor final
	{
		size_t operator()( const FundamentalType& fundamental ) const
		{
			return size_t(fundamental.fundamental_type);
		}

		size_t operator()( const FunctionPtr& function ) const
		{
			return ProcessFunctionType( *function );
		}

		size_t operator()( const ArrayPtr& array ) const
		{
			return llvm::hash_combine( array->type.Hash(), array->size );
		}

		size_t operator()( const RawPointerPtr& raw_pointer ) const
		{
			return raw_pointer->type.Hash();
		}

		size_t operator()( const Tuple& tuple ) const
		{
			size_t hash= 0;
			for( const Type& element : tuple.elements )
				hash= llvm::hash_combine( hash, element.Hash() );
			return hash;
		}

		size_t operator()( const ClassProxyPtr& class_ ) const
		{
			return size_t(reinterpret_cast<uintptr_t>(class_.get()));
		}

		size_t operator()( const EnumPtr& enum_ ) const
		{
			return size_t(reinterpret_cast<uintptr_t>(enum_));
		}

		size_t operator()( const FunctionPointerPtr& function_pointer ) const
		{
			return ProcessFunctionType( function_pointer->function );
		}

	private:
		size_t ProcessFunctionType( const Function& function ) const
		{
			size_t hash= 0;
			for( const Function::Arg& arg : function.args )
				hash= llvm::hash_combine( hash, arg.type.Hash(), arg.is_reference, arg.is_mutable );

			hash=
				llvm::hash_combine(
					hash,
					function.return_type.Hash(),
					function.return_value_is_reference,
					function.return_value_is_mutable,
					function.unsafe );

			for( const Function::ArgReference& arg_reference : function.return_references )
				hash= llvm::hash_combine( hash, arg_reference );

			for( const Function::ReferencePollution& reference_pollution : function.references_pollution )
				hash= llvm::hash_combine( hash, reference_pollution.dst, reference_pollution.src );

			return hash;
		}
	};

	return llvm::hash_combine( something_.index(), std::visit( Visitor(), something_ ) );
}

bool operator==( const Type& l, const Type& r )
{
	if( l.something_.index() != r.something_.index() )
		return false;

	if( l.something_.index() == 0 )
	{
		return *l.GetFundamentalType() == *r.GetFundamentalType();
	}
	else if( l.something_.index() == 1 )
	{
		return *l.GetFunctionType() == *r.GetFunctionType();
	}
	else if( l.something_.index() == 2 )
	{
		return *l.GetArrayType() == *r.GetArrayType();
	}
	else if( l.something_.index() == 3 )
	{
		return *l.GetRawPointerType() == *r.GetRawPointerType();
	}
	else if( l.something_.index() == 4 )
	{
		return l.GetClassTypeProxy() == r.GetClassTypeProxy();
	}
	else if( l.something_.index() == 5 )
	{
		return l.GetEnumType() == r.GetEnumType();
	}
	else if( l.something_.index() == 6 )
	{
		return *l.GetFunctionPointerType() == *r.GetFunctionPointerType();
	}
	else if( l.something_.index() == 7 )
	{
		return *l.GetTupleType() == *r.GetTupleType();
	}

	U_ASSERT(false);
	return false;
}

bool operator!=( const Type& l, const Type& r )
{
	return !( l == r );
}

//
// Array
//

bool operator==( const Array& l, const Array& r )
{
	return l.type == r.type && l.size == r.size;
}

bool operator!=( const Array& l, const Array& r )
{
	return !( l == r );
}

bool operator==( const RawPointer& l, const RawPointer& r )
{
	return l.type == r.type;
}

bool operator!=( const RawPointer& l, const RawPointer& r )
{
	return !( l == r );
}

//
// Function
//

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
	for( const Function::ArgReference& src_inner_arg_reference : src_function_type.return_references )
	{
		bool found= false;
		for( const Function::ArgReference& dst_inner_arg_reference : dst_function_type.return_references )
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

bool Function::IsStructRet() const
{
	return
		!return_value_is_reference &&
		( return_type.GetClassType() != nullptr || return_type.GetArrayType() != nullptr || return_type.GetTupleType() != nullptr );
}

bool operator==( const Function::Arg& l, const Function::Arg& r )
{
	return l.type == r.type && l.is_mutable == r.is_mutable && l.is_reference == r.is_reference;
}

bool operator!=( const Function::Arg& l, const Function::Arg& r )
{
	return !( l == r );
}

bool Function::ReferencePollution::operator==( const ReferencePollution& other ) const
{
	return this->dst == other.dst && this->src == other.src;
}

bool Function::ReferencePollution::operator<( const ReferencePollution& other ) const
{
	// Order is significant, because references pollution is part of stable function type.
	if( this->dst != other.dst )
		return this->dst < other.dst;
	return this->src < other.src;
}

bool operator==( const Function& l, const Function& r )
{
	return
		l.return_type == r.return_type &&
		l.return_value_is_mutable == r.return_value_is_mutable &&
		l.return_value_is_reference == r.return_value_is_reference &&
		l.args == r.args &&
		l.return_references == r.return_references &&
		l.references_pollution == r.references_pollution &&
		l.unsafe == r.unsafe;
}

bool operator!=( const Function& l, const Function& r )
{
	return !( l == r );
}

constexpr size_t Function::c_arg_reference_tag_number;

//
// FunctionPointer
//

bool operator==( const FunctionPointer& l, const FunctionPointer& r )
{
	return l.function == r.function;
}
bool operator!=( const FunctionPointer& l, const FunctionPointer& r )
{
	return !( r == l );
}

} //namespace CodeBuilderLLVMPrivate

} // namespace U
