Inheritance
===========

Ü supports inheritance for classes.
Inheritance allows to create classes which borrow contents and behavior of parent classes.
Inheritance allows also to implement dynamic polymorphism.

Only polymorph classes can participate in inheritance.
Polymorh are classes that are defined with usage of keywords ``polymorph``, ``abstract`` or ``inteface`` or that have parents.

.. code-block:: u_spr

   class A polymorph // A simple polymorph class. It's possible to inherit from this class.
   {}
   
   class B interface // An interface. It's possible to inherit from it, but it has some limitations.
   {}
   
   class C abstract // Also a polymorph class, but non-implemented virtual methods are allowed inside it.
   {}
   
   class D : C // Also a polymorph class, because it has a parent.
   {}
   
   class E : B // A polymorph class with interface parent.
   {}
   
   class F : A, B // A polymorph class with two parents one of which is an interface.
   {}
   
   class G final : F // A polymorph class from which it's impossible to inherit.
   {}

*******************
*Inheritance rules*
*******************

A class may have no more than one non-interface parent and any number of interface parents.
Non-interface parent class is assumed to be base class.
It may be accessed with usage of ``base`` keyword.
``base`` is a ``this`` reference converted to base class type.

There are following polymorph class kinds:

* ``interface`` class is a polymorph class than can't have base class, fields, virtual methods with implementation.
* ``final`` class is a polymorph class from which it's not allowed to inherit.
* ``abstract`` class is a polymorph class that may have non-implemented virtual methods.
  All virtual methods of polymorph classes that are not ``interface`` or ``abstract`` should have an implementation.

********************************************
*Abstract and interface classes limitations*
********************************************

Abstract and interface classes are not like other classes, they have some limitations:

* It's not allowed to construct an instance of abstract or interface class.
  It's possible only to inherit from such class and create an instance of a child class.
* Interfaces can't have constructors.
  There is no necessity for them to have one, because they have no fields that need to be initialized.
* Abstract classes may have constructors, but whole ``this`` is not available inside them.
  Only fields may be accessed, no methods may be called within an abstract class constructor.
* Interfaces and abstract classes may have destructors, but whole ``this`` within them is not available.

**********************
*Reference convertion*
**********************

A reference to a child class may be implicitly converted into a reference to parent class.
Implicit conversions are possible:

* In reference variable or reference field initialization
* In function argument passing
* In return

*******************
*What is inherited*
*******************

A child class inherit from its parent ``public`` and ``protected`` members, ``private`` members are not inherited.
Constructors, destructors, assignment operators, equality compare operators are not inherited.
Other methods are inherited, but ``this`` remains that of parent class type, except a virtual method is overrided.

*****************
*Virtual methods*
*****************

A virtual method is a method that is indirectly called and its implementation may be different.
A virtual method defined in a class may be overrided in a child class.

A virtual method is defined with usage of following keywords:

* ``virtual`` - a method is first defined.
  In such case no parent of the class should have such method.
* ``virtual pure`` - a method is first defined.
  It's not allowed to implement it.
* ``virtual override`` - a method overrides at least one parent method.
* ``virtual final`` - a method overrides at least one parent method.
  Furter override of this method is not allowed.

All virtual methods should have ``this`` parameter.
It should be mutable or immutable reference, ``byval`` ``this`` is not allowed in virtual methods.

.. code-block:: u_spr

   class A interface
   {
       fn virtual pure Foo( this, i32 x );
   }
   
   class B polymorph
   {
       fn virtual Bar( mut this, f32 y );
   }
   
   class C : A, B
   {
       fn virtual override Foo( this, i32 x );
       fn virtual final Bar( mut this, f32 y );
   }

   fn CallFoo( A& a, i32 x )
   {
       a.Foo(x);
   }
   
   fn CallBar( B &mut b, f32 y )
   {
       b.Bar(y);
   }
   
   fn Test()
   {
       var C mut c;
       CallFoo( c, 42 ); // С::Foo method will be called
       CallBar( c, 0.25f ); // C::Bar method will be called
       var B mut b;
       CallBar( b, 3.14f ); // B::Bar method will be called
   }

A destructor of a polymorph class is always virtual.
It may be defined as virtual explicitlely, but there is no reason to do this.
