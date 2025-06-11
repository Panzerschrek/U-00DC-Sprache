using Microsoft.VisualStudio.ComponentModelHost;
using Microsoft.VisualStudio.LanguageServer.Client;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.Threading;
using Microsoft.VisualStudio.Utilities;
using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Ü_extension
{
	[ContentType("Ü")]
	[Export(typeof(ILanguageClient))]
	class LanguageClient : ILanguageClient
	{
		public string Name => "Ü_extension";

		public IEnumerable<string> ConfigurationSections => new List<string> { "My_option0_path", "My_option1_command_line" };

		public object InitializationOptions => null;

		public IEnumerable<string> FilesToWatch => null;

		public event AsyncEventHandler<EventArgs> StartAsync;
		public event AsyncEventHandler<EventArgs> StopAsync;

		LanguageServerSettingsModel settings_model_;

		[ImportingConstructor]
		public LanguageClient( [Import] LanguageServerSettingsModel settings_model)
		{
			this.settings_model_ = settings_model;
		}

		public async Task<Connection> ActivateAsync(CancellationToken token)
		{
			await Task.Yield();

			ProcessStartInfo info = new ProcessStartInfo();
			info.FileName = this.settings_model_.executable_path;
			info.Arguments = this.settings_model_.command_line;
			info.RedirectStandardInput = true;
			info.RedirectStandardOutput = true;
			info.RedirectStandardError = true;
			info.UseShellExecute = false;
			info.CreateNoWindow = true;

			Process process = new Process();
			process.StartInfo = info;

			if (process.Start())
			{
				return new Connection(process.StandardOutput.BaseStream, process.StandardInput.BaseStream);
			}

			return null;
		}

		public async Task OnLoadedAsync()
		{
			await StartAsync.InvokeAsync(this, EventArgs.Empty);
		}

		public Task OnServerInitializeFailedAsync(Exception e)
		{
			return Task.CompletedTask;
		}

		public Task OnServerInitializedAsync()
		{
			return Task.CompletedTask;
		}
	}
}
