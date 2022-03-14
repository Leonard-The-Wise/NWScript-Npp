/** @file NWScriptCompiler.cpp
 * Invokes various functions from NscLib compiler/interpreter library.
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <string>
#include <vector>

#include "Nsc.h"
#include "Common.h"

#include "Settings.h"
#include "NWScriptLogger.h"

namespace NWScriptPlugin
{
	class NWScriptCompiler
	{
	public:
		NWScriptCompiler() : 
			_resourceManager(nullptr), _settings(nullptr) {}

		bool isInitialized() {
			return _resourceManager != nullptr;
		}

		bool Initialize(Settings* settings);

		void FlushResources() {
			_resourceManager = nullptr;
			ResourcesLoaded = false;
			InstallDir = TEXT("");
		}

		void SetLoggerMessageCallback(void (*MessageCallback)(NWScriptLogger::CompilerMessage&))	{
			logger.setMessageCallback(MessageCallback);
		}

		NWScriptLogger& Log() {
			return logger;
		}

		bool CompileScript(filesystem::path& scriptPath);

	private:
		std::unique_ptr<ResourceManager> _resourceManager;

		generic_string NWNHome;
		generic_string InstallDir;
		bool ResourcesLoaded = false;
		bool CompilerCreated = false;

		Settings* _settings;

		NWScriptLogger logger;


		bool LoadScriptResources();
	};
}