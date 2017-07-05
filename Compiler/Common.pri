CONFIG -= app_bundle
CONFIG -= qt
CONFIG += c++11
CONFIG += ordered


CONFIG( debug, debug|release ) {
	CONFIG+= U_DUBUG
}

U_DUBUG {
	DEFINES+= U_DEBUG
} else {
}

# CykaBlat, death to Qt, Qmake and QtCreator!
# This shitty system can`t correctly just provide variable like FUCKING_OUTPUT_DIR
# Create it manually.
CONFIG( debug, debug|release ) {
	U_BUILD_OUT_DIR= $${OUT_PWD}/debug
} else {
	U_BUILD_OUT_DIR= $${OUT_PWD}/release
}

LLVM_BASE_DIR= ../../llvm-3.7.1.src
LLVM_BUILD_DIR= $$LLVM_BASE_DIR/build
LLVM_LIBS_DIR= $$LLVM_BUILD_DIR/lib
LLVM_INCLUDES_DIR= $$LLVM_BASE_DIR/include
LLVM_GEN_INCLUDES_DIR= $$LLVM_BUILD_DIR/include
BOOST_BASE_DIR= ../../boost_1_60_0

INCLUDEPATH+= $$LLVM_INCLUDES_DIR
INCLUDEPATH+= $$LLVM_GEN_INCLUDES_DIR
INCLUDEPATH+= $$BOOST_BASE_DIR

# TODO - maybe use "link_prl"/"create_prl" ?

# libLLVMInterpreter and libLLVMExecutionEngine - for test.
LIBS+= $$LLVM_LIBS_DIR/libLLVMInterpreter.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMExecutionEngine.a
# Other - for code builder.
LIBS+= $$LLVM_LIBS_DIR/libLLVMCodeGen.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMCore.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMMC.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMSupport.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMTarget.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMAsmParser.a
LIBS+= $$LLVM_LIBS_DIR/libLLVMBitWriter.a
