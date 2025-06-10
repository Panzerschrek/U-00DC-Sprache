using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Utilities;
namespace Ü_extension
{
	internal static class FileAndContentTypeDefinitions
	{
		[Export]
		[Name("Ü")]
		[BaseDefinition("text")]
		internal static ContentTypeDefinition ü_content_file_definition;

		[Export]
		[FileExtension(".u")]
		[ContentType("Ü")]
		internal static FileExtensionToContentTypeDefinition ü_file_extension_definition;
	}
}
