Classes
=======

A class is a type kind that is similar to a struct, but have are some differences relative to it:

* A class can't be initialized with ``{}`` initializer.
  Only constructors may be used for initialization.
* Copy-constructor and copy-assignment operator are not generated for classed by-default.
  But there is a way to force the compiler to generate such methods, if it is necessary.
* A class can't be a ``constexpr`` type.
* A class may participate in inheritance (may have parents and/or children).

A class is defined with ``class`` keyword instead of ``struct``.

*******************
*Visibility labels*
*******************

Visibility labels ``public``, ``private``, ``protected`` may be used inside classes.
All class members defined after a visibility label will have such visibility.
A visibility from a class beginning down to the first visibility label is considered to be ``public``.

``public`` visibility means that this class member may be accessed from any place in a program.
``private`` visibility means that this class member may be accessed only inside class members namespace (including namespaces of nested classes).
``protected`` visibility is like ``private``, but class members with this visibility may be accessed not only within this class, but also within its children.

.. code-block:: u_spr

   class A
   {
   public:
       fn GetX(this) : i32 { return x_; }
       fn SetX(mut this, i32 in_x) { x_= in_x; }

   private:
       i32 x_= 0;
   }
   
   fn Foo()
   {
       var A mut a;
       a.SetX(42); // Ok, access "public" member
       auto x= a.GetX(); // Ok, access "public" member
       a.x_= 42; // An error will be fenerated - non-public member access
   }
