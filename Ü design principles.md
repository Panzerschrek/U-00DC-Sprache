This document describes design principles used in Ü development.
There principles have been codified only in June 2026, earlier they weren't codified, but were used implicitly.
Here are they:

* *I* Safety and performance are both important.
* *II* Safety and performance are equally important.
* *III* A program written in Ü and using no unsafe code or only correctly-written unsafe code should not contain memory-related bugs like reading uninitialized memory, use-after-free, double-free and others memory-related errors, race conditions and any other kind of undefined behavior. Ü language design should guarantee that.
* *IV* If some error, mistake or other behavior widely-considered to be undesirable can be prevented in compile-time (via compiler checks), it should be prevented.
* *V* Ü should provide means for reduction of repetitive code.
* *VI* Ü should provide means simplifying usage of widely-used code patterns.
* *VII* Increasing language complexity is welcome as long as such an increase allows improving safety and correctness, gaining runtime performance, improving expressiveness or simplifying writing code.
* *VIII* It's preferred to implement some functionality as a piece of library code rather than making it a part of the language itself, unless the second alternative gives substantial benefits (safety, correctness, runtime speed, expressiveness) compared to the first one.
* *IX* It's better to add a feature into the language having many benefits but some disadvantages rather then trying designing an ideal version of it having no disadvantages and failing delivering it at all. But this is not applicable to feature ideas bringing degradation (more runtime costs, less safety and correctness, etc.) to other features.
* *X* No single language feature or standard library functionality is allowed to cause performance overhead for usage of some other feature(s) and functionality piece(s), unless it's strictly needed to achieve safety and there is no way of doing so without some performance overhead.
* *XI* Language constructions affecting control flow in such a way that some piece of code may not be executed should be explicit. In other words, if execution of some piece of code can be skipped, this should be visible.
* *XII* Exact syntax of a particular language feature isn't that important, overall syntax consistency isn't important too. Ü syntax should be good enough to be pretty readable and be able to express underlying language concepts.
* *XIII* Having a formal description of the language is considered to be unnecessary and having little to no value. But if such a description does exist, actual Ü implementation should have priority over it.
* *XIV* Code readability is important, one should be able to roughly reason about some specific code piece without additional tools (using only a plain text editor). Thus Ü should provide means allowing writing such readable code. *Spukhafte Fernwirkung* should be avoided.
* *XV* Ü should not enforce some specific formatting style or naming convention. Programmers using Ü should have freedom using their favorite style.
* *XVI* Ü targets professional programmers or other people, who spend much time doing actual programming, but not beginners and/or hobbyists with little programming experience.
* *XVII* Safety, correctness, performance, expressiveness or any other language aspect simplifying writing code should not be sacrificed in order to make it easier to learn Ü. Non-trivial learning costs are considered to be an investment giving long-term benefits.
* *XVIII* The Ü compiler shouldn't know about specifics of the Ü standard library and/or use some specific functionality from it, since such a dependency can substantially increase language complexity and prevent usage of Ü without its standard library.
* *XIX* Compilation speed can be sacrificed to gain more compile-time safety checks.
* *XX* Compilation speed can be sacrificed to gain more runtime speed.
* *XXI* Compilation speed can be sacrificed to reduce memory consumption of the result program.
* *XXII* Compilation speed can be sacrificed to reduce result binary size.
* *XXIII* Spending time fixing compilation errors and waiting for compilation to finish is better than debugging.
* *XXIV* Performance of debug builds is mostly irrelevant.
* *XXV* End-users of programs written in Ü should not pay for functionality needed for developing and debugging of these programs. Such costs are performance costs, runtime memory costs and result binary size costs.
* *XXVI* Programmers writing Ü should not be forced to deal with rare conditions caused by broken integrity of the whole execution environment.
* *XXVII* Memory allocation is generally expected not to fail.
* *XXVIII* Stack overflow is expected not to happen.
* *XXIX* Preventing deadlocks is assumed to be impossible. The only guarantee in case of a deadlock is that no further code execution of deadlocked thread(s) is performed.
* *XXX* No further execution of Ü code in a program should happen if a runtime safety check has failed.
* *XXXI* There should be a way to disable runtime safety checks - for cases, where the last bit of performance can be gained by sacrificing safety.
* *XXXII* Ü is allowed to choose whatever underlying data representation and layout it wants except it was asked explicitly to use some specific representation and layout.
* *XXXIII* Binary compatibility between different Ü versions isn't important at all and should not be guaranteed.
* *XXXIV* It should be possible to write programs partially written in Ü and partially written in other programming language(s) and thus there should be a way of communication between Ü code and code written in these language(s).
* *XXXV* Binary compatibility with other languages should be guaranteed only where it's needed. In other places it can be sacrificed.
* *XXXVI* *Garbage in/garbage out* approach is allowed for language operators/other constructions and for standard library code (but not for the Ü compiler), as long as output "garbage" is predictable and inputting "garbage" doesn't cause undefined behavior and only if using such approach in a particular place gives performance benefits.
* *XXXVII* Ü should be good enough to be used for wide variety of programs in (almost) any imaginable area. But the main focus should lay on user-space programs running under an operating system, since such programs are the majority of all programs.
* *XXXVIII* Code written in Ü should be generally source-level portable across different platforms (architectures, environments, operating systems), but full portability including every nuance of the language or its standard library isn't required, since this may introduce performance overhead where it can be avoided.
* *XXXIX* Backward compatibility in 100% cases can't be achieved (Hyrum's Law), unless no development (even bug-fixes) is ever done. So, introducing breaking changes is allowed, but only if breaking something gives substantial benefits and only if there is an easy and straightforward way of fixing existing Ü code broken by such change.

The principes listed above are numbered in order to allow referencing them, but their exact order isn't important.
If in some particular case one principle contradicts another, some common ground should be found between them.

These principles are considered to be axiomatic and thus require no strictly-formulated reason behind them.
Changing/removing existing principles or introducing new ones should be done with extreme caution and all possible consequences of such changes should be kept in mind.
Any change should preserve sequential numbers (no reordering is allowed).
