#include "../../lex_synt_lib_common/assert.hpp"

#include "class.hpp"
#include "enum.hpp"
#include "calling_convention_info.hpp"

namespace U
{

namespace
{

llvm::Type* GetSingleScalarType( llvm::Type* type )
{
	U_ASSERT( type->isSized() && "expected sized type!" );

	while( true )
	{
		if( type->isStructTy() && type->getStructNumElements() == 1 )
		{
			type= type->getStructElementType(0);
			continue;
		}
		if( type->isArrayTy() && type->getArrayNumElements() == 1 )
		{
			type= type->getArrayElementType();
			continue;
		}

		break; // Not a composite.
	}

	if( type->isIntegerTy() || type->isFloatingPointTy() || type->isPointerTy() )
		return type;

	return nullptr;
}

class CallingConventionInfoDefault final : public ICallingConventionInfo
{
public:
	explicit CallingConventionInfoDefault( llvm::DataLayout data_layout );

public: // ICallingConventionInfo
	virtual ArgumentPassing CalculareValueArgumentPassingInfo( const Type& type ) override;

private:
	const llvm::DataLayout data_layout_;
};

CallingConventionInfoDefault::CallingConventionInfoDefault( llvm::DataLayout data_layout )
	: data_layout_( std::move(data_layout) )
{}

ICallingConventionInfo::ArgumentPassing CallingConventionInfoDefault::CalculareValueArgumentPassingInfo( const Type& type )
{
	if( const auto f= type.GetFundamentalType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= f->llvm_type;
		argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( f->llvm_type ).value() );
		if( IsSignedInteger( f->fundamental_type ) )
			argument_passing.sext= true;
		else if(
			IsUnsignedInteger( f->fundamental_type ) ||
			IsChar( f->fundamental_type ) ||
			IsByte( f->fundamental_type ) ||
			f->fundamental_type == U_FundamentalType::bool_ )
			argument_passing.zext= true;

		return argument_passing;
	}

	if( const auto e= type.GetEnumType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= e->underlying_type.llvm_type;
		argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( e->underlying_type.llvm_type ).value() );
		argument_passing.zext= true; // Enums are usniged.
		return argument_passing;
	}

	if( const auto fp= type.GetFunctionPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= fp->llvm_type;
		argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( fp->llvm_type ).value() );
		// It seems like zero extension isn't necessary for pointers.
		return argument_passing;
	}

	if( const auto p= type.GetRawPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= p->llvm_type;
		argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( p->llvm_type ).value() );
		// It seems like zero extension isn't necessary for pointers.
		return argument_passing;
	}

	if( const auto c= type.GetClassType() )
	{
		if( const auto single_scalar= GetSingleScalarType( c->llvm_type ) )
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= single_scalar;
			argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( single_scalar ).value() );
			// TODO - set sext/zext?
			return argument_passing;
		}
		else
			return ArgumentPassingByPointer{};
	}

	if( const auto a= type.GetArrayType() )
	{
		if( const auto single_scalar= GetSingleScalarType( a->llvm_type ) )
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= single_scalar;
			argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( single_scalar ).value() );
			// TODO - set sext/zext?
			return argument_passing;
		}
		else
			return ArgumentPassingByPointer{};
	}

	if( const auto t= type.GetTupleType() )
	{
		if( const auto single_scalar= GetSingleScalarType( t->llvm_type ) )
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= single_scalar;
			argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( single_scalar ).value() );
			// TODO - set sext/zext?
			return argument_passing;
		}
		else
			return ArgumentPassingByPointer{};
	}

	// Unhandled type kind.
	U_ASSERT(false);
	return ArgumentPassingByPointer{};
}

class CallingConventionInfoSystemVX86_64 final : public ICallingConventionInfo
{
public:
	explicit CallingConventionInfoSystemVX86_64( llvm::DataLayout data_layout );

public: // ICallingConventionInfo
	virtual ArgumentPassing CalculareValueArgumentPassingInfo( const Type& type ) override;

private:
	const llvm::DataLayout data_layout_;
};

