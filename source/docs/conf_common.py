from sprache_lexer import *
from sphinx.highlighting import lexers

project = "Ü Sprache"
author = "Panzerschrek"
copyright = "Panzerschrek 2019-2023"
html_theme = "sphinxdoc"
html_copy_source = False

lexers['u_spr'] = SpracheLexer(startinline=False)