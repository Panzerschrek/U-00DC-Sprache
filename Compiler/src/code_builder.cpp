#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include "pop_llvm_warnings.hpp"

#include "assert.hpp"
#include "keywords.hpp"
#include "lang_types.hpp"
#include "mangling.hpp"

#include "code_builder.hpp"

namespace U
{

namespace
{

typedef std::map< ProgramString, U_FundamentalType > TypesMap;

const TypesMap g_types_map=
{
	{ Keyword( Keywords::void_ ), U_FundamentalType::Void },
	{ Keyword( Keywords::bool_ ), U_FundamentalType::Bool },
	{ Keyword( Keywords::i8_  ), U_FundamentalType::i8  },
	{ Keyword( Keywords::u8_  ), U_FundamentalType::u8  },
	{ Keyword( Keywords::i16_ ), U_FundamentalType::i16 },
	{ Keyword( Keywords::u16_ ), U_FundamentalType::u16 },
	{ Keyword( Keywords::i32_ ), U_FundamentalType::i32 },
	{ Keyword( Keywords::u32_ ), U_FundamentalType::u32 },
	{ Keyword( Keywords::i64_ ), U_FundamentalType::i64 },
	{ Keyword( Keywords::u64_ ), U_FundamentalType::u64 },
	{ Keyword( Keywords::f32_ ), U_FundamentalType::f32 },
	{ Keyword( Keywords::f64_ ), U_FundamentalType::f64 },
};

} // namespace

namespace CodeBuilderPrivate
{

CodeBuilder::FunctionContext::FunctionContext(
	const Type in_return_type,
	const bool in_return_value_is_mutable,
	const bool in_return_value_is_reference,
	llvm::LLVMContext& llvm_context,
	llvm::Function* in_function )
	: return_type(in_return_type)
	, return_value_is_mutable(in_return_value_is_mutable)
	, return_value_is_reference(in_return_value_is_reference)
	, function(in_function)
	, alloca_basic_block( llvm::BasicBlock::Create( llvm_context, "allocations", function ) )
	, alloca_ir_builder( alloca_basic_block )
	, function_basic_block( llvm::BasicBlock::Create( llvm_context, "func_code", function ) )
	, llvm_ir_builder( function_basic_block )
{
}

void CodeBuilder::DestructiblesStorage::RegisterVariable( Variable variable )
{
	if( variable.type.HaveDestructor() )
		variables.push_back( std::move( variable ) );
}

CodeBuilder::CodeBuilder()
	: llvm_context_( llvm::getGlobalContext() )
{
	fundamental_llvm_types_. i8= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_. u8= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.i16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.u16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.i32= llvm::Type::getInt32Ty( llvm_context_ );
	fundamental_llvm_types_.u32= llvm::Type::getInt32Ty( llvm_context_ );
	fundamental_llvm_types_.i64= llvm::Type::getInt64Ty( llvm_context_ );
	fundamental_llvm_types_.u64= llvm::Type::getInt64Ty( llvm_context_ );

	fundamental_llvm_types_.f32= llvm::Type::getFloatTy( llvm_context_ );
	fundamental_llvm_types_.f64= llvm::Type::getDoubleTy( llvm_context_ );

	fundamental_llvm_types_.invalid_type_= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.void_= llvm::Type::getVoidTy( llvm_context_ );
	fundamental_llvm_types_.bool_= llvm::Type::getInt1Ty( llvm_context_ );

	invalid_type_= FundamentalType( U_FundamentalType::InvalidType, fundamental_llvm_types_.invalid_type_ );
	void_type_= FundamentalType( U_FundamentalType::Void, fundamental_llvm_types_.void_ );
	bool_type_= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );
}

CodeBuilder::~CodeBuilder()
{
}

CodeBuilder::BuildResult CodeBuilder::BuildProgram( const ProgramElements& program_elements )
{
	module_= std::unique_ptr<llvm::Module>( new llvm::Module( "U-Module", llvm_context_ ) );
	errors_.clear();
	error_count_= 0u;

	// In some places outside functions we need to execute expression evaluation.
	// Create for this dummy function context.
	llvm::Function* const dummy_function=
		llvm::Function::Create(
			llvm::FunctionType::get( fundamental_llvm_types_.void_, false ),
			llvm::Function::LinkageTypes::ExternalLinkage,
			"",
			module_.get() );

	FunctionContext dummy_function_context(
		void_type_,
		false, false,
		llvm_context_,
		dummy_function );
	dummy_function_context.destructibles_stack.emplace_back();

	dummy_function_context_= &dummy_function_context;

	// Create global namespace.
	NamesScope global_names( ""_SpC, nullptr );
	FillGlobalNamesScope( global_names );

	// Build program body.
	BuildNamespaceBody( program_elements, global_names );

	dummy_function->eraseFromParent(); // Kill dummy function.

	if( error_count_ > 0u )
		errors_.push_back( ReportBuildFailed() );

	BuildResult result;
	result.errors= errors_;
	errors_.clear();
	result.module= std::move( module_ );
	return result;
}

void CodeBuilder::FillGlobalNamesScope( NamesScope& global_names_scope )
{
	for( const auto& fundamental_type_value : g_types_map )
	{
		global_names_scope.AddName(
			fundamental_type_value.first,
			Type( FundamentalType( fundamental_type_value.second, GetFundamentalLLVMType( fundamental_type_value.second ) ) ) );
	}
}

Type CodeBuilder::PrepareType(
	const FilePos& file_pos,
	const TypeName& type_name,
	const NamesScope& names_scope )
{
	Type result;
	Type* last_type= &result;

	constexpr unsigned int c_max_array_dimensions= 16u;
	Type* arrays_stack[ c_max_array_dimensions ];
	unsigned int arrays_count= 0u;

	// Fill arrays hierarchy.
	for( auto rit= type_name.array_sizes.rbegin(); rit != type_name.array_sizes.rend(); ++rit )
	{
		if( arrays_count >= c_max_array_dimensions )
		{
			U_ASSERT( false && "WTF?" );
		}

		arrays_stack[ arrays_count ]= last_type;
		arrays_count++;

		const IExpressionComponent& num= * *rit;

		*last_type= Array();
		Array& array_type= *last_type->GetArrayType();
		array_type.size= 1u;

		const Value size_expression= BuildExpressionCode( num, names_scope, *dummy_function_context_ );
		if( const Variable* const size_variable= size_expression.GetVariable() )
		{
			if( size_variable->constexpr_value != nullptr )
			{
				if( const FundamentalType* const size_fundamental_type= size_variable->type.GetFundamentalType() )
				{
					if( IsInteger( size_fundamental_type->fundamental_type ) )
					{
						const llvm::APInt& size_value= size_variable->constexpr_value->getUniqueInteger();
						if( IsSignedInteger( size_fundamental_type->fundamental_type ) && size_value.isNegative() )
							errors_.push_back( ReportArraySizeIsNegative( num.file_pos_ ) );
						else
							array_type.size= size_t( size_value.getLimitedValue() );
					}
					else
						errors_.push_back( ReportArraySizeIsNotInteger( num.file_pos_ ) );
				}
				else
					U_ASSERT( false && "Nonfundamental constexpr? WTF?" );
			}
			else
				errors_.push_back( ReportExpectedConstantExpression( num.file_pos_ ) );
		}
		else
			errors_.push_back( ReprotExpectedVariableInArraySize( num.file_pos_, size_expression.GetType().ToString() ) );

		last_type= &array_type.type;
	}

	*last_type= FundamentalType( U_FundamentalType::InvalidType, fundamental_llvm_types_.invalid_type_ );

	if( const NamesScope::InsertedName* name=
		names_scope.ResolveName( type_name.name ) )
	{
		if( const Type* const type= name->second.GetTypeName() )
			*last_type= *type;
		else
			errors_.push_back( ReportNameIsNotTypeName( file_pos, name->first ) );
	}
	else
		errors_.push_back( ReportNameNotFound( file_pos, type_name.name ) );

	// Setup arrays llvm types.
	if( arrays_count > 0u )
	{
		{
			Array* const array_type= arrays_stack[ arrays_count - 1u ]->GetArrayType();
			U_ASSERT( array_type != nullptr );

				array_type->llvm_type=
				llvm::ArrayType::get(
					last_type->GetLLVMType(),
					array_type->size );
		}

		for( unsigned int i= arrays_count - 1u; i > 0u; i-- )
		{
			Array* const array_type= arrays_stack[ i - 1u ]->GetArrayType();
			U_ASSERT( array_type != nullptr );

			array_type->llvm_type=
				llvm::ArrayType::get(
					arrays_stack[i]->GetLLVMType(),
					array_type->size );
		}
	}

	return result;
}

void CodeBuilder::PrepareClass( const ClassDeclaration& class_declaration, NamesScope& names_scope )
{
	const ProgramString& class_name= class_declaration.name_.components.back();
	if( IsKeyword( class_name ) )
		errors_.push_back( ReportUsingKeywordAsName( class_declaration.file_pos_ ) );

	if( class_declaration.is_forward_declaration_ )
	{
		if( class_declaration.name_.components.size() != 1u )
		{
			errors_.push_back( ReportClassDeclarationOutsideItsScope( class_declaration.file_pos_ ) );
			return;
		}

		const ClassPtr the_class= std::make_shared<Class>( class_name, &names_scope );
		the_class->llvm_type= llvm::StructType::create( llvm_context_, MangleClass( names_scope, class_name ) );
		const Type class_type= the_class;

		const NamesScope::InsertedName* const inserted_name= names_scope.AddName( class_name, class_type );
		if( inserted_name == nullptr )
		{
			errors_.push_back( ReportRedefinition( class_declaration.file_pos_, class_name ) );
			return;
		}
		return;
	}

	ClassPtr the_class;

	if( const NamesScope::InsertedName* const previous_declaration=
		names_scope.ResolveName( class_declaration.name_ ) )
	{
		if( const Type* const previous_type= previous_declaration->second.GetTypeName() )
		{
			if( const ClassPtr previous_calss_ptr= previous_type->GetClassType() )
			{
				if( !previous_calss_ptr->is_incomplete )
				{
					errors_.push_back( ReportClassBodyDuplication( class_declaration.file_pos_ ) );
					return;
				}
				the_class= previous_calss_ptr;
			}
			else
			{
				errors_.push_back( ReportRedefinition( class_declaration.file_pos_, class_name ) );
				return;
			}
		}
		else
		{
			errors_.push_back( ReportRedefinition( class_declaration.file_pos_, class_name ) );
			return;
		}
	}
	else
	{
		if( class_declaration.name_.components.size() != 1u )
		{
			errors_.push_back( ReportClassDeclarationOutsideItsScope( class_declaration.file_pos_ ) );
			return;
		}

		the_class= std::make_shared<Class>( class_name, &names_scope );
		the_class->llvm_type= llvm::StructType::create( llvm_context_, MangleClass( names_scope, class_name ) );
		Type class_type;
		class_type= the_class;

		const NamesScope::InsertedName* const inserted_name= names_scope.AddName( class_name, class_type );
		if( inserted_name == nullptr )
		{
			errors_.push_back( ReportRedefinition( class_declaration.file_pos_, class_name ) );
			return;
		}
	}
	U_ASSERT( the_class != nullptr );
	Type class_type;
	class_type= the_class;

	std::vector<llvm::Type*> fields_llvm_types;

	for( const ClassDeclaration::Member& member : class_declaration.members_ )
	{
		// TODO - maybe apply visitor?
		if( const ClassDeclaration::Field* const in_field=
			boost::get< ClassDeclaration::Field >( &member ) )
		{
			ClassField out_field;
			out_field.type= PrepareType( in_field->file_pos, in_field->type, the_class->members );
			out_field.index= the_class->field_count;
			out_field.class_= the_class;

			if( out_field.type.IsIncomplete() )
			{
				errors_.push_back( ReportUsingIncompleteType( class_declaration.file_pos_, out_field.type.ToString() ) );
				continue;
			}

			fields_llvm_types.emplace_back( out_field.type.GetLLVMType() );

			const NamesScope::InsertedName* const inserted_field=
				the_class->members.AddName( in_field->name, std::move( out_field ) );
			if( inserted_field == nullptr )
				errors_.push_back( ReportRedefinition( in_field->file_pos, in_field->name ) );

			the_class->field_count++;
		}
		else if( const std::unique_ptr<FunctionDeclaration>* const function_declaration=
			boost::get< std::unique_ptr<FunctionDeclaration> >( &member ) )
		{
			// First time, push only prototypes.
			U_ASSERT( *function_declaration != nullptr );
			PrepareFunction( **function_declaration, true, the_class, the_class->members );
		}
		else if( const std::unique_ptr<ClassDeclaration>* const inner_class=
			boost::get< std::unique_ptr<ClassDeclaration> >( &member ) )
		{
			U_ASSERT( *inner_class != nullptr );
			PrepareClass( **inner_class, the_class->members );
		}
		else
		{
			U_ASSERT( false );
		}
	}

	// Search for explicit noncopy constructors.
	if( const NamesScope::InsertedName* const constructors_name=
		the_class->members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
	{
		const OverloadedFunctionsSet* const constructors= constructors_name->second.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		for( const FunctionVariable& constructor : *constructors )
		{
			const Function& constructor_type= *constructor.type.GetFunctionType();

			U_ASSERT( constructor_type.args.size() >= 1u && constructor_type.args.front().type == class_type );
			if( !( constructor_type.args.size() == 2u && constructor_type.args.back().type == class_type && !constructor_type.args.back().is_mutable ) )
			{
				the_class->have_explicit_noncopy_constructors= true;
				break;
			}
		};
	}

	the_class->llvm_type->setBody( fields_llvm_types );
	the_class->is_incomplete= false;

	TryGenerateDefaultConstructor( *the_class, class_type );
	TryGenerateCopyConstructor( *the_class, class_type );
	TryGenerateDestructor( *the_class, class_type );

	// Build functions with body.
	for( const ClassDeclaration::Member& member : class_declaration.members_ )
	{
		if( const std::unique_ptr<FunctionDeclaration>* const function_declaration=
			boost::get< std::unique_ptr<FunctionDeclaration> >( &member ) )
		{
			U_ASSERT( *function_declaration != nullptr );
			if( (*function_declaration)->block_ != nullptr )
				PrepareFunction( **function_declaration, false, the_class, the_class->members );
		}
	}
}

void CodeBuilder::TryGenerateDefaultConstructor( Class& the_class, const Type& class_type )
{
	// Search for explicit default constructor.
	if( const NamesScope::InsertedName* const constructors_name=
		the_class.members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
	{
		const OverloadedFunctionsSet* const constructors= constructors_name->second.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		for( const FunctionVariable& constructor : *constructors )
		{
			const Function& constructor_type= *constructor.type.GetFunctionType();

			U_ASSERT( constructor_type.args.size() >= 1u && constructor_type.args.front().type == class_type );
			if( ( constructor_type.args.size() == 1u ) )
			{
				the_class.is_default_constructible= true;
				return;
			}
		};
	}

	// Generating of default constructor disabled, if class have other explicit constructors, except copy constructors.
	if( the_class.have_explicit_noncopy_constructors )
		return;

	// Generate default constructor, if all fields is default constructible.
	bool all_fields_is_default_constructible= true;

	the_class.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;

			if( !field->type.IsDefaultConstructible() )
				all_fields_is_default_constructible= false;
		} );

	if( !all_fields_is_default_constructible )
		return;

	// Generate function

	Function constructor_type;
	constructor_type.return_type= void_type_;
	constructor_type.args.emplace_back();
	constructor_type.args.back().type= class_type;
	constructor_type.args.back().is_mutable= true;
	constructor_type.args.back().is_reference= true;

	std::vector<llvm::Type*> args_llvm_types;
	args_llvm_types.push_back( llvm::PointerType::get( class_type.GetLLVMType(), 0u ) );

	constructor_type.llvm_function_type=
		llvm::FunctionType::get(
			fundamental_llvm_types_.void_,
			llvm::ArrayRef<llvm::Type*>( args_llvm_types.data(), args_llvm_types.size() ),
			false );

	llvm::Function* const llvm_constructor_function=
		llvm::Function::Create(
			constructor_type.llvm_function_type,
			llvm::Function::LinkageTypes::ExternalLinkage, // TODO - select linkage
			MangleFunction( the_class.members, Keyword( Keywords::constructor_ ), constructor_type, true ),
			module_.get() );

	llvm_constructor_function->setUnnamedAddr( true );
	llvm_constructor_function->addAttribute( 1u, llvm::Attribute::NonNull ); // this is nonnull

	FunctionContext function_context(
		constructor_type.return_type,
		constructor_type.return_value_is_mutable,
		constructor_type.return_value_is_reference,
		llvm_context_,
		llvm_constructor_function );

	llvm::Value* const this_llvm_value= llvm_constructor_function->args().begin();
	this_llvm_value->setName( KeywordAscii( Keywords::this_ ) );

	the_class.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;

			Variable field_variable;
			field_variable.type= field->type;
			field_variable.value_type= ValueType::Reference;

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			field_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( this_llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			ApplyEmptyInitializer( member.first, FilePos()/*TODO*/, field_variable, function_context );
		} );

