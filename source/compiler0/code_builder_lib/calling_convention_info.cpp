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
	virtual ReturnValuePassing CalculateReturnValuePassingInfo( const Type& type ) override;
	virtual CallInfo CalculateFunctionCallInfo( const FunctionType& function_type ) override;

private:
	ArgumentPassing CalculateValueArgumentPassingInfo( const Type& type );

private:
	const llvm::DataLayout data_layout_;
};

CallingConventionInfoDefault::CallingConventionInfoDefault( llvm::DataLayout data_layout )
	: data_layout_( std::move(data_layout) )
{}

ICallingConventionInfo::ReturnValuePassing CallingConventionInfoDefault::CalculateReturnValuePassingInfo( const Type& type )
{
	if( const auto f= type.GetFundamentalType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= f->llvm_type;
		return return_value_passing;
	}

	if( const auto e= type.GetEnumType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= e->underlying_type.llvm_type;
		return return_value_passing;
	}

	if( const auto fp= type.GetFunctionPointerType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= fp->llvm_type;
		return return_value_passing;
	}

	if( const auto p= type.GetRawPointerType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= p->llvm_type;
		return return_value_passing;
	}

	if( const auto c= type.GetClassType() )
	{
		if( const auto single_scalar= GetSingleScalarType( c->llvm_type ) )
		{
			ReturnValuePassingDirect return_value_passing;
			return_value_passing.llvm_type= single_scalar;
			return return_value_passing;
		}
		else
			return ReturnValuePassingByPointer{};
	}

	if( const auto a= type.GetArrayType() )
	{
		if( const auto single_scalar= GetSingleScalarType( a->llvm_type ) )
		{
			ReturnValuePassingDirect return_value_passing;
			return_value_passing.llvm_type= single_scalar;
			return return_value_passing;
		}
		else
			return ReturnValuePassingByPointer{};
	}

	if( const auto t= type.GetTupleType() )
	{
		if( const auto single_scalar= GetSingleScalarType( t->llvm_type ) )
		{
			ReturnValuePassingDirect return_value_passing;
			return_value_passing.llvm_type= single_scalar;
			return return_value_passing;
		}
		else
			return ReturnValuePassingByPointer{};
	}

	// Unhandled type kind.
	U_ASSERT(false);
	return ReturnValuePassingByPointer{};
}

ICallingConventionInfo::CallInfo CallingConventionInfoDefault::CalculateFunctionCallInfo( const FunctionType& function_type )
{
	CallInfo call_info;

	if( function_type.return_value_type == ValueType::Value )
		call_info.return_value_passing= CalculateReturnValuePassingInfo( function_type.return_type );
	else
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= function_type.return_type.GetLLVMType()->getPointerTo();
		call_info.return_value_passing= std::move(return_value_passing);
	}

	call_info.arguments_passing.resize( function_type.params.size() );
	for( size_t i= 0; i < function_type.params.size(); ++i )
	{
		const FunctionType::Param& param= function_type.params[i];
		if( param.value_type == ValueType::Value )
			call_info.arguments_passing[i]= CalculateValueArgumentPassingInfo( param.type );
		else
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= param.type.GetLLVMType()->getPointerTo();
			call_info.arguments_passing[i]= std::move(argument_passing);
		}
	}

	return call_info;
}


ICallingConventionInfo::ArgumentPassing CallingConventionInfoDefault::CalculateValueArgumentPassingInfo( const Type& type )
{
	if( const auto f= type.GetFundamentalType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= f->llvm_type;
		return argument_passing;
	}

	if( const auto e= type.GetEnumType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= e->underlying_type.llvm_type;
		return argument_passing;
	}

	if( const auto fp= type.GetFunctionPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= fp->llvm_type;
		return argument_passing;
	}

	if( const auto p= type.GetRawPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= p->llvm_type;
		return argument_passing;
	}

	if( const auto c= type.GetClassType() )
	{
		if( const auto single_scalar= GetSingleScalarType( c->llvm_type ) )
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= single_scalar;
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
			return argument_passing;
		}
		else
			return ArgumentPassingByPointer{};
	}

	// Unhandled type kind.
	U_ASSERT(false);
	return ArgumentPassingByPointer{};
}

class CallingConventionInfoSystemV_X86_64 final : public ICallingConventionInfo
{
public:
	explicit CallingConventionInfoSystemV_X86_64( llvm::DataLayout data_layout );

public: // ICallingConventionInfo
	virtual ReturnValuePassing CalculateReturnValuePassingInfo( const Type& type ) override;
	virtual CallInfo CalculateFunctionCallInfo( const FunctionType& function_type ) override;

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

private:
	void ClassifyType_r( llvm::Type& llvm_type, ArgumentPartClasses& out_classes, const uint32_t offset );

