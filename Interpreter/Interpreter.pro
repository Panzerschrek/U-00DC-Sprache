TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += c++11


CONFIG( debug, debug|release ) {
	CONFIG+= U_DUBUG
}

U_DUBUG {
	DEFINES+= U_DEBUG
} else {
}

LLVM_BASE_DIR= ../../llvm-3.7.1.src
LLVM_BUILD_DIR= $$LLVM_BASE_DIR/build
LLVM_LIBS_DIR= $$LLVM_BUILD_DIR/lib
LLVM_INCLUDES_DIR= $$LLVM_BASE_DIR/include
LLVM_GEN_INCLUDES_DIR= $$LLVM_BUILD_DIR/include
BOOST_BASE_DIR= ../../boost_1_60_0

# libLLVMInterpreter and libLLVMExecutionEngine - for test.
LIBS+= $$LLVM_LIBS_DIR/libLLVMInterpreter.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMExecutionEngine.a
# Other - for code builder.
LIBS+= $$LLVM_LIBS_DIR/libLLVMCodeGen.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMCore.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMMC.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMSupport.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMTarget.a

INCLUDEPATH+= $$LLVM_INCLUDES_DIR
INCLUDEPATH+= $$LLVM_GEN_INCLUDES_DIR
INCLUDEPATH+= $$BOOST_BASE_DIR

SOURCES += \
	src/code_builder_errors.cpp \
	src/code_builder.cpp \
	src/code_builder_types.cpp \
	src/inverse_polish_notation.cpp \
	src/keywords.cpp \
	src/lexical_analyzer.cpp \
	src/main.cpp \
	src/program_string.cpp \
	src/syntax_analyzer.cpp \
	src/syntax_elements.cpp \
	src/tests/code_builder_errors_test.cpp \
	src/tests/code_builder_test.cpp \
	src/tests/inverse_polish_notation_test.cpp \


HEADERS += \
	src/assert.hpp \
	src/code_builder_errors.hpp \
	src/code_builder.hpp \
	src/code_builder_types.hpp \
	src/inverse_polish_notation.hpp \
	src/keywords.hpp \
	src/lexical_analyzer.hpp \
	src/pop_llvm_warnings.hpp \
	src/program_string.hpp \
	src/push_disable_llvm_warnings.hpp \
	src/syntax_elements.hpp \
	src/syntax_analyzer.hpp \
	src/tests/code_builder_errors_test.hpp \
	src/tests/code_builder_test.hpp \
	src/tests/inverse_polish_notation_test.hpp \
