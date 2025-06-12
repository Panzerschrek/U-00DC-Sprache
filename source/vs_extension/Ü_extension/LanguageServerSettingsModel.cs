using System;
using System.ComponentModel.Composition;

namespace Ü_extension
{
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
			set { executable_path_ = value; }
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
		public LanguageServerSettingsModel([Import(typeof(Microsoft.VisualStudio.Shell.SVsServiceProvider))] IServiceProvider sync_service_provider)
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