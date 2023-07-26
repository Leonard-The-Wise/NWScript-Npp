/** @file NWScriptCompiler.cpp
 * Invokes various functions from NscLib compiler/interpreter library.
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <string>
#include <vector>

#include "Native Compiler/exobase.h"		// New oficial compiler provided by Beamdog itself.
#include "Native Compiler/scriptcomp.h"		// 
#include "Nsc.h"							// Here we are using NscLib for older features like preprocessor and make dependency
#include "Common.h"

#include "Settings.h"
#include "NWScriptLogger.h"

namespace NWScriptPlugin
{

	// Copied from NscCompiler.cpp -> Resource Cache structures
	struct ResourceCacheKey
	{
		NWN::ResRef32 ResRef;
		NWN::ResType  ResType;

		inline bool operator < (const ResourceCacheKey& other) const
		{
			return (ResType < other.ResType) ||
				(memcmp(&ResRef, &other.ResRef, sizeof(ResRef)) < 0);
		}

		inline bool operator == (const ResourceCacheKey& other) const
		{
			return (ResType == other.ResType) &&
				(memcmp(&ResRef, &other.ResRef, sizeof(ResRef)) == 0);
		}
	};

	struct ResourceCacheEntry
	{
		bool                Allocated;
		char*				Contents;
		UINT32              Size;
		std::string         Location;
	};

	typedef std::map<ResourceCacheKey, ResourceCacheEntry> ResourceCache;

	struct NativeCompileResult
	{
		int32_t code;
		char* str; // static buffer
	};

	// Function pointers to resolve new compiler Resource API requirements
	static BOOL ResManUpdateResourceDirectory(const char* sAlias);
	static int32_t ResManWriteToFile(const char* sFileName, RESTYPE nResType, const uint8_t* pData, size_t nSize, bool bBinary);
	static const char* ResManLoadScriptSourceFile(const char* sFileName, RESTYPE nResType);
	static const char* TlkResolve(STRREF strRef);

	class NWScriptCompilerV2 final
	{
	public:

		NWScriptCompilerV2();

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
			_compilerV2 = nullptr;
			_compiler = nullptr;
			_includePaths.clear();
			_fetchPreprocessorOnly = false;
			_makeDependencyView = false;
			_sourcePath = "";
			_destDir = "";
			setMode(0);
			_processingEndCallback = nullptr;
			clearLog();

			// Free memory from Resource Cache to avoid memory leaks
			for (auto& entry : _ResourceCache)
			{
				if (entry.second.Allocated && entry.second.Contents)
				{
					delete[] entry.second.Contents;
					entry.second.Contents = nullptr;
					entry.second.Allocated = false;
				}
			}

			_ResourceCache.clear();
		}

		// Sets destination to a VALID and existing directory (or else get an error)
		void setDestinationDirectory(fs::path dest) {
			if (!isValidDirectory(str2wstr(dest.string()).c_str()))
				throw;
			_destDir = dest;
		}

		// Sets source path to a VALID and existing file path (or else get an error)
		void setSourceFilePath(fs::path source) {
			if (!PathFileExists(source.c_str()))
				throw;
			_sourcePath = source;
		}

		// Returns the current set Destination Directory
		fs::path getDestinationDirectory() {
			return _destDir;
		}

		// Returns the current set Source File path
		fs::path getSourceFilePath() {
			return _sourcePath;
		}

		// Set function callback for calling after finishing processing file
		void setProcessingEndCallback(void (*processingEndCallback)(HRESULT returnCode))
		{
			_processingEndCallback = processingEndCallback;
		}

		// Sets function callback for receiving logger messages
		void setLoggerMessageCallback(void (*MessageCallback)(const NWScriptLogger::CompilerMessage&)) {
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

		inline int getMode() const {
			return _compilerMode;
		}

		inline bool isViewDependencies() const {
			return _makeDependencyView;
		}

		inline bool isFetchPreprocessorOnly() const {
			return _fetchPreprocessorOnly;
		}

		NWScriptLogger& logger() {
			return _logger;
		}

		std::vector<std::string>& includePaths() {
			return _includePaths;
		}

		// Returns if an output path is required for operation
		inline bool isOutputDirRequired() {
			return !(_fetchPreprocessorOnly || _makeDependencyView);
		}

		inline ResourceCache& getResourceCache() {
			return _ResourceCache;
		}


		void processFile(bool fromMemory, char* fileContents);

	private:

		std::unique_ptr<ResourceManager> _resourceManager;
		ResourceCache _ResourceCache;
		std::unique_ptr<CScriptCompiler> _compilerV2;

		// # TODO: Remove old compiler references
		std::unique_ptr<NscCompiler> _compiler;

		bool _fetchPreprocessorOnly = false;
		bool _makeDependencyView = false;
		int _compilerMode = 0;
		void (*_processingEndCallback)(HRESULT returnCode) = nullptr;

		generic_string NWNHome;
		std::vector<std::string> _includePaths;
		fs::path _sourcePath;
		fs::path _destDir;

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
		bool compileScript(std::string& fileContents,
			const NWN::ResType& fileResType, const NWN::ResRef32& fileResRef);

		bool compileScriptV2(std::string& fileContents,
			const NWN::ResType& fileResType, const NWN::ResRef32& fileResRef);

		// Disassemble a binary file into a pcode assembly text format
		bool disassemblyBinary(std::string& fileContents,
			const NWN::ResType& fileResType, const NWN::ResRef32& fileResRef);

		// Dependencies files and views
		bool MakeDependenciesView(const std::set<std::string>& dependencies);
		bool MakeDependenciesFile(const std::set<std::string>& dependencies);
	};
}