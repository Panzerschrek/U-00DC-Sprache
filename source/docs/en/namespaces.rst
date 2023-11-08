Namespaces
==========

Ü has a namespaces mechanism, that allows to structure a program.
Each namespace has a name and can contain definitions inside, including nested namespaces.
Namespaced are defined with usage of ``namespace`` keyword and ``{}``.
Inside ``{}`` namespace members are specified.
It's allowed to define same namespace multiple times, further defenitions will extend previous ones.
All namespaces and other things are located inside the global namespace or in one of nested namespaces.

.. code-block:: u_spr

   namespace A
   {
       class SomeClass{ i32 x; }
   
       namespace Inner // Nested namespace.
       {
           fn Bar(i32 x);
       }
   }
   
   namespace A // Extend existing namespace.
   {
       fn AFunc();
   }
   
   namespace F
   {
       class SomeClass{ f32 y; } // A class with the same name already exists, but in other namespace, which means that this is not a redefinition error.
   }
   
   // Global namespace elements.
   struct S{}
   fn Foo();

It's allowed to define things with the same name in different namespaces, including child/parent namespaces.
In the last case child namespace names will shadow same names in the parent namespace.

*************
*Name lookup*
*************

When encounting a name the compiler performs name lookup.
Lookup starts in current namespace and continues to its ancestors until target name will be found.

.. code-block:: u_spr

   namespace A
   {
       namespace B
       {
           struct S{}
           namespace C
           {
                fn Foo()
                {
                    var S s; // A::B::S will be found
                }
            }
        }
   }


It is possible to perform lookup from specific namespace by specifying partial name address with ``::`` separator.
In this case namespace lookup happens first (using rules describing above) and than specific namespace lookup is performed.
For global namespace specifying a name should start with ``::``.

.. code-block:: u_spr

   namespace A
   {
       struct S{}
       namespace B
       {
           struct S{}
           namespace C
           {
                fn Foo()
                {
                    var A::S s0; // A::S will be found
                    var B::S s1; // A::B::S will be found
                    var A::B::S s2; // A::B::S will be found
                    var ::A::S s3; // A::S will be found
                    var ::A::B::S s4; //  A::B::S will be found
                }
            }
        }
   }

****************************
*Additional namespace kinds*
****************************

Structs, classes, enums are also namespaces.
It's possible to access their members with ``::``.

.. code-block:: u_spr
   
   struct S
   {
       type Int= i32;
   }
   
   class C
   {
       fn Bar();
   
       struct F
       {
           type FT= f64;
       }
   }
   
   enum E
   {
       R,
       G,
       B,
   }
   
   fn Foo()
   {
       var S::Int i= 0; // Access to an type alias which is a struct member.
       C::Bar(); // Access to a function which is a class member.
       var C::F::FT f= 0.0; // Access to a type alias which is a member of struct inside a class.
       var E e= E::G; // Access to an enum element.
   }
