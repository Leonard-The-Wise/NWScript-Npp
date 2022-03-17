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

		// Append user settings
		void appendSettings(Settings* settings) {
			_settings = settings;
		}

		// Initialize resource manager
		bool initialize();

		// Reset compiler state
		void reset() {
			_resourceManager = nullptr;
			_compiler = nullptr;
			_includePaths.clear();
			_fetchPreprocessorOnly = false;
			_makeDependencyView = false;
			_sourcePath = "";
			_destDir = "";
			setMode(0);
			_processingEndCallback = nullptr;
			clearLog();
		}

		// Sets destination to a VALID and existing directory (or else get an error)
		void setDestinationDirectory(filesystem::path dest) {
			if (!isValidDirectory(str2wstr(dest.filename().string()).c_str()))
				throw;
			dest = _destDir;
		}

		// Sets source path to a VALID and existing file path (or else get an error)
		void setSourceFilePath(filesystem::path source) {
			if (!PathFileExists(str2wstr(source.filename().string()).c_str()))
				throw;
			_sourcePath = source;
		}

		// Returns the current set Destination Directory
		filesystem::path getDestinationDirectory() {
			return _destDir;
		}

		// Returns the current set Source File path
		filesystem::path getSourceFilePath() {
			return _sourcePath;
		}

		// Set function callback for calling after finishing processing file
		void setProcessingEndCallback(void (*processingEndCallback)(HRESULT returnCode))
		{
			_processingEndCallback = processingEndCallback;
		}

		// Sets function callback for receiving logger messages
		void setLoggerMessageCallback(void (*MessageCallback)(NWScriptLogger::CompilerMessage&)) {
			_logger.setMessageCallback(MessageCallback);
		}

		// Only write dependencies view to the logger
		void setViewDependencies() {
			setMode(0);
			_makeDependencyView = true;
		}

		// Fetchs only preprocessor's output
		void setFetchPreprocessorOnly() {
			setMode(0);
			_fetchPreprocessorOnly = true;
		}

		// Clears the log
		void clearLog() {
			_logger.clear();
		}

		// Sets compiler mode: 0 = compile, 1 = disassemble
		void setMode(int compilerMode) {
			if (compilerMode < 0 || compilerMode > 1)
				throw;
			_compilerMode = compilerMode;
			_fetchPreprocessorOnly = false;
			_makeDependencyView = false;
		}

		NWScriptLogger& logger() {
			return _logger;
		}

		void processFile(bool fromMemory = false, char* fileContents = NULL);

	private:
		std::unique_ptr<ResourceManager> _resourceManager;
		unique_ptr<NscCompiler> _compiler;
		bool _fetchPreprocessorOnly = false;
		bool _makeDependencyView = false;
		int _compilerMode = 0;
		void (*_processingEndCallback)(HRESULT returnCode) = nullptr;

		generic_string NWNHome;
		std::vector<std::string> _includePaths;
		filesystem::path _sourcePath;
		filesystem::path _destDir;

		Settings* _settings;

		NWScriptLogger _logger;

		// Notify Caller of processing results
		void notifyCaller(bool success) {
			if (_processingEndCallback)
				_processingEndCallback(static_cast<HRESULT>((int)success));
		}

		// Load Base script resources
		bool loadScriptResources();

		// Compile a plain text script into binary format
		bool compileScript(bool fromMemory, std::string& fileContents,
			const NWN::ResType& fileResType, const NWN::ResRef32& fileResRef);

		// Disassemble a binary file into a pcode assembly text format
		bool disassembleBinary(bool fromMemory, std::string& fileContents,
			const NWN::ResType& fileResType, const NWN::ResRef32& fileResRef);

		// Dependencies files and views
		bool MakeDependenciesView(const std::set<std::string>& dependencies);
		bool MakeDependenciesFile(const std::set<std::string>& dependencies);
	};
}