using System.ComponentModel.Composition;
using Microsoft.VisualStudio.LanguageServer.Client;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Utilities;

namespace Ü_extension
{
	internal static class FileAndContentTypeDefinitions
	{
		public const string c_content_type = "Ü";

		[Export]
		[Name(c_content_type)]
		[BaseDefinition(CodeRemoteContentDefinition.CodeRemoteContentTypeName)]
		internal static ContentTypeDefinition ü_content_file_definition;

		[Export]
		[FileExtension(".u")]
		[ContentType(c_content_type)]
		internal static FileExtensionToContentTypeDefinition ü_source_file_extension_definition;

		[Export]
		[FileExtension(".uh")]
		[ContentType(c_content_type)]
		internal static FileExtensionToContentTypeDefinition ü_header_file_extension_definition;

		[Export]
		[FileExtension(".ü")]
		[ContentType(c_content_type)]
		internal static FileExtensionToContentTypeDefinition ü_alternative_source_file_extension_definition;
	}
}
