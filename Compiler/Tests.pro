include (Common.pri)

TEMPLATE = app
CONFIG += console
CONFIG += link_prl

MAKEFILE= Makefile.Tests

PRE_TARGETDEPS+= $$U_BUILD_OUT_DIR/libCompilerLib.a
LIBS+= $$U_BUILD_OUT_DIR/libCompilerLib.a

SOURCES += \
	src/tests/code_builder_errors_test.cpp \
	src/tests/code_builder_test.cpp \
	src/tests/inverse_polish_notation_test.cpp \
	src/tests/tests_main.cpp \

HEADERS += \
	src/tests/code_builder_errors_test.hpp \
	src/tests/code_builder_test.hpp \
	src/tests/inverse_polish_notation_test.hpp \
