This document describes design principles used in Ü development.
There principles are codified only in June 2026, earlier they weren't codified, but were used implicitly.

* Safety and performance are equally important.
* A program written in Ü and using no unsafe code or only correctly-written unsafe code should not contain memory-related bugs like access to uninitialized memory, use-after-free, double-free and others. Also it should not contain race conditions. Ü language design should guarantee that.
* If some error, mistake or other behavior widely-considered to be undesirable can be prevented in compile time (via compiler checks), it should be prevented.
* Ü should provide means for reduction of repetitive code.
* Ü should provide means simplifying usage of widely-used code patterns.
* Increasing language complexity is welcome as long as such an increase allows improving safety and correctness, gaining runtime performance or simplifying writing code.
* It's preferred to implement some functionality as a piece of library code rather than making it a part of the language itself, unless the second alternative gives substantial benefits (safety, correctness, runtime speed, convenience) compared to the first one.
* It's better to add a feature into the language having many benefits but some disadvantages rather then trying designing an ideal version of it with no disadvantages and failing delivering it at all. But this is not applicable to feature ideas bringing degradation (more runtime costs, less safety and correctness) to other features.
* Language constructions affecting control flow in such a way that some piece of code may not be executed should be explicit. In other words, if execution of some piece of code can be skipped, this should be visible.
* Exact syntax of a particular language feature isn't that important, overall syntax consistency isn't important too. Ü syntax should be good enough to be pretty readable and be able to express underlying language concepts.
* Code readability is important, one should be able to roughly reason about some specific code piece without additional tools (using only a plain text editor). Thus Ü should provide means allowing writing such readable code. *Spukhafte Fernwirkung* should be avoided.
* The Ü compiler shouldn't know about specifics of the Ü standard library and use some specific functionality from it, since such a dependency can substantially increase language complexity and prevent usage of Ü without its standard library.
* Compilation time can be sacrificed to gain more compile-time safety checks.
* Compilation time can be sacrificed to gain more runtime speed.
* Compilation time can be sacrificed to reduce memory consumption of the result program.
* Compilation time can be sacrificed to reduce result binary size.
* Performance of debug builds is mostly irrelevant.
* End-users of programs written in Ü should not pay for functionality needed for developing and debugging of these programs. Such costs are performance costs, runtime memory costs and result binary size costs.
* Programmers writing Ü should not be forced to deal with rare conditions caused by broken integrity of the whole execution environment.
* Memory allocation is generally expected not to fail.
* Stack overflow is expected not to happen.
* No further execution of Ü code in a program should happen if a runtime safety check was failed.
* There should be a way to disable runtime safety checks - for cases, where the last bit of performance can be gained by sacrificing safety.
* Ü is allowed to choose whatever underlying data representation and layout it wants except it was asked explicitly to use some specific representation and layout.
* Binary compatibility between different Ü versions isn't important at all and should not be guaranteed.
* It should be possible to write programs partially written in Ü and partially written in other programming language(s).
* Binary compatibility with other languages should be guaranteed only where it's needed. In other places it can be sacrificed.
* *Garbage in/garbage out* approach is allowed for language operators/other constructions and for standard library code (but not for the Ü compiler), as long as output "garbage" is predictable and inputting "garbage" doesn't cause undefined behavior and only if using such approach in a particular place gives performance benefits.
* Ü should be good enough to be used for wide variety of programs in (almost) any imaginable area. But the main focus should lay on user-space programs running under an operating system, since such programs are the majority of all programs.

The exact order of the principles listed above isn't important.
If for some reason one principle contradicts another, some common ground should be found between them.
