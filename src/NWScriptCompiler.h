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
	class NWScriptCompiler final
	{
	public:
		NWScriptCompiler() : 
			_resourceManager(nullptr), _settings(nullptr), _compiler(nullptr) {}

		bool isInitialized() {
			return _resourceManager != nullptr;
		}

		void appendSettings(Settings* settings) {
			_settings = settings;
		}

		bool initialize();

		void reset() {
			_resourceManager = nullptr;
			_compiler = nullptr;
			_includePaths.clear();
		}

		void setLoggerMessageCallback(void (*MessageCallback)(NWScriptLogger::CompilerMessage&)) {
			_logger.setMessageCallback(MessageCallback);
		}

		void clearLog() {
			_logger.clear();
		}

		NWScriptLogger& logger() {
			return _logger;
		}

		bool processFile(filesystem::path& sourcePath, filesystem::path& destDir, bool fromMemory = false, char* fileContents = NULL);

	private:
		std::unique_ptr<ResourceManager> _resourceManager;
		unique_ptr<NscCompiler> _compiler;

		generic_string NWNHome;
		std::vector<std::string> _includePaths;

		Settings* _settings;

		NWScriptLogger _logger;

		// Load Base script resources
		bool LoadScriptResources();

		// Compile a plain text script into binary format
		bool compileScript(filesystem::path& sourcePath, filesystem::path& destDir, bool fromMemory, std::string& fileContents,
			const NWN::ResType& fileResType, const NWN::ResRef32& fileResRef);

		// Disassemble a binary file into a pcode assembly text format
		bool disassembleBinary(filesystem::path& sourcePath, filesystem::path& destDir, bool fromMemory, std::string& fileContents,
			const NWN::ResType& fileResType, const NWN::ResRef32& fileResRef);

		// Write dependencies file
		bool WriteDependencies(const filesystem::path& sourcePath, const filesystem::path& destDir, const std::set<std::string>& dependencies);
	};
}