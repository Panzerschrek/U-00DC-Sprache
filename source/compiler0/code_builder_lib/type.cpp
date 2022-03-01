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
llvm::Type* GetLLVMTypeImpl( const std::shared_ptr<const T>& boxed )
{
	U_ASSERT(boxed != nullptr);
	return GetLLVMTypeImpl( *boxed );
}

llvm::Type* GetLLVMTypeImpl( const ClassPtr& class_ )
{
	U_ASSERT( class_ != nullptr );
	return class_->llvm_type;
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

bool operator==( const TupleType& l, const TupleType& r )
{
	return l.elements == r.elements;
}

bool operator!=( const TupleType& l, const TupleType& r )
{
	return !( l == r );
}

//
// Type
//

// No more, than 3 pointers on 64 bit platform.
static_assert( sizeof(Type) <= 24u, "Type is too heavy!" );

Type::Type( FundamentalType fundamental_type )
	: something_( std::move(fundamental_type) )
{}

Type::Type( FunctionType function_type )
	: something_( std::make_shared<FunctionType>( std::move(function_type) ) )
{}

Type::Type( FunctionPointerType function_pointer_type )
	: something_( std::make_shared<FunctionPointerType>( std::move(function_pointer_type) ) )
{}

Type::Type( ArrayType array_type )
	: something_( std::make_shared<ArrayType>( std::move(array_type) ) )
{}

Type::Type( RawPointerType raw_pointer_type )
	: something_( std::make_shared<RawPointerType>( std::move(raw_pointer_type) ) )
{}

Type::Type( TupleType tuple_type )
	: something_( std::make_shared<TupleType>( std::move(tuple_type) ) )
{}

Type::Type( ClassPtr class_type )
	: something_( std::move(class_type) )
{}

Type::Type( EnumPtr enum_type )
	: something_( std::move(enum_type) )
{}

const FundamentalType* Type::GetFundamentalType() const
{
	return std::get_if<FundamentalType>( &something_ );
}

const FunctionType* Type::GetFunctionType() const
{
	if( const auto function_type= std::get_if<FunctionPtr>( &something_ )  )
		return function_type->get();
	return nullptr;
}

const FunctionPointerType* Type::GetFunctionPointerType() const
{
	if( const auto function_pointer_type= std::get_if<FunctionPointerPtr>( &something_ ) )
		return function_pointer_type->get();
	return nullptr;
}

const ArrayType* Type::GetArrayType() const
{
	if( const auto array_type= std::get_if<ArrayPtr>( &something_ ) )
		return array_type->get();
	return nullptr;
}

const RawPointerType* Type::GetRawPointerType() const
{
	if( const auto raw_pointer_type= std::get_if<RawPointerPtr>( &something_ ) )
		return raw_pointer_type->get();
	return nullptr;
}

const TupleType* Type::GetTupleType() const
{
	if( const auto tuple_type= std::get_if<TupleTypePtr>( &something_ ) )
		return tuple_type->get();
	return nullptr;
}

ClassPtr Type::GetClassType() const
{
	if( const auto class_type= std::get_if<ClassPtr>( &something_ ) )
		return *class_type;
	return nullptr;
}

EnumPtr Type::GetEnumType() const
{
	if( const auto enum_type= std::get_if<EnumPtr>( &something_ ) )
		return *enum_type;
	return nullptr;
}

bool Type::ReferenceIsConvertibleTo( const Type& other ) const
{
	if( *this == other )
		return true;

	const Class* const class_type= GetClassType();
	const Class* const other_class_type= other.GetClassType();
	if( class_type != nullptr && other_class_type != nullptr )
	{
		for( const Class::Parent& parent : class_type->parents )
		{
			if( parent.class_ == other_class_type ||
				Type(parent.class_).ReferenceIsConvertibleTo( other ) )
				return true;
		}
	}

	return false;
}

bool Type::IsDefaultConstructible() const
{
	if( const auto fundamental_type= GetFundamentalType() )
		return fundamental_type->fundamental_type == U_FundamentalType::Void;
	else if( const auto class_type= GetClassType() )
		return class_type->is_default_constructible;
	else if( const auto array_type= GetArrayType() )
		return array_type->size == 0u || array_type->type.IsDefaultConstructible();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool default_constructible= true;
		for( const Type& element : tuple_type->elements )
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
	else if( const auto class_type= GetClassType() )
		return class_type->is_copy_constructible;
	else if( const auto array_type= GetArrayType() )
		return array_type->size == 0u || array_type->type.IsCopyConstructible();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool copy_constructible= true;
		for( const Type& element : tuple_type->elements )
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
	else if( const auto class_type= GetClassType() )
		return class_type->is_copy_assignable;
	else if( const auto array_type= GetArrayType() )
		return array_type->size == 0u || array_type->type.IsCopyAssignable();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool copy_assignable= true;
		for( const Type& element : tuple_type->elements )
			copy_assignable= copy_assignable && element.IsCopyAssignable();
		return copy_assignable;
	}

	return false;
}

