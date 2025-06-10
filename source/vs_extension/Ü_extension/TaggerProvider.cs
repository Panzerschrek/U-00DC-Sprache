using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Editor;
using Microsoft.VisualStudio.Text.Operations;
using Microsoft.VisualStudio.Text.Tagging;
using Microsoft.VisualStudio.Utilities;
using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Ü_extension
{
	[Export(typeof(IViewTaggerProvider))]
	[ContentType("Ü")]
	[TagType(typeof(TextMarkerTag))]
	class TaggerProvider : IViewTaggerProvider
	{
		[Import]
		internal ITextSearchService TextSearchService { get; set; }

		[Import]
		internal ITextStructureNavigatorSelectorService TextStructureNavigatorSelector { get; set; }

		public ITagger<T> CreateTagger<T>(ITextView textView, ITextBuffer buffer) where T : ITag
		{
			//provide highlighting only on the top buffer 
			if (textView.TextBuffer != buffer)
				return null;

			ITextStructureNavigator textStructureNavigator =
				TextStructureNavigatorSelector.GetTextStructureNavigator(buffer);

			return new HighlightWordTagger(textView, buffer, TextSearchService, textStructureNavigator) as ITagger<T>;
		}
	}
}