	function_context.llvm_ir_builder.CreateRetVoid();
	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );

	// Add generated constructor
	FunctionVariable constructor_variable;
	constructor_variable.type= std::move( constructor_type );
	constructor_variable.have_body= true;
	constructor_variable.is_this_call= true;
	constructor_variable.is_generated= true;
	constructor_variable.llvm_function= llvm_constructor_function;

	if( NamesScope::InsertedName* const constructors_name=
		the_class.members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
	{
		OverloadedFunctionsSet* const constructors= constructors_name->second.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		constructors->push_back( std::move( constructor_variable ) );
	}
	else
	{
		OverloadedFunctionsSet constructors;
		constructors.push_back( std::move( constructor_variable ) );
		the_class.members.AddName( Keyword( Keywords::constructor_ ), std::move( constructors ) );
	}

	// After default constructor generation, class is default-constructible.
	the_class.is_default_constructible= true;
}

void CodeBuilder::TryGenerateCopyConstructor( Class& the_class, const Type& class_type )
{
	// Search for explicit copy constructor.
	if( const NamesScope::InsertedName* const constructors_name=
		the_class.members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
	{
		const OverloadedFunctionsSet* const constructors= constructors_name->second.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		for( const FunctionVariable& constructor : *constructors )
		{
			const Function& constructor_type= *constructor.type.GetFunctionType();

			U_ASSERT( constructor_type.args.size() >= 1u && constructor_type.args.front().type == class_type );
			if( constructor_type.args.size() == 2u &&
				constructor_type.args.back().type == class_type && !constructor_type.args.back().is_mutable )
			{
				the_class.is_copy_constructible= true;
				return;
			}
		}
	}

	bool all_fields_is_copy_constructible= true;

	the_class.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;

			if( !field->type.IsCopyConstructible() )
				all_fields_is_copy_constructible= false;
		} );

	if( !all_fields_is_copy_constructible )
		return;

	// Generate copy-constructor
	Function constructor_type;
	constructor_type.return_type= void_type_;
	constructor_type.args.resize(2u);
	constructor_type.args[0].type= class_type;
	constructor_type.args[0].is_mutable= true;
	constructor_type.args[0].is_reference= true;
	constructor_type.args[1].type= class_type;
	constructor_type.args[1].is_mutable= false;
	constructor_type.args[1].is_reference= true;

	std::vector<llvm::Type*> args_llvm_types;
	args_llvm_types.push_back( llvm::PointerType::get( class_type.GetLLVMType(), 0u ) );
	args_llvm_types.push_back( llvm::PointerType::get( class_type.GetLLVMType(), 0u ) );

	constructor_type.llvm_function_type=
		llvm::FunctionType::get(
			fundamental_llvm_types_.void_,
			llvm::ArrayRef<llvm::Type*>( args_llvm_types.data(), args_llvm_types.size() ),
			false );

	llvm::Function* const llvm_constructor_function=
		llvm::Function::Create(
			constructor_type.llvm_function_type,
			llvm::Function::LinkageTypes::ExternalLinkage, // TODO - select linkage
			MangleFunction( the_class.members, Keyword( Keywords::constructor_ ), constructor_type, true ),
			module_.get() );

	llvm_constructor_function->setUnnamedAddr( true );
	llvm_constructor_function->addAttribute( 1u, llvm::Attribute::NonNull ); // this is nonnull
	llvm_constructor_function->addAttribute( 2u, llvm::Attribute::NonNull ); // and src is nonnull

	FunctionContext function_context(
		constructor_type.return_type,
		constructor_type.return_value_is_mutable,
		constructor_type.return_value_is_reference,
		llvm_context_,
		llvm_constructor_function );

	llvm::Value* const this_llvm_value= &*llvm_constructor_function->args().begin();
	this_llvm_value->setName( KeywordAscii( Keywords::this_ ) );
	llvm::Value* const src_llvm_value= &*(++llvm_constructor_function->args().begin());
	src_llvm_value->setName( "src" );

	the_class.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;
			U_ASSERT( field->type.IsCopyConstructible() );

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );

			BuildCopyConstructorPart(
				function_context.llvm_ir_builder.CreateGEP( src_llvm_value , llvm::ArrayRef<llvm::Value*>( index_list, 2u ) ),
				function_context.llvm_ir_builder.CreateGEP( this_llvm_value, llvm::ArrayRef<llvm::Value*>( index_list, 2u ) ),
				field->type,
				function_context );

		} ); // For fields.

	function_context.llvm_ir_builder.CreateRetVoid();
	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );

	// Add generated constructor
	FunctionVariable constructor_variable;
	constructor_variable.type= std::move( constructor_type );
	constructor_variable.have_body= true;
	constructor_variable.is_this_call= true;
	constructor_variable.is_generated= true;
	constructor_variable.llvm_function= llvm_constructor_function;

	if( NamesScope::InsertedName* const constructors_name=
		the_class.members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
	{
		OverloadedFunctionsSet* const constructors= constructors_name->second.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		constructors->push_back( std::move( constructor_variable ) );
	}
	else
	{
		OverloadedFunctionsSet constructors;
		constructors.push_back( std::move( constructor_variable ) );
		the_class.members.AddName( Keyword( Keywords::constructor_ ), std::move( constructors ) );
	}

	// After default constructor generation, class is copy-constructible.
	the_class.is_copy_constructible= true;
}

void CodeBuilder::TryGenerateDestructor( Class& the_class, const Type& class_type )
{
	// Search for explicit destructor.
	if( const NamesScope::InsertedName* const destructor_name=
		the_class.members.GetThisScopeName( Keyword( Keywords::destructor_ ) ) )
	{
		const OverloadedFunctionsSet* const destructors= destructor_name->second.GetFunctionsSet();
		U_ASSERT( destructors != nullptr && destructors->size() == 1u );
		U_UNUSED( destructors );
		the_class.have_destructor= true;
		return;
	}

	// SPRACHE_TODO - maybe not generate default destructor for classes, that have no fields with destructors?
	// SPRACHE_TODO - maybe mark generated destructor for this cases as "empty"?

	// Generate destructor.
	Function destructor_type;
	destructor_type.return_type= void_type_;
	destructor_type.args.resize(1u);
	destructor_type.args[0].type= class_type;
	destructor_type.args[0].is_mutable= true;
	destructor_type.args[0].is_reference= true;

	llvm::Type* const this_llvm_type= llvm::PointerType::get( class_type.GetLLVMType(), 0u );
	destructor_type.llvm_function_type=
		llvm::FunctionType::get(
			fundamental_llvm_types_.void_,
			llvm::ArrayRef<llvm::Type*>( &this_llvm_type, 1u ),
			false );

	llvm::Function* const llvm_destructor_function=
		llvm::Function::Create(
			destructor_type.llvm_function_type,
			llvm::Function::LinkageTypes::ExternalLinkage, // TODO - select linkage
			MangleFunction( the_class.members, Keyword( Keywords::destructor_ ), destructor_type, true ),
			module_.get() );
	llvm_destructor_function->setUnnamedAddr( true );
	llvm_destructor_function->addAttribute( 1u, llvm::Attribute::NonNull ); // this is nonnull

	llvm::Value* const this_llvm_value= &*llvm_destructor_function->args().begin();
	this_llvm_value->setName( KeywordAscii( Keywords::this_ ) );

	Variable this_;
	this_.type= class_type;
	this_.location= Variable::Location::Pointer;
	this_.value_type= ValueType::Reference;
	this_.llvm_value= this_llvm_value;

	FunctionContext function_context(
		destructor_type.return_type,
		destructor_type.return_value_is_mutable,
		destructor_type.return_value_is_reference,
		llvm_context_,
		llvm_destructor_function );
	function_context.this_= &this_;

	CallMembersDestructors( function_context );
	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );
	function_context.llvm_ir_builder.CreateRetVoid();

	// Add generated destructor.
	FunctionVariable destructor_variable;
	destructor_variable.type= std::move( destructor_type );
	destructor_variable.have_body= true;
	destructor_variable.is_this_call= true;
	destructor_variable.is_generated= true;
	destructor_variable.llvm_function= llvm_destructor_function;

	// TODO - destructor have no overloads. Maybe store it as FunctionVariable, not as FunctionsSet?
	OverloadedFunctionsSet destructors;
	destructors.push_back( std::move( destructor_variable ) );

	the_class.members.AddName(
		Keyword( Keywords::destructor_ ),
		Value( std::move( destructors ) ) );

	// Say "we have destructor".
	the_class.have_destructor= true;
}

