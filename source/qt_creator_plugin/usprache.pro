CONFIG += c++1z
DEFINES += USPRACHE_LIBRARY

isEmpty( QT_CREATOR_SOURCE_ROOT ) {
	error( "QT_CREATOR_SOURCE_ROOT not defined. You can define path to QtCreator sources, using variable QT_CREATOR_SOURCE_ROOT" )
} else {
	IDE_SOURCE_TREE= $$QT_CREATOR_SOURCE_ROOT

}

isEmpty( QT_CREATOR_BINARY_ROOT ) {
	error( "QT_CREATOR_BINARY_ROOT not defined. You can define path to QtCreator binary, using variable QT_CREATOR_BINARY_ROOT" )
} else {
	IDE_BUILD_TREE= $$QT_CREATOR_BINARY_ROOT
}

# USprache files

SOURCES+= \
	*.cpp \
	../compiler0/lex_synt_lib/*.cpp \

HEADERS+= \
	*.hpp \
	../compiler0/lex_synt_lib/*.hpp \

INCLUDEPATH += gen/

## uncomment to build plugin into user config directory
## <localappdata>/plugins/<ideversion>
##    where <localappdata> is e.g.
##    "%LOCALAPPDATA%\QtProject\qtcreator" on Windows Vista and later
##    "$XDG_DATA_HOME/data/QtProject/qtcreator" or "~/.local/share/data/QtProject/qtcreator" on Linux
##    "~/Library/Application Support/QtProject/Qt Creator" on OS X
USE_USER_DESTDIR= yes

###### If the plugin can be depended upon by other plugins, this code needs to be outsourced to
###### <dirname>_dependencies.pri, where <dirname> is the name of the directory containing the
###### plugin's sources.

QTC_PLUGIN_NAME= USprache
QTC_LIB_DEPENDS+= \
	utils \
	cplusplus \

QTC_PLUGIN_DEPENDS+= \
	coreplugin \
	texteditor \

QTC_PLUGIN_RECOMMENDS+= \
    # optional plugin dependencies. nothing here at this time

###### End _dependencies.pri contents ######

include($$IDE_SOURCE_TREE/src/qtcreatorplugin.pri)
