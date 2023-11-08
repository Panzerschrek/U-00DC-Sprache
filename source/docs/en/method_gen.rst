Methods generation
==================

The Ü compiler may generate some methods automatically, there is no necessity to specify them manually.

Methods, that may be generated:

* Default-constructors
* Copy-constructors
* Copy-assignment operators
* Destructors
* Equality-compare operators (``==``)

There must be some conditions to be true in order for a method to be generated:

* For default-constructor generation all struct or class fields should have default iniitializer. There should be no reference fields without own initializers.
* For copy-constructor generation all struct or class fields must be copy-constructoble.
* For copy-assignment operator generation all struct or class fields should be copy-assignable and there should be no immutable fields or reference fields.
* Destructor may be always generated.
* For equality-compare operator generation all struct or class fields should be equality-comparable. There should be no reference fields.

But how it is possible to enable/disable methods generation explicitely?
There are special constructions for it - ``= delete`` and ``= default``.
``= delete`` tells the compiler that this method generation should be disabled, even if generation is possible.
``= default`` tells the compiler that it must generate this method. If generation isn't possible a compilation error will be produced.

Examples:

.. code-block:: u_spr

   class A
   {
   public:
       fn constructor(i32 x) (x_= x) {}
       fn constructor()= default; // Without this default-constructor can't be generated, because there is an explicit constructor except copy constructor.
       fn constructor(A& other)= default; // Without this copy-constructor can't be generated, because classes are assumed to be non-copyable by-default.
       op=(mut this, A& other)= default; //  Without this copy-assignment operator can't be generated, because classes are assumed to be non-copyable by-default.
       op==(A& l, A& r) : bool = default; // Without this equality-compare operator can't be generated, because classes are assumed to be non-equality-comparable by-default.
   
   private:
       i32 x_= 0;
   }
   
   struct B
   {
       fn constructor(B& other)= delete; // Without this copy constructor will be generated, because structs are assumed to be copy-constructible by default.
       op=(mut this, B& other)= delete; // Without this copy-assignment operator will be generated, because structs are assumed to be copy-assignable by default.
       op==(B& l, B& r) : bool = delete;// Without this equality-compare operator will be generated, because structs are assumed to be equality-comparable by default.
       i32 x;
   }
