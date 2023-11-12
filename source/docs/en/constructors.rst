Constructors
============

A constructor is a special method that is automatically called during a struct or class instance creation.
It's required in order to initialize fields and perform any other operations that are required for proper initialization.

A constructor is a method with special name ``constructor``.
It's possible to avoid specifying of ``this`` parameter, in such case implicit ``this`` will be created.
``this`` in a constructor can't be declared as ``byval``.
It's possible to overload a constructor - like any other function.

A constructor may have special section inside it - fields initialization list.
This list is placed before constructor body in ``{}``, consists of ``()`` and field names with initializers separated by comma.
Struct or class fields may be initialized inside this list.
Fields without default initializer, ``imut`` fields, reference fields may be explicitly initialized in this list.

A constructor is called not like any other method, but with usage of ``()`` initializer.
A constructor with no parameters (default-constructor) may be implicitly called.
Directly accessing a constructor is allowed only inside ``unsafe`` code.

Constructor usage example:

.. code-block:: u_spr

   struct Vec
   {
       i32 x;
       i32 y;

       fn constructor()
       ( x= 0, y= 0 )
       {}

       fn constructor( i32 in_x, i32 in_y )
       ( x= in_x, y= in_y )
       {}
   }
   
   fn Foo()
   {
       var Vec v0(); // Explicitly call default-constructor
       var Vec v1; // Implicitly call default-constructor
       var Vec v2( 5, -3 ); // Call the constructor with two arguments
   }

**********************
*Constructors effects*
**********************

A constructor with single immutable reference parameter with type equal to ``this`` type is considered to be a copy constructor.
This constructor may be explicitly called by the compiler in places where copying of a value is required.

A struct, that has explicit constructor (except copy-constructor) can't be initialized with member-by-member initializer.
Initialization of this struct is possible only via some constructor.

*************************
*Constructors generation*
*************************

The compiler can generate default constructor for a struct or class if this constructor is not defined explicitly.
But for such auto-generation all fields must be default-constructible and there must be no reference fields.

The compiler can generate copy constructor for a struct, but only if this constructor is not defined explicitly.
But for such auto-generation all fields (except reference fields) must be copy-constructible.

.. code-block:: u_spr

   struct Vec
   {
       i32 x= 0;
       i32 y= 0;
   }
   
   fn Foo()
   {
       var Vec v0; // Auto-generated default-constructor will be called
       var Vec v1(v0), v2= v0; // In both cases auto-generated copy-constructor will be called
   }

************************
*Conversion constructor*
************************

There is a special constructor kind - conversion constructor.
It exists in order to allow implicit conversions from some value to a value of a struct/class.
This constructor is defined via ``conversion_constructor`` keyword as function name.
This constructor must have only one parameter (except ``this``).

A conversion constructor may be used as any other constructor.
Additionally it may be called implicitly where a type conversion is required.

.. code-block:: u_spr

   struct IntWrapper
   {
       i32 x;
       fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
   }
   
   fn Bar( IntWrapper iw );
   
   fn Foo()
   {
       var IntWrapper iw0(55); // Use the constructor as usual.
       var IntWrapper iw1= 42; // Implicit usage if the conversion constructor during initialization.
       Bar(66); // Implicit conversion in a function call.
   }
