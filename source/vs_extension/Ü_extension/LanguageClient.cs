using Microsoft.VisualStudio.LanguageServer.Client;
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

		public IEnumerable<string> ConfigurationSections => null;

		public object InitializationOptions => null;

		public IEnumerable<string> FilesToWatch => null;

		public event AsyncEventHandler<EventArgs> StartAsync;
		public event AsyncEventHandler<EventArgs> StopAsync;

		public async Task<Connection> ActivateAsync(CancellationToken token)
		{
			await Task.Yield();

			ProcessStartInfo info = new ProcessStartInfo();
			info.FileName = "C:/Users/user/Documents/Projects/other/U-00DC-Sprache/other/install/bin/u.._language_server.exe";
			info.Arguments = "--log-file C:/Users/user/Documents/Projects/other/U-00DC-Sprache/other/vs_extension_language_server.log";
			info.RedirectStandardInput = true;
			info.RedirectStandardOutput = true;
			info.UseShellExecute = false;
			info.CreateNoWindow = true;
			info.StandardOutputEncoding = Encoding.UTF8;

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
