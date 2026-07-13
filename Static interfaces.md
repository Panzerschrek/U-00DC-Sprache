### Primary static interfaces motivation

Sometimes it's necessary to hide internals of a class, like methods or fields.
Using `private` isn't sometimes enough, since it's still necessary to import files where types of fields and arguments of private methods are defined.
Such import has some cost in terms of compilation time.
Sometimes an import can't be created at all due to some name conflicts.

In such case one may use an interface and place its actual implementation in some other file, where creating necessary imports isn't problematic.
But it adds some runtime overhead - it requires a virtual table pointer to be stored inside each instance of a polymorph class and virtual method calls require indirection, which isn't fast and prevents inlining.

C solves this problem in a type-unsafe way by exposing only a `void` pointer for such type and by casting this pointer to a pointer to actual implementation in a *c* file.
One more way of doing this in C is to use an incomplete type instead of `void` - it requires less type-unsafe casts, but in other aspects works in the same way.
C++ does a better job - it's possible to implement so-called *pimpl* idiom in it, when a class contains a pointer to another incomplete class inside, with this internal class implemented in some *cpp* file and manual redirection of outer class method calls to inner implementation methods.
In both C and C++ such approach requires no overhead for virtual table pointer, method calls are usually not inlined except link-time-optimization is enabled.
But even non-inlined static calls are better than virtual calls, since they have slightly better performance due to better predictability bu CPUs branch predictor.

Ü doesn't have incomplete classes like C++.
A long time ago they were supported, but eventually they have been removed because of some nasty problems.
So, some other approach is necessary for implementing something like *pimpl* idiom from C++.


### Static interfaces as a way to restrict method access

Sometimes it's necessary to pass to some code a reference to some class instance, so that the caller can call some methods of this instance.
This works generally without problems.
But there is an access policy consideration.
If some piece of code has a reference to some class instance, it's able to call all its public methods, or at least all methods with immutable `this` parameter if the given reference isn't mutable.
But sometimes it would be better to restrict access to class methods and allow calling only some subset of them.

In C++ this can be done by creating a proxy class for the given class, which stores a reference/pointer to it inside and has methods which call methods of the internal class.
This works fine, but requires some manual labor for writing such proxy methods.

An approach similar to C++ can be implemented in Ü too.
But it still requires importing a file containing the definition of the wrapped class in the place where a proxy is defined.
And one should still write proxy methods manually.
It would be nice to be able to automate creation of such proxies, so that at least writing proxy methods manually isn't needed.


### Static interfaces as a way to hide implementation details and restrict methods access

