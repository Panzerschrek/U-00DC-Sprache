using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Text.Editor;
using Microsoft.VisualStudio.Text.Operations;
using Microsoft.VisualStudio.Text.Tagging;
using Microsoft.VisualStudio.Utilities;
using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Windows.Media;

namespace Ü_extension
{
	internal class HighlightWordTagger : ITagger<HighlightWordTag>
	{
		public event EventHandler<SnapshotSpanEventArgs> TagsChanged;

		public HighlightWordTagger(ITextView view, ITextBuffer sourceBuffer, ITextSearchService textSearchService,
ITextStructureNavigator textStructureNavigator)
		{
		}

		public IEnumerable<ITagSpan<HighlightWordTag>> GetTags(NormalizedSnapshotSpanCollection spans)
		{
			foreach (SnapshotSpan s in spans)
			{
				String span_text = s.GetText();
				int start_index = 0;
				while (true)
				{
					String fn = "fn";
					int fn_index = span_text.IndexOf(fn, start_index);
					if (fn_index == -1)
						break;
					start_index = fn_index + fn.Length;

					yield return new TagSpan<HighlightWordTag>(new SnapshotSpan(s.Snapshot, s.Start + fn_index, fn.Length), new HighlightWordTag());
				}
			}
		}
	}

	internal class HighlightWordTag : TextMarkerTag
	{
		public HighlightWordTag() : base("MarkerFormatDefinition/HighlightWordFormatDefinition") { }
	}

	[Export(typeof(EditorFormatDefinition))]
	[Name("MarkerFormatDefinition/HighlightWordFormatDefinition")]
	[Microsoft.VisualStudio.Text.Classification.UserVisible(true)]
internal class HighlightWordFormatDefinition : MarkerFormatDefinition
	{
		public HighlightWordFormatDefinition()
		{
			this.BackgroundColor = Colors.LightBlue;
			this.ForegroundColor = Colors.DarkBlue;
			this.DisplayName = "Highlight Word";
			this.ZOrder = 5;
		}
	}
}