bool Type::IsEqualityComparable() const
{
	if( GetFundamentalType() != nullptr ||
		GetEnumType() != nullptr ||
		GetRawPointerType() != nullptr ||
		GetFunctionPointerType() != nullptr )
		return true;
	else if( const auto class_type= GetClassType() )
		return class_type->is_equality_comparable;
	else if( const auto array_type= GetArrayType() )
		return array_type->type.IsEqualityComparable();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool equality_comparable= true;
		for( const Type& element : tuple_type->elements )
			equality_comparable= equality_comparable && element.IsEqualityComparable();
		return equality_comparable;
	}

	return false;
}

bool Type::HaveDestructor() const
{
	if( const auto class_type= GetClassType() )
		return class_type->have_destructor;
	else if( const auto array_type= GetArrayType() )
		return array_type->type.HaveDestructor();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool have_destructor= false;
		for( const Type& element : tuple_type->elements )
			have_destructor= have_destructor || element.HaveDestructor();
		return have_destructor;
	}

	return false;
}

bool Type::CanBeConstexpr() const
{
	if( GetFundamentalType() != nullptr ||
		GetEnumType() != nullptr ||
		GetFunctionPointerType() != nullptr )
		return true;
	else if( GetRawPointerType() != nullptr )
		return false; // Raw pointer type is not constexpr.
	else if( const auto class_type= GetClassType() )
		return class_type->can_be_constexpr;
	else if( const auto array_type= GetArrayType() )
		return array_type->type.CanBeConstexpr();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool can_be_constexpr= true;
		for( const Type& element : tuple_type->elements )
			can_be_constexpr= can_be_constexpr && element.CanBeConstexpr();
		return can_be_constexpr;
	}

	return false;
}