void CodeBuilder::BuildCopyConstructorPart(
	llvm::Value* const src, llvm::Value* const dst,
	const Type& type,
	FunctionContext& function_context )
{
	if( const FundamentalType* const fundamental_type= type.GetFundamentalType() )
	{
		// Create simple load-store.
		U_UNUSED( fundamental_type );
		llvm::Value* const val= function_context.llvm_ir_builder.CreateLoad( src );
		function_context.llvm_ir_builder.CreateStore( val, dst );
	}
	else if( const Array* const array_type_ptr= type.GetArrayType() )
	{
		const Array& array_type= *array_type_ptr;

		GenerateLoop(
			array_type.size,
			[&](llvm::Value* const counter_value)
			{
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= counter_value;

				BuildCopyConstructorPart(
					function_context.llvm_ir_builder.CreateGEP( src, llvm::ArrayRef<llvm::Value*>( index_list, 2u ) ),
					function_context.llvm_ir_builder.CreateGEP( dst, llvm::ArrayRef<llvm::Value*>( index_list, 2u ) ),
					array_type.type,
					function_context );
			},
			function_context);
	}
	else if( const ClassPtr class_type= type.GetClassType() )
	{
		const Type filed_class_type= class_type;

		// Search copy constructor.
		const NamesScope::InsertedName* constructor_name=
			class_type->members.GetThisScopeName( Keyword( Keywords::constructor_ ) );
		U_ASSERT( constructor_name != nullptr );
		const OverloadedFunctionsSet* const constructors_set= constructor_name->second.GetFunctionsSet();
		U_ASSERT( constructors_set != nullptr );

		const FunctionVariable* constructor= nullptr;;
		for( const FunctionVariable& candidate_constructor : *constructors_set )
		{
			const Function& constructor_type= *candidate_constructor.type.GetFunctionType();

			if( constructor_type.args.size() == 2u &&
				constructor_type.args.back().type == filed_class_type && !constructor_type.args.back().is_mutable )
			{
				constructor= &candidate_constructor;
				break;
			}
		}
		U_ASSERT( constructor != nullptr );

		// Call it.
		std::vector<llvm::Value*> llvm_args;
		llvm_args.push_back( dst );
		llvm_args.push_back( src );
		function_context.llvm_ir_builder.CreateCall(
			llvm::dyn_cast<llvm::Function>(constructor->llvm_function),
			llvm_args );
	}
	else
	{
		U_ASSERT(false);
	}
}

void CodeBuilder::TryCallCopyConstructor(
	const FilePos& file_pos,
	llvm::Value* const this_, llvm::Value* const src,
	const ClassPtr& class_,
	FunctionContext& function_context )
{
	U_ASSERT( class_ != nullptr );
	const Type class_type= class_;

	if( !class_->is_copy_constructible )
	{
		// TODO - print more reliable message.
		errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, class_type.ToString() ) );
		return;
	}

	// Search for copy-constructor.
	const NamesScope::InsertedName* const constructos_name= class_->members.GetThisScopeName( Keyword( Keywords::constructor_ ) );
	U_ASSERT( constructos_name != nullptr );
	const OverloadedFunctionsSet* const constructors= constructos_name->second.GetFunctionsSet();
	U_ASSERT(constructors != nullptr );
	const FunctionVariable* constructor= nullptr;
	for( const FunctionVariable& candidate : *constructors )
	{
		const Function& constructor_type= *candidate.type.GetFunctionType();
		if( candidate.is_this_call && constructor_type.args.size() == 2u &&
			constructor_type.args.back().type == class_type && constructor_type.args.back().is_reference && !constructor_type.args.back().is_mutable )
		{
			constructor= &candidate;
			break;
		}
	}

	// Call it
	U_ASSERT(constructor != nullptr);
	llvm::Value* const constructor_args[2u]= { this_, src };
	function_context.llvm_ir_builder.CreateCall(
		constructor->llvm_function,
		llvm::ArrayRef<llvm::Value*>( constructor_args, 2u ) );
}

void CodeBuilder::GenerateLoop(
	const size_t iteration_count,
	const std::function<void(llvm::Value* counter_value)>& loop_body,
	FunctionContext& function_context)
{
	U_ASSERT( loop_body != nullptr );
	if( iteration_count == 0u )
		return;

	llvm::Value* const zero_value=
		llvm::Constant::getIntegerValue( fundamental_llvm_types_.u32, llvm::APInt( 32u, uint64_t(0) ) );
	llvm::Value* const one_value=
		llvm::Constant::getIntegerValue( fundamental_llvm_types_.u32, llvm::APInt( 32u, uint64_t(1u) ) );
	llvm::Value* const loop_count_value=
		llvm::Constant::getIntegerValue( fundamental_llvm_types_.u32, llvm::APInt( 32u, uint64_t(iteration_count) ) );
	llvm::Value* const couter_address= function_context.alloca_ir_builder.CreateAlloca( fundamental_llvm_types_.u32 );
	couter_address->setName( "loop_counter" );
	function_context.llvm_ir_builder.CreateStore( zero_value, couter_address );

	llvm::BasicBlock* const loop_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const block_after_loop= llvm::BasicBlock::Create( llvm_context_ );

	function_context.llvm_ir_builder.CreateBr( loop_block );
	function_context.function->getBasicBlockList().push_back( loop_block );
	function_context.llvm_ir_builder.SetInsertPoint( loop_block );

	llvm::Value* const current_counter_value= function_context.llvm_ir_builder.CreateLoad( couter_address );
	loop_body( current_counter_value );

	llvm::Value* const counter_value_plus_one= function_context.llvm_ir_builder.CreateAdd( current_counter_value, one_value );
	function_context.llvm_ir_builder.CreateStore( counter_value_plus_one, couter_address );
	llvm::Value* const counter_test= function_context.llvm_ir_builder.CreateICmpULT( counter_value_plus_one, loop_count_value );
	function_context.llvm_ir_builder.CreateCondBr( counter_test, loop_block, block_after_loop );

	function_context.function->getBasicBlockList().push_back( block_after_loop );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_loop );
}

void CodeBuilder::CallDestructors(
	const DestructiblesStorage& destructibles_storage,
	FunctionContext& function_context )
{
	// Call destructors in reverse order.
	for( auto it = destructibles_storage.variables.rbegin(); it != destructibles_storage.variables.rend(); ++it )
		CallDestructor( it->llvm_value, it->type, function_context );
}

void CodeBuilder::CallDestructor(
	llvm::Value* const ptr,
	const Type& type,
	FunctionContext& function_context )
{
	U_ASSERT( type.HaveDestructor() );

	if( const ClassPtr class_= type.GetClassType() )
	{
		const NamesScope::InsertedName* const destructor_name= class_->members.GetThisScopeName( Keyword( Keywords::destructor_ ) );
		U_ASSERT( destructor_name != nullptr );
		const OverloadedFunctionsSet* const destructors= destructor_name->second.GetFunctionsSet();
		U_ASSERT(destructors != nullptr && destructors->size() == 1u );

		const FunctionVariable& destructor= destructors->front();
		llvm::Value* const destructor_args[]= { ptr };
		function_context.llvm_ir_builder.CreateCall(
			destructor.llvm_function,
			llvm::ArrayRef<llvm::Value*>( destructor_args, 1u ) );
	}
	else if( const Array* const array_type= type.GetArrayType() )
	{
		// SPRACHE_TODO - maybe call destructors of arrays in reverse order?
		GenerateLoop(
			array_type->size,
			[&]( llvm::Value* const index )
			{
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= index;
				CallDestructor(
					function_context.llvm_ir_builder.CreateGEP( ptr, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) ),
					array_type->type,
					function_context );
			},
			function_context );
	}
	else
	{
		U_ASSERT( false && "WTF? strange type for variable" );
	}
}

void CodeBuilder::CallDestructorsForLoopInnerVariables( FunctionContext& function_context )
{
	U_ASSERT( !function_context.loops_stack.empty() );

	// Destroy all local variables before "break"/"continue" in all blocks inside loop.
	size_t undestructed_stack_size= function_context.destructibles_stack.size();
	for(
		auto it= function_context.destructibles_stack.rbegin();
		it != function_context.destructibles_stack.rend() &&
		undestructed_stack_size > function_context.loops_stack.back().destructibles_stack_size;
		++it, --undestructed_stack_size )
	{
		CallDestructors( *it, function_context );
	}
}

void CodeBuilder::CallDestructorsBeforeReturn( FunctionContext& function_context )
{
	// We must call ALL destructors of local variables, arguments, etc before each return.
	for( auto it= function_context.destructibles_stack.rbegin(); it != function_context.destructibles_stack.rend(); ++it )
		CallDestructors( *it, function_context );
}

void CodeBuilder::CallMembersDestructors( FunctionContext& function_context )
{
	U_ASSERT( function_context.this_ != nullptr );
	const ClassPtr class_= function_context.this_->type.GetClassType();
	U_ASSERT( class_ != nullptr );

	class_->members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;
			if( !field->type.HaveDestructor() )
				return;

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			CallDestructor(
				function_context.llvm_ir_builder.CreateGEP( function_context.this_->llvm_value, index_list ),
				field->type,
				function_context );
		} );
}

void CodeBuilder::BuildNamespaceBody(
	const ProgramElements& body_elements,
	NamesScope& names_scope )
{
	for( const IProgramElementPtr& program_element : body_elements )
	{
		if( const FunctionDeclaration* const func=
			dynamic_cast<const FunctionDeclaration*>( program_element.get() ) )
		{
			PrepareFunction( *func, false, nullptr, names_scope );
		}
		else if(
			const ClassDeclaration* const class_=
			dynamic_cast<const ClassDeclaration*>( program_element.get() ) )
		{
			PrepareClass( *class_, names_scope );
		}
		else if(
			const Namespace* const namespace_=
			dynamic_cast<const Namespace*>( program_element.get() ) )
		{
			NamesScope* result_scope= &names_scope;

			const NamesScope::InsertedName* const same_name=
				names_scope.GetThisScopeName( namespace_->name_ );
			if( same_name != nullptr )
			{
				if( const NamesScopePtr same_namespace= same_name->second.GetNamespace() )
					result_scope= same_namespace.get(); // Extend existend namespace.
				else
					errors_.push_back( ReportRedefinition( namespace_->file_pos_, namespace_->name_ ) );
			}
			else
			{
				const NamesScopePtr new_names_scope= std::make_shared<NamesScope>( namespace_->name_, &names_scope );
				names_scope.AddName( namespace_->name_, new_names_scope );
				result_scope= new_names_scope.get();
			}

			BuildNamespaceBody( namespace_->elements_, *result_scope );
		}
		else if(
			const ClassTemplateDeclaration* const class_template_declaration=
			dynamic_cast<const ClassTemplateDeclaration*>( program_element.get() ) )
		{
			ClassTemplatePtr class_template( new ClassTemplate );
			class_template->class_syntax_element= class_template_declaration;

			const ComplexName& name= class_template_declaration->class_->name_;
			if( name.components.size() == 1u )
			{
				names_scope.AddName(  name.components.front(), Value( class_template ) );
			}
			else
			{
				// TODO - search, using complex name
			}
			// TODO
			//names_scope.AddName( class_template_declaration->class_->name_, Value( class_template ) );
		}
		else
		{
			U_ASSERT(false);
		}
	} // for program elements
}

