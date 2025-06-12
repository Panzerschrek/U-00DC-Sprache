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

	[Export]
	class LanguageServerSettingsModel
	{
		private const string c_settings_category = "Ü_extension/language_server";
		private const string c_execitable_path_setting_name = "executable_path";
		private const string c_command_line_setting_name = "command_line";

		private String executable_path_;
		private String command_line_;
		private bool data_loaded_ = false;

		private readonly Microsoft.VisualStudio.Settings.WritableSettingsStore user_settings_store_;

		public String ExecutablePath
		{
			get
			{
				LoadData();
				return executable_path_;
			}
			set{ executable_path_ = value; }
		}

		public String CommandLine
		{
			get
			{
				LoadData();
				return command_line_;
			}
			set { command_line_ = value; }
		}

		[ImportingConstructor]
		public LanguageServerSettingsModel([Import(typeof(SVsServiceProvider))] IServiceProvider sync_service_provider)
		{
			var settings_manager = new Microsoft.VisualStudio.Shell.Settings.ShellSettingsManager(sync_service_provider);
			user_settings_store_ = settings_manager.GetWritableSettingsStore(Microsoft.VisualStudio.Settings.SettingsScope.UserSettings);
		}
		public void LoadData()
		{
			if (data_loaded_)
			{
				return;
			}
			data_loaded_ = true;

			if (!user_settings_store_.CollectionExists(c_settings_category))
			{
				user_settings_store_.CreateCollection(c_settings_category);
			}

			executable_path_ = user_settings_store_.GetString(c_settings_category, c_execitable_path_setting_name, "u.._language_server.exe");
			command_line_ = user_settings_store_.GetString(c_settings_category, c_command_line_setting_name, "");
		}

		public void SaveData()
		{
			user_settings_store_.SetString(c_settings_category, c_execitable_path_setting_name, executable_path_);
			user_settings_store_.SetString(c_settings_category, c_command_line_setting_name, command_line_);
		}
	}
}
