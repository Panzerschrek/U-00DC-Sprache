from sprache_lexer import *
from sphinx.highlighting import lexers

project = "Ü Sprache"
author = "Panzerschrek"
copyright = "Panzerschrek 2019-2024"
html_theme = "sphinxdoc"
html_copy_source = False
html_logo = "../logo-Gebrochene-Grotesk.png"
html_favicon = "../favicon.ico"

lexers['u_spr'] = SpracheLexer(startinline=False)