void CodeBuilder::PrepareFunction(
	const FunctionDeclaration& func,
	const bool is_class_method_predeclaration,
	ClassPtr base_class,
	NamesScope& func_definition_names_scope /* scope, where this function appears */ )
{
	const ProgramString& func_name= func.name_.components.back();

	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	const bool is_special_method= is_constructor || is_destructor;

	if( !is_special_method && IsKeyword( func_name ) )
		errors_.push_back( ReportUsingKeywordAsName( func.file_pos_ ) );

	const Block* const block= is_class_method_predeclaration ? nullptr : func.block_.get();

	// Base scope (class, namespace), where function is declared.
	// Arguments, return value, body names all resolved from this scope.
	NamesScope* func_base_names_scope= &func_definition_names_scope;

	if( func.name_.components.size() >= 2u )
	{
		// Complex name - search scope for this function.
		ComplexName base_space_name= func.name_;
		base_space_name.components.pop_back();
		if( const NamesScope::InsertedName* const scope_name=
			func_definition_names_scope.ResolveName( base_space_name ) )
		{
			bool base_space_is_class= false;
			if( const Type* const type= scope_name->second.GetTypeName() )
			{
				if( const ClassPtr class_= type->GetClassType() )
				{
					func_base_names_scope= &class_->members;
					base_class= class_; // TODO - check here if base_class nonnull and diffrs from class_?
					base_space_is_class= true;
				}
			}

			if( base_space_is_class ) {}
			else if( const NamesScopePtr namespace_= scope_name->second.GetNamespace() )
			{
				func_base_names_scope= namespace_.get();
			}
			else
			{
				errors_.push_back( ReportNameNotFound( func.file_pos_, base_space_name ) );
				return;
			}
		}
		else
		{
			errors_.push_back( ReportFunctionDeclarationOutsideItsScope( func.file_pos_ ) );
			return;
		}
	}

	if( is_special_method && base_class == nullptr )
	{
		errors_.push_back( ReportConstructorOrDestructorOutsideClass( func.file_pos_ ) );
		return;
	}
	if( !is_constructor && func.constructor_initialization_list_ != nullptr )
	{
		errors_.push_back( ReportInitializationListInNonconstructor(  func.constructor_initialization_list_->file_pos_ ) );
		return;
	}
	if( is_destructor && !func.arguments_.empty() )
	{
		errors_.push_back( ReportExplicitArgumentsInDestructor( func.file_pos_ ) );
		return;
	}

	FunctionVariable func_variable;
	func_variable.type= Function();
	Function& function_type= *func_variable.type.GetFunctionType();

	if( func.return_type_.name.components.empty() )
		function_type.return_type= void_type_;
	else
	{
		function_type.return_type= PrepareType( func.file_pos_, func.return_type_, *func_base_names_scope );
		if( function_type.return_type == invalid_type_ )
			return;
	}

	// SPRACHE_TODO - make variables without explicit mutability modifiers immutable.
	if( func.return_value_mutability_modifier_ == MutabilityModifier::Immutable )
		function_type.return_value_is_mutable= false;
	else
		function_type.return_value_is_mutable= true;
	function_type.return_value_is_reference= func.return_value_reference_modifier_ == ReferenceModifier::Reference;

	if( !function_type.return_value_is_reference &&
		!( function_type.return_type.GetFundamentalType() != nullptr ||
		   function_type.return_type.GetClassType() != nullptr ) )
	{
		errors_.push_back( ReportNotImplemented( func.file_pos_, "return value types except fundamental and classes" ) );
		return;
	}

	if( is_special_method && function_type.return_type != void_type_ )
		errors_.push_back( ReportConstructorAndDestructorMustReturnVoid( func.file_pos_ ) );

	// Args.
	function_type.args.reserve( func.arguments_.size() );

	// Generate "this" arg for constructors.
	if( is_special_method )
	{
		func_variable.is_this_call= true;

		function_type.args.emplace_back();
		Function::Arg& arg= function_type.args.back();
		arg.type= base_class;
		arg.is_reference= true;
		arg.is_mutable= true;
	}

	for( const FunctionArgumentDeclarationPtr& arg : func.arguments_ )
	{
		const bool is_this= arg == func.arguments_.front() && arg->name_ == Keywords::this_;

		if( !is_this && IsKeyword( arg->name_ ) )
			errors_.push_back( ReportUsingKeywordAsName( arg->file_pos_ ) );

		if( is_this && is_special_method )
			errors_.push_back( ReportExplicitThisInConstructorOrDestructor( arg->file_pos_ ) );

		function_type.args.emplace_back();
		Function::Arg& out_arg= function_type.args.back();

		if( is_this )
		{
			func_variable.is_this_call= true;
			if( base_class == nullptr )
			{
				errors_.push_back( ReportThisInNonclassFunction( func.file_pos_, func_name ) );
				return;
			}
			out_arg.type= base_class;
		}
		else
			out_arg.type= PrepareType( arg->file_pos_, arg->type_, *func_base_names_scope );

		// TODO - make variables without explicit mutability modifiers immutable.
		if( arg->mutability_modifier_ == MutabilityModifier::Immutable )
			out_arg.is_mutable= false;
		else
			out_arg.is_mutable= true;
		out_arg.is_reference= is_this || arg->reference_modifier_ == ReferenceModifier::Reference;

		if( !out_arg.is_reference &&
			!( out_arg.type.GetFundamentalType() != nullptr ||
			   out_arg.type.GetClassType() != nullptr ) )
		{
			errors_.push_back( ReportNotImplemented( func.file_pos_, "parameters types except fundamental and classes" ) );
			return;
		}

		if( out_arg.type.IsIncomplete() && !out_arg.is_reference )
		{
			errors_.push_back( ReportUsingIncompleteType( arg->file_pos_, out_arg.type.ToString() ) );
		}
	}

	NamesScope::InsertedName* const previously_inserted_func=
		func_base_names_scope->GetThisScopeName( func_name );
	if( previously_inserted_func == nullptr )
	{
		if( func.name_.components.size() > 1u )
		{
			errors_.push_back( ReportFunctionDeclarationOutsideItsScope( func.file_pos_ ) );
			return;
		}

		OverloadedFunctionsSet functions_set;
		functions_set.push_back( std::move( func_variable ) );

		// New name in this scope - insert it.
		NamesScope::InsertedName* const inserted_func=
			func_base_names_scope->AddName( func_name, std::move( functions_set ) );
		U_ASSERT( inserted_func != nullptr );

		BuildFuncCode(
			inserted_func->second.GetFunctionsSet()->front(),
			base_class,
			*func_base_names_scope,
			func_name,
			func.arguments_,
			block,
			func.constructor_initialization_list_.get() );
	}
	else
	{
		Value& value= previously_inserted_func->second;
		if( OverloadedFunctionsSet* const functions_set= value.GetFunctionsSet() )
		{
			if( FunctionVariable* const same_function=
				GetFunctionWithExactSignature(
					*func_variable.type.GetFunctionType(),
					*functions_set ) )
			{
				if( func.block_ == nullptr )
				{
					errors_.push_back( ReportFunctionPrototypeDuplication( func.file_pos_, func_name ) );
					return;
				}
				if( same_function->have_body )
				{
					errors_.push_back( ReportFunctionBodyDuplication( func.file_pos_, func_name ) );
					return;
				}
				if( same_function->type != func_variable.type )
				{
					// In this place we have only possible error
					errors_.push_back( ReportReturnValueDiffersFromPrototype( func.file_pos_ ) );
					return;
				}

				BuildFuncCode(
					*same_function,
					base_class,
					*func_base_names_scope,
					func_name,
					func.arguments_,
					block,
					func.constructor_initialization_list_.get() );
			}
			else
			{
				if( func.name_.components.size() > 1u )
				{
					errors_.push_back( ReportFunctionDeclarationOutsideItsScope( func.file_pos_ ) );
					return;
				}

				try
				{
					ApplyOverloadedFunction( *functions_set, func_variable, func.file_pos_ );
				}
				catch( const ProgramError& )
				{
					return;
				}

				BuildFuncCode(
					functions_set->back(),
					base_class,
					*func_base_names_scope,
					func_name,
					func.arguments_,
					block,
					func.constructor_initialization_list_.get() );
			}
		}
		else
			errors_.push_back( ReportRedefinition( func.file_pos_, previously_inserted_func->first ) );
	}
}

