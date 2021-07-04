#pragma once
#include "../lex_synt_lib/program_string.hpp"
#include "template_types.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

class Class final
{
public:
	static const std::string c_template_class_name; // Each template class have same name.

public:
	Class( std::string name, NamesScope* parent_scope );

	Class( const Class& )= delete;
	Class& operator=( const Class& )= delete;

	ClassMemberVisibility GetMemberVisibility( const std::string& member_name ) const;
	void SetMemberVisibility( const std::string& member_name, ClassMemberVisibility visibility );

	bool HaveAncestor( const ClassProxyPtr& class_ ) const;

public:
	struct BaseTemplate
	{
		TypeTemplatePtr class_template;
		TemplateArgs signature_args;
	};

	enum class Kind
	{
		Struct,
		NonPolymorph,
		Interface,
		Abstract,
		PolymorphNonFinal,
		PolymorphFinal,
	};

	struct VirtualTableEntry
	{
		std::string name;
		FunctionVariable function_variable;
		bool is_pure= false;
		bool is_final= false;

		// Virtual table may consist of many nested structs, because virtual table of child contains wirtual tables of parents.
		uint32_t index_in_table = ~0u;
		uint32_t parent_virtual_table_index= ~0u;
	};

public:
	// If you change this, you must change CodeBuilder::CopyClass too!

	NamesScopePtr members;

	// have no visibility for member, means it is public.
	ProgramStringMap< ClassMemberVisibility > members_visibility;

	const Synt::Class* syntax_element= nullptr;

	size_t field_count= 0u;
	InnerReferenceType inner_reference_type= InnerReferenceType::None;
	bool is_complete= false;
	bool have_explicit_noncopy_constructors= false;
	bool is_default_constructible= false;
	bool is_copy_constructible= false;
	bool have_destructor= false;
	bool is_copy_assignable= false;
	bool can_be_constexpr= false;
	bool have_shared_state= false;

	SrcLoc forward_declaration_src_loc;
	SrcLoc body_src_loc;

	llvm::StructType* llvm_type;

	// Names of this class fields in order of field number. Empty string for parent classes fields.
	std::vector<std::string> fields_order;

	// Exists only for classes, generated from class templates.
	std::optional<BaseTemplate> base_template;

	// If this class is typeinfo, contains source type.
	std::optional<Type> typeinfo_type;

	Kind kind= Kind::Struct;

	struct Parent
	{
		ClassProxyPtr class_;
		unsigned int field_number= ~0u; // Allways 0 for base class.
	};
	ClassProxyPtr base_class;
	std::vector<Parent> parents; // Parents, include base class.

	std::vector<VirtualTableEntry> virtual_table;
	llvm::StructType* virtual_table_llvm_type= nullptr;
	llvm::GlobalVariable* virtual_table_llvm_variable= nullptr; // May be null for interfaces and abstract classes.
	llvm::GlobalVariable* polymorph_type_id= nullptr; // Exists in polymorph classes.
};

} //namespace CodeBuilderPrivate

} // namespace U
