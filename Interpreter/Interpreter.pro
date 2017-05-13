TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += c++11


CONFIG( debug, debug|release ) {
	CONFIG+= U_DUBUG
}


LLVM_BASE_DIR= ../../llvm-3.7.1.src
LLVM_BUILD_DIR= $$LLVM_BASE_DIR/build
LLVM_LIBS_DIR= $$LLVM_BUILD_DIR/lib
LLVM_INCLUDES_DIR= $$LLVM_BASE_DIR/include
LLVM_GEN_INCLUDES_DIR= $$LLVM_BUILD_DIR/include

# Include ALL *.a libraries from llvm build.
# TODO - know, which libs we need.
LIBS+= $$LLVM_LIBS_DIR/*.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMCore.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMCodeGen.a

INCLUDEPATH+= $$LLVM_INCLUDES_DIR
INCLUDEPATH+= $$LLVM_GEN_INCLUDES_DIR

SOURCES += \
	src/lexical_analyzer.cpp \
	src/main.cpp \
	src/program_string.cpp \
	src/syntax_analyzer.cpp \
	src/syntax_elements.cpp \
	src/tests/inverse_polish_notation_test.cpp \
	src/tests/code_builder_llvm_test.cpp \
	src/code_builder_llvm.cpp \
	src/code_builder_llvm_types.cpp \
	src/inverse_polish_notation.cpp \
	src/keywords.cpp

HEADERS += \
	src/assert.hpp \
	src/lexical_analyzer.hpp \
	src/program_string.hpp \
	src/syntax_analyzer.hpp \
	src/syntax_elements.hpp \
	src/tests/inverse_polish_notation_test.hpp \
	src/tests/code_builder_llvm_test.hpp \
	src/code_builder_llvm.hpp \
	src/code_builder_llvm_types.hpp \
	src/inverse_polish_notation.hpp \
	src/keywords.cpp

U_DUBUG {
	DEFINES+= U_DEBUG
} else {
}
