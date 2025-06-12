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
		private LanguageServerSettingsModel settings_model;


		public LanguageServerSettingsPage()
		{
			var component_model = ServiceProvider.GlobalProvider.GetService(typeof(SComponentModel)) as IComponentModel;

			this.settings_model = component_model.GetService<LanguageServerSettingsModel>();
		}

		[Category("Ü extension")]
		[DisplayName("Language server executable path")]
		[Description("Where Ü language server executable is located")]
		public String OptionLanguageServerExecutablePath
		{
			get { return settings_model.ExecutablePath; }
			set { settings_model.ExecutablePath = value; }
		}

		[Category("Ü extension")]
		[DisplayName("Language server command line")]
		[Description("Command line for the language server. See its help for more details.")]
		public String OptionLanguageServerCommandLine
		{
			get { return settings_model.CommandLine; }
			set { settings_model.CommandLine = value; }
		}

		public override void LoadSettingsFromStorage()
		{
			this.settings_model.LoadData();
		}

		public override void SaveSettingsToStorage()
		{
			this.settings_model.SaveData();
		}
	}
}
