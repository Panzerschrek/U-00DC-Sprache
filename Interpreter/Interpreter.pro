TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += c++11


CONFIG( debug, debug|release ) {
	CONFIG+= U_DUBUG
}

SOURCES += \
	src/lexical_analyzer.cpp \
	src/main.cpp \
	src/program_string.cpp \
	src/syntax_analyzer.cpp \
	src/syntax_elements.cpp \
	src/vm.cpp \
	src/tests/vm_test.cpp \
	src/tests/inverse_polish_notation_test.cpp \
	src/tests/code_builder_test.cpp \
	src/code_builder.cpp \
	src/inverse_polish_notation.cpp

HEADERS += \
	src/assert.hpp \
	src/lexical_analyzer.hpp \
	src/program_string.hpp \
	src/syntax_analyzer.hpp \
	src/syntax_elements.hpp \
	src/vm.hpp \
	src/vm.inl \
	src/tests/vm_test.hpp \
	src/tests/inverse_polish_notation_test.hpp \
	src/tests/code_builder_test.hpp \
	src/code_builder.hpp \
	src/inverse_polish_notation.hpp

U_DUBUG {
	DEFINES+= DEBUG
} else {
}
