Информация о типе
=================

В Ü есть возможность статически получить различную информацию о типе.

Для получения информации о типе существует специальный оператор - ``typeinfo``.
Данному оператору передаётся имя типа в ``<//>`` скобках. Результат данного оператора - ссылка на ``constexpr`` структуру, содержащую информацию о типе.


.. code-block:: u_spr

   fn Foo()
   {
       auto &constexpr int_info = typeinfo</i32/>;
   }

**************************
*Состав информации о типе*
**************************

Структура с информацией о любом типе содержит следующие поля:

* ``size_type size_of`` - размер типа, в байтах
* ``size_type align_of`` - выравнивание типа, в байтах
* ``bool is_fundamental`` - является ли тип фундаментальным
* ``bool is_enum`` - является ли тип перечислением
* ``bool is_array`` - является ли тип массивом
* ``bool is_tuple`` - является ли тип кортежем
* ``bool is_raw_pointer`` - является ли тип сырым указателем
* ``bool is_class`` - является ли тип структурой или классом
* ``bool is_function_pointer`` - является ли тип указателем на функцию
* ``size_type reference_tag_count`` - количество внутренних ссылочных тегов
* ``bool is_default_constructible`` - является ли тип конструируемым по умолчанию
* ``bool is_copy_constructible`` - можно ли сконструировать значение типа как копию
* ``bool is_copy_assignable`` - можно ли копировать значения типа присваиванием
* ``bool is_equality_comparable`` - можно ли сравнивать значения типа на равенство

.. code-block:: u_spr

   static_assert( typeinfo</ char8 />.size_of == 1s );
   static_assert( typeinfo</ f64 />.align_of == 8s );
   static_assert( typeinfo</i32/>.is_fundamental );
   static_assert( typeinfo</ [f64, 4] />.is_array );
   static_assert( typeinfo</ tup[i32, bool] />.is_tuple );
   static_assert( typeinfo</ fn() />.is_function_pointer );
   struct S{}
   static_assert( typeinfo</ S />.is_class );

Кроме этих полей, есть ряд полей, зависящих от вида типа.

******************************************
*Состав информации о фундаментальном типе*
******************************************

* ``bool is_integer`` - является ли тип целочисленным
* ``bool is_numeric`` - является ли тип числовым
* ``bool is_signed_integer`` - является ли тип целочисленным знаковым
* ``bool is_unsigned_integer`` - является ли тип целочисленным беззнаковым
* ``bool is_float`` - является ли тип вещественным
* ``bool is_char`` - является ли тип символьным
* ``bool is_bool`` - этот тип - ``bool``
* ``bool is_void`` - этот тип - ``void``

.. code-block:: u_spr

   static_assert( typeinfo</i16/>.is_integer );
   static_assert( typeinfo</u64/>.is_numeric );
   static_assert( typeinfo</i32/>.is_signed_integer );
   static_assert( typeinfo</u32/>.is_unsigned_integer );
   static_assert( typeinfo</f32/>.is_float );
   static_assert( typeinfo</char16/>.is_char );
   static_assert( typeinfo</bool/>.is_bool );
   static_assert( typeinfo</void/>.is_void );

**********************************
*Состав информации о перечислении*
**********************************

* ``size_type element_count`` - количество элементов в перечислении
* ``typeinfo & underlying_type`` - информация о типе, на котором основано перечисление
* ``tup[] elements_list`` - кортеж, каждый элемент которого описывает элемент перечисления

Описание каждого элемента перечисления содержит:

* ``[char8, size]& name`` - имя элемента
* ``underlying_type value`` - значение элемента

.. code-block:: u_spr

   enum E : u8 { A, B, C }
   auto &info = typeinfo</E/>;
   static_assert( info.element_count == 3s );
   static_assert( info.underlying_type.is_unsigned_integer );
   static_assert( info.underlying_type.size_of == 1s );
   static_assert( info.elements_list[0].value == 0u8 );
   static_assert( info.elements_list[1].value == 1u8 );
   static_assert( info.elements_list[2].value == 2u8 );
   static_assert( info.elements_list[0].name[0] == 'A' );
   static_assert( info.elements_list[1].name[0] == 'B' );
   static_assert( info.elements_list[2].name[0] == 'C' );

*****************************
*Состав информации о массиве*
*****************************

* ``size_type element_count`` - количество элементов в массиве
* ``typeinfo & element_type`` - информация о типе элемента массива

.. code-block:: u_spr

   static_assert( typeinfo</ [ i32, 7 ] />.element_count == 7s );
   static_assert( typeinfo</ [ f64, 1 ] />.element_type.is_float );

*****************************
*Состав информации о кортеже*
*****************************

* ``size_type element_count`` - количество элементов в кортеже
* ``tup[] elements_list`` - кортеж, каждый элемент которого описывает элемент кортежа

Описание каждого элемента кортежа содержит:

* ``typeinfo & type`` - описание типа элемента кортежа
* ``size_type index`` - порядковый номер элемента кортежа
* ``size_type offset`` - смещение, в байтах, адреса элемента кортежа относительно начала кортежа