void CodeBuilder::BuildFuncCode(
	FunctionVariable& func_variable,
	const ClassPtr base_class,
	const NamesScope& parent_names_scope,
	const ProgramString& func_name,
	const FunctionArgumentsDeclaration& args,
	const Block* const block,
	const StructNamedInitializer* const constructor_initialization_list ) noexcept
{
	std::vector<llvm::Type*> args_llvm_types;
	Function* const function_type= func_variable.type.GetFunctionType();

	bool first_arg_is_sret= false;
	if( !function_type->return_value_is_reference )
	{
		if( function_type->return_type.GetFundamentalType() != nullptr )
		{}
		else if( const ClassPtr class_type= function_type->return_type.GetClassType() )
		{
			// Add return-value ponter as "sret" argument for class types.
			args_llvm_types.push_back( llvm::PointerType::get( class_type->llvm_type, 0u ) );
			first_arg_is_sret= true;
			func_variable.return_value_is_sret= true;
		}
		else
		{ U_ASSERT( false ); }
	}

	for( const Function::Arg& arg : function_type->args )
	{
		llvm::Type* type= arg.type.GetLLVMType();
		if( arg.is_reference )
			type= llvm::PointerType::get( type, 0u );
		else
		{
			if( arg.type.GetFundamentalType() != nullptr )
			{}
			else if( arg.type.GetClassType() != nullptr )
			{
				// Mark value-parameters of class types as pointer. Lately this parameters will be marked as "byval".
				type= llvm::PointerType::get( type, 0u );
			}
			else
			{ U_ASSERT( false ); }
		}
		args_llvm_types.push_back( type );
	}

	llvm::Type* llvm_function_return_type;
	if( first_arg_is_sret )
		llvm_function_return_type= fundamental_llvm_types_.void_;
	else
	{
		llvm_function_return_type= function_type->return_type.GetLLVMType();
		if( function_type->return_value_is_reference )
			llvm_function_return_type= llvm::PointerType::get( llvm_function_return_type, 0u );
	}

	function_type->llvm_function_type=
		llvm::FunctionType::get(
			llvm_function_return_type,
			llvm::ArrayRef<llvm::Type*>( args_llvm_types.data(), args_llvm_types.size() ),
			false );

	llvm::Function* llvm_function;
	if( func_variable.llvm_function == nullptr )
	{
		llvm_function=
			llvm::Function::Create(
				function_type->llvm_function_type,
				llvm::Function::LinkageTypes::ExternalLinkage, // TODO - select linkage
				MangleFunction( parent_names_scope, func_name, *function_type, func_variable.is_this_call ),
				module_.get() );

		// Merge functions with identical code.
		// We doesn`t need different addresses for different functions.
		llvm_function->setUnnamedAddr( true );

		// Mark reference-parameters as nonnull.
		// Mark fake-pointer parameters of struct type as "byvall".
		// TODO - maybe mark immutable reference-parameters as "noalias"?
		for( size_t i= 0u; i < function_type->args.size(); i++ )
		{
			const size_t arg_attr_index= i + 1u + (first_arg_is_sret ? 1u : 0u );
			if (function_type->args[i].is_reference )
				llvm_function->addAttribute( arg_attr_index, llvm::Attribute::NonNull );
			else
			{
				if( function_type->args[i].type.GetClassType() != nullptr )
				{
					llvm_function->addAttribute( arg_attr_index, llvm::Attribute::NonNull );
					llvm_function->addAttribute( arg_attr_index, llvm::Attribute::ByVal );
				}
			}
		}

		if( first_arg_is_sret )
			llvm_function->addAttribute( 1u, llvm::Attribute::StructRet );

		func_variable.llvm_function= llvm_function;
	}
	else
		llvm_function= llvm::dyn_cast<llvm::Function>( func_variable.llvm_function );

	if( block == nullptr )
	{
		// This is only prototype, then, function preparing work is done.
		func_variable.have_body= false;
		return;
	}

	func_variable.have_body= true;

	NamesScope function_names( ""_SpC, &parent_names_scope );
	FunctionContext function_context(
		function_type->return_type,
		function_type->return_value_is_mutable,
		function_type->return_value_is_reference,
		llvm_context_,
		llvm_function );

	function_context.destructibles_stack.emplace_back();

	// push args
	Variable this_;
	Variable s_ret;
	unsigned int arg_number= 0u;

	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	const bool is_special_method= is_constructor || is_destructor;

	for( llvm::Argument& llvm_arg : llvm_function->args() )
	{
		// Skip "sret".
		if( first_arg_is_sret && &llvm_arg == &*llvm_function->arg_begin() )
		{
			s_ret.location= Variable::Location::Pointer;
			s_ret.value_type= ValueType::Reference;
			s_ret.type= function_type->return_type;
			s_ret.llvm_value= &llvm_arg;
			llvm_arg.setName( "_return_value" );
			function_context.s_ret_= &s_ret;
			continue;
		}

		const Function::Arg& arg= function_type->args[ arg_number ];

		if( is_special_method && arg_number == 0u )
		{
			this_.location= Variable::Location::Pointer;
			this_.value_type= ValueType::Reference;
			this_.type= arg.type;
			this_.llvm_value= &llvm_arg;
			llvm_arg.setName( KeywordAscii( Keywords::this_ ) );
			function_context.this_= &this_;
			arg_number++;
			continue;
		}

		const FunctionArgumentDeclaration& declaration_arg= *args[ is_special_method ? ( arg_number - 1u ) : arg_number ];
		const ProgramString& arg_name= declaration_arg.name_;

		const bool is_this= arg_number == 0u && arg_name == Keywords::this_;
		U_ASSERT( !( is_this && !arg.is_reference ) );
		U_ASSERT( !( is_special_method && is_this ) );

		Variable var;
		var.location= Variable::Location::LLVMRegister;
		var.value_type= ValueType::Reference;
		var.type= arg.type;
		var.llvm_value= &llvm_arg;

		// TODO - make variables without explicit mutability modifiers immutable.
		if( declaration_arg.mutability_modifier_ == MutabilityModifier::Immutable )
			var.value_type= ValueType::ConstReference;

		if( arg.is_reference )
			var.location= Variable::Location::Pointer;
		else
		{
			if( arg.type.GetFundamentalType() != nullptr )
			{
				// Move parameters to stack for assignment possibility.
				// TODO - do it, only if parameters are not constant.
				llvm::Value* address= function_context.alloca_ir_builder.CreateAlloca( var.type.GetLLVMType() );
				address->setName( ToStdString( arg_name ) );
				function_context.llvm_ir_builder.CreateStore( var.llvm_value, address );

				var.llvm_value= address;
				var.location= Variable::Location::Pointer;
			}
			else if( arg.type.GetClassType() != nullptr )
			{
				// Value classes parameters using llvm-pointers with "byval" attribute.
				var.location= Variable::Location::Pointer;
			}
			else
			{ U_ASSERT( false ); }
		}

		if( is_this )
		{
			// Save "this" in function context for accessing inside class methods.
			this_= std::move(var);
			function_context.this_= &this_;
		}
		else
		{
			const NamesScope::InsertedName* const inserted_arg=
				function_names.AddName(
					arg_name,
					std::move(var) );
			if( !inserted_arg )
			{
				errors_.push_back( ReportRedefinition( declaration_arg.file_pos_, arg_name ) );
				return;
			}

			if( !arg.is_reference )
				function_context.destructibles_stack.back().RegisterVariable( *inserted_arg->second.GetVariable() );
		}

		llvm_arg.setName( "_arg_" + ToStdString( arg_name ) );
		++arg_number;
	}

	if( is_constructor )
	{
		U_ASSERT( base_class != nullptr );
		U_ASSERT( function_context.this_ != nullptr );

		function_context.is_constructor_initializer_list_now= true;

		if( constructor_initialization_list == nullptr )
		{
			// Create dummy initialization list for constructors without explicit initialization list.
			const StructNamedInitializer dumy_initialization_list{ FilePos() };

			BuildConstructorInitialization(
				*function_context.this_,
				*base_class,
				function_names,
				function_context,
				dumy_initialization_list );
		}
		else
			BuildConstructorInitialization(
				*function_context.this_,
				*base_class,
				function_names,
				function_context,
				*constructor_initialization_list );

		function_context.is_constructor_initializer_list_now= false;
	}

	if( is_destructor )
		function_context.destructor_end_block= llvm::BasicBlock::Create( llvm_context_ );

	const BlockBuildInfo block_build_info=
		BuildBlockCode( *block, function_names, function_context );
	U_ASSERT( function_context.destructibles_stack.size() == 1u || !errors_.empty() || error_count_ > 0u );

	// We need call destructors for arguments only if function returns "void".
	// In other case, we have "return" in all branches and destructors call before each "return".

	if(  function_type->return_type == void_type_ )
	{
		// Manually generate "return" for void-return functions.
		if( !block_build_info.have_unconditional_return_inside )
		{
			CallDestructors( function_context.destructibles_stack.back(), function_context );

			if( function_context.destructor_end_block == nullptr )
				function_context.llvm_ir_builder.CreateRetVoid();
			else
			{
				// In explicit destructor, break to block with destructor calls for class members.
				function_context.llvm_ir_builder.CreateBr( function_context.destructor_end_block );
			}
		}
	}
	else
	{
		if( !block_build_info.have_unconditional_return_inside )
		{
			errors_.push_back( ReportNoReturnInFunctionReturningNonVoid( block->file_pos_ ) );
			return;
		}
	}

	llvm::Function::BasicBlockListType& bb_list = llvm_function->getBasicBlockList();

	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );

	if( is_destructor )
	{
		// Fill destructors block.
		U_ASSERT( function_context.destructor_end_block != nullptr );
		function_context.llvm_ir_builder.SetInsertPoint( function_context.destructor_end_block );
		bb_list.push_back( function_context.destructor_end_block );

		CallMembersDestructors( function_context );
		function_context.llvm_ir_builder.CreateRetVoid();
	}

	// Remove duplicated terminator instructions at end of all function blocks.
	// This needs, for example, when "return" is last operator inside "if" or "while" blocks.
	for (llvm::BasicBlock& block : bb_list)
	{
		llvm::BasicBlock::InstListType& instr_list = block.getInstList();
		while (instr_list.size() >= 2u && instr_list.back().isTerminator() &&
			(std::next(instr_list.rbegin()))->isTerminator())
		{
			instr_list.pop_back();
		}
	}

	// Remove basic blocks without instructions. Such block can exist after if/else with return in all branches, for example.
	auto it= bb_list.begin();
	while(it != bb_list.end())
	{
		if( &*it != function_context.function_basic_block && it->empty() )
			it= bb_list.erase(it);
		else
			++it;
	}
}

