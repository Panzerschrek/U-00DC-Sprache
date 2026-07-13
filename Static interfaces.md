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
