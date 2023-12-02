Type information
================

There is a possibility in Ü to obtain in compile-type various information about a type.

Special operator ``typeinfo`` exists for obtaining of type information.
Type name is specified in ``<//>``.
The result of this operator is a reference to a constant structure that contains information about the given type.

.. code-block:: u_spr

   fn Foo()
   {
       auto &constexpr int_info = typeinfo</i32/>;
   }

***************************
*Type information contents*
***************************

Typeinfo for each type has following fields:

* ``size_type size_of`` - type size in bytes
* ``size_type align_of`` - type alignment in bytes
* ``bool is_fundamental`` - is a type fundamental
* ``bool is_enum`` - is a type an enum
* ``bool is_array`` - is a type an array
* ``bool is_tuple`` - is a type a tuple
* ``bool is_raw_pointer`` - is a type a raw pointer
* ``bool is_class`` - is a type a struct or class
* ``bool is_function_pointer`` - is a type a function pointer
* ``size_type references_tags_count`` - number of inner reference tags
* ``bool is_default_constructible`` - is a type default-constructible
* ``bool is_copy_constructible`` - is a type copy-constructible
* ``bool is_copy_assignable`` - is a type copy-assignable
* ``bool is_equality_comparable`` - is a type equality-comparable

.. code-block:: u_spr

   static_assert( typeinfo</ char8 />.size_of == 1s );
   static_assert( typeinfo</ f64 />.align_of == 8s );
   static_assert( typeinfo</i32/>.is_fundamental );
   static_assert( typeinfo</ [f64, 4] />.is_array );
   static_assert( typeinfo</ tup[i32, bool] />.is_tuple );
   static_assert( typeinfo</ fn() />.is_function_pointer );
   struct S{}
   static_assert( typeinfo</ S />.is_class );

There are also additional fields depending on the type kind.

****************************************
*Type information for fundamental types*
****************************************

* ``bool is_integer`` - is a type integer
* ``bool is_numeric`` - is a type numeric
* ``bool is_signed_integer`` - is a type signed integer
* ``bool is_unsigned_integer`` - is a type unsigned integer
* ``bool is_float`` - is a type floating point
* ``bool is_char`` - is a type char
* ``bool is_bool`` - this type is ``bool``
* ``bool is_void`` - this type is ``void``

.. code-block:: u_spr

   static_assert( typeinfo</i16/>.is_integer );
   static_assert( typeinfo</u64/>.is_numeric );
   static_assert( typeinfo</i32/>.is_signed_integer );
   static_assert( typeinfo</u32/>.is_unsigned_integer );
   static_assert( typeinfo</f32/>.is_float );
   static_assert( typeinfo</char16/>.is_char );
   static_assert( typeinfo</bool/>.is_bool );
   static_assert( typeinfo</void/>.is_void );

****************************
*Type information for enums*
****************************

* ``size_type element_count`` - number of members in an enum
* ``typeinfo & underlying_type`` - type information for enum underlying type
* ``tup[] elements_list`` - a tuple, each element of which contains information about an enum member

Each enum member description contains:

* ``[char8, size]& name`` - member name
* ``underlying_type value`` - member value

.. code-block:: u_spr

   enum E : u8 { A, B, C }
   auto &info = typeinfo</E/>;
   static_assert( info.element_count == 3s );
   static_assert( info.underlying_type.is_unsigned_integer );
   static_assert( info.underlying_type.size_of == 1s );
   static_assert( info.elements_list[0].value == 0u8 );
   static_assert( info.elements_list[1].value == 1u8 );
   static_assert( info.elements_list[2].value == 2u8 );
   static_assert( info.elements_list[0].name[0] == "A"c8 );
   static_assert( info.elements_list[1].name[0] == "B"c8 );
   static_assert( info.elements_list[2].name[0] == "C"c8 );

*****************************
*Type information for arrays*
*****************************

* ``size_type element_count`` - number of elements in an array
* ``typeinfo & element_type`` - type information for array element type

.. code-block:: u_spr

   static_assert( typeinfo</ [ i32, 7 ] />.element_count == 7s );
   static_assert( typeinfo</ [ f64, 1 ] />.element_type.is_float );

*****************************
*Type information for tuples*
*****************************

* ``size_type element_count`` - number of elements in a tuple
* ``tup[] elements_list`` - a tuple, each element of which contains information about a tuple element

Each tuple element description contains:

* ``typeinfo & type`` - type information for element type
* ``size_type index`` - index of this element in the tuple
* ``size_type offset`` - offset in bytes of address of this element relative to address of the tuple

.. code-block:: u_spr

   static_assert( typeinfo</ tup[] />.element_count == 0s );
   static_assert( typeinfo</ tup[ f32, i32 ] />.element_count == 2s );
   static_assert( typeinfo</ tup[ f32, bool, i32 ] />.elements_list[1].type.is_bool );
   static_assert( typeinfo</ tup[ f64 ] />.elements_list[0].type.size_of == 8s );
   static_assert( typeinfo</ tup[ i32, bool ] />.elements_list[1].offset == 4s );
   static_assert( typeinfo</ tup[ i16, i16, i16, bool ] />.elements_list[3].index == 3s );