void CodeBuilder::BuildConstructorInitialization(
	const Variable& this_,
	const Class& base_class,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const StructNamedInitializer& constructor_initialization_list ) noexcept
{
	std::set<ProgramString> initialized_fields;

	// Check for errors, build list of initialized fields.
	bool have_fields_errors= false;
	for( const StructNamedInitializer::MemberInitializer& field_initializer : constructor_initialization_list.members_initializers )
	{
		const NamesScope::InsertedName* const class_member=
			base_class.members.GetThisScopeName( field_initializer.name );

		if( class_member == nullptr )
		{
			have_fields_errors= true;
			errors_.push_back( ReportNameNotFound( constructor_initialization_list.file_pos_, field_initializer.name ) );
			continue;
		}

		const ClassField* const field= class_member->second.GetClassField();
		if( field == nullptr )
		{
			have_fields_errors= true;
			errors_.push_back( ReportInitializerForNonfieldStructMember( constructor_initialization_list.file_pos_, field_initializer.name ) );
			continue;
		}

		if( initialized_fields.find( field_initializer.name ) != initialized_fields.end() )
		{
			have_fields_errors= true;
			errors_.push_back( ReportDuplicatedStructMemberInitializer( constructor_initialization_list.file_pos_, field_initializer.name ) );
			continue;
		}

		initialized_fields.insert( field_initializer.name );
		function_context.uninitialized_this_fields.insert( field );
	} // for fields initializers

	std::set<ProgramString> uninitialized_fields;

	base_class.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;

			if( initialized_fields.find( member.first ) == initialized_fields.end() )
				uninitialized_fields.insert( member.first );
		} );

	// Initialize fields, missing in initializer list.
	for( const ProgramString& field_name : uninitialized_fields )
	{
		const NamesScope::InsertedName* const class_member=
			base_class.members.GetThisScopeName( field_name );
		U_ASSERT( class_member != nullptr );
		const ClassField* const field= class_member->second.GetClassField();
		U_ASSERT( field != nullptr );

		try
		{
			Variable field_variable;
			field_variable.type= field->type;
			field_variable.location= Variable::Location::Pointer;
			field_variable.value_type= ValueType::Reference;

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			field_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( this_.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			ApplyEmptyInitializer( field_name, constructor_initialization_list.file_pos_, field_variable, function_context );
		}
		catch( const ProgramError& ){}
	}

	if( have_fields_errors )
		return;

	for( const StructNamedInitializer::MemberInitializer& field_initializer : constructor_initialization_list.members_initializers )
	{
		const NamesScope::InsertedName* const class_member=
			base_class.members.GetThisScopeName( field_initializer.name );
		U_ASSERT( class_member != nullptr );
		const ClassField* const field= class_member->second.GetClassField();
		U_ASSERT( field != nullptr );

		try
		{
			Variable field_variable;
			field_variable.type= field->type;
			field_variable.location= Variable::Location::Pointer;
			field_variable.value_type= ValueType::Reference;

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			field_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( this_.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			U_ASSERT( field_initializer.initializer != nullptr );
			ApplyInitializer( field_variable, *field_initializer.initializer, names_scope, function_context );
		}
		catch( const ProgramError& ){}

		function_context.uninitialized_this_fields.erase( field );
	} // for fields initializers
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockCode(
	const Block& block,
	const NamesScope& names,
	FunctionContext& function_context ) noexcept
{
	NamesScope block_names( ""_SpC, &names );
	BlockBuildInfo block_build_info;

	function_context.destructibles_stack.emplace_back();

	for( const IBlockElementPtr& block_element : block.elements_ )
	{
		const IBlockElement* const block_element_ptr= block_element.get();

		const auto try_report_unreachable_code=
		[&]
		{
			const unsigned int block_element_index= &block_element - block.elements_.data();
			if( block_element_index + 1u < block.elements_.size() )
				errors_.push_back( ReportUnreachableCode( block.elements_[ block_element_index + 1u ]->file_pos_ ) );
		};

		try
		{
			if( const VariablesDeclaration* variables_declaration=
				dynamic_cast<const VariablesDeclaration*>( block_element_ptr ) )
			{
				BuildVariablesDeclarationCode( *variables_declaration, block_names, function_context );
			}
			else if( const AutoVariableDeclaration* auto_variable_declaration=
				dynamic_cast<const AutoVariableDeclaration*>( block_element_ptr ) )
			{
				BuildAutoVariableDeclarationCode( *auto_variable_declaration, block_names, function_context );
			}
			else if(
				const SingleExpressionOperator* expression=
				dynamic_cast<const SingleExpressionOperator*>( block_element_ptr ) )
			{
				BuildExpressionCodeAndDestroyTemporaries(
					*expression->expression_,
					block_names,
					function_context );
			}
			else if(
				const AssignmentOperator* assignment_operator=
				dynamic_cast<const AssignmentOperator*>( block_element_ptr ) )
			{
				BuildAssignmentOperatorCode( *assignment_operator, block_names, function_context );
			}
			else if(
				const AdditiveAssignmentOperator* additive_assignment_operator=
				dynamic_cast<const AdditiveAssignmentOperator*>( block_element_ptr ) )
			{
				BuildAdditiveAssignmentOperatorCode( *additive_assignment_operator, block_names, function_context );
			}
			else if(
				const IncrementOperator* increment_operator=
				dynamic_cast<const IncrementOperator*>( block_element_ptr ) )
			{
				BuildDeltaOneOperatorCode(
					*increment_operator->expression,
					increment_operator->file_pos_,
					true,
					block_names,
					function_context );
			}
			else if(
				const DecrementOperator* decrement_operator=
				dynamic_cast<const DecrementOperator*>( block_element_ptr ) )
			{
				BuildDeltaOneOperatorCode(
					*decrement_operator->expression,
					decrement_operator->file_pos_,
					false,
					block_names,
					function_context );
			}
			else if(
				const ReturnOperator* return_operator=
				dynamic_cast<const ReturnOperator*>( block_element_ptr ) )
			{
				BuildReturnOperatorCode(
					*return_operator,
					block_names,
					function_context );

				block_build_info.have_unconditional_return_inside= true;
				try_report_unreachable_code();
			}
			else if(
				const WhileOperator* while_operator=
				dynamic_cast<const WhileOperator*>( block_element_ptr ) )
			{
				BuildWhileOperatorCode(
					*while_operator,
					block_names,
					function_context );
			}
			else if(
				const BreakOperator* break_operator=
				dynamic_cast<const BreakOperator*>( block_element_ptr ) )
			{
				BuildBreakOperatorCode(
					*break_operator,
					function_context );

				block_build_info.have_uncodnitional_break_or_continue= true;
				try_report_unreachable_code();
			}
			else if(
				const ContinueOperator* continue_operator=
				dynamic_cast<const ContinueOperator*>( block_element_ptr ) )
			{
				BuildContinueOperatorCode(
					*continue_operator,
					function_context );

				block_build_info.have_uncodnitional_break_or_continue= true;
				try_report_unreachable_code();
			}
			else if(
				const IfOperator* if_operator=
				dynamic_cast<const IfOperator*>( block_element_ptr ) )
			{
				const CodeBuilder::BlockBuildInfo if_block_info=
					BuildIfOperatorCode(
						*if_operator,
						block_names,
						function_context );

				block_build_info.have_unconditional_return_inside=
					block_build_info.have_unconditional_return_inside || if_block_info.have_unconditional_return_inside;
				block_build_info.have_uncodnitional_break_or_continue=
					block_build_info.have_uncodnitional_break_or_continue || if_block_info.have_uncodnitional_break_or_continue;

				if( if_block_info.have_unconditional_return_inside ||
					block_build_info.have_uncodnitional_break_or_continue )
					try_report_unreachable_code();
			}
			else if(
				const StaticAssert* static_assert_=
				dynamic_cast<const StaticAssert*>( block_element_ptr ) )
			{
				BuildStaticAssert( *static_assert_, block_names );
			}
			else if(
				const Block* block=
				dynamic_cast<const Block*>( block_element_ptr ) )
			{
				const BlockBuildInfo inner_block_build_info=
					BuildBlockCode( *block, block_names, function_context );

				block_build_info.have_unconditional_return_inside=
					block_build_info.have_unconditional_return_inside || inner_block_build_info.have_unconditional_return_inside;
				block_build_info.have_uncodnitional_break_or_continue=
					block_build_info.have_uncodnitional_break_or_continue || inner_block_build_info.have_uncodnitional_break_or_continue;

				if( inner_block_build_info.have_unconditional_return_inside ||
					block_build_info.have_uncodnitional_break_or_continue )
					try_report_unreachable_code();
			}
			else
			{
				U_ASSERT(false);
			}
		}
		catch( const ProgramError& )
		{
			error_count_++;
		}
	}

	// If there are undconditional "break", "continue", "return" operators,
	// we didn`t need call destructors, it must be called in this operators.
	if( !( block_build_info.have_uncodnitional_break_or_continue || block_build_info.have_unconditional_return_inside ) )
		CallDestructors( function_context.destructibles_stack.back(), function_context );

	function_context.destructibles_stack.pop_back();

	return block_build_info;
}

void CodeBuilder::BuildVariablesDeclarationCode(
	const VariablesDeclaration& variables_declaration,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	const Type type= PrepareType( variables_declaration.file_pos_, variables_declaration.type, block_names );
	if( type.IsIncomplete() )
	{
		errors_.push_back( ReportUsingIncompleteType( variables_declaration.file_pos_, type.ToString() ) );
		return;
	}

	for( const VariablesDeclaration::VariableEntry& variable_declaration : variables_declaration.variables )
	{
		if( IsKeyword( variable_declaration.name ) )
		{
			errors_.push_back( ReportUsingKeywordAsName( variables_declaration.file_pos_ ) );
			continue;
		}

		if( variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
			!type.CanBeConstexpr() )
		{
			errors_.push_back( ReportInvalidTypeForConstantExpressionVariable( variables_declaration.file_pos_ ) );
			continue;
		}

		Variable variable;
		variable.type= type;
		variable.location= Variable::Location::Pointer;
		variable.value_type= ValueType::Reference;

		if( variable_declaration.reference_modifier == ReferenceModifier::None )
		{
			variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType() );
			variable.llvm_value->setName( ToStdString( variable_declaration.name ) );

			if( variable_declaration.initializer != nullptr )
				variable.constexpr_value=
					ApplyInitializer( variable, *variable_declaration.initializer, block_names, function_context );
			else
				ApplyEmptyInitializer( variable_declaration.name, variables_declaration.file_pos_, variable, function_context );

			// Make immutable, if needed, only after initialization, because in initialization we need call constructors, which is mutable methods.
			// SPRACHE_TODO - make variables without explicit mutability modifiers immutable.
			if( variable_declaration.mutability_modifier == MutabilityModifier::Immutable ||
				variable_declaration.mutability_modifier == MutabilityModifier::Constexpr )
				variable.value_type= ValueType::ConstReference;
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			// Mark references immutable before initialization.
			// SPRACHE_TODO - make variables without explicit mutability modifiers immutable.
			if( variable_declaration.mutability_modifier == MutabilityModifier::Immutable ||
				variable_declaration.mutability_modifier == MutabilityModifier::Constexpr )
				variable.value_type= ValueType::ConstReference;

			if( variable_declaration.initializer == nullptr )
			{
				errors_.push_back( ReportExpectedInitializer( variables_declaration.file_pos_, variable_declaration.name ) );
				continue;
			}

			const IExpressionComponent* initializer_expression= nullptr;
			if( const ExpressionInitializer* const expression_initializer=
				dynamic_cast<const ExpressionInitializer*>( variable_declaration.initializer.get() ) )
			{
				initializer_expression= expression_initializer->expression.get();
			}
			else if( const ConstructorInitializer* const constructor_initializer=
				dynamic_cast<const ConstructorInitializer*>( variable_declaration.initializer.get() ) )
			{
				if( constructor_initializer->call_operator.arguments_.size() != 1u )
				{
					errors_.push_back( ReportReferencesHaveConstructorsWithExactlyOneParameter( constructor_initializer->file_pos_ ) );
					continue;
				}
				initializer_expression= constructor_initializer->call_operator.arguments_.front().get();
			}
			else
			{
				errors_.push_back( ReportUnsupportedInitializerForReference( variable_declaration.initializer->file_pos_ ) );
				continue;
			}

			const Value expression_result_value=
				BuildExpressionCodeAndDestroyTemporaries( *initializer_expression, block_names, function_context );

			if( expression_result_value.GetType() != variable.type )
			{
				errors_.push_back( ReportTypesMismatch( variables_declaration.file_pos_, variable.type.ToString(), expression_result_value.GetType().ToString() ) );
				continue;
			}
			const Variable& expression_result= *expression_result_value.GetVariable();

			if( expression_result.value_type == ValueType::Value )
			{
				errors_.push_back( ReportExpectedReferenceValue( variables_declaration.file_pos_ ) );
				continue;
			}
			if( expression_result.value_type == ValueType::ConstReference &&
				variable.value_type == ValueType::Reference )
			{
				errors_.push_back( ReportBindingConstReferenceToNonconstReference( variables_declaration.file_pos_ ) );
				continue;
			}

			// TODO - maybe make copy of varaible address in new llvm register?
			variable.llvm_value= expression_result.llvm_value;
			variable.constexpr_value= expression_result.constexpr_value;
		}
		else
		{
			U_ASSERT(false);
		}

		if( variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
			variable.constexpr_value == nullptr )
		{
			errors_.push_back( ReportVariableInitializerIsNotConstantExpression( variables_declaration.file_pos_ ) );
			continue;
		}

		// Reset constexpr initial value for mutable variables.
		if( variable.value_type != ValueType::ConstReference )
			variable.constexpr_value= nullptr;

		const NamesScope::InsertedName* inserted_name=
			block_names.AddName( variable_declaration.name, std::move(variable) );

		if( !inserted_name )
		{
			errors_.push_back( ReportRedefinition( variables_declaration.file_pos_, variable_declaration.name ) );
			continue;
		}

		if( variable_declaration.reference_modifier == ReferenceModifier::None )
			function_context.destructibles_stack.back().RegisterVariable( *inserted_name->second.GetVariable() );
	}
}

void CodeBuilder::BuildAutoVariableDeclarationCode(
	const AutoVariableDeclaration& auto_variable_declaration,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	const Value initializer_experrsion_value=
		BuildExpressionCodeAndDestroyTemporaries( *auto_variable_declaration.initializer_expression, block_names, function_context );

	{ // Check expression type. Expression can have exotic types, such "Overloading functions set", "class name", etc.
		const Type& type= initializer_experrsion_value.GetType();
		const bool type_is_ok=
			type.GetFundamentalType() != nullptr ||
			type.GetArrayType() != nullptr ||
			type.GetClassType() != nullptr;
		if( !type_is_ok )
		{
			errors_.push_back( ReportInvalidTypeForAutoVariable( auto_variable_declaration.file_pos_, initializer_experrsion_value.GetType().ToString() ) );
			return;
		}
	}

	const Variable& initializer_experrsion= *initializer_experrsion_value.GetVariable();

	Variable variable;
	variable.location= Variable::Location::Pointer;

	// SPRACHE_TODO - make variables without explicit mutability modifiers immutable.
	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Immutable||
		auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr )
		variable.value_type= ValueType::ConstReference;
	else
		variable.value_type= ValueType::Reference;

	variable.type= initializer_experrsion.type;

	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
		!variable.type.CanBeConstexpr() )
	{
		errors_.push_back( ReportInvalidTypeForConstantExpressionVariable( auto_variable_declaration.file_pos_ ) );
		return;
	}

	if( auto_variable_declaration.reference_modifier == ReferenceModifier::Reference )
	{
		if( initializer_experrsion.value_type == ValueType::Value )
		{
			errors_.push_back( ReportExpectedReferenceValue( auto_variable_declaration.file_pos_ ) );
			return;
		}
		if( initializer_experrsion.value_type == ValueType::ConstReference &&
			variable.value_type != ValueType::ConstReference )
		{
			errors_.push_back( ReportBindingConstReferenceToNonconstReference( auto_variable_declaration.file_pos_ ) );
			return;
		}

		variable.llvm_value= initializer_experrsion.llvm_value;
		variable.constexpr_value= initializer_experrsion.constexpr_value;
	}
	else if( auto_variable_declaration.reference_modifier == ReferenceModifier::None )
	{
		variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType() );
		variable.llvm_value->setName( ToStdString( auto_variable_declaration.name ) );

		if( const FundamentalType* const fundamental_type= variable.type.GetFundamentalType() )
		{
			U_UNUSED(fundamental_type);
			llvm::Value* const value_for_assignment= CreateMoveToLLVMRegisterInstruction( initializer_experrsion, function_context );
			function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );

			variable.constexpr_value= initializer_experrsion.constexpr_value;
		}
		else
		{
			errors_.push_back( ReportNotImplemented( auto_variable_declaration.file_pos_, "expression initialization for nonfundamental types" ) );
			return;
		}
	}
	else
	{
		U_ASSERT(false);
	}

	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
		variable.constexpr_value == nullptr )
	{
		errors_.push_back( ReportVariableInitializerIsNotConstantExpression( auto_variable_declaration.file_pos_ ) );
		return;
	}

	// Reset constexpr initial value for mutable variables.
	if( variable.value_type != ValueType::ConstReference )
		variable.constexpr_value= nullptr;

	const NamesScope::InsertedName* inserted_name=
		block_names.AddName( auto_variable_declaration.name, std::move(variable) );

	if( inserted_name == nullptr )
	{
		errors_.push_back( ReportRedefinition( auto_variable_declaration.file_pos_, auto_variable_declaration.name ) );
		return;
	}

	if( auto_variable_declaration.reference_modifier == ReferenceModifier::None )
		function_context.destructibles_stack.back().RegisterVariable( *inserted_name->second.GetVariable() );
}

