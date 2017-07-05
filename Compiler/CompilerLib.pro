include (Common.pri)
TEMPLATE = lib
CONFIG += staticlib
CONFIG += create_prl

MAKEFILE= Makefile.CompilerLib

SOURCES += \
	src/code_builder_errors.cpp \
	src/code_builder.cpp \
	src/code_builder_types.cpp \
	src/inverse_polish_notation.cpp \
	src/keywords.cpp \
	src/lexical_analyzer.cpp \
	src/program_string.cpp \
	src/syntax_analyzer.cpp \
	src/syntax_elements.cpp \

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