CallingConventionInfoSystemVX86_64::CallingConventionInfoSystemVX86_64( llvm::DataLayout data_layout )
	: data_layout_( std::move(data_layout ) )
{}

ICallingConventionInfo::ArgumentPassing CallingConventionInfoSystemVX86_64::CalculareValueArgumentPassingInfo( const Type& type )
{
	// TODO - rework this properly.

	if( const auto f= type.GetFundamentalType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= f->llvm_type;
		argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( f->llvm_type ).value() );
		if( IsSignedInteger( f->fundamental_type ) )
			argument_passing.sext= true;
		else if(
			IsUnsignedInteger( f->fundamental_type ) ||
			IsChar( f->fundamental_type ) ||
			IsByte( f->fundamental_type ) ||
			f->fundamental_type == U_FundamentalType::bool_ )
			argument_passing.zext= true;

		return argument_passing;
	}

	if( const auto e= type.GetEnumType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= e->underlying_type.llvm_type;
		argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( e->underlying_type.llvm_type ).value() );
		argument_passing.zext= true; // Enums are usniged.
		return argument_passing;
	}

	if( const auto fp= type.GetFunctionPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= fp->llvm_type;
		argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( fp->llvm_type ).value() );
		// It seems like zero extension isn't necessary for pointers.
		return argument_passing;
	}

	if( const auto p= type.GetRawPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= p->llvm_type;
		argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( p->llvm_type ).value() );
		// It seems like zero extension isn't necessary for pointers.
		return argument_passing;
	}

	if( const auto c= type.GetClassType() )
	{
		if( const auto single_scalar= GetSingleScalarType( c->llvm_type ) )
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= single_scalar;
			argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( single_scalar ).value() );
			// TODO - set sext/zext?
			return argument_passing;
		}
		else
			return ArgumentPassingByPointer{};
	}

	if( const auto a= type.GetArrayType() )
	{
		if( const auto single_scalar= GetSingleScalarType( a->llvm_type ) )
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= single_scalar;
			argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( single_scalar ).value() );
			// TODO - set sext/zext?
			return argument_passing;
		}
		else
			return ArgumentPassingByPointer{};
	}

	if( const auto t= type.GetTupleType() )
	{
		if( const auto single_scalar= GetSingleScalarType( t->llvm_type ) )
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= single_scalar;
			argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( single_scalar ).value() );
			// TODO - set sext/zext?
			return argument_passing;
		}
		else
			return ArgumentPassingByPointer{};
	}

	// Unhandled type kind.
	U_ASSERT(false);
	return ArgumentPassingByPointer{};
}

} // namespace

CallingConventionInfos CreateCallingConventionInfos( const llvm::Triple& target_triple, const llvm::DataLayout& data_layout )
{
	CallingConventionInfos calling_convention_infos;

	const auto default_info= std::make_shared<CallingConventionInfoDefault>( data_layout );

	calling_convention_infos[ size_t( CallingConvention::Default ) ]= default_info;
	calling_convention_infos[ size_t( CallingConvention::C ) ]= default_info;
	calling_convention_infos[ size_t( CallingConvention::Fast ) ]= default_info;
	calling_convention_infos[ size_t( CallingConvention::Cold ) ]= default_info;
	calling_convention_infos[ size_t( CallingConvention::System ) ]= default_info;

	if( target_triple.getArch() == llvm::Triple::x86_64 )
	{
		// TODO - check for more system using this ABI.
		if( target_triple.getOS() == llvm::Triple::Linux || target_triple.getOS() == llvm::Triple::FreeBSD )
		{

			const auto system_v_x86_64_info= std::make_shared<CallingConventionInfoSystemVX86_64>( data_layout );
			calling_convention_infos[ size_t( CallingConvention::C ) ]= system_v_x86_64_info;
			calling_convention_infos[ size_t( CallingConvention::System ) ]= system_v_x86_64_info;
		}
	}
	else
	{
		// TODO - handle other architectures.
	}

	return calling_convention_infos;
}

} // namespace U