void CodeBuilder::BuildAssignmentOperatorCode(
	const AssignmentOperator& assignment_operator,
	const NamesScope& block_names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expressions.
	function_context.destructibles_stack.emplace_back();

	const IExpressionComponent& l_value= *assignment_operator.l_value_;
	const IExpressionComponent& r_value= *assignment_operator.r_value_;

	const Value l_var_value= BuildExpressionCode( l_value, block_names, function_context );
	const Value r_var_value= BuildExpressionCode( r_value, block_names, function_context );
	const Variable* const l_var= l_var_value.GetVariable();
	const Variable* const r_var= r_var_value.GetVariable();
	if( l_var == nullptr )
		errors_.push_back( ReportExpectedVariableInAssignment( assignment_operator.file_pos_, l_var_value.GetType().ToString() ) );
	if( r_var == nullptr )
		errors_.push_back( ReportExpectedVariableInAssignment( assignment_operator.file_pos_, r_var_value.GetType().ToString() ) );
	if( l_var == nullptr || r_var == nullptr )
	{
		// TODO
		return;
	}

	if( l_var->value_type != ValueType::Reference )
	{
		errors_.push_back( ReportExpectedReferenceValue( assignment_operator.file_pos_ ) );
		return;
	}
	if( l_var->type != r_var->type )
	{
		errors_.push_back( ReportTypesMismatch( assignment_operator.file_pos_, l_var->type.ToString(), r_var->type.ToString() ) );
		return;
	}

	const FundamentalType* const fundamental_type= l_var->type.GetFundamentalType();
	if( fundamental_type != nullptr )
	{
		if( l_var->location != Variable::Location::Pointer )
		{
			U_ASSERT(false);
			return;
		}
		llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( *r_var, function_context );
		function_context.llvm_ir_builder.CreateStore( value_for_assignment, l_var->llvm_value );
	}
	else
	{
		// TODO - functions is not copyable.
		// TODO - arrays not copyable.
		// TODO - make classes copyable.
		errors_.push_back( ReportNotImplemented( assignment_operator.file_pos_, "nonfundamental types assignment." ) );
		return;
	}

	// Destruct temporary variables of right and left expressions.
	CallDestructors( function_context.destructibles_stack.back(), function_context );
	function_context.destructibles_stack.pop_back();
}

void CodeBuilder::BuildAdditiveAssignmentOperatorCode(
	const AdditiveAssignmentOperator& additive_assignment_operator,
	const NamesScope& block_names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expressions.
	function_context.destructibles_stack.emplace_back();

	const Value l_var_value=
		BuildExpressionCode(
			*additive_assignment_operator.l_value_,
			block_names,
			function_context );
	const Value r_var_value=
		BuildExpressionCode(
			*additive_assignment_operator.r_value_,
			block_names,
			function_context );

	const Variable* const l_var= l_var_value.GetVariable();
	const Variable* const r_var= r_var_value.GetVariable();

	if( l_var == nullptr )
		errors_.push_back( ReportExpectedVariableInAdditiveAssignment( additive_assignment_operator.file_pos_, l_var_value.GetType().ToString() ) );
	if( r_var == nullptr )
		errors_.push_back( ReportExpectedVariableInAdditiveAssignment( additive_assignment_operator.file_pos_, r_var_value.GetType().ToString() ) );
	if( l_var == nullptr || r_var == nullptr )
		throw ProgramError();

	const FundamentalType* const l_var_fundamental_type= l_var->type.GetFundamentalType();
	const FundamentalType* const r_var_fundamental_type= r_var->type.GetFundamentalType();
	if( l_var_fundamental_type != nullptr && r_var_fundamental_type != nullptr )
	{
		// Generate binary operator and assignment for fundamental types.
		// SPRACHE_TODO - do not call "BuildBinaryOperator", when operators overloading will be implemented.
		const Variable operation_result=
			BuildBinaryOperator(
				*l_var, *r_var,
				additive_assignment_operator.additive_operation_,
				additive_assignment_operator.file_pos_,
				function_context );

		if( l_var->value_type != ValueType::Reference )
		{
			errors_.push_back( ReportExpectedReferenceValue( additive_assignment_operator.file_pos_ ) );
			return;
		}
		if( operation_result.type != l_var->type )
		{
			errors_.push_back( ReportTypesMismatch( additive_assignment_operator.file_pos_, l_var->type.ToString(), operation_result.type.ToString() ) );
			return;
		}

		U_ASSERT( l_var->location == Variable::Location::Pointer );
		llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( operation_result, function_context );
		function_context.llvm_ir_builder.CreateStore( value_in_register, l_var->llvm_value );
	}
	else
	{
		// SPRACHE_TODO - search for overloaded operators.
		errors_.push_back( ReportNotImplemented( additive_assignment_operator.file_pos_, "additive operations for nonfundamental types" ) );
		return;
	}

	// Destruct temporary variables of right and left expressions.
	CallDestructors( function_context.destructibles_stack.back(), function_context );
	function_context.destructibles_stack.pop_back();
}

void CodeBuilder::BuildDeltaOneOperatorCode(
	const IExpressionComponent& expression,
	const FilePos& file_pos,
	bool positive, // true - increment, false - decrement
	const NamesScope& block_names,
	FunctionContext& function_context )
{
	const Value value= BuildExpressionCodeAndDestroyTemporaries( expression, block_names, function_context );
	const Variable* const variable= value.GetVariable();
	if( variable == nullptr )
	{
		errors_.push_back( ReportExpectedVariableInIncrementOrDecrement( file_pos, value.GetType().ToString() ) );
		return;
	}

	if( const FundamentalType* const fundamental_type= variable->type.GetFundamentalType() )
	{
		if( !IsInteger( fundamental_type->fundamental_type ) )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, variable->type.ToString() ) );
			return;
		}
		if( variable->value_type != ValueType::Reference )
		{
			errors_.push_back( ReportExpectedReferenceValue( file_pos ) );
			return;
		}

		llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( *variable, function_context );
		llvm::Value* const one=
			llvm::Constant::getIntegerValue(
				fundamental_type->llvm_type,
				llvm::APInt( variable->type.SizeOf() * 8u, uint64_t(1u) ) );

		llvm::Value* const new_value=
			positive
				? function_context.llvm_ir_builder.CreateAdd( value_in_register, one )
				: function_context.llvm_ir_builder.CreateSub( value_in_register, one );

		U_ASSERT( variable->location == Variable::Location::Pointer );
		function_context.llvm_ir_builder.CreateStore( new_value, variable->llvm_value );
	}
	else
	{
		// SPRACHE_TODO - search for overloaded operators.
		errors_.push_back( ReportNotImplemented( file_pos, "++ and -- for nonfundamental types" ) );
		return;
	}
}

void CodeBuilder::BuildReturnOperatorCode(
	const ReturnOperator& return_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	if( return_operator.expression_ == nullptr )
	{
		if( function_context.return_type != void_type_ )
		{
			errors_.push_back( ReportTypesMismatch( return_operator.file_pos_, void_type_.ToString(), function_context.return_type.ToString() ) );
			return;
		}

		CallDestructorsBeforeReturn( function_context );

		if( function_context.destructor_end_block == nullptr )
			function_context.llvm_ir_builder.CreateRetVoid();
		else
		{
			// In explicit destructor, break to block with destructor calls for class members.
			function_context.llvm_ir_builder.CreateBr( function_context.destructor_end_block );
		}

		return;
	}

	// Destruction frame for temporary variables of result expression.
	function_context.destructibles_stack.emplace_back();

	const Value expression_result_value=
		BuildExpressionCode(
			*return_operator.expression_,
			names,
			function_context );

	// Destruct temporary variables of return expression only after assignment of
	// return value. Destroy all other function variables after this.
	const auto call_destructors=
	[&]()
	{
		CallDestructors( function_context.destructibles_stack.back(), function_context );
		function_context.destructibles_stack.pop_back();

		CallDestructorsBeforeReturn( function_context );
	};

	if( expression_result_value.GetType() != function_context.return_type )
	{
		errors_.push_back( ReportTypesMismatch( return_operator.file_pos_, function_context.return_type.ToString(), expression_result_value.GetType().ToString() ) );
		return;
	}
	const Variable& expression_result= *expression_result_value.GetVariable();

	if( function_context.return_value_is_reference )
	{
		if( expression_result.value_type == ValueType::Value )
		{
			errors_.push_back( ReportExpectedReferenceValue( return_operator.file_pos_ ) );
			return;
		}
		if( expression_result.value_type == ValueType::ConstReference && function_context.return_value_is_mutable )
		{
			errors_.push_back( ReportBindingConstReferenceToNonconstReference( return_operator.file_pos_ ) );
			return;
		}

		call_destructors();
		function_context.llvm_ir_builder.CreateRet( expression_result.llvm_value );
	}
	else
	{
		if( function_context.s_ret_ != nullptr )
		{
			const ClassPtr class_= function_context.s_ret_->type.GetClassType();
			TryCallCopyConstructor( return_operator.file_pos_, function_context.s_ret_->llvm_value, expression_result.llvm_value, class_, function_context );

			call_destructors();
			function_context.llvm_ir_builder.CreateRetVoid();
		}
		else
		{
			// Now we can return by value only fundamentals.
			U_ASSERT( expression_result.type.GetFundamentalType() != nullptr );

			call_destructors();

			llvm::Value* value_for_return= CreateMoveToLLVMRegisterInstruction( expression_result, function_context );
			function_context.llvm_ir_builder.CreateRet( value_for_return );
		}
	}
}

void CodeBuilder::BuildWhileOperatorCode(
	const WhileOperator& while_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	llvm::BasicBlock* test_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* while_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* block_after_while= llvm::BasicBlock::Create( llvm_context_ );

	// Break to test block. We must push terminal instruction at and of current block.
	function_context.llvm_ir_builder.CreateBr( test_block );

	// Test block code.
	function_context.function->getBasicBlockList().push_back( test_block );
	function_context.llvm_ir_builder.SetInsertPoint( test_block );

	const Value condition_expression= BuildExpressionCodeAndDestroyTemporaries( *while_operator.condition_, names, function_context );

	if( condition_expression.GetType() != bool_type_ )
	{
		errors_.push_back(
			ReportTypesMismatch(
				while_operator.condition_->file_pos_,
				bool_type_.ToString(),
				condition_expression.GetType().ToString() ) );
		return;
	}

	llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( *condition_expression.GetVariable(), function_context );

	function_context.llvm_ir_builder.CreateCondBr( condition_in_register, while_block, block_after_while );

	// While block code.

	function_context.loops_stack.emplace_back();
	function_context.loops_stack.back().block_for_break= block_after_while;
	function_context.loops_stack.back().block_for_continue= test_block;
	function_context.loops_stack.back().destructibles_stack_size= function_context.destructibles_stack.size();

	function_context.function->getBasicBlockList().push_back( while_block );
	function_context.llvm_ir_builder.SetInsertPoint( while_block );

	BuildBlockCode( *while_operator.block_, names, function_context );
	function_context.llvm_ir_builder.CreateBr( test_block );

	function_context.loops_stack.pop_back();

	// Block after while code.
	function_context.function->getBasicBlockList().push_back( block_after_while );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_while );
}

void CodeBuilder::BuildBreakOperatorCode(
	const BreakOperator& break_operator,
	FunctionContext& function_context ) noexcept
{
	if( function_context.loops_stack.empty() )
	{
		errors_.push_back( ReportBreakOutsideLoop( break_operator.file_pos_ ) );
		return;
	}
	U_ASSERT( function_context.loops_stack.back().block_for_break != nullptr );

	CallDestructorsForLoopInnerVariables( function_context );
	function_context.llvm_ir_builder.CreateBr( function_context.loops_stack.back().block_for_break );
}

