include (Common.pri)

TEMPLATE = app
CONFIG += console
CONFIG += link_prl

MAKEFILE= Makefile.Tests

PRE_TARGETDEPS+= $$U_BUILD_OUT_DIR/libCompilerLib.a
LIBS+= $$U_BUILD_OUT_DIR/libCompilerLib.a

SOURCES += \
	src/tests/auto_variables_test.cpp \
	src/tests/code_builder_errors_test.cpp \
	src/tests/code_builder_test.cpp \
	src/tests/initializers_errors_test.cpp \
	src/tests/initializers_test.cpp \
	src/tests/inverse_polish_notation_test.cpp \
	src/tests/tests_main.cpp \
	src/tests/tests.cpp \

HEADERS += \
	src/tests/auto_variables_test.hpp \
	src/tests/code_builder_errors_test.hpp \
	src/tests/code_builder_test.hpp \
	src/tests/initializers_errors_test.hpp \
	src/tests/initializers_test.hpp \
	src/tests/inverse_polish_notation_test.hpp \
	src/tests/tests.hpp \