	static void MergeArgumentClasses( ArgumentClass& dst, const ArgumentClass src );

	static void PostMergeArgumentClasses( ArgumentPartClasses& classes );

private:
	const llvm::DataLayout data_layout_;
};

CallingConventionInfoSystemV_X86_64::CallingConventionInfoSystemV_X86_64( llvm::DataLayout data_layout )
	: data_layout_( std::move(data_layout ) )
{}

ICallingConventionInfo::ReturnValuePassing CallingConventionInfoSystemV_X86_64::CalculateReturnValuePassingInfo( const Type& type )
{
	if( const auto f= type.GetFundamentalType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= f->llvm_type;
		if( IsSignedInteger( f->fundamental_type ) )
			return_value_passing.sext= true;
		else if(
			IsUnsignedInteger( f->fundamental_type ) ||
			IsChar( f->fundamental_type ) ||
			IsByte( f->fundamental_type ) ||
			f->fundamental_type == U_FundamentalType::bool_ )
			return_value_passing.zext= true;

		return return_value_passing;
	}

	if( const auto e= type.GetEnumType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= e->underlying_type.llvm_type;
		return_value_passing.zext= true; // Enums are unsigned.
		return return_value_passing;
	}

	if( const auto fp= type.GetFunctionPointerType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= fp->llvm_type;
		// It seems like zero extension isn't necessary for pointers.
		return return_value_passing;
	}

	if( const auto p= type.GetRawPointerType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= p->llvm_type;
		// It seems like zero extension isn't necessary for pointers.
		return return_value_passing;
	}

	// Composite types are left.

	if( type.GetClassType() != nullptr || type.GetArrayType() != nullptr || type.GetTupleType() != nullptr )
	{
		llvm::Type* const llvm_type= type.GetLLVMType();

		const uint64_t type_size= data_layout_.getTypeAllocSize( llvm_type );
		if( type_size > 16 )
			return ReturnValuePassingByPointer{};

		if( type_size == 0 )
		{
			// TODO - handle zero-sized structs properly.
			return ReturnValuePassingByPointer{};
		}

		ArgumentPartClasses classes;
		for( ArgumentClass& c : classes )
			c= ArgumentClass::NoClass;

		ClassifyType_r( *llvm_type, classes, 0u );
		PostMergeArgumentClasses( classes );

		if( classes[0] == ArgumentClass::Memory )
			return ReturnValuePassingByPointer{};

		llvm::LLVMContext& llvm_context= llvm_type->getContext();

		ReturnValuePassingDirect return_value_passing;

		if( type_size <= 8 )
		{
			if( classes[0] == ArgumentClass::Integer )
				return_value_passing.llvm_type= llvm::IntegerType::get( llvm_context, uint32_t(type_size) * 8 );
			else if( classes[0] == ArgumentClass::SSE )
				return_value_passing.llvm_type= type_size <= 4 ? llvm::Type::getFloatTy( llvm_context ) : llvm::Type::getDoubleTy( llvm_type->getContext() );
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

			// Create a tuple for two parts.
			return_value_passing.llvm_type= llvm::StructType::get( llvm_context, llvm::ArrayRef<llvm::Type*>( types ) );
			// TODO - set zext/sext?
		}
		else U_ASSERT( false );

		return return_value_passing;
	}
	else
	{
		// Unhandled type kind.
		U_ASSERT(false);
		return ReturnValuePassingByPointer{};
	}
}

void CallingConventionInfoSystemV_X86_64::ClassifyType_r( llvm::Type& llvm_type, ArgumentPartClasses& out_classes, const uint32_t offset )
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
			ClassifyType_r( *element_type, out_classes, offset + uint32_t( element_index * element_size ) );
	}
	else if( const auto struct_type= llvm::dyn_cast<llvm::StructType>( &llvm_type ) )
	{
		const llvm::StructLayout* const struct_layout= data_layout_.getStructLayout( struct_type );
		for( uint32_t element_index= 0; element_index < struct_type->getNumElements(); ++element_index )
			ClassifyType_r( *struct_type->getElementType( element_index ), out_classes, offset + uint32_t( struct_layout->getElementOffset( element_index ) ) );
	}
	else U_ASSERT( false ); // Unhandled type kind.
}

