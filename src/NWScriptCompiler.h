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

		bool initialize(Settings* settings);

		void reset() {
			_resourceManager = nullptr;
			_compiler = nullptr;
			includePathsBuilt = false;
			_compilerCreated = false;
			_includePaths.clear();
		}

		void setLoggerMessageCallback(void (*MessageCallback)(NWScriptLogger::CompilerMessage&)) {
			_logger.setMessageCallback(MessageCallback);
		}

		NWScriptLogger& logger() {
			return _logger;
		}

		bool processFile(filesystem::path& sourcePath, filesystem::path& destDir, bool fromMemory = false, char* fileContents = NULL);

	private:
		std::unique_ptr<ResourceManager> _resourceManager;
		unique_ptr<NscCompiler> _compiler;

		bool compileScript(filesystem::path& sourcePath, filesystem::path& destDir, bool fromMemory, std::string& fileContents,
			const NWN::ResType& fileResType, const NWN::ResRef32& fileResRef);
		bool disassembleBinary(filesystem::path& sourcePath, filesystem::path& destDir, bool fromMemory, std::string& fileContents,
			const NWN::ResType& fileResType, const NWN::ResRef32& fileResRef);

		generic_string NWNHome;
		bool includePathsBuilt = false;
		std::vector<std::string> _includePaths;
		bool _compilerCreated;

		Settings* _settings;

		NWScriptLogger _logger;


		bool LoadScriptResources();
	};
}