**************************************
*Type information for structs/classes*
**************************************

* ``size_type field_count`` - number of fields
* ``size_type parent_count`` - number of parents
* ``bool is_struct`` - is a type a struct
* ``bool is_polymorph`` - is a type polymorph
* ``bool is_final`` - is a type final (from which it's not possible to inherit)
* ``bool is_abstract`` - is a type abstract (values of this type can't be constructed)
* ``bool is_interface`` - is a type an interface
* ``bool is_typeinfo`` - is a type a ``typeinfo`` struct or its part
* ``bool is_coroutine`` - is a type a coroutine type
* ``tup[] fields_list`` - a tuple, each element of which contains information about a field of the struct or class
* ``tup[] types_list`` - a tuple, each element of which contains information about a nested type of the struct or class
* ``tup[] functions_list`` - a tuple, each element of which contains information about a struct or class function
* ``tup[] parents_list`` - a tuple, each element of which contains information about a parent of the class

Each field, nested type, function description contains:

* ``[char8, size]& name`` - a name of a member
* ``bool is_public`` - is a member ``public``
* ``bool is_private`` - is a member ``private``
* ``bool is_protected`` - is a member ``protected``

Each field description contains:

* ``typeinfo & type`` - type information for field type
* ``typeinfo & class_type`` - type information for struct or class in which this field is located
* ``size_type offset`` - offset in bytes of address of this field relative to address of the struct or class
* ``bool is_reference`` - is a field reference
* ``bool is_mutable`` - is a field mutable

Each nested type description contains:

* ``typeinfo & type`` - type information

Each function description contains:

* ``typeinfo & type`` - function type description
* ``bool is_this_call`` - is first parameter ``this``
* ``bool is_generated`` - is function generated by the compiler
* ``bool is_deleted`` - is function marked as (``= delete``)
* ``bool is_virtual`` - is this a virtual method

Each parent class description contains:

* ``typeinfo & type`` - type information for parent class
* ``size_type offset`` - offset in bytes of address of this parent relative to address of the class

Type information for polymorph classes contains also field ``size_type& type_id``. See. "type_id".

Type information for  coroutines also contains following fields:

* ``is_generator`` - is this a generator coroutine
* ``typeinfo & coroutine_return_type`` - return type of a coroutine
* ``bool coroutine_return_value_is_reference`` - does coroutine return reference
* ``bool coroutine_return_value_is_mutable`` - does coroutine return mutable value

.. code-block:: u_spr

   struct S{ i32 a; f32 b; bool c; }
   class I interface {}
   class A abstract {}
   class NP {}
   class PNF : I {}
   class PF final : I {}
   
   static_assert( typeinfo</S/>.is_struct );
   static_assert( typeinfo</S/>.is_final );
   static_assert( typeinfo</I/>.is_polymorph );
   static_assert( typeinfo</I/>.is_abstract );
   static_assert( typeinfo</I/>.is_interface );
   static_assert( typeinfo</A/>.is_polymorph );
   static_assert( typeinfo</A/>.is_abstract );
   static_assert( typeinfo</NP/>.is_final );
   static_assert( typeinfo</PNF/>.is_polymorph );
   static_assert( typeinfo</PF/>.is_polymorph );
   static_assert( typeinfo</PF/>.is_final );
   static_assert( typeinfo</S/>.parent_count == 0s );
   static_assert( typeinfo</PNF/>.parent_count == 1s );
   static_assert( typeinfo</S/>.field_count == 3s );

****************************************
*Type information for function pointers*
****************************************

* ``typeinfo & return_type`` - type information for return type
* ``bool return_value_is_reference`` -  does function return reference
* ``bool return_value_is_mutable`` - does function return mutable value
* ``bool unsafe`` - is function marked as ``unsafe``
* ``tup[] arguments_list`` - a tuple, each element of which contains information about a function parameter

Each parameter description contains:

* ``typeinfo & type`` -  type information for parameter type
* ``bool is_reference`` - is this parameter reference
* ``bool is_mutable`` - is this parameter mutable

.. code-block:: u_spr

   type fn_ptr= fn( i32 x, f32& y, bool &mut z ) : i32;
   auto& info = typeinfo</fn_ptr/>;
   static_assert( info.return_type.is_signed_integer );
   static_assert( info.return_type.size_of == 4s );
   static_assert( !info.unsafe );
   static_assert( info.arguments_list[1].type.is_float );
   static_assert( info.arguments_list[1].is_reference );
   static_assert( info.arguments_list[2].is_mutable );


***********************************
*Type information for raw pointers*
***********************************

* ``typeinfo & element_type`` - type information for pointer element type
