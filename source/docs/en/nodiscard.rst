nodiscard
=========

Structs and classes may be marked with ``nodiscard`` flag.
A value with type having this flag isn't allowed to ignore, it should be used - passed somewhere or used for a variable initialization.

.. code-block:: u_spr

   struct SomeStruct nodiscard { i32 x; }
   fn Bar() : SomeStruct;
   fn Foo()
   {
       Bar(); // Error - a value of "nodiscard" type is ignored.
   }

For classes ``nodiscard`` flag is specified after possible class kind and list of parent classes:

.. code-block:: u_spr

   class SomeInterface interface {}
   class SomeClass final : SomeInterface nodiscard {}

Also enums may be marked as ``nodiscard``:

.. code-block:: u_spr

   enum Ampel nodiscard { Rot, Gelb, Gruen }
   enum Richtungen : u32 nodiscard { Nord, Sued, Ost, West }
