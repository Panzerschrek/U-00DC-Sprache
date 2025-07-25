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
	enum class ArgumentClass
	{
		NoClass,
		Integer,
		SSE,
		SSEUp,
		x87,
		x87Up,
		Complex_x87,
		Memory,
	};

	static constexpr size_t c_max_argument_parts= 2;
	using ArgumentPartClasses= std::array<ArgumentClass, c_max_argument_parts>;

	void ClassifyType_r( llvm::Type& llvm_type, ArgumentPartClasses& out_classes, const uint64_t offset );

	static void MergeArgumentClasses( ArgumentClass& dst, const ArgumentClass src );

	static void PostMergeArgumentClasses( ArgumentPartClasses& classes );

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

	// Composite types are left.

	if( type.GetClassType() != nullptr || type.GetArrayType() != nullptr || type.GetTupleType() != nullptr )
	{
		llvm::Type* const llvm_type= type.GetLLVMType();

		const uint64_t type_size= data_layout_.getTypeAllocSize( llvm_type );
		if( type_size > 16 )
			return ArgumentPassingInStack{};

		if( type_size == 0 )
		{
			// TODO - handle zero-sized structs properly.
			return ArgumentPassingInStack{};
		}

		ArgumentPartClasses classes;
		for( ArgumentClass& c : classes )
			c= ArgumentClass::NoClass;

		uint64_t offset= 0;
		ClassifyType_r( *llvm_type, classes, offset );
		PostMergeArgumentClasses( classes );

		if( classes[0] == ArgumentClass::Memory )
			return ArgumentPassingInStack{};

		llvm::LLVMContext& llvm_context= llvm_type->getContext();

		ArgumentPassingDirect argument_passing;
		// Always use original type alignment.
		argument_passing.load_store_alignment= uint16_t( data_layout_.getABITypeAlign( llvm_type ).value() );

		if( type_size <= 8 )
		{
			if( classes[0] == ArgumentClass::Integer )
			{
				argument_passing.llvm_type= llvm::IntegerType::get( llvm_context, uint32_t(type_size) * 8 );
				argument_passing.zext= true; // TODO - check if it's correct.
			}
			else if( classes[0] == ArgumentClass::SSE )
				argument_passing.llvm_type= type_size <= 4 ? llvm::Type::getFloatTy( llvm_context ) : llvm::Type::getDoubleTy( llvm_type->getContext() );
			else U_ASSERT(false);
		}
		else if( type_size <= 16 )
		{
			constexpr size_t num_parts= 2;
			std::array<llvm::Type*, num_parts> types{};
			const uint32_t part_sizes[2]{ 8u, uint32_t(type_size)  - 8u };
			for( size_t part= 0; part < num_parts; ++part )
			{
				if( classes[part] == ArgumentClass::Integer )
					types[part]= llvm::IntegerType::get( llvm_context, part_sizes[part] * 8 );
				else if( classes[part] == ArgumentClass::SSE )
					types[part]= part_sizes[part] <= 4 ? llvm::Type::getFloatTy( llvm_context ) : llvm::Type::getDoubleTy( llvm_type->getContext() );
				else U_ASSERT(false);
			}

			// Create a touple for two parts.
			argument_passing.llvm_type= llvm::StructType::get( llvm_context, llvm::ArrayRef<llvm::Type*>( types ) );
			// TODO - set zext/sext?
		}
		else U_ASSERT( false );

		return argument_passing;
	}
	else
	{

		// Unhandled type kind.
		U_ASSERT(false);
		return ArgumentPassingByPointer{};
	}
}

void CallingConventionInfoSystemVX86_64::ClassifyType_r( llvm::Type& llvm_type, ArgumentPartClasses& out_classes, const uint64_t offset )
{
	if( llvm_type.isPointerTy() )
		MergeArgumentClasses( out_classes[ offset >> 3 ], ArgumentClass::Integer );
	else if( llvm_type.isIntegerTy() )
	{
		const uint64_t size= llvm_type.getIntegerBitWidth() / 8;
		MergeArgumentClasses( out_classes[ offset >> 3 ], ArgumentClass::Integer );
		if( size > 8 )
			MergeArgumentClasses( out_classes[ ( offset + 8 ) >> 3 ], ArgumentClass::Integer );
	}
	else if( llvm_type.isFloatTy() || llvm_type.isDoubleTy() )
		MergeArgumentClasses( out_classes[ offset >> 3 ], ArgumentClass::SSE );
	else if( const auto array_type= llvm::dyn_cast<llvm::ArrayType>( &llvm_type ) )
	{
		llvm::Type* const element_type= array_type->getElementType();
		const uint64_t element_size= data_layout_.getTypeAllocSize( element_type );
		for( uint64_t element_index= 0; element_index < array_type->getNumElements(); ++element_index )
			ClassifyType_r( *element_type, out_classes, offset + element_index * element_size );
	}
	else if( const auto struct_type= llvm::dyn_cast<llvm::StructType>( &llvm_type ) )
	{
		const llvm::StructLayout* const struct_layout= data_layout_.getStructLayout( struct_type );
		for( uint32_t element_index= 0; element_index < struct_type->getNumElements(); ++element_index )
			ClassifyType_r( *llvm_type.getStructElementType( element_index ), out_classes, offset + struct_layout->getElementOffset( element_index ) );
	}
	else U_ASSERT( false ); // Unhandled type kind.
}

void CallingConventionInfoSystemVX86_64::MergeArgumentClasses( ArgumentClass& dst, const ArgumentClass src )
{
	if( dst == src )
	{}
	else if( src == ArgumentClass::NoClass )
	{}
	else if( dst == ArgumentClass::NoClass )
		dst= src;
	else if( src == ArgumentClass::Memory )
		dst= ArgumentClass::Memory;
	else if( src == ArgumentClass::Integer )
		dst= ArgumentClass::Integer;
	else if( dst == ArgumentClass::Integer )
	{}
	else if( src == ArgumentClass::x87 || src == ArgumentClass::x87Up || src == ArgumentClass::Complex_x87 )
		dst= ArgumentClass::Memory;
	else
		dst= ArgumentClass::SSE;
}

void CallingConventionInfoSystemVX86_64::PostMergeArgumentClasses( ArgumentPartClasses& classes )
{
	// If some of classes is memory - make all memory.
	for( const ArgumentClass c : classes )
	{
		if( c == ArgumentClass::Memory )
		{
			for( ArgumentClass& other_c : classes )
				other_c= ArgumentClass::Memory;
			return;
		}
	}

	// Do not bother for now with x87.
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
