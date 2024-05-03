Mixins
======

Mixin is a special program element, which consists of ``mixin`` keyword and an expression in ``()``.
This expression should be a ``constexpr`` array of ``char8`` elements, which contains a text of a program fragment.
This text is compiled like any other source file text.

``mixin`` code fragment text may be generated during compilation via ``constexpr`` calculations, including ``constexpr`` functions.
This allows, for example, to create a code, which isn't possible to create using :doc:`macros<macros>` or :doc:`templates<type_templates>`.


**********************
*mixin usage contexts*
**********************

``mixin`` may be used in namespaces:


.. code-block:: u_spr

   mixin( "fn Foo() : i32 { return 52; }" ); // create a global function Foo

   namespace Some
   {
       mixin( "fn Bar() : f32 { return 0.0f; }" ); // create function Some::Bar
   }


It's also possible to use ``mixin`` in structs and classes:

.. code-block:: u_spr

   struct S
   {
       // Create a couple of fields and a method declaration
       mixin( "i32 x; f64 y; fn Foo(this) : u32;" );
   }
   fn Foo()
   {
       var S s{ .x= 67, .y= 0.124 };
       s.Foo();
   }


********************
*mixins limitations*
********************

``import`` inside ``mixin`` isn't possible.

Macros declaration in ``mixin`` isn't possible.
But it's allowed to use macros available in the file, where this ``mixin`` is declared.

It's allowed to declare a ``mixin`` in another ``mixin`` expansion result.
But expansion depth is limited and thus recursive ``mixin`` expansions aren't possible.

All mixins outside templates and all mixins inside single template class are expanded in two steps.
First all ``mixin`` expressions are evaluated, than symbols from these mixins are added.
This makes impossible to use in expression of one ``mixin`` symbols defined in expansion of another ``mixin``.
