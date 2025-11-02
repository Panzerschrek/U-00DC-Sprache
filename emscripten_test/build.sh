../build-release-clang-llvm-src/compiler0/Compiler ../source/examples/hello_world.u  ../source/ustlib/src/unix/main_wrapper.u ../source/ustlib/src/unix/stdout.u --include-dir ../source/ustlib/imports -O2 --internalize --target-os=emscripten --target-arch=wasm32 -o test.o &&\
source ../../emsdk/emsdk_env.sh &&\
emcc test.o -O2 --shell-file html_template/shell_minimal.html -o test.html &&\
echo end
