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

HEADERS += \
    src/assert.hpp \
    src/lexical_analyzer.hpp \
    src/program_string.hpp \
    src/syntax_analyzer.hpp \
    src/syntax_elements.hpp \
	src/vm.hpp \
	src/vm.inl

U_DUBUG {
	DEFINES+= DEBUG
} else {
}
