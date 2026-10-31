#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Hashing.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "class.hpp"
#include "coroutine.hpp"
#include "enum.hpp"
#include "template_types.hpp"
#include "../../lex_synt_lib_common/size_assert.hpp"
#include "type.hpp"

namespace U
{

namespace
{

template<typename T>
llvm::Type* GetLLVMTypeImpl( const T& el )
{
	return el.llvm_type;
}

template<typename T>
llvm::Type* GetLLVMTypeImpl( const RcPtr<T>& boxed )
{
	U_ASSERT(boxed != nullptr);
	return GetLLVMTypeImpl( *boxed );
}

llvm::Type* GetLLVMTypeImpl( const ClassPtr class_ )
{
	U_ASSERT( class_ != nullptr );
	return class_->llvm_type;
}

llvm::Type* GetLLVMTypeImpl( const EnumPtr enum_ )
{
	U_ASSERT( enum_ != nullptr );
	return enum_->underlying_type.llvm_type;
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

void FundamentalType::AppendToString( std::string& s ) const
{
	s+= GetFundamentalTypeName( fundamental_type );
}

bool operator==( const FundamentalType& l, const FundamentalType& r )
{
	return l.fundamental_type == r.fundamental_type;
}

//
// Tuple
//

void TupleType::AppendToString( std::string& s ) const
{
	if( element_types.empty() )
		s+= "tup[]";
	else
	{
		s+= "tup[ ";

		for( const Type& element_type : element_types )
		{
			element_type.AppendToString( s );
			if( &element_type != & element_types.back() )
				s+= ", ";
		}
		s+= " ]";
	}
}

bool operator==( const TupleType& l, const TupleType& r )
{
	return l.element_types == r.element_types;
}

//
// Type
//

// No more, than 3 pointers on 64 bit platform.
SIZE_ASSERT( Type, 24u )

Type::Type( FundamentalType fundamental_type )
	: something_( std::move(fundamental_type) )
{}

Type::Type( FunctionPointerType function_pointer_type )
	: something_( MakeRcPtr( std::move(function_pointer_type) ) )
{}

Type::Type( ArrayType array_type )
	: something_( MakeRcPtr( std::move(array_type) ) )
{}

Type::Type( RawPointerType raw_pointer_type )
	: something_( MakeRcPtr( std::move(raw_pointer_type) ) )
{}

Type::Type( TupleType tuple_type )
	: something_( MakeRcPtr( std::move(tuple_type) ) )
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

const FunctionPointerType* Type::GetFunctionPointerType() const
{
	if( const auto function_pointer_type= std::get_if<FunctionPointerPtr>( &something_ ) )
		return &**function_pointer_type;
	return nullptr;
}

const ArrayType* Type::GetArrayType() const
{
	if( const auto array_type= std::get_if<ArrayPtr>( &something_ ) )
		return &**array_type;
	return nullptr;
}

const RawPointerType* Type::GetRawPointerType() const
{
	if( const auto raw_pointer_type= std::get_if<RawPointerPtr>( &something_ ) )
		return &**raw_pointer_type;
	return nullptr;
}

const TupleType* Type::GetTupleType() const
{
	if( const auto tuple_type= std::get_if<TupleTypePtr>( &something_ ) )
		return &**tuple_type;
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
		return fundamental_type->fundamental_type == U_FundamentalType::void_;
	else if( const auto class_type= GetClassType() )
		return class_type->is_default_constructible;
	else if( const auto array_type= GetArrayType() )
		return array_type->element_count == 0u || array_type->element_type.IsDefaultConstructible();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool default_constructible= true;
		for( const Type& element : tuple_type->element_types )
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
		return array_type->element_count == 0u || array_type->element_type.IsCopyConstructible();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool copy_constructible= true;
		for( const Type& element : tuple_type->element_types )
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
		return array_type->element_count == 0u || array_type->element_type.IsCopyAssignable();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool copy_assignable= true;
		for( const Type& element : tuple_type->element_types )
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
		return array_type->element_type.IsEqualityComparable();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool equality_comparable= true;
		for( const Type& element : tuple_type->element_types )
			equality_comparable= equality_comparable && element.IsEqualityComparable();
		return equality_comparable;
	}

	return false;
}

bool Type::HasDestructor() const
{
	if( const auto class_type= GetClassType() )
		return class_type->has_destructor;
	else if( const auto array_type= GetArrayType() )
		return array_type->element_type.HasDestructor();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool has_destructor= false;
		for( const Type& element : tuple_type->element_types )
			has_destructor= has_destructor || element.HasDestructor();
		return has_destructor;
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
		return array_type->element_type.CanBeConstexpr();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool can_be_constexpr= true;
		for( const Type& element : tuple_type->element_types )
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
		return array_type->element_count > 0u && array_type->element_type.IsAbstract();
	else if( const auto tuple_type= GetTupleType() )
	{
		bool is_abstract= false;
		for( const Type& element : tuple_type->element_types )
			is_abstract= is_abstract || element.IsAbstract();
		return is_abstract;
	}

	return false;
}

bool Type::IsValidForTemplateVariableArgument() const
{
	if( const FundamentalType* const fundamental= GetFundamentalType() )
	{
		return
			IsInteger( fundamental->fundamental_type ) ||
			IsChar( fundamental->fundamental_type ) ||
			IsByte( fundamental->fundamental_type ) ||
			fundamental->fundamental_type == U_FundamentalType::bool_;
	}
	else if( GetEnumType() != nullptr )
		return true; // Assuming floats aren't possible for enums underlying types.
	else if( const auto array_type= GetArrayType() )
	{
		// Arrays are allowed, as long as element types are valid.
		return array_type->element_type.IsValidForTemplateVariableArgument();
	}
	else if( const auto tuple_type= GetTupleType() )
	{
		// Tuples are allowed, as long as element types are valid.
		for( const Type& element_type : tuple_type->element_types )
		{
			if( !element_type.IsValidForTemplateVariableArgument() )
				return false;
		}
		return true;
	}

	return false;
}

bool Type::IsNoDiscard() const
{
	if( const auto class_type= GetClassType() )
		return class_type->no_discard;
	else if( const auto enum_type= GetEnumType() )
		return enum_type->no_discard;
	else if( const auto array_type= GetArrayType() )
		return array_type->element_type.IsNoDiscard();
	else if( const auto tuple_type= GetTupleType() )
	{
		for( const Type& element : tuple_type->element_types )
			if( element.IsNoDiscard() )
				return true;
		return false;
	}

	// Other type kinds aren't "nodiscard".

	return false;
}

size_t Type::ReferenceTagCount() const
{
	if( const auto class_type= GetClassType() )
		return class_type->inner_references.size();
	else if( const auto array_type= GetArrayType() )
		return array_type->element_type.ReferenceTagCount();
	else if( const auto tuple_type= GetTupleType() )
	{
		// Combine all tags of tuple elements.
		size_t result= 0;
		for( const Type& element : tuple_type->element_types )
			result+= element.ReferenceTagCount();
		return result;
	}

	return 0;
}

InnerReferenceKind Type::GetInnerReferenceKind( const size_t index ) const
{
	U_ASSERT( index < ReferenceTagCount() );

	if( const auto class_type= GetClassType() )
	{
		U_ASSERT( index < class_type->inner_references.size() );
		return class_type->inner_references[index].kind;
	}
	else if( const auto array_type= GetArrayType() )
		return array_type->element_type.GetInnerReferenceKind(index);
	else if( const auto tuple_type= GetTupleType() )
	{
		size_t offset= 0;
		for( const Type& element : tuple_type->element_types )
		{
			const size_t count= element.ReferenceTagCount();
			if( index >= offset && index < offset + count )
				return element.GetInnerReferenceKind( index - offset );
			offset+= count;
		}
		U_ASSERT(false); // Unreachable.
		return InnerReferenceKind::Imut;
	}

	U_ASSERT(false); // Unreachable - other types have 0 reference tags.
	return InnerReferenceKind::Imut;
}

SecondOrderInnerReferenceKind Type::GetSecondOrderInnerReferenceKind( const size_t index ) const
{
	U_ASSERT( index < ReferenceTagCount() );

	if( const auto class_type= GetClassType() )
	{
		U_ASSERT( index < class_type->inner_references.size() );
		return class_type->inner_references[index].second_order_kind;
	}
	else if( const auto array_type= GetArrayType() )
		return array_type->element_type.GetSecondOrderInnerReferenceKind(index);
	else if( const auto tuple_type= GetTupleType() )
	{
		size_t offset= 0;
		for( const Type& element : tuple_type->element_types )
		{
			const size_t count= element.ReferenceTagCount();
			if( index >= offset && index < offset + count )
				return element.GetSecondOrderInnerReferenceKind( index - offset );
			offset+= count;
		}
		U_ASSERT(false); // Unreachable.
		return SecondOrderInnerReferenceKind::None;
	}

	U_ASSERT(false); // Unreachable - other types have 0 reference tags.
	return SecondOrderInnerReferenceKind::None;
}

size_t Type::GetReferenceIndirectionDepth() const
{
	if( GetFundamentalType() != nullptr ||
		GetEnumType() != nullptr ||
		GetRawPointerType() != nullptr ||
		GetFunctionPointerType() != nullptr )
		return 0; // These type kinds don't contain references inside.

	if( const auto class_type= GetClassType() )
	{
		size_t depth= 0;
		for( const InnerReference& inner_reference : class_type->inner_references )
		{
			// For now no more than depth 2 is possible.
			depth= std::max( depth, size_t(1) + size_t(inner_reference.second_order_kind == SecondOrderInnerReferenceKind::None ? 0 : 1 ) );
		}

		return depth;
	}

	if( const auto array_type= GetArrayType() )
		return array_type->element_type.GetReferenceIndirectionDepth();

	if( const auto tuple_type= GetTupleType() )
	{
		size_t depth= 0;
		for( const Type& element_type : tuple_type->element_types )
			depth= std::max( depth, element_type.GetReferenceIndirectionDepth() );

		return depth;
	}

	U_ASSERT(false); // Unreachable type kind.
	return 0;
}

bool Type::ContainsMutableReferences() const
{
	if( GetFundamentalType() != nullptr ||
		GetEnumType() != nullptr ||
		GetRawPointerType() != nullptr ||
		GetFunctionPointerType() != nullptr )
		return false;
	else if( const auto class_type= GetClassType() )
	{
		for( const InnerReference& inner_reference : class_type->inner_references )
			if( inner_reference.kind == InnerReferenceKind::Mut || inner_reference.second_order_kind == SecondOrderInnerReferenceKind::Mut )
				return true;
	}
	else if( const auto array_type= GetArrayType() )
		return array_type->element_type.ContainsMutableReferences();
	else if( const auto tuple_type= GetTupleType() )
	{
		for( const Type& element : tuple_type->element_types )
			if( element.ContainsMutableReferences() )
				return true;
	} else U_ASSERT(false);

	return false;
}

llvm::Type* Type::GetLLVMType() const
{
	return std::visit( []( const auto& el ){ return GetLLVMTypeImpl( el ); }, something_ );
}

std::string Type::ToString() const
{
	std::string result;
	AppendToString( result );
	return result;
}

template<typename T> void TypeAppendToStringImpl( const T& t, std::string& s ) { t.AppendToString(s); }
template<typename T> void TypeAppendToStringImpl( T* const t, std::string& s ) { t->AppendToString(s); }
template<typename T> void TypeAppendToStringImpl( const RcPtr<T>& t, std::string& s ) { t->AppendToString(s); }

void Type::AppendToString( std::string& s ) const
{
	std::visit( [&]( const auto& t ) { TypeAppendToStringImpl( t, s ); }, something_ );
}

size_t Type::Hash() const
{
	struct Visitor
	{
		size_t operator()( const FundamentalType& fundamental ) const
		{
			return size_t(fundamental.fundamental_type);
		}

		size_t operator()( const ArrayPtr& array ) const
		{
			return llvm::hash_combine( array->element_type.Hash(), array->element_count );
		}

		size_t operator()( const RawPointerPtr& raw_pointer ) const
		{
			return raw_pointer->element_type.Hash();
		}

		size_t operator()( const TupleTypePtr& tuple ) const
		{
			size_t hash= 0;
			for( const Type& element : tuple->element_types )
				hash= llvm::hash_combine( hash, element.Hash() );
			return hash;
		}

		size_t operator()( const ClassPtr class_ ) const
		{
			return size_t(reinterpret_cast<uintptr_t>(class_));
		}

		size_t operator()( const EnumPtr enum_ ) const
		{
			return size_t(reinterpret_cast<uintptr_t>(enum_));
		}

		size_t operator()( const FunctionPointerPtr& function_pointer ) const
		{
			const FunctionType& function= function_pointer->function_type;

			size_t hash= 0;
			for( const FunctionType::Param& param : function.params )
				hash= llvm::hash_combine( hash, param.type.Hash(), param.value_type );

			hash=
				llvm::hash_combine(
					hash,
					function.return_type.Hash(),
					function.return_value_type,
					function.unsafe,
					function.is_variadic,
					function.calling_convention );

			for( const FunctionType::ParamReference& param_reference : function.return_references )
				hash= llvm::hash_combine( hash, param_reference );

			for( const auto& tags_set : function.return_inner_references )
				for( const FunctionType::ParamReference& param_reference : tags_set )
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

	switch( l.something_.index() )
	{
	case 0: return *l.GetFundamentalType() == *r.GetFundamentalType();
	case 1: return *l.GetArrayType() == *r.GetArrayType();
	case 2: return *l.GetRawPointerType() == *r.GetRawPointerType();
	case 3: return l.GetClassType() == r.GetClassType();
	case 4: return l.GetEnumType() == r.GetEnumType();
	case 5: return *l.GetFunctionPointerType() == *r.GetFunctionPointerType();
	case 6: return *l.GetTupleType() == *r.GetTupleType();
	}

	U_ASSERT(false);
	return false;
}

//
// Array
//

void ArrayType::AppendToString( std::string& s ) const
{
	s+= "[ ";
	element_type.AppendToString( s );
	s+= ", ";
	s+= std::to_string( element_count );
	s+= " ]";
}

bool operator==( const ArrayType& l, const ArrayType& r )
{
	return l.element_type == r.element_type && l.element_count == r.element_count;
}

//
// RawPointerType
//

void RawPointerType::AppendToString( std::string& s ) const
{
	s+= "$( ";
	element_type.AppendToString( s );
	s+= " )";
}

bool operator==( const RawPointerType& l, const RawPointerType& r )
{
	return l.element_type == r.element_type;
}

//
// CallingConvention
//

std::optional<CallingConvention> StringToCallingConvention( const std::string_view s )
{
	if( s == "default" || s == "Ü" )
		return CallingConvention::Default;
	if( s == "C" )
		return CallingConvention::C;
	if( s == "fast" )
		return CallingConvention::Fast;
	if( s == "cold" )
		return CallingConvention::Cold;
	if( s == "system" )
		return CallingConvention::System;

	return std::nullopt;
}

std::string_view CallingConventionToString( const CallingConvention c )
{
	switch(c)
	{
	case CallingConvention::Default: return "default";
	case CallingConvention::C: return "C";
	case CallingConvention::Fast: return "fast";
	case CallingConvention::Cold: return "cold";
	case CallingConvention::System: return "system";
	}

	U_ASSERT(false);
	return "";
}

//
// FunctionType
//

constexpr uint8_t FunctionType::c_param_reference_number;

bool FunctionType::PointerCanBeConvertedTo( const FunctionType& other ) const
{
	const FunctionType& src_function_type= *this;
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
	for( const ParamReference& src_inner_arg_reference : src_function_type.return_references )
	{
		bool found= false;
		for( const ParamReference& dst_inner_arg_reference : dst_function_type.return_references )
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

	// TODO - perform checks for inner tags.
	if( src_function_type.return_inner_references != dst_function_type.return_inner_references )
		return false;

	// We can convert function, linkink less references to function, linking more references
	for( const ReferencePollution& src_pollution : src_function_type.references_pollution )
	{
		 // TODO - maybe compare with mutability conversion possibility?
		if( !std::binary_search( // Use binary search, since this list should be sorted.
				dst_function_type.references_pollution.begin(),
				dst_function_type.references_pollution.end(),
				src_pollution ) )
			return false;
	}

	if( src_function_type.unsafe && !dst_function_type.unsafe )
		return false; // Conversion from unsafe to safe function is forbidden.

	if( src_function_type.is_variadic != dst_function_type.is_variadic )
		return false; // Changing variadic flag is forbiden.

	if( src_function_type.calling_convention != dst_function_type.calling_convention )
		return false;

	// Finally, we check all conditions
	return true;
}

bool FunctionType::ReturnsCompositeValue() const
{
	return
		return_value_type == ValueType::Value &&
		( return_type.GetClassType() != nullptr || return_type.GetArrayType() != nullptr || return_type.GetTupleType() != nullptr );
}

void FunctionType::AppendToString( std::string& s ) const
{
	s+= "fn";

	if( params.empty() )
	{
		if( is_variadic )
			s+= "( ... )";
		else
			s+= "()";
	}
	else
	{
		s+= "( ";
		s+= FunctionParamsToString( params );

		if( is_variadic )
			s+= ", ...";

		s+= " )";
	}
	if( unsafe )
		s+= " unsafe";

	switch( calling_convention )
	{
	case CallingConvention::Default:
		break;
	case CallingConvention::C:
		s+= " call_conv(\"C\")";
		break;
	case CallingConvention::Fast:
		s+= " call_conv(\"fast\")";
		break;
	case CallingConvention::Cold:
		s+= " call_conv(\"cold\")";
		break;
	case CallingConvention::System:
		s+= " call_conv(\"system\")";
		break;
	}

	s+= " : ";
	return_type.AppendToString( s );

	if( return_value_type == ValueType::ReferenceMut )
		s += " &mut";
	if( return_value_type == ValueType::ReferenceImut )
		s += " &imut";
}

bool operator==( const FunctionType::Param& l, const FunctionType::Param& r )
{
	return l.type == r.type && l.value_type == r.value_type;
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
		l.return_inner_references == r.return_inner_references &&
		l.references_pollution == r.references_pollution &&
		l.unsafe == r.unsafe &&
		l.is_variadic == r.is_variadic &&
		l.calling_convention == r.calling_convention;
}

std::string FunctionParamsToString( const llvm::ArrayRef<FunctionType::Param> params )
{
	std::string result;
	for( const FunctionType::Param& param : params )
	{
		result+= param.type.ToString();
		result+= " ";

		if( param.value_type == ValueType::ReferenceMut )
			result+= "&mut ";
		if( param.value_type == ValueType::ReferenceImut )
			result+= "&imut ";

		result+= "_"; // use some dummy for param name.

		if( &param != &params.back() )
			result+= ", ";
	}

	return result;
}

//
// FunctionPointer
//

void FunctionPointerType::AppendToString( std::string& s ) const
{
	function_type.AppendToString( s );
}

bool operator==( const FunctionPointerType& l, const FunctionPointerType& r )
{
	return l.function_type == r.function_type;
}

} // namespace U
