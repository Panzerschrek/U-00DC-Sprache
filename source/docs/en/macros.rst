Macros
======

Ü has a mechanism for program syntax extention via user-defined constructions.
Such mechanism is implemented via macros.

A programmer may define a macro that starts with some name.
When encounting this name syntax analyzer of the compiler will apply described in the macro syntax rules, perform parsing and expand the result.

Simpliest example:

.. code-block:: u_spr

   ?macro <? Double:expr ( ?e:expr ) ?>  ->  <? ?e * 2 ?>
   var i32 x= Double(10);

In the example abouve a macro named ``Double`` is defined, which contains from an expression in ``()``.
This macro expands itno an expression multiplied by 2.

*******************
*Macros definition*
*******************

Macros are defined at the beginning of a source file - after imports list but before other code.
Macros are imported from imported files.
Macros are global - are available averywhere and are located in the single scope (with context separation).

Also, what are macros made of?
A macro definition starts with ``?macro`` identifier with following pair of brackets ``<?`` and ``?>``.
The first element inside these brackets should be a macro name and its context after ``:``.
Further follows a sequence of match rules.
Than follows a macro expansion block in ``<?`` ``?>`` brackets, that follows after ``->`` lexem.

**********************
*Macro usage contexts*
**********************

Each macro has a context where it is used.
There are following contexts:

* ``namespace`` - context inside a namespace (including root namespace).
* ``class`` - context inside a struct or class.
* ``block`` - context inside a block.
* ``expr`` - context inside an expression.

All contexts are independent on each other.
It's possible to define macros with the same name in different contexts.
But for the same context it's not allowed to define more than one macro with the same name.

**********************
*Match block elements*
**********************

The simpliest match elements are regular lexems.
This includes identifiers, brackets, operators symbols, etc.
They are matched directly.

Other elements are named.
They begin with ```?``` symbol and an identifier.
Than follows ``:``, after which follows element class.
Inside single match block it's not allowed to define elements with the same name.
But it's allowed to use same names for elements of different (even nested) match blocks.

Elements may be simple and composite.
Simple elements are:

* ``ident`` - single identifier.
* ``ty`` - type name.
* ``expr`` - expression.
* ``block`` - block.

Composite elements are optionals and sequences.

Optional elements are of class ``opt``.
After optional element definition follows a sequence of its nested elements in ``<?`` ``?>`` brackets.

Sequence elements are of class ``rep``.
After sequential element definition follows a sequence of its nested elements in ``<?`` ``?>`` brackets.
Optionally a separator lexem may be specified - a single lexem in ``<?`` ``?>`` brackets.

Optionals and sequences require fixed lexem at the beginning of its elements sequence or a fixed lexem after optional/sequence.
It is needed in order to know what to do during parsing - parse optional/sequence or parse an element after it.
The fixed lexem of the beginning of an optional/sequence element must be different from the fixed lexem after optional/sequence.

Elements example:

.. code-block:: u_spr

   ?macro <? Skip:expr ?some_ident:ident ?>  ->  <? ?> // identifier
   ?macro <? Skip:namespace ?some_type:ty ?>  ->  <? ?> // type name
   ?macro <? Skip:expr ?some_expression:expr ?>  ->  <? ?> // expression
   ?macro <? Skip:block ?some_block:block ?>  ->  <? ?> // block
   ?macro <? Skip:expr ?some_opt:opt<? ?i:ident ?> ?>  ->  <? ?> // optional identifier
   ?macro <? Skip:expr ?some_opt:opt<? ( ?b:block ) ?> ?>  ->  <? ?> // optional block in round brackets
   ?macro <? Skip:expr ( ?some_sequence:rep<? ?e:expr ?> ) ?>  ->  <? ?> // sequence in round brackets
   ?macro <? Skip:expr START ?some_sequence:rep<? ?e:expr ?><?,?> END ?>  ->  <? ?> // sequence with 0 or many comma-separated expressions between START/END words

******************
*Macros expansion*
******************

Macros are expanded according to the rules specified inside macro expansion block.

Fixed lexems are allowed inside it, they will be expanded directly.

Also a macro expansion block may contain macro-variabled defined previously in the match block.
They begin with ``?`` symbol.
In a macro expansion they are replaced with passed macro-arguments.

Simple elements are expanded directly.

Composite elements are expanded recursively.
It's necessary to specify a macro block in ``<?`` ``?>`` brackets for them, where inner elements expansion rules are specified.

Macro block of an optional element is expanded if optional element was passed.

Macro block of a sequence element is expanded so many times as sequence elements were passed.
In each expansion variables of corresponding input elements are used.

Expansion examples:

.. code-block:: u_spr

   ?macro <? DECLARE_VAR:namespace ?name:ident ?init:expr ?t:ty ?>  ->  <? var ?t ?name = ?init; ?>

   DECLARE_VAR pi 3.14f f32
   // Will expand into
   var i32 pi = 3.14f;

.. code-block:: u_spr

   ?macro <? DECLARE_VAR:namespace ?name:ident ?init:expr ?t:ty ?m:opt<?MUT?> ?>  ->  <? var ?t ?m<?mut?> ?name = ?init; ?>

   DECLARE_VAR pi 3.14f f32
   // Will expand into
   var i32 f32 = 3.14f;

   DECLARE_VAR x 0 i32 MUT
   // Will expand into
   var i32 mut pi = 0;

.. code-block:: u_spr

   ?macro <? DECLARE_FOO:namespace ( ?params:rep<? ?t:ty ?name:ident ?><?,?> ) ?>  ->  <? fn Foo( ?params<? ?t ?name ?><?,?> ); ?>

   DECLARE_FOO()
   // Will expand into
   fn Foo()

   DECLARE_FOO(i32 x, f32 y)
   // Will expand into
   fn Foo(i32 x, f32 y)

**************************
*Unique macro identifiers*
**************************

It's possible to specify unique macro-identifiers in a macro expansion block.
These identifiers start with ``??``.
They are replaced with unique for this macro expansion identifiers, that are guaranteed to be unique compared to any other identifiers (including other macro unique identifiers expansions).

Such unique macro identifiers allow to perform macro expansion without any possile name collisions against names defined somewhere else.

Example:

.. code-block:: u_spr

   ?macro <? FOR:block ?count:expr ?b:block ?>  ->
   <?
   {
           var size_type mut ??counter= 0s;
           while( ??counter < size_type(?count) )
           {
               ?b
               ++??counter;
           }
   }
   ?>

   fn Foo();

   fn Bar()
   {
       FOR 32
       {
           var i32 counter= 0;
           Foo();
       }
       // This macro will be expanded into something like this:
       // var size_type mut ??counter= 0s;
       // while( _macro_ident_counter_140734899778672_0 < size_type(32) )
       // {
       //     {
       //         var i32 counter= 0; // There is no name collision here
       //         Foo();
       //     }
       //     ++_macro_ident_counter_140734899778672_0;
       // }
   }