void CallingConventionInfoSystemV_X86_64::MergeArgumentClasses( ArgumentClass& dst, const ArgumentClass src )
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

void CallingConventionInfoSystemV_X86_64::PostMergeArgumentClasses( ArgumentPartClasses& classes )
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

ICallingConventionInfo::CallInfo CallingConventionInfoSystemV_X86_64::CalculateFunctionCallInfo( const FunctionType& function_type )
{
	// Count registers used for parameters passing.
	// It's neccessary because of the rule, which says, that a composite argument sholdn't be passed partially in registers and partially in stack.
	// So if we have no place in registers for such argument, pass it entirely in stack.

	// We have 6 integer register available for integer parameters passing: %rdi, %rsi, %rdx, %rcx, %r8, %r8.
	// We have 8 floating point registers available for floating-point parameters passing: %xmm0, %xmm1, %xmm2, %xmm3, %xmm4, %xmm5, %xmm6, %xmm7.
	size_t num_integer_registers_left= 6, num_floating_point_registers_left= 8;

	CallInfo call_info;

	if( function_type.return_value_type == ValueType::Value )
	{
		call_info.return_value_passing= CalculateReturnValuePassingInfo( function_type.return_type );

		if( std::holds_alternative<ReturnValuePassingByPointer>( call_info.return_value_passing ) )
		{
			// Consume an integer register for "sret" pointer.
			--num_integer_registers_left;
		}
	}
	else
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= function_type.return_type.GetLLVMType()->getPointerTo();
		call_info.return_value_passing= std::move(return_value_passing);
	}

	call_info.arguments_passing.resize( function_type.params.size() );
	for( size_t i= 0; i < function_type.params.size(); ++i )
	{
		const FunctionType::Param& param= function_type.params[i];
		if( param.value_type == ValueType::Value )
		{
			if( const auto f= param.type.GetFundamentalType() )
			{
				ArgumentPassingDirect argument_passing;
				argument_passing.llvm_type= f->llvm_type;
				// sext/zext flags are necessary for scalars.
				if( IsSignedInteger( f->fundamental_type ) )
					argument_passing.sext= true;
				else if(
					IsUnsignedInteger( f->fundamental_type ) ||
					IsChar( f->fundamental_type ) ||
					IsByte( f->fundamental_type ) ||
					f->fundamental_type == U_FundamentalType::bool_ )
					argument_passing.zext= true;

				call_info.arguments_passing[i]= std::move(argument_passing);

				if( IsFloatingPoint( f->fundamental_type ) )
				{
					// Floating-point scalar arg consumes one floating-point register.
					if( num_floating_point_registers_left > 0 )
						--num_floating_point_registers_left;
				}
				else
				{
					// Integer arg consumes one integer register.
					if( num_integer_registers_left > 0 )
						--num_integer_registers_left;
				}
			}
			else if( const auto e= param.type.GetEnumType() )
			{
				ArgumentPassingDirect argument_passing;
				argument_passing.llvm_type= e->underlying_type.llvm_type;
				argument_passing.zext= true; // Enums are unsigned and thus require zero extension.
				call_info.arguments_passing[i]= std::move(argument_passing);

				// Enum arg consumes one integer register.
				if( num_integer_registers_left > 0 )
					--num_integer_registers_left;
			}
			else if( const auto fp= param.type.GetFunctionPointerType() )
			{
				ArgumentPassingDirect argument_passing;
				argument_passing.llvm_type= fp->llvm_type;
				// It seems like zero extension isn't necessary for pointers. Is it?
				call_info.arguments_passing[i]= std::move(argument_passing);

				// Pointer arg consumes one integer register.
				if( num_integer_registers_left > 0 )
					--num_integer_registers_left;
			}
			else if( const auto p= param.type.GetRawPointerType() )
			{
				ArgumentPassingDirect argument_passing;
				argument_passing.llvm_type= p->llvm_type;
				// It seems like zero extension isn't necessary for pointers. Is it?
				call_info.arguments_passing[i]= std::move(argument_passing);

				// Pointer arg consumes one integer register.
				if( num_integer_registers_left > 0 )
					--num_integer_registers_left;
			}
			else if(
				param.type.GetClassType() != nullptr ||
				param.type.GetArrayType() != nullptr ||
				param.type.GetTupleType() != nullptr )
			{
				llvm::Type* const llvm_type= param.type.GetLLVMType();

				const uint64_t type_size= data_layout_.getTypeAllocSize( llvm_type );

				if( type_size > 16 )
				{
					call_info.arguments_passing[i]= ArgumentPassingInStack{};

					// No registers are consumed for in-stack passing.
				}
				else if( type_size == 0 )
				{
					// TODO - handle zero-sized structs properly.
					call_info.arguments_passing[i]= ArgumentPassingInStack{};

					// No registers are consumed for in-stack passing.
				}
				else
				{
					ArgumentPartClasses classes;
					for( ArgumentClass& c : classes )
						c= ArgumentClass::NoClass;

					ClassifyType_r( *llvm_type, classes, 0u );
					PostMergeArgumentClasses( classes );

					if( classes[0] == ArgumentClass::Memory )
					{
						call_info.arguments_passing[i]= ArgumentPassingInStack{};

						// No registers are consumed for in-stack passing.
					}
					else
					{
						llvm::LLVMContext& llvm_context= llvm_type->getContext();

						if( type_size <= 8 )
						{
							ArgumentPassingDirect argument_passing;

							if( classes[0] == ArgumentClass::Integer )
							{
								argument_passing.llvm_type= llvm::IntegerType::get( llvm_context, uint32_t(type_size) * 8 );
								argument_passing.zext= true; // TODO - check if it's correct.

								if( num_integer_registers_left > 0 )
									--num_integer_registers_left;
							}
							else if( classes[0] == ArgumentClass::SSE )
							{
								argument_passing.llvm_type= type_size <= 4 ? llvm::Type::getFloatTy( llvm_context ) : llvm::Type::getDoubleTy( llvm_type->getContext() );

								if( num_floating_point_registers_left > 0 )
									--num_floating_point_registers_left;
							}
							else U_ASSERT(false);

							call_info.arguments_passing[i]= std::move(argument_passing);
						}
						else if( type_size <= 16 )
						{
							constexpr size_t num_parts= 2;
							std::array<llvm::Type*, num_parts> types{};
							const uint32_t part_sizes[2]{ 8u, uint32_t(type_size)  - 8u };
							size_t num_integer_registers_needed= 0, num_floating_point_registers_needed= 0;
							for( size_t part= 0; part < num_parts; ++part )
							{
								if( classes[part] == ArgumentClass::Integer )
								{
									types[part]= llvm::IntegerType::get( llvm_context, part_sizes[part] * 8 );
									++num_integer_registers_needed;
								}
								else if( classes[part] == ArgumentClass::SSE )
								{
									types[part]= part_sizes[part] <= 4 ? llvm::Type::getFloatTy( llvm_context ) : llvm::Type::getDoubleTy( llvm_type->getContext() );
									++num_floating_point_registers_needed;
								}
								else U_ASSERT(false);
							}

							if( num_integer_registers_needed <= num_integer_registers_left &&
								num_floating_point_registers_needed <= num_floating_point_registers_left )
							{
								ArgumentPassingDirect argument_passing;
								// Create a tuple for two parts.
								argument_passing.llvm_type= llvm::StructType::get( llvm_context, llvm::ArrayRef<llvm::Type*>( types ) );
								// TODO - set zext/sext?

								call_info.arguments_passing[i]= std::move(argument_passing);

								num_integer_registers_left-= num_integer_registers_needed;
								num_floating_point_registers_left-= num_floating_point_registers_needed;
							}
							else
							{
								// If we have not enough registers to pass all components of this composite - pass it in stack.
								call_info.arguments_passing[i]= ArgumentPassingInStack{};

								// No registers are consumed for in-stack passing.
							}
						}
						else U_ASSERT( false );
					}
				}
			}
			else
			{
				// Unhandled type kind.
				U_ASSERT(false);
				call_info.arguments_passing[i]= ArgumentPassingByPointer{};
			}
		}
		else
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= param.type.GetLLVMType()->getPointerTo();
			call_info.arguments_passing[i]= std::move(argument_passing);

			// Reference arg consumes one integer register.
			if( num_integer_registers_left > 0 )
				--num_integer_registers_left;
		}
	}

	return call_info;
}

