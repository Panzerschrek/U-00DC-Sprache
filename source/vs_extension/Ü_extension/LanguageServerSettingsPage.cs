using Microsoft.VisualStudio.ComponentModelHost;
using Microsoft.VisualStudio.Shell;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.ComponentModel.Composition;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

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
			get { return settings_model.executable_path; }
			set { settings_model.executable_path = value; }
		}

		[Category("Ü extension")]
		[DisplayName("Language server command line")]
		[Description("Command line for the language server. See its help for more details.")]
		public String OptionLanguageServerCommandLine
		{
			get { return settings_model.command_line; }
			set { settings_model.command_line = value; }
		}
	}

	[Export]
	class LanguageServerSettingsModel
	{
		public String executable_path = "u.._language_server.exe";
		public String command_line = "";

	}
}
