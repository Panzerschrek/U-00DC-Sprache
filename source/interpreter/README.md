### About

Ü interpreter allows to compile and launch Ü code, provided in input files.
It is possible to use JIT, if interpreter was build with JIT support.

The interpreter has a bunch of limitations, it can't link code against any static library and has limited support of dynamic libraries usage.
See documentation for LLVM execution engine for more information.


### Emscirpten

It is possible to build the interpreter with Emscripten (for WebAssembly).
This allows to run the interpreter in any browser with WebAssembly support.

Emscripten build of the interpreter includes embedded _ustlib_ files.
Input and import files may be provided by the internal emscripten filesystem.


#### How to build
* Download and install [emsdk](https://github.com/emscripten-core/emsdk.git), see Emscripten documentation for more information
* Download LLVM sources
* Configure the project. Disable target other than interpreter target and set cmake option U_EMSCRIPTEN to true
* Build the project

See file *ci/build_emscripten.sh* for more details.