class CallingConventionInfoMSVC_X86_64 final : public ICallingConventionInfo
{
public:
	explicit CallingConventionInfoMSVC_X86_64( llvm::DataLayout data_layout );

public: // ICallingConventionInfo
	virtual ReturnValuePassing CalculateReturnValuePassingInfo( const Type& type ) override;
	virtual CallInfo CalculateFunctionCallInfo( const FunctionType& function_type ) override;

private:
	ArgumentPassing CalculateValueArgumentPassingInfo( const Type& type );

private:
	const llvm::DataLayout data_layout_;
};

CallingConventionInfoMSVC_X86_64::CallingConventionInfoMSVC_X86_64( llvm::DataLayout data_layout )
	: data_layout_( std::move(data_layout) )
{}

ICallingConventionInfo::ReturnValuePassing CallingConventionInfoMSVC_X86_64::CalculateReturnValuePassingInfo( const Type& type )
{
	if( const auto f= type.GetFundamentalType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= f->llvm_type;
		return return_value_passing;
	}

	if( const auto e= type.GetEnumType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= e->underlying_type.llvm_type;
		return return_value_passing;
	}

	if( const auto fp= type.GetFunctionPointerType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= fp->llvm_type;
		return return_value_passing;
	}

	if( const auto p= type.GetRawPointerType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= p->llvm_type;
		return return_value_passing;
	}

	// Composite types are left.

	// Return composites with integer sizes as integers (even if a composite contains floating-point values).
	const auto size= data_layout_.getTypeAllocSize( type.GetLLVMType() );
	if( size == 1 || size == 2 || size == 4 || size == 8 )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= llvm::IntegerType::get( type.GetLLVMType()->getContext(), uint32_t(size) * 8 );
		return return_value_passing;
	}

	// Return other composites via sret pointer.
	return ReturnValuePassingByPointer{};
}

