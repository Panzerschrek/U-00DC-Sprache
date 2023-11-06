import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), ".."))
from sprache_lexer import *
from sphinx.highlighting import lexers


project = "Ü Sprache"
author = "Panzerschrek"
copyright = "Panzerschrek 2019-2023"
html_theme = "sphinxdoc"
language = "ru"
html_title = "Ü документация"
html_copy_source = False
html_logo = "logo-Gebrochene-Grotesk.png"
html_favicon = "favicon.ico"

lexers['u_spr'] = SpracheLexer(startinline=False)
