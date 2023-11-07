Enums
=====

Enum is a type which values may be equal to one of named values specifeid in enum declaration.

Enum declaration example:

.. code-block:: u_spr

   enum FixedColor
   {
       Black,
       White,
       Red,
       Green,
       Blue,
       Yellow,
       Magenta,
       Cyan,
   };

   enum Component : i16 // Underlaying type is specified
   {
       One,
       Two,
       Three,
   };

Usage:

.. code-block:: u_spr

   var FixedColor mut c= FixedColor::Black;
   c= FixedColor::Red;
   if( c == FixedColor::Red ) {}

Enum values may be assigned, compared, converted into an integer and used as template arguments.
Result of enum to integer conversion is an integer value that is equal to the number of this enum value in enum declaration.

It is possible to compare enum values via ``==``, ``!=``, ``<``, ``<=``, ``>``, ``>=``, ``<=>``.
One enum value is greater than other, if it is declared later.

It is possible to specify underlaying integer type for an enum.
The size of enum values will be equal to the size of underlaying type values.
If no underlaying type is specified it will be chosen automatically - ``u8``, ``u16`` or ``u32`` depending on the number of possible enum values.

Enums can't be empty - they should have at least one value.
An enum may have no more values than maximum value of its underlaying type.