ICallingConventionInfo::CallInfo CallingConventionInfoMSVC_X86_64::CalculateFunctionCallInfo( const FunctionType& function_type )
{
	CallInfo call_info;

	if( function_type.return_value_type == ValueType::Value )
		call_info.return_value_passing= CalculateReturnValuePassingInfo( function_type.return_type );
	else
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= function_type.return_type.GetLLVMType()->getPointerTo();
		call_info.return_value_passing= std::move(return_value_passing);
	}

	call_info.arguments_passing.resize( function_type.params.size() );
	for( size_t i= 0; i < function_type.params.size(); ++i )
	{
		const FunctionType::Param& param= function_type.params[i];
		if( param.value_type == ValueType::Value )
			call_info.arguments_passing[i]= CalculateValueArgumentPassingInfo( param.type );
		else
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= param.type.GetLLVMType()->getPointerTo();
			call_info.arguments_passing[i]= std::move(argument_passing);
		}
	}

	return call_info;
}

ICallingConventionInfo::ArgumentPassing CallingConventionInfoMSVC_X86_64::CalculateValueArgumentPassingInfo( const Type& type )
{
	if( const auto f= type.GetFundamentalType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= f->llvm_type;
		return argument_passing;
	}

	if( const auto e= type.GetEnumType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= e->underlying_type.llvm_type;
		return argument_passing;
	}

	if( const auto fp= type.GetFunctionPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= fp->llvm_type;
		return argument_passing;
	}

	if( const auto p= type.GetRawPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= p->llvm_type;
		return argument_passing;
	}

	// Composite types are left.

	// Pass composites with integer sizes as integers (even if a composite contains floating-point values).
	const auto size= data_layout_.getTypeAllocSize( type.GetLLVMType() );
	if( size == 1 || size == 2 || size == 4 || size == 8 )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= llvm::IntegerType::get( type.GetLLVMType()->getContext(), uint32_t(size) * 8 );
		return argument_passing;
	}

	// Pass other composites via pointer.
	return ArgumentPassingByPointer{};
}

class CallingConventionInfoMSVC_X86 final : public ICallingConventionInfo
{
public:
	explicit CallingConventionInfoMSVC_X86( llvm::DataLayout data_layout );

public: // ICallingConventionInfo
	virtual ReturnValuePassing CalculateReturnValuePassingInfo( const Type& type ) override;
	virtual CallInfo CalculateFunctionCallInfo( const FunctionType& function_type ) override;

private:
	ArgumentPassing CalculateValueArgumentPassingInfo( const Type& type );

private:
	const llvm::DataLayout data_layout_;
};

CallingConventionInfoMSVC_X86::CallingConventionInfoMSVC_X86( llvm::DataLayout data_layout )
	: data_layout_( std::move(data_layout) )
{}

