import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), ".."))
from sprache_lexer import *
from sphinx.highlighting import lexers


project = "Ü Sprache"
author = "Panzerschrek"
copyright = "Panzerschrek 2019-2023"
html_theme = "sphinxdoc"
language = "en"
html_title = "Ü documentation"
html_copy_source = False

lexers['u_spr'] = SpracheLexer(startinline=False)
