from pygments.lexer import RegexLexer
from pygments import token

# Simple regexp-based lexer for Ü.

class SpracheLexer(RegexLexer):
	name = 'u_spr'
	aliases = ['u_spr']
	filenames = ['*.u']
	
	tokens = {
		'root': [
			# Comments
			(r'//[^\n]*\n', token.Comment),
			# Strings
			(r'\"([^\\\"]|(\\n)|(\\r)|(\\t)|(\\b)|(\\f)|(\\\")|(\\0)|(\\\\)|(\\u[0-9a-fA-F]{4,4}))*\"', token.String),
			# Whitespaces
			(r'[\ \t\n\r]+', token.Whitespace),
			# Numbers
			(r'0b[0-1]+(\.[0-1]+)?', token.Number),
			(r'0o[0-7]+(\.[0-7]+)?', token.Number),
			(r'0x[0-9a-fA-F]+(\.[0-9a-fA-F]+)?', token.Number),
			(r'[0-9]+(\.[0-9]+)?(e((-)|(\+))?[0-9]+)?', token.Number),
			# Keywords
			(r'fn(?![a-zA-Z_0-9])', token.Keyword),
			(r'op(?![a-zA-Z_0-9])', token.Keyword),
			(r'var(?![a-zA-Z_0-9])', token.Keyword),
			(r'auto(?![a-zA-Z_0-9])', token.Keyword),
			(r'return(?![a-zA-Z_0-9])', token.Keyword),
			(r'while(?![a-zA-Z_0-9])', token.Keyword),
			(r'break(?![a-zA-Z_0-9])', token.Keyword),
			(r'continue(?![a-zA-Z_0-9])', token.Keyword),
			(r'if(?![a-zA-Z_0-9])', token.Keyword),
			(r'static_if(?![a-zA-Z_0-9])', token.Keyword),
			(r'enable_if(?![a-zA-Z_0-9])', token.Keyword),
			(r'else(?![a-zA-Z_0-9])', token.Keyword),
			(r'move(?![a-zA-Z_0-9])', token.Keyword),
			(r'select(?![a-zA-Z_0-9])', token.Keyword),
			(r'tup(?![a-zA-Z_0-9])', token.Keyword),
			(r'struct(?![a-zA-Z_0-9])', token.Keyword),
			(r'class(?![a-zA-Z_0-9])', token.Keyword),
			(r'final(?![a-zA-Z_0-9])', token.Keyword),
			(r'polymorph(?![a-zA-Z_0-9])', token.Keyword),
			(r'interface(?![a-zA-Z_0-9])', token.Keyword),
			(r'abstract(?![a-zA-Z_0-9])', token.Keyword),
			(r'ordered(?![a-zA-Z_0-9])', token.Keyword),
			(r'nomangle(?![a-zA-Z_0-9])', token.Keyword),
			(r'virtual(?![a-zA-Z_0-9])', token.Keyword),
			(r'override(?![a-zA-Z_0-9])', token.Keyword),
			(r'pure(?![a-zA-Z_0-9])', token.Keyword),
			(r'namespace(?![a-zA-Z_0-9])', token.Keyword),
			(r'public(?![a-zA-Z_0-9])', token.Keyword),
			(r'private(?![a-zA-Z_0-9])', token.Keyword),
			(r'protected(?![a-zA-Z_0-9])', token.Keyword),
			(r'void(?![a-zA-Z_0-9])', token.Keyword),
			(r'bool(?![a-zA-Z_0-9])', token.Keyword),
			(r'i8(?![a-zA-Z_0-9])', token.Keyword),
			(r'u8(?![a-zA-Z_0-9])', token.Keyword),
			(r'i16(?![a-zA-Z_0-9])', token.Keyword),
			(r'u16(?![a-zA-Z_0-9])', token.Keyword),
			(r'i32(?![a-zA-Z_0-9])', token.Keyword),
			(r'u32(?![a-zA-Z_0-9])', token.Keyword),
			(r'i64(?![a-zA-Z_0-9])', token.Keyword),
			(r'u64(?![a-zA-Z_0-9])', token.Keyword),
			(r'i128(?![a-zA-Z_0-9])', token.Keyword),
			(r'u128(?![a-zA-Z_0-9])', token.Keyword),
			(r'f32(?![a-zA-Z_0-9])', token.Keyword),
			(r'f64(?![a-zA-Z_0-9])', token.Keyword),
			(r'char8(?![a-zA-Z_0-9])', token.Keyword),
			(r'char16(?![a-zA-Z_0-9])', token.Keyword),
			(r'char32(?![a-zA-Z_0-9])', token.Keyword),
			(r'size_type(?![a-zA-Z_0-9])', token.Keyword),
			(r'true(?![a-zA-Z_0-9])', token.Keyword),
			(r'false(?![a-zA-Z_0-9])', token.Keyword),
			(r'mut(?![a-zA-Z_0-9])', token.Keyword),
			(r'imut(?![a-zA-Z_0-9])', token.Keyword),
			(r'constexpr(?![a-zA-Z_0-9])', token.Keyword),
			(r'zero_init(?![a-zA-Z_0-9])', token.Keyword),
			(r'uninitialized(?![a-zA-Z_0-9])', token.Keyword),
			(r'this(?![a-zA-Z_0-9])', token.Keyword),
			(r'base(?![a-zA-Z_0-9])', token.Keyword),
			(r'constructor(?![a-zA-Z_0-9])', token.Keyword),
			(r'destructor(?![a-zA-Z_0-9])', token.Keyword),
			(r'conversion_constructor(?![a-zA-Z_0-9])', token.Keyword),
			(r'static_assert(?![a-zA-Z_0-9])', token.Keyword),
			(r'halt(?![a-zA-Z_0-9])', token.Keyword),
			(r'safe(?![a-zA-Z_0-9])', token.Keyword),
			(r'unsafe(?![a-zA-Z_0-9])', token.Keyword),
			(r'type(?![a-zA-Z_0-9])', token.Keyword),
			(r'typeinfo(?![a-zA-Z_0-9])', token.Keyword),
			(r'typeof(?![a-zA-Z_0-9])', token.Keyword),
			(r'template(?![a-zA-Z_0-9])', token.Keyword),
			(r'enum(?![a-zA-Z_0-9])', token.Keyword),
			(r'cast_ref(?![a-zA-Z_0-9])', token.Keyword),
			(r'cast_ref_unsafe(?![a-zA-Z_0-9])', token.Keyword),
			(r'cast_imut(?![a-zA-Z_0-9])', token.Keyword),
			(r'cast_mut(?![a-zA-Z_0-9])', token.Keyword),
			(r'import(?![a-zA-Z_0-9])', token.Keyword),
			(r'default(?![a-zA-Z_0-9])', token.Keyword),
			(r'delete(?![a-zA-Z_0-9])', token.Keyword),
			(r'for(?![a-zA-Z_0-9])', token.Keyword),
			# Identifiers
			(r'[a-zA-Z][a-zA-Z_0-9]*', token.Name),
			# Other lexems
			(r'(\()|(\)|(\[)|(\])|(\{)|(\}))', token.Punctuation),
			(r'[,.:;?=+\-*/%<>&|\^~!\']', token.Operator),
			(r'(</)|(/>)|(::)|(\+\+)|(--)|(==)|(!=)|(<=)|(>=)|(&&)|(\|\|)|(\+=)|(-=)|(\*=)|(/=)|(%=)|(&=)|(\|=)|(\^=)|(<<)|(>>)|(<-)|(->)', token.Operator),
			(r'(<<=)|(>>=)|(\.\.\.)', token.Operator),
		]
	}