ICallingConventionInfo::ReturnValuePassing CallingConventionInfoMSVC_X86::CalculateReturnValuePassingInfo( const Type& type )
{
	if( const auto f= type.GetFundamentalType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= f->llvm_type;
		return return_value_passing;
	}

	if( const auto e= type.GetEnumType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= e->underlying_type.llvm_type;
		return return_value_passing;
	}

	if( const auto fp= type.GetFunctionPointerType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= fp->llvm_type;
		return return_value_passing;
	}

	if( const auto p= type.GetRawPointerType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= p->llvm_type;
		return return_value_passing;
	}

	// Composite types are left.

	// Return composites with integer sizes as integers (even if a composite contains floating-point values).
	const auto size= data_layout_.getTypeAllocSize( type.GetLLVMType() );
	if( size == 1 || size == 2 || size == 4 || size == 8 )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= llvm::IntegerType::get( type.GetLLVMType()->getContext(), uint32_t(size) * 8 );
		return return_value_passing;
	}

	// Return other composites via sret pointer.
	return ReturnValuePassingByPointer{};
}

ICallingConventionInfo::CallInfo CallingConventionInfoMSVC_X86::CalculateFunctionCallInfo( const FunctionType& function_type )
{
	CallInfo call_info;

	if( function_type.return_value_type == ValueType::Value )
		call_info.return_value_passing= CalculateReturnValuePassingInfo( function_type.return_type );
	else
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= function_type.return_type.GetLLVMType()->getPointerTo();
		call_info.return_value_passing= std::move(return_value_passing);
	}

	call_info.arguments_passing.resize( function_type.params.size() );
	for( size_t i= 0; i < function_type.params.size(); ++i )
	{
		const FunctionType::Param& param= function_type.params[i];
		if( param.value_type == ValueType::Value )
			call_info.arguments_passing[i]= CalculateValueArgumentPassingInfo( param.type );
		else
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= param.type.GetLLVMType()->getPointerTo();
			call_info.arguments_passing[i]= std::move(argument_passing);
		}
	}

	return call_info;
}

ICallingConventionInfo::ArgumentPassing CallingConventionInfoMSVC_X86::CalculateValueArgumentPassingInfo( const Type& type )
{
	if( const auto f= type.GetFundamentalType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= f->llvm_type;
		return argument_passing;
	}

	if( const auto e= type.GetEnumType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= e->underlying_type.llvm_type;
		return argument_passing;
	}

	if( const auto fp= type.GetFunctionPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= fp->llvm_type;
		return argument_passing;
	}

	if( const auto p= type.GetRawPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= p->llvm_type;
		return argument_passing;
	}

	// Composite types are left.
	// TODO - make sure it's correct to pass float/double arrays/tuples via stack and not via x87 registers.

	return ArgumentPassingInStack{};
}

class CallingConventionInfoSystemV_AArch64 final : public ICallingConventionInfo
{
public:
	explicit CallingConventionInfoSystemV_AArch64( llvm::DataLayout data_layout );

public: // ICallingConventionInfo
	virtual ReturnValuePassing CalculateReturnValuePassingInfo( const Type& type ) override;
	virtual CallInfo CalculateFunctionCallInfo( const FunctionType& function_type ) override;

private:
	ArgumentPassing CalculateValueArgumentPassingInfo( const Type& type );
	void CollectScalarTypes_r( llvm::Type& llvm_type, llvm::SmallVectorImpl<llvm::Type*>& out_types );

private:
	const llvm::DataLayout data_layout_;
};

CallingConventionInfoSystemV_AArch64::CallingConventionInfoSystemV_AArch64( llvm::DataLayout data_layout )
	: data_layout_( std::move(data_layout) )
{}

ICallingConventionInfo::ReturnValuePassing CallingConventionInfoSystemV_AArch64::CalculateReturnValuePassingInfo( const Type& type )
{
	if( const auto f= type.GetFundamentalType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= f->llvm_type;
		return return_value_passing;
	}

	if( const auto e= type.GetEnumType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= e->underlying_type.llvm_type;
		return return_value_passing;
	}

	if( const auto fp= type.GetFunctionPointerType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= fp->llvm_type;
		return return_value_passing;
	}

	if( const auto p= type.GetRawPointerType() )
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= p->llvm_type;
		return return_value_passing;
	}

	// Composite types.

	llvm::Type* const llvm_type= type.GetLLVMType();

	const auto size= data_layout_.getTypeAllocSize( llvm_type );
	if( size > 32 )
	{
		// Return composites with size larger than 32 by pointer.
		return ReturnValuePassingByPointer{};
	}

	llvm::SmallVector<llvm::Type*, 16> scalar_types;
	CollectScalarTypes_r( *llvm_type, scalar_types );

	if( scalar_types.size() == 1 )
	{
		// Return composites with single scalar inside using this scalar.
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= scalar_types.front();
		return return_value_passing;
	}

	if( scalar_types.size() <= 4 )
	{
		bool all_scalar_elements_are_same= true;
		for( const llvm::Type* const t : scalar_types )
			all_scalar_elements_are_same &= t == scalar_types.front();

		if( all_scalar_elements_are_same && ( scalar_types.front()->isFloatTy() || scalar_types.front()->isDoubleTy() ) )
		{
			// Homogeneous Floating-point Aggregate - they are passed directly, if there is no more than 4 elements.
			// [ f64, 4 ] array is largest composite type, which can be passed directly.
			ReturnValuePassingDirect return_value_passing;
			return_value_passing.llvm_type= llvm::ArrayType::get( scalar_types.front(), scalar_types.size() );
			return return_value_passing;
		}
	}

	if( size > 16 )
	{
		// Composites which are not Homogeneous Floating-point Aggregates with size greater than 16 are passed by pointer.
		return ReturnValuePassingByPointer{};
	}

	// Small composite types are passed as integers.
	// This includes even structs consisting of f32/f64 pairs.
	ReturnValuePassingDirect return_value_passing;
	return_value_passing.llvm_type= llvm::IntegerType::get( llvm_type->getContext(), uint32_t(size) * 8 );
	return return_value_passing;
}

