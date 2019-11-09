../build-release-llvm-9/Compiler test.u -o test.o -march=wasm32 -filetype=obj -O2 --include-dir /home/panzerschrek/Projects/U-Sprache/source/ustlib/
../build-release-llvm-9/llvm/bin/wasm-ld test.o -o test.wasm --no-entry --export-all --allow-undefined
