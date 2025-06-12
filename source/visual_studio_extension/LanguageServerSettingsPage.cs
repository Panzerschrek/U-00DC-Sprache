using Microsoft.VisualStudio.ComponentModelHost;
using Microsoft.VisualStudio.Shell;
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace Ü_extension
{
	[Guid("2BEE44DF-6D49-42A9-BAA7-3652F39173C2")]
	public class LanguageServerSettingsPage : DialogPage
	{
		private readonly LanguageServerSettingsModel settings_model_;

		public LanguageServerSettingsPage()
		{
			var component_model = ServiceProvider.GlobalProvider.GetService(typeof(SComponentModel)) as IComponentModel;

			this.settings_model_ = component_model.GetService<LanguageServerSettingsModel>();
		}

		[Category("Ü extension")]
		[DisplayName("Language server executable path")]
		[Description("Where Ü language server executable is located")]
		public String OptionLanguageServerExecutablePath
		{
			get { return settings_model_.ExecutablePath; }
			set { settings_model_.ExecutablePath = value; }
		}

		[Category("Ü extension")]
		[DisplayName("Language server command line")]
		[Description("Command line for the language server. See its help for more details.")]
		public String OptionLanguageServerCommandLine
		{
			get { return settings_model_.CommandLine; }
			set { settings_model_.CommandLine = value; }
		}
		public override void LoadSettingsFromStorage()
		{
			settings_model_.LoadData();
		}
		public override void SaveSettingsToStorage()
		{
			settings_model_.SaveData();
		}
	}
}
