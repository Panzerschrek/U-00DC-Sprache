using Microsoft.VisualStudio.Shell;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Ü_extension
{
	public class LanguageServerSettingsPage : DialogPage
	{
		private String language_server_executable_path_ = "u.._language_server.exe";
		private String language_server_command_line_ = "";

		[Category("Ü extension")]
		[DisplayName("Language server executable path")]
		[Description("Where Ü language server executable is located")]
		public String OptionLanguageServerExecutablePath
		{
			get { return language_server_executable_path_; }
			set { language_server_executable_path_ = value; }
		}

		[Category("Ü extension")]
		[DisplayName("Language server command line")]
		[Description("Command line for the language server. See its help for more details.")]
		public String OptionLanguageServerCommandLine
		{
			get { return language_server_command_line_; }
			set { language_server_command_line_ = value; }
		}
	}
}