bool Type::IsAbstract() const
{
	if( GetFundamentalType() != nullptr ||
		GetEnumType() != nullptr ||
		GetRawPointerType() != nullptr ||
		GetFunctionPointerType() != nullptr )
		return false;
	else if( const auto class_type= GetClassType() )
		return class_type->kind == Class::Kind::Abstract || class_type->kind == Class::Kind::Interface;
	else if( const auto array_type= GetArrayType() )
		return array_type->size > 0u && array_type->type.IsAbstract();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool is_abstract= false;
		for( const Type& element : tuple_type->elements )
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
	if( const auto class_type= GetClassType() )
		return class_type->inner_reference_type;
	else if( const auto array_type= GetArrayType() )
		return array_type->type.GetInnerReferenceType();
	else if( const auto tuple_type= GetTupleType() )
	{
		InnerReferenceType result= InnerReferenceType::None;
		for( const Type& element : tuple_type->elements )
			result= std::max( result, element.GetInnerReferenceType() );
		return result;
	}

	return InnerReferenceType::None;
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

		std::string operator()( const TupleTypePtr& tuple ) const
		{
			std::string res= "tup[ ";

			for( const Type& element_type : tuple->elements )
			{
				res+= element_type.ToString();
				if( &element_type != & tuple->elements.back() )
					res+= ", ";
			}
			res+= " ]";
			return res;
		}

		std::string operator()( const ClassPtr& class_ ) const
		{
			std::string result;
			if( class_->typeinfo_type != std::nullopt )
			{
				result= Keyword(Keywords::typeof_);
				result+= "(";
				result+= Keyword(Keywords::typeinfo_);
				result+= "</";
				result+= class_->typeinfo_type->ToString();
				result+= "/>";
				result+=")";
			}
			else if( class_->base_template != std::nullopt )
			{
				// Skip template parameters namespace.
				const std::string template_namespace_name= class_->members->GetParent()->GetParent()->ToString();
				if( !template_namespace_name.empty() )
					result+= template_namespace_name + "::";

				const std::string& class_name= class_->base_template->class_template->syntax_element->name_;
				result+= class_name;
				result+= "</";
				for( const TemplateArg& arg : class_->base_template->signature_args )
				{
					if( const Type* const param_as_type = std::get_if<Type>( &arg ) )
						result+= param_as_type->ToString();
					else if( const Variable* const param_as_variable= std::get_if<Variable>( &arg ) )
						result+= ConstantVariableToString( *param_as_variable );
					else U_ASSERT(false);

					if( &arg != &class_->base_template->signature_args.back() )
						result+= ", ";
				}
				result+= "/>";
			}
			else
				result+= class_->members->ToString();
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
		std::string ProcessFunctionType( const FunctionType& function ) const
		{
			// TODO - actualize this
			std::string result;
			result+= "fn ";
			result+= function.return_type.ToString();
			result+= " ( ";
			result+= FunctionParamsToString( function.params );
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

		size_t operator()( const TupleTypePtr& tuple ) const
		{
			size_t hash= 0;
			for( const Type& element : tuple->elements )
				hash= llvm::hash_combine( hash, element.Hash() );
			return hash;
		}

		size_t operator()( const ClassPtr& class_ ) const
		{
			return size_t(reinterpret_cast<uintptr_t>(class_));
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
		size_t ProcessFunctionType( const FunctionType& function ) const
		{
			size_t hash= 0;
			for( const FunctionType::Param& param : function.params )
				hash= llvm::hash_combine( hash, param.type.Hash(), param.value_type );

			hash=
				llvm::hash_combine(
					hash,
					function.return_type.Hash(),
					function.return_value_type,
					function.unsafe );

			for( const FunctionType::ParamReference& param_reference : function.return_references )
				hash= llvm::hash_combine( hash, param_reference );

			for( const FunctionType::ReferencePollution& reference_pollution : function.references_pollution )
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
		return l.GetClassType() == r.GetClassType();
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

bool operator==( const ArrayType& l, const ArrayType& r )
{
	return l.type == r.type && l.size == r.size;
}

bool operator!=( const ArrayType& l, const ArrayType& r )
{
	return !( l == r );
}

bool operator==( const RawPointerType& l, const RawPointerType& r )
{
	return l.type == r.type;
}

bool operator!=( const RawPointerType& l, const RawPointerType& r )
{
	return !( l == r );
}

//
// FunctionType
//

constexpr uint8_t FunctionType::c_arg_reference_tag_number;

bool FunctionType::PointerCanBeConvertedTo( const FunctionType& other ) const
{
	const FunctionType&  src_function_type= *this;
	const FunctionType& dst_function_type= other;
	if( src_function_type.return_type != dst_function_type.return_type )
		return false;

	if( ( src_function_type.return_value_type == ValueType::Value ) != ( dst_function_type.return_value_type == ValueType::Value ) )
		return false;

	if( src_function_type.return_value_type == ValueType::ReferenceImut && dst_function_type.return_value_type == ValueType::ReferenceMut )
		return false; // Allow mutability conversions, except mut->imut

	if( src_function_type.params.size() != dst_function_type.params.size() )
		return false;
	for( size_t i= 0u; i < src_function_type.params.size(); ++i )
	{
		if( src_function_type.params[i].type != dst_function_type.params[i].type )
			return false;

		// Disable conversion for reference and value args.
		if( ( src_function_type.params[i].value_type == ValueType::Value ) != ( dst_function_type.params[i].value_type == ValueType::Value ) )
			return false;

		// Allow mutability conversions, except mut->imut
		if( src_function_type.params[i].value_type == ValueType::ReferenceMut && dst_function_type.params[i].value_type == ValueType::ReferenceImut )
			return false;
	}

	// We can convert function, returning less references to function, returning more referenes.
	for( const FunctionType::ParamReference& src_inner_arg_reference : src_function_type.return_references )
	{
		bool found= false;
		for( const FunctionType::ParamReference& dst_inner_arg_reference : dst_function_type.return_references )
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
	for( const FunctionType::ReferencePollution& src_pollution : src_function_type.references_pollution )
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

bool FunctionType::IsStructRet() const
{
	return
		return_value_type == ValueType::Value &&
		( return_type.GetClassType() != nullptr || return_type.GetArrayType() != nullptr || return_type.GetTupleType() != nullptr );
}

bool operator==( const FunctionType::Param& l, const FunctionType::Param& r )
{
	return l.type == r.type && l.value_type == r.value_type;
}

bool operator!=( const FunctionType::Param& l, const FunctionType::Param& r )
{
	return !( l == r );
}

bool FunctionType::ReferencePollution::operator==( const ReferencePollution& other ) const
{
	return this->dst == other.dst && this->src == other.src;
}

bool FunctionType::ReferencePollution::operator<( const ReferencePollution& other ) const
{
	// Order is significant, because references pollution is part of stable function type.
	if( this->dst != other.dst )
		return this->dst < other.dst;
	return this->src < other.src;
}

bool operator==( const FunctionType& l, const FunctionType& r )
{
	return
		l.return_type == r.return_type &&
		l.return_value_type == r.return_value_type &&
		l.params == r.params &&
		l.return_references == r.return_references &&
		l.references_pollution == r.references_pollution &&
		l.unsafe == r.unsafe &&
		l.calling_convention == r.calling_convention;
}

bool operator!=( const FunctionType& l, const FunctionType& r )
{
	return !( l == r );
}

std::string FunctionParamsToString( const ArgsVector<FunctionType::Param>& params )
{
	std::string result;
	for( const FunctionType::Param& param : params )
	{
		if( param.value_type == ValueType::ReferenceMut )
			result+= "&mut ";
		if( param.value_type == ValueType::ReferenceImut )
			result+= "&imut ";

		result+= param.type.ToString();
		if( &param != &params.back() )
			result+= ", ";
	}

	return result;
}

//
// FunctionPointer
//

bool operator==( const FunctionPointerType& l, const FunctionPointerType& r )
{
	return l.function == r.function;
}
bool operator!=( const FunctionPointerType& l, const FunctionPointerType& r )
{
	return !( r == l );
}

} // namespace U
