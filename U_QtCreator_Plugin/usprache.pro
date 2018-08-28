DEFINES += USPRACHE_LIBRARY

INCLUDEPATH+= ../../boost_1_65_1/

# USprache files

SOURCES += \
	uspracheplugin.cpp \
	../Compiler/src/keywords.cpp \
	../Compiler/src/lang_types.cpp \
	../Compiler/src/lexical_analyzer.cpp \
	../Compiler/src/operators.cpp \
	../Compiler/src/program_string.cpp \
	../Compiler/src/syntax_analyzer.cpp \
	../Compiler/src/syntax_elements.cpp \

HEADERS += \
	uspracheplugin.h \
	usprache_global.h \
	uspracheconstants.h \
	../Compiler/src/keywords.hpp \
	../Compiler/src/lang_types.hpp \
	../Compiler/src/lexical_analyzer.hpp \
	../Compiler/src/operators.hpp \
	../Compiler/src/program_string.hpp \
	../Compiler/src/syntax_analyzer.hpp \
	../Compiler/src/syntax_elements.hpp \

# Qt Creator linking

## Either set the IDE_SOURCE_TREE when running qmake,
## or set the QTC_SOURCE environment variable, to override the default setting
isEmpty(IDE_SOURCE_TREE): IDE_SOURCE_TREE = $$(QTC_SOURCE)
isEmpty(IDE_SOURCE_TREE): IDE_SOURCE_TREE = "../../qt-creator/"

## Either set the IDE_BUILD_TREE when running qmake,
## or set the QTC_BUILD environment variable, to override the default setting
isEmpty(IDE_BUILD_TREE): IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE): IDE_BUILD_TREE = "../../../Qt5.9.0/Tools/QtCreator"

## uncomment to build plugin into user config directory
## <localappdata>/plugins/<ideversion>
##    where <localappdata> is e.g.
##    "%LOCALAPPDATA%\QtProject\qtcreator" on Windows Vista and later
##    "$XDG_DATA_HOME/data/QtProject/qtcreator" or "~/.local/share/data/QtProject/qtcreator" on Linux
##    "~/Library/Application Support/QtProject/Qt Creator" on OS X
USE_USER_DESTDIR = yes

###### If the plugin can be depended upon by other plugins, this code needs to be outsourced to
###### <dirname>_dependencies.pri, where <dirname> is the name of the directory containing the
###### plugin's sources.

QTC_PLUGIN_NAME = USprache
QTC_LIB_DEPENDS += \
	utils \

QTC_PLUGIN_DEPENDS += \
	coreplugin \
	texteditor \

QTC_PLUGIN_RECOMMENDS += \
    # optional plugin dependencies. nothing here at this time

###### End _dependencies.pri contents ######

include($$IDE_SOURCE_TREE/src/qtcreatorplugin.pri)
