#pragma once
#include "../lex_synt_lib/program_string.hpp"
#include "names_scope.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

using TemplateParameter= std::variant< Variable, Type >;

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

public:
	struct BaseTemplate
	{
		TypeTemplatePtr class_template;
		std::vector<TemplateParameter> template_parameters;
		std::vector<TemplateParameter> signature_parameters;
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
	};

public:
	// If you change this, you must change CodeBuilder::CopyClass too!

	NamesScope members;

	// have no visibility for member, means it is public.
	ProgramStringMap< ClassMemberVisibility > members_visibility;

	const Synt::Class* syntax_element= nullptr;

	size_t field_count= 0u;
	size_t references_tags_count= 0u;
	TypeCompleteness completeness= TypeCompleteness::Incomplete;
	bool have_explicit_noncopy_constructors= false;
	bool is_default_constructible= false;
	bool is_copy_constructible= false;
	bool have_destructor= false;
	bool is_copy_assignable= false;
	bool can_be_constexpr= false;
	bool have_shared_state= false;

	FilePos forward_declaration_file_pos= FilePos{ 0u, 0u, 0u };
	FilePos body_file_pos= FilePos{ 0u, 0u, 0u };

	llvm::StructType* llvm_type;

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
	llvm::GlobalVariable* this_class_virtual_table= nullptr; // May be null for interfaces and abstract classes.
	llvm::GlobalVariable* polymorph_type_id= nullptr; // Exists in polymorph classes.

	// Key - sequence of classes from child to parent. This class not included.
	// Virtual table destination is lats key element.
	std::map< std::vector<ClassProxyPtr>, llvm::GlobalVariable* > ancestors_virtual_tables;
};

} //namespace CodeBuilderPrivate

} // namespace U