ICallingConventionInfo::ArgumentPassing CallingConventionInfoSystemV_AArch64::CalculateValueArgumentPassingInfo( const Type& type )
{
	if( const auto f= type.GetFundamentalType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= f->llvm_type;
		return argument_passing;
	}

	if( const auto e= type.GetEnumType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= e->underlying_type.llvm_type;
		return argument_passing;
	}

	if( const auto fp= type.GetFunctionPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= fp->llvm_type;
		return argument_passing;
	}

	if( const auto p= type.GetRawPointerType() )
	{
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= p->llvm_type;
		return argument_passing;
	}

	// Composite types.

	llvm::Type* const llvm_type= type.GetLLVMType();

	const auto size= data_layout_.getTypeAllocSize( llvm_type );
	if( size > 32 )
	{
		// Pass composites with size larger than 32 by pointer.
		return ArgumentPassingByPointer{};
	}

	llvm::SmallVector<llvm::Type*, 16> scalar_types;
	CollectScalarTypes_r( *llvm_type, scalar_types );

	if( scalar_types.size() == 1 )
	{
		// Pass composites with single scalar inside using this scalar.
		ArgumentPassingDirect argument_passing;
		argument_passing.llvm_type= scalar_types.front();
		return argument_passing;
	}

	if( scalar_types.size() <= 4 )
	{
		bool all_scalar_elements_are_same= true;
		for( const llvm::Type* const t : scalar_types )
			all_scalar_elements_are_same &= t == scalar_types.front();

		if( all_scalar_elements_are_same && ( scalar_types.front()->isFloatTy() || scalar_types.front()->isDoubleTy() ) )
		{
			// Homogeneous Floating-point Aggregate - they are passed directly, if there is no more than 4 elements.
			// [ f64, 4 ] array is largest composite type, which can be passed directly.
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= llvm::ArrayType::get( scalar_types.front(), scalar_types.size() );
			return argument_passing;
		}
	}

	if( size > 16 )
	{
		// Composites which are not Homogeneous Floating-point Aggregates with size greater than 16 are passed by pointer.
		return ArgumentPassingByPointer{};
	}

	// Small composite types are passed as integers.
	// This includes even structs consisting of f32/f64 pairs.
	ArgumentPassingDirect argument_passing;
	if( size <= 8 )
	{
		// Use single integer for a composite less than 8 bytes.
		argument_passing.llvm_type= llvm::IntegerType::get( llvm_type->getContext(), uint32_t(size) * 8 );
	}
	else
	{
		// Use pair of integers for larger composites.
		// We can't use something like i128, since it should be aligned to even register index.
		argument_passing.llvm_type=
			llvm::StructType::get(
				llvm::IntegerType::get( llvm_type->getContext(), 8 * 8 ),
				llvm::IntegerType::get( llvm_type->getContext(), ( uint32_t(size) - 8 ) * 8 ) );
	}

	return argument_passing;
}

void CallingConventionInfoSystemV_AArch64::CollectScalarTypes_r( llvm::Type& llvm_type, llvm::SmallVectorImpl<llvm::Type*>& out_types )
{
	if( llvm_type.isIntegerTy() || llvm_type.isFloatingPointTy() || llvm_type.isPointerTy() )
		out_types.push_back( &llvm_type );
	else if( const auto array_type= llvm::dyn_cast<llvm::ArrayType>( &llvm_type ) )
	{
		llvm::Type* const element_type= array_type->getElementType();
		for( uint64_t element_index= 0; element_index < array_type->getNumElements(); ++element_index )
			CollectScalarTypes_r( *element_type, out_types );
	}
	else if( const auto struct_type= llvm::dyn_cast<llvm::StructType>( &llvm_type ) )
	{
		for( uint32_t element_index= 0; element_index < struct_type->getNumElements(); ++element_index )
			CollectScalarTypes_r( *struct_type->getElementType( element_index ), out_types );
	}
	else U_ASSERT( false ); // Unhandled type kind.
}

