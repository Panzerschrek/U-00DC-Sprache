### Motivation

Ü is able calling external C functions. Almost.
There is one feature of C, which makes this a little bit harder.
This is variadic functions (with variable number of arguments).

In order to call variadic C functions, Ü should support such functions too.
Declaring a prototype for an external variadic C function as non-variadic and calling it works, but only sometimes.
Investigations have shown, that at least on some architectures calling a variadic function is not equivalent to calling a non-variadic one having the same number of arguments passed.
ABI for such two cases isn't identical.
This behavior has been noticed at least on Apple systems with AArch64 architecture.


### Solution

So, Ü needs a way of calling such variadic functions properly.

This feature is needed only for C code interaction.
For Ü to Ü calls itself variadic functions aren't necessary.
They are generally type-unsafe.
Also Ü has other means for achieving the same as C achieves with variadic functions.
So, variadic functions support in Ü should be focused only on supporting calls to external variadic functions.
The way how it's implemented should prioritize implementation simplicity (in the Ü compiler).


### Implementation details

Variadic functions in Ü should be allowed only for functions having "C" calling convention only.
Supporting it for other functions isn't necessary.

Syntax like in C may be used for declaration of variadic functions.
It seems to be pretty straightforward.

It should not be allowed to overload variadic functions.
Such prohibition should simplify code in the Ü compiler selecting overloaded function in case of variadic arguments.

It should be allowed to pass only scalars into variadic functions - numbers, `bool`-values, char-types, byte-types, enums, raw pointers, function pointers.
Passing structs/classes, arrays, tuples should not be allowed.
Passing `void` should also not be allowed, since it's zero-sized and passing it may not work properly, because C doesn't have zero-sized types.
Arguments should be passed only by-value, not by-reference.

Integer promotion and `f32` to `f64` promotion should be implemented.
C standard requires this.

Having variadic class/struct member functions seems to be unnecessary.
At least for this-call methods variadic params support isn't necessary.

Overloaded operators should not be allowed to be variadic.
It's useless.

Templated variadic functions should be also prohibited.
Supporting such functions would lead to increased complexety in code instantiating template functions, which should be avoided.

Variadic lambdas shouldn't be allowed.
It's useless.
It would be easier to avoid supporting such lambdas.

Variadic coroutines shouldn't be allowed.
It gives nothing, but would increase code complexity.

Variadic functions may be located in non-root namespace.
This is mostly useless, but this seems to break nothing.

*nomangle* for variadic functions should not be necessary, even if in practice *nomangle* is needed for calling C code.

Defining variadic functions (having body) may be allowed.
But adding functionality to access variadic arguments isn't necessary.

It should be required to mark variadic functions as *unsafe*.
It's necessary, since one should make sure, that the number of variadic args provided matches what is expected.
Like in *printf* one should specify an arg for each `%` in the format string.

Code in the compiler building function calls should be modified to support variadic functions.
The easiest way of doing this is to pass and process extra (variadic) arguments in a separate loop.
They should not require things like creation of references graph lock nodes (since no references are supported).

The order of variadic args evaluation should be the same as for normal ones.
C does it differently, but it doesn't matter, since order of evaluation has no effect on ABI.

Variadic function pointers should work properly.
Variadic flag should add distinction to function type.
Casting non-variadic function to variadic and back shouldn't be allowed.

C++ header converter should be modified to support creating bindings for variadic functions.
This seems to be pretty easy.

*ustlib* code for Unix systems should be modified, so that it calls variadic external functions as variadic ones.
For Windows this seems to be unneeded, WinAPI doesn't use variadic functions.

Testing variadic functions support requires writing test-cases performing actual calls to C or C++ code.
Doing so in a usual way (in test run by the Ü interpreter) isn't possible.