To be able to hide implementation details of a class and to avoid too much overhead caused by dynamic polymorphism (virtual calls), it would be nice to have some kind of static interfaces in Ü.
A static interface is a class marked as `static_interface` (or something similar), having no fields and having zero or more method prototypes without implementation (like `pure` methods in polymorph interfaces).
Such static interface can be implemented by some other class, but exactly one such implementation is allowed.
An implementation class should provide implementations for all interface methods (like it's required for non-final polymorph classes implementing polymorph interfaces).
The compiler should statically redirect all calls to interface methods to methods of its implementation.
It should be possible to cast a reference to a class implementing some specific static interface to a reference to this interface.
Downcasts should not be allowed (even they may be technically possible) to force method access restriction.

Implementing many static interfaces should be possible.
It creates an opportunity to have as many restricted proxies for a class as needed without adding overhead.

Templated static interfaces or static interfaces located directly or indirectly within a templated struct/class shouldn't'd be allowed.
Such static interfaces have no sense, since it's impossible to guarantee that an actual implementation will be provided for each instantiation of a hypothetical templated static interface.

Proxy methods generated for a static interface implemented somewhere should have public visibility if this static interface is defined in an imported file.
It's necessary to prevent implementing a static interface in more than one compiled file.
`public` visibility allows preventing this by providing redefinition errors in link-time.
If a static interface is defined in an imported file located in public imports directory of a library build target, no internalization of proxy methods of this static interface should take place.


### Details of static interfaces and further considerations

If a class implements more than one static interface and at least two of them define the same method, all these interface methods should be redirected to the same method of the implementation.
This makes it necessary to create proper LLVM proxy functions (or maybe aliases) for static interface methods, rather than just reusing LLVM function of an interface method for its implementation.

A class shouldn't be allowed to implements a static interface more than once.
It should be checked in compile-time that the list of static interfaces for implementing has no duplicates.

Destructor of a static interface should call destructor of its implementation.
Even if a custom destructor for a static interface is defined, it should implicitly call destructor of its implementation in any cases, probably as its first action.

Static interfaces should have `non_sync` marker - as any other class can have.
Implementing a non-`non_sync` static interface via a `non_sync` class shouldn't be allowed.

It's unclear how to define static interfaces having more than zero inner reference tag.
Since no fields for them are allowed, specifying inner reference tags is impossible.
But this limitation isn't unique for static interfaces, polymorph interfaces have this problem too, so, some common solution for both static and polymorh interfaces should be found sometime in future.

It should be enforced that no inner reference tags (including second order tags) are added in implementing of a static interface.
Changing inner reference tags is now considered to be an error in polymorph inheritance, since it would allow erasing information about stored references, which leads to loopholes in reference checking.
This limitation should be enforced for static interfaces too.

No object of a static interface type should be allowed to be constructed.
This includes local variables, variable params of functions, elements of tuples and non-zero-sized arrays, value fields of structs, value return results of functions, results of `move` and `take` expressions.

It's unclear what to do with copy-assignment operator for static interfaces.
It has almost no sense and seems to be bug-prone.

It's unclear what to do with equality-comparison operator for static interfaces.
Comparing them makes no sense, since they have no fields and thus two values of a static interface class are always equal to each other.
But actual implementations may be different from each other, so, `==` seems to be bug-prone.

For at least first time it should'n be allowed for static interface methods to be coroutine methods (generators and async functions).
It adds too much complexity.

It's probably not a problem for a static interface to have some methods implemented within it.
Such methods can call external functions, non-this-call methods and methods which are required to be implemented.

Templated methods requiring implementation for static interfaces shouldn't be allowed.
They have no sense.
No static interface implementation can provide an implementation for each instantiation of a templated method.

`auto` for return type shouldn't be allowed for static interface methods requiring implementation.
`auto` requires function body to be present, which isn't possible if such body will be implemented later somewhere else.

`enable_if` should work for methods of static interfaces.
But the programmer should care that `enable_if` condition for a particular method is always the same in both static interface and its implementation.

It should be possible to define type aliases, structs, enums, static asserts, mixins, global variables in static interfaces.
These names should be accessible within implementation class with no extra `::` or `SomeInterface::` needed.
It should be possible to override names defined in a static interface in its implementation.

Special methods of a static interface (copy-assignment operator, destructor, equality-compare operator) shouldn't be visible in its implementation.

It should be possible to convert a method of a static interface into a function pointer.
Calling this function pointer (with `this` provided) should be equivalent to calling a method of the implementation directly.

Static interfaces should support visibility labels `public`, `private`, `protected`.
An implementation of a static interface should be able to access `public` and `protected` names, but not `private` names.
External code (code outside an interface or its implementation) should be able to access only `public` names.

Since visibility labels are necessary for static interfaces, structs shouldn't be allowed to be static interfaces, only classes.
And only classes are allowed to implement static interfaces.
Since static interfaces and their implementations are required to be classes, they can't be `constexpr` types.

It's unclear whether it should be allowed for a static interface to extend or partially implement another static interface or multiple static interfaces.
If this doesn't create fundamental problems, it should be allowed.
Since only single implementation of each static interface is allowed, diamond-like hierarchies with static interfaces aren't possible, which may allow for static interfaces what isn't allowed for polymorph interfaces.
Hierarchies of static interfaces may even allow creating of static interfaces which have no non-implemented methods at all, but for some reason can be still extended (implemented).

It's better to forbid reimplementing a method of a static interface (if static interface hierarchies are possible) in some other static interface located deeper in hierarchy.
This is likely bug-prone.
So, there should exist exactly one implementation of a method of a static interface in each hierarchy.

If a class implements a static interface *A* having a non-implemented method *X* and a static interface *B* having the same, but implemented method *X*, *A::X* method should call *B::X* method.
This should work for even more complex cases.

It should be considered to allow for static interfaces to have fields.
This can't lead to duplication of fields, since the single implementation rule prevents creating diamond-like hierarchies.
But having fields may create problems with casting of references to implementation classes into static interface classes.
If more than one static interface is involved, such casts may require using additional `getelementptr` instruction.

Static interfaces should be marked in a special way in `typeinfo`.
Implementation classes probably should be marked in a special-way too.
But it shouldn't be possible to get `typeinfo` for an implementation having `typeinfo` for static interface.

It's unclear how static interfaces should interact with virtual table pointer-based polymorhpism.
There are many open questions.
Should it be allowed for a static interface to extend a polymorph interface?
Should it be allowed for a polymorph interface to extend a static interface?
Should it be allowed for an abstract polymorph class to implement a static interface?
Should it be allowed for a non-abstract class to implement both static and polymorph interfaces?
For at least first implementation of static interfaces it may be reasonable to disallow any interaction with polymorphism.

*ustlib* containers should support storing static interfaces as they now support polymorph interfaces.
This includes *box*, *box_nullable*, various shared pointer containers.

Ü compiler code and build system code should use static interfaces where it makes sense, instead of polymorph interfaces.