ICallingConventionInfo::CallInfo CallingConventionInfoSystemV_AArch64::CalculateFunctionCallInfo( const FunctionType& function_type )
{
	CallInfo call_info;

	if( function_type.return_value_type == ValueType::Value )
		call_info.return_value_passing= CalculateReturnValuePassingInfo( function_type.return_type );
	else
	{
		ReturnValuePassingDirect return_value_passing;
		return_value_passing.llvm_type= function_type.return_type.GetLLVMType()->getPointerTo();
		call_info.return_value_passing= std::move(return_value_passing);
	}

	call_info.arguments_passing.resize( function_type.params.size() );
	for( size_t i= 0; i < function_type.params.size(); ++i )
	{
		const FunctionType::Param& param= function_type.params[i];
		if( param.value_type == ValueType::Value )
			call_info.arguments_passing[i]= CalculateValueArgumentPassingInfo( param.type );
		else
		{
			ArgumentPassingDirect argument_passing;
			argument_passing.llvm_type= param.type.GetLLVMType()->getPointerTo();
			call_info.arguments_passing[i]= std::move(argument_passing);
		}
	}

	return call_info;
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

	const llvm::Triple::ArchType arch= target_triple.getArch();
	const llvm::Triple::OSType os= target_triple.getOS();

	if( arch == llvm::Triple::x86_64 )
	{
		if( os == llvm::Triple::Linux ||
			os == llvm::Triple::FreeBSD ||
			os == llvm::Triple::Darwin ||
			os == llvm::Triple::MacOSX )
		{
			const auto system_v_x86_64_info= std::make_shared<CallingConventionInfoSystemV_X86_64>( data_layout );
			calling_convention_infos[ size_t( CallingConvention::C ) ]= system_v_x86_64_info;
			calling_convention_infos[ size_t( CallingConvention::System ) ]= system_v_x86_64_info;
		}
		else if( os == llvm::Triple::Win32 )
		{
			const auto msvc_x86_64_info= std::make_shared<CallingConventionInfoMSVC_X86_64>( data_layout );
			calling_convention_infos[ size_t( CallingConvention::C ) ]= msvc_x86_64_info;
			calling_convention_infos[ size_t( CallingConvention::System ) ]= msvc_x86_64_info;
		}
		else
		{
			// TODO - handle other operating systems.
		}
	}
	else if( arch == llvm::Triple::x86 )
	{
		if( os == llvm::Triple::Linux ||
			os == llvm::Triple::FreeBSD ||
			os == llvm::Triple::Darwin ||
			os == llvm::Triple::MacOSX )
		{
			// TODO - support x86 calling conventions on GNU/Linux and other systems using System V ABI.
		}
		else if( os == llvm::Triple::Win32 )
		{
			const auto msvc_x86_info= std::make_shared<CallingConventionInfoMSVC_X86>( data_layout );
			calling_convention_infos[ size_t( CallingConvention::C ) ]= msvc_x86_info;
			// "system" calling convention on x86 Windows is actually separate convention - stdcall.
			// TODO - check if it's correct to use the same info as for cdecl.
			calling_convention_infos[ size_t( CallingConvention::System ) ]= msvc_x86_info;
		}
		else
		{
			// TODO - handle other operating systems.
		}
	}
	else if( arch == llvm::Triple::aarch64 )
	{
		if( os == llvm::Triple::Linux ||
			os == llvm::Triple::FreeBSD ||
			os == llvm::Triple::Darwin ||
			os == llvm::Triple::MacOSX )
		{

			const auto system_v_aarch64_info= std::make_shared<CallingConventionInfoSystemV_AArch64>( data_layout );
			calling_convention_infos[ size_t( CallingConvention::C ) ]= system_v_aarch64_info;
			calling_convention_infos[ size_t( CallingConvention::System ) ]= system_v_aarch64_info;
		}
		else
		{
			// TODO - handle other operating systems.
		}
	}
	else
	{
		// TODO - handle other architectures.
	}

	return calling_convention_infos;
}

} // namespace U