void CodeBuilder::BuildContinueOperatorCode(
	const ContinueOperator& continue_operator,
	FunctionContext& function_context ) noexcept
{
	if( function_context.loops_stack.empty() )
	{
		errors_.push_back( ReportContinueOutsideLoop( continue_operator.file_pos_ ) );
		return;
	}
	U_ASSERT( function_context.loops_stack.back().block_for_continue != nullptr );

	CallDestructorsForLoopInnerVariables( function_context );
	function_context.llvm_ir_builder.CreateBr( function_context.loops_stack.back().block_for_continue );
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildIfOperatorCode(
	const IfOperator& if_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	U_ASSERT( !if_operator.branches_.empty() );

	BlockBuildInfo if_operator_blocks_build_info;
	bool have_return_in_all_branches= true;
	bool have_break_or_continue_in_all_branches= true;

	// TODO - optimize this method. Make less basic blocks.
	//

	llvm::BasicBlock* block_after_if= llvm::BasicBlock::Create( llvm_context_ );

	llvm::BasicBlock* next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
	// Break to first condition. We must push terminal instruction at end of current block.
	function_context.llvm_ir_builder.CreateBr( next_condition_block );

	for( unsigned int i= 0u; i < if_operator.branches_.size(); i++ )
	{
		const IfOperator::Branch& branch= if_operator.branches_[i];

		llvm::BasicBlock* body_block= llvm::BasicBlock::Create( llvm_context_ );
		llvm::BasicBlock* current_condition_block= next_condition_block;

		if( i + 1u < if_operator.branches_.size() )
			next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
		else
			next_condition_block= block_after_if;

		// Build condition block.
		function_context.function->getBasicBlockList().push_back( current_condition_block );
		function_context.llvm_ir_builder.SetInsertPoint( current_condition_block );

		if( branch.condition == nullptr )
		{
			U_ASSERT( i + 1u == if_operator.branches_.size() );

			// Make empty condition block - move to it unconditional break to body.
			function_context.llvm_ir_builder.CreateBr( body_block );
		}
		else
		{
			const Value condition_expression= BuildExpressionCodeAndDestroyTemporaries( *branch.condition, names, function_context );

			if( condition_expression.GetType() != bool_type_ )
			{
				errors_.push_back(
					ReportTypesMismatch(
						branch.condition->file_pos_,
						bool_type_.ToString(),
						condition_expression.GetType().ToString() ) );
				throw ProgramError();
			}

			llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( *condition_expression.GetVariable(), function_context );
			function_context.llvm_ir_builder.CreateCondBr( condition_in_register, body_block, next_condition_block );
		}

		// Make body block code.
		function_context.function->getBasicBlockList().push_back( body_block );
		function_context.llvm_ir_builder.SetInsertPoint( body_block );

		const BlockBuildInfo block_build_info=
			BuildBlockCode( *branch.block, names, function_context );

		have_return_in_all_branches= have_return_in_all_branches && block_build_info.have_unconditional_return_inside;
		have_break_or_continue_in_all_branches= have_break_or_continue_in_all_branches && block_build_info.have_uncodnitional_break_or_continue;

		function_context.llvm_ir_builder.CreateBr( block_after_if );
	}

	U_ASSERT( next_condition_block == block_after_if );

	if( if_operator.branches_.back().condition != nullptr )
	{
		have_return_in_all_branches= false;
		have_break_or_continue_in_all_branches= false;
	}

	// Block after if code.
	function_context.function->getBasicBlockList().push_back( block_after_if );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_if );

	if_operator_blocks_build_info.have_unconditional_return_inside= have_return_in_all_branches;
	if_operator_blocks_build_info.have_uncodnitional_break_or_continue= have_break_or_continue_in_all_branches;
	return if_operator_blocks_build_info;
}

void CodeBuilder::BuildStaticAssert(
	const StaticAssert& static_assert_,
	const NamesScope& names )
{
	const Value expression_result= BuildExpressionCode( *static_assert_.expression, names, *dummy_function_context_ );
	if( expression_result.GetType() != bool_type_ )
	{
		errors_.push_back( ReportStaticAssertExpressionMustHaveBoolType( static_assert_.file_pos_ ) );
		return;
	}

	const Variable* const variable= expression_result.GetVariable();
	U_ASSERT( variable != nullptr );

	if( variable->constexpr_value == nullptr )
	{
		errors_.push_back( ReportStaticAssertExpressionIsNotConstant( static_assert_.file_pos_ ) );
		return;
	}

	if( !variable->constexpr_value->isOneValue() )
	{
		errors_.push_back( ReportStaticAssertionFailed( static_assert_.file_pos_ ) );
		return;
	}
}

FunctionVariable* CodeBuilder::GetFunctionWithExactSignature(
	const Function& function_type,
	OverloadedFunctionsSet& functions_set )
{
	for( FunctionVariable& function_varaible : functions_set )
	{
		const Function& set_function_type= *function_varaible.type.GetFunctionType();

		if( set_function_type.args.size() != function_type.args.size() )
			continue;

		bool is_equal= true;
		for( size_t i= 0u; i < function_type.args.size(); ++i )
		{
			if( function_type.args[i] != set_function_type.args[i] )
			{
				is_equal= false;
				break;
			}
		}

		if( is_equal )
			return &function_varaible;
	}

	return nullptr;
}

void CodeBuilder::ApplyOverloadedFunction(
	OverloadedFunctionsSet& functions_set,
	const FunctionVariable& function,
	const FilePos& file_pos )
{
	if( functions_set.empty() )
	{
		functions_set.push_back(function);
		return;
	}

	const Function* function_type= function.type.GetFunctionType();
	U_ASSERT(function_type);

	/*
	Algorithm for overloading applying:
	If parameter count differs - overload function.
	If "ArgOverloadingClass" of one or more arguments differs - overload function.
	*/
	for( const FunctionVariable& set_function : functions_set )
	{
		const Function& set_function_type= *set_function.type.GetFunctionType(); // Must be function type 100 %

		// If argument count differs - allow overloading.
		// SPRACHE_TODO - handle default arguments.
		if( function_type->args.size() != set_function_type.args.size() )
			continue;

		unsigned int arg_is_same_count= 0u;
		for( size_t i= 0u; i < function_type->args.size(); i++ )
		{
			const Function::Arg& arg= function_type->args[i];
			const Function::Arg& set_arg= set_function_type.args[i];

			if( arg.type != set_arg.type )
				continue;

			if( GetArgOverloadingClass( arg ) == GetArgOverloadingClass( set_arg ) )
				arg_is_same_count++;
		} // For args.

		if( arg_is_same_count == function_type->args.size() )
		{
			errors_.push_back( ReportCouldNotOverloadFunction(file_pos) );
			throw ProgramError();
		}
	} // For functions in set.

	// No error - add function to set.
	functions_set.push_back(function);
}

const FunctionVariable& CodeBuilder::GetOverloadedFunction(
	const OverloadedFunctionsSet& functions_set,
	const std::vector<Function::Arg>& actual_args,
	const bool first_actual_arg_is_this,
	const FilePos& file_pos )
{
	U_ASSERT( !functions_set.empty() );
	U_ASSERT( !( first_actual_arg_is_this && actual_args.empty() ) );

	// If we have only one function - return it.
	// Caller can generate error, if arguments does not match.
	if( functions_set.size() == 1u )
		return functions_set.front();

	enum class MatchType
	{
		// Overloading class and type match for all parameters.
		Exact,
		// Exact types match for all parameters and mutable reference to immutable reference conversions for some parameters.
		MutToImutReferenceConversion,
		// Types conversion for some parameters ( including references conversion ) and possible mutable to immutable references conversion.
		TypeConversions,
		NoMatch,
	};

	// TODO - use here something like small vectors, or cache this vectors.
	std::vector<unsigned int> exact_match_functions;
	std::vector<unsigned int> match_with_mut_to_imut_cast_functions;
	std::vector<unsigned int> match_with_types_conversion_functions;

	for( const FunctionVariable& function : functions_set )
	{
		const Function& function_type= *function.type.GetFunctionType();

		size_t actial_arg_count;
		const Function::Arg* actual_args_begin;
		if( first_actual_arg_is_this && !function.is_this_call )
		{
			// In case of static function call via "this" compare actual args without "this".
			actual_args_begin= actual_args.data() + 1u;
			actial_arg_count= actual_args.size() - 1u;
		}
		else
		{
			actual_args_begin= actual_args.data();
			actial_arg_count= actual_args.size();
		}

		// SPRACHE_TODO - handle functions with default arguments.
		if( function_type.args.size() != actial_arg_count )
			continue;

		MatchType match_type= MatchType::Exact;
		for( unsigned int i= 0u; i < actial_arg_count; i++ )
		{
			// SPRACHE_TODO - support type-casting for function call.
			// SPRACHE_TODO - support references-casting.
			// Now - only exactly compare types.
			if( actual_args_begin[i].type != function_type.args[i].type )
			{
				match_type= MatchType::NoMatch;
				break;
			}

			const ArgOverloadingClass arg_overloading_class= GetArgOverloadingClass( actual_args_begin[i] );
			const ArgOverloadingClass parameter_overloading_class= GetArgOverloadingClass( function_type.args[i] );
			if( arg_overloading_class == parameter_overloading_class )
			{
				// All ok, exact match
			}
			else if( parameter_overloading_class == ArgOverloadingClass::MutalbeReference &&
				arg_overloading_class != ArgOverloadingClass::MutalbeReference )
			{
				// We can only bind nonconst-reference arg to nonconst-reference parameter.
				match_type= MatchType::NoMatch;
				break;
			}
			else if( parameter_overloading_class == ArgOverloadingClass::ImmutableReference &&
				arg_overloading_class == ArgOverloadingClass::MutalbeReference )
			{
				if( match_type == MatchType::Exact )
					match_type= MatchType::MutToImutReferenceConversion;
			}
			else
			{
				U_ASSERT(false);
			}
		} // for candidate function args.

		const unsigned int function_index= &function - functions_set.data();
		switch( match_type )
		{
		case MatchType::Exact:
			exact_match_functions.push_back( function_index );
			break;
		case MatchType::MutToImutReferenceConversion:
			match_with_mut_to_imut_cast_functions.push_back( function_index );
			break;
		case MatchType::TypeConversions:
			match_with_types_conversion_functions.push_back( function_index );
			break;
		case MatchType::NoMatch:
			break;
		};
	} // for functions

	if( !exact_match_functions.empty() )
	{
		if( exact_match_functions.size() == 1u )
			return functions_set[ exact_match_functions.front() ];
		else
		{
			errors_.push_back( ReportTooManySuitableOverloadedFunctions( file_pos ) );
			throw ProgramError();
		}
	}
	else if( !match_with_mut_to_imut_cast_functions.empty() )
	{
		if( match_with_mut_to_imut_cast_functions.size() == 1u )
			return functions_set[ match_with_mut_to_imut_cast_functions.front() ];
		else
		{
			errors_.push_back( ReportTooManySuitableOverloadedFunctions( file_pos ) );
			throw ProgramError();
		}
	}
	else if( !match_with_types_conversion_functions.empty() )
	{
		if( match_with_types_conversion_functions.size() == 1u )
			return functions_set[ match_with_types_conversion_functions.front() ];
		else
		{
			errors_.push_back( ReportTooManySuitableOverloadedFunctions( file_pos ) );
			throw ProgramError();
		}
	}
	else
	{
		// Not found any function.
		errors_.push_back( ReportCouldNotSelectOverloadedFunction( file_pos ) );
		throw ProgramError();
	}
}

U_FundamentalType CodeBuilder::GetNumericConstantType( const NumericConstant& number )
{
	if( number.type_suffix_.empty() )
	{
		if( number.has_fractional_point_ )
			return U_FundamentalType::f64;
		else
			return U_FundamentalType::i32;
	}

	// Allow simple "u" suffix for unsigned 32bit values.
	// SPRACHE_TODO - maybe add "i" suffix for i32 type?
	if( number.type_suffix_ == "u"_SpC )
		return U_FundamentalType::u32;
	// Simple "f" suffix for 32bit floats.
	else if( number.type_suffix_ == "f"_SpC )
		return U_FundamentalType::f32;

	auto it= g_types_map.find( number.type_suffix_ );
	if( it == g_types_map.end() )
		return U_FundamentalType::InvalidType;

	return it->second;
}

llvm::Type* CodeBuilder::GetFundamentalLLVMType( const U_FundamentalType fundmantal_type )
{
	switch( fundmantal_type )
	{
	case U_FundamentalType::InvalidType:
		return fundamental_llvm_types_.invalid_type_;
	case U_FundamentalType::LastType:
		break;

	case U_FundamentalType::Void:
		return fundamental_llvm_types_.void_;
	case U_FundamentalType::Bool:
		return fundamental_llvm_types_.bool_;
	case U_FundamentalType::i8 :
		return fundamental_llvm_types_.i8 ;
	case U_FundamentalType::u8 :
		return fundamental_llvm_types_.u8 ;
	case U_FundamentalType::i16:
		return fundamental_llvm_types_.i16;
	case U_FundamentalType::u16:
		return fundamental_llvm_types_.u16;
	case U_FundamentalType::i32:
		return fundamental_llvm_types_.i32;
	case U_FundamentalType::u32:
		return fundamental_llvm_types_.u32;
	case U_FundamentalType::i64:
		return fundamental_llvm_types_.i64;
	case U_FundamentalType::u64:
		return fundamental_llvm_types_.u64;
	case U_FundamentalType::f32:
		return fundamental_llvm_types_.f32;
	case U_FundamentalType::f64:
		return fundamental_llvm_types_.f64;
	};

	U_ASSERT(false);
	return nullptr;
}

llvm::Value*CodeBuilder::CreateMoveToLLVMRegisterInstruction(
	const Variable& variable, FunctionContext& function_context )
{
	// Contant values always are register-values.
	if( variable.constexpr_value != nullptr )
		return variable.constexpr_value;

	switch( variable.location )
	{
	case Variable::Location::LLVMRegister:
		return variable.llvm_value;
	case Variable::Location::Pointer:
		return function_context.llvm_ir_builder.CreateLoad( variable.llvm_value );
	};

	U_ASSERT(false);
	return nullptr;
}

} // namespace CodeBuilderLLVMPrivate

} // namespace U
