include (Common.pri)

TEMPLATE = app
CONFIG += console
CONFIG += link_prl

MAKEFILE= Makefile.Compiler

PRE_TARGETDEPS+= $$U_BUILD_OUT_DIR/libCompilerLib.a
LIBS+= $$U_BUILD_OUT_DIR/libCompilerLib.a

SOURCES += \
	src/main.cpp \

