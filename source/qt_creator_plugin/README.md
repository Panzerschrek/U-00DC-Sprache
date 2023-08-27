### About

This directory contains sources of QtCreator plugin.

The plugin targets QtCreater version 4.13. It is pretty outdated.

Early this plugin had a handful of features, including indenting, document symbols tree displaying and syntax highlighting.
But later this was changed.

Indenting was removed, since it was not worked in QtCreator 4.13.

Since Ü language server was implemented, symbols tree construction was removed from the plugin, because language server now constructs document symbols.

Syntax highlighting code was removed in favor of generic syntax highlighter with custom config for Ü.
See _u_kde_syntax_highlighting.xml_ file.
You need to put it into directory with custom syntax highlighting configs in order to allow highlighting for Ü sources.
See *preferences/text editor* menu in order to know where this directory located in your system.

The only remaining functionality in the plugin is custom text editor, that does almost nothing, except some custom configuring of editor itself and its context menu.

### How to build
* Download QtCreator.  
* Download QtCreator sources.  
* Run qmake for _usprace.pro_. You must set path to QtCreator sources and binaries in qmake arguments.  
* Perform the build.
