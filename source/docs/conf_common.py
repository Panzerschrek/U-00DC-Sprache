from sprache_lexer import *
from sphinx.highlighting import lexers

project = "Ü Sprache"
author = "Panzerschrek"
copyright = "Panzerschrek 2019-2025"
html_theme = "sphinxdoc"
html_copy_source = False
html_logo = "../logo-Gebrochene-Grotesk.png"
html_favicon = "../favicon.ico"
master_doc = "contents"

lexers['u_spr'] = SpracheLexer(startinline=False)
