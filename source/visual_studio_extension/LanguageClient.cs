﻿using Microsoft.VisualStudio.LanguageServer.Client;
using Microsoft.VisualStudio.Threading;
using Microsoft.VisualStudio.Utilities;
using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;

namespace Ü_extension
{
	[ContentType(FileAndContentTypeDefinitions.c_content_type)]
	[Export(typeof(ILanguageClient))]
	class LanguageClient : ILanguageClient
	{
		public string Name => "Ü_extension";

		public IEnumerable<string> ConfigurationSections => null;

		public object InitializationOptions => null;

		public IEnumerable<string> FilesToWatch => null;

		public event AsyncEventHandler<EventArgs> StartAsync;
		public event AsyncEventHandler<EventArgs> StopAsync;

		private readonly LanguageServerSettingsModel settings_model_;

		[ImportingConstructor]
		public LanguageClient( [Import] LanguageServerSettingsModel settings_model)
		{
			settings_model_ = settings_model;
		}

		public async Task<Connection> ActivateAsync(CancellationToken token)
		{
			await Task.Yield();

			Process process = new Process();
			process.StartInfo =
				new ProcessStartInfo
				{
					FileName = settings_model_.ExecutablePath,
					Arguments = settings_model_.CommandLine,
					RedirectStandardInput = true,
					RedirectStandardOutput = true,
					RedirectStandardError = true,
					UseShellExecute = false,
					CreateNoWindow = true
				};

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