.. code-block:: u_spr

   static_assert( typeinfo</ tup[] />.element_count == 0s );
   static_assert( typeinfo</ tup[ f32, i32 ] />.element_count == 2s );
   static_assert( typeinfo</ tup[ f32, bool, i32 ] />.elements_list[1].type.is_bool );
   static_assert( typeinfo</ tup[ f64 ] />.elements_list[0].type.size_of == 8s );
   static_assert( typeinfo</ tup[ i32, bool ] />.elements_list[1].offset == 4s );
   static_assert( typeinfo</ tup[ i16, i16, i16, bool ] />.elements_list[3].index == 3s );

**************************************
*Состав информации о структуре/классе*
**************************************

* ``size_type field_count`` - количество полей
* ``size_type parent_count`` - количество родительских классов
* ``bool is_struct`` - является ли тип структурой
* ``bool is_polymorph`` - является ли тип полиморфным классом
* ``bool is_final`` - является ли тип конечным (от которого нельзя унаследоваться)
* ``bool is_abstract`` - является ли тип абстрактным (значение которого нельзя сконструировать)
* ``bool is_interface`` - является ли тип интерфейсом
* ``bool is_typeinfo`` - является ли тип ``typeinfo`` или какой-то его частью
* ``bool is_coroutine`` - является ли класс типом корутины
* ``tup[] fields_list`` - кортеж, каждый элемент которого описывает поле структуры/класса
* ``tup[] types_list`` - кортеж, каждый элемент которого описывает вложенный в структуру/класс тип
* ``tup[] functions_list`` - кортеж, каждый элемент которого описывает функцию структура/класса
* ``tup[] parents_list`` - кортеж, каждый элемент которого описывает родительский класс

Описание каждого поля, вложенного типа, функции содержит:

* ``[char8, size]& name`` - имя поля, типа (как он объявлен в структуре/классе), функции
* ``bool is_public`` - является ли член класса ``public``
* ``bool is_private`` - является ли член класса ``private``
* ``bool is_protected`` - является ли член класса ``protected``

Описание каждого поля содержит:

* ``typeinfo & type`` - описание типа поля
* ``typeinfo & class_type`` - описание типа класса, которому принадлежит поле
* ``size_type offset`` - смещение, в байтах, адреса поля относительно начала структуры/класса
* ``bool is_reference`` - является ли поле ссылочным
* ``bool is_mutable`` - является ли поле изменяемым

Описание каждого вложенного типа содержит:

* ``typeinfo & type`` - описание типа

Описание каждой функции содержит:

* ``typeinfo & type`` - описание типа функции
* ``bool is_this_call`` - первый аргумент функции - ``this``
* ``bool is_generated`` - функция сгенерирована компилятором
* ``bool is_deleted`` - функция объявлена как удалённая (``= delete``)
* ``bool is_virtual`` - функция является виртуальным методом

Описание каждого родительского класса содержит:

* ``typeinfo & type`` - описание типа родительского класса
* ``size_type offset`` - смещение, в байтах, адреса родителя относительно начала структуры/класса

Информация о типе для полиморфных классов имеет также поле ``size_type& type_id``. См. "type_id".

Информация о типе для корутин также содержит поля:

* ``is_generator`` - является ли тип корутины генератором
* ``typeinfo & coroutine_return_type`` - тип возвращаемого значения корутины
* ``bool coroutine_return_value_is_reference`` - возвращает ли корутина ссылочное значение
* ``bool coroutine_return_value_is_mutable`` - возвращает ли корутина изменяемое значение

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

*******************************************
*Состав информации об указателе на функцию*
*******************************************

* ``typeinfo & return_type`` - описание типа возвращаемого значения
* ``bool return_value_is_reference`` - является ли возвращаемое значение ссылкой
* ``bool return_value_is_mutable`` - является ли возвращаемое значение изменяемым
* ``bool unsafe`` - помечена ли функция как ``unsafe``
* ``tup[] params_list`` - кортеж, каждый элемент которого описывает параметр функции
* ``return_references`` - массив с описанием возвращаемых ссылок (как в нотации с ``@``)
* ``return_inner_references`` - кортеж с описанием внутренних возвращаемых ссылок (как в нотации с ``@``)
* ``references_pollution`` - массив с описанием связывания ссылок (как в нотации с ``@``)

Описание каждого аргумента содержит:

* ``typeinfo & type`` - описание типа аргумента
* ``bool is_reference`` - является ли аргумент ссылкой
* ``bool is_mutable`` - является ли аргумент изменяемым

.. code-block:: u_spr

   type fn_ptr= fn( i32 x, f32& y, bool &mut z ) : i32;
   auto& info = typeinfo</fn_ptr/>;
   static_assert( info.return_type.is_signed_integer );
   static_assert( info.return_type.size_of == 4s );
   static_assert( !info.unsafe );
   static_assert( info.arguments_list[1].type.is_float );
   static_assert( info.arguments_list[1].is_reference );
   static_assert( info.arguments_list[2].is_mutable );


*************************************
*Состав информации о сыром указателе*
*************************************

* ``typeinfo & element_type`` - информация о типе, на которую указывает указатель
