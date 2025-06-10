using System.ComponentModel.Composition;
using Microsoft.VisualStudio.LanguageServer.Client;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Utilities;
namespace Ü_extension
{
	internal static class FileAndContentTypeDefinitions
	{
		[Export]
		[Name("Ü")]
		[BaseDefinition(CodeRemoteContentDefinition.CodeRemoteContentTypeName)]
		internal static ContentTypeDefinition ü_content_file_definition;

		[Export]
		[FileExtension(".u")]
		[ContentType("Ü")]
		internal static FileExtensionToContentTypeDefinition ü_source_file_extension_definition;

		[Export]
		[FileExtension(".uh")]
		[ContentType("Ü")]
		internal static FileExtensionToContentTypeDefinition ü_geader_file_extension_definition;

		[Export]
		[FileExtension(".ü")]
		[ContentType("Ü")]
		internal static FileExtensionToContentTypeDefinition ü_alternative_source_file_extension_definition;
	}
}
