/** @file PluginMain.cpp
 * Controls the Plugin settings
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "jpcre2.hpp"

#include "Common.h"
#include "MiniINI.h"
#include "VersionInfoEx.h"

using namespace mINI;

namespace NWScriptPlugin {

	struct Settings {

		void InitSettings(generic_string& sPluginConfigPath)
		{
			iniFilePath = std::make_unique<INIFile>(sPluginConfigPath);
			iniFile = std::make_unique<INIStructure>();
			fileFiltersCompile = TEXT("*.nss");
			fileFiltersDisasm = TEXT("*.ncs");
		}
		// Declare variables and set some defaults

		// Menu command "Use Auto Indentation" enable/disable flag
		bool enableAutoIndentation = false;
		// Warning about Auto-Indentation conflict was accepted by user?
		bool autoIndentationWarningAccepted = false;
		// Has user setup a restart hook previously?
		RestartMode notepadRestartMode = RestartMode::None;
		// Which function called the restart?
		RestartFunctionHook notepadRestartFunction = RestartFunctionHook::None;

		// Controls Notepad++ versions and auto-install Dark Theme
		bool darkThemePreviouslyInstalled = false;
		bool darkThemeInstallAttempt = false;
		VersionInfoEx notepadVersion;

		// Compiler settings
		bool compilerSettingsCreated = false;
		int neverwinterInstallChoice = 0;
		generic_string neverwinterOneInstallDir;
		generic_string neverwinterTwoInstallDir;
		bool ignoreInstallPaths = false;
		UINT32 compilerFlags = 0;
		bool optimizeScript = true;
		bool useNonBiowareExtenstions = false;
		bool generateSymbols = false;
		int compileVersion = 174;
		bool useScriptPathToCompile = true;
		generic_string outputCompileDir;

		// Batch process files settings
		generic_string startingBatchFolder;
		int batchCompileMode = 0;
		bool recurseSubFolders = false;
		bool continueCompileOnFail = false;
		bool useScriptPathToBatchCompile = true;
		generic_string batchOutputCompileDir;

		// User's preferences
		bool autoDisplayDisassembled = true;
		bool autoDisplayDebugSymbols = true;
		bool autoInstallDarkTheme = false;
		generic_string lastOpenedDir;

		// Plugin statistics
		int compileAttempts = 0;
		int compileSuccesses = 0;
		int compileFails = 0;
		int disassembledFiles = 0;

		int engineStructs = 0;
		int engineFunctionCount = 0;
		int engineConstants = 0;
		int userStructures = 0;
		int userFunctionCount = 0;
		int userConstants = 0;

		// Compiler window settings
		int compilerWindowSelectedTab = 1;
		bool compilerWindowShowErrors = true;
		bool compilerWindowShowWarnings = true;
		bool compilerWindowShowInfos = false;
		bool compilerWindowConsoleWordWrap = true;
		bool compilerWindowConsoleShowErrors = false;
		bool compilerWindowConsoleShowWarnings = true;
		bool compilerWindowConsoleShowInfos = true;


		std::string getChosenInstallDir() {
			return neverwinterInstallChoice == 0 ? properDirNameA(wstr2str(neverwinterOneInstallDir)) : properDirNameA(wstr2str(neverwinterTwoInstallDir));
		}

		generic_string getIncludeDirs() {
			return additionalIncludeDirs;
		}

		generic_string getFileFiltersCompile() {
			return fileFiltersCompile;
		}
		generic_string getFileFiltersDisasm() {
			return fileFiltersDisasm;
		}

		void setIncludeDirs(const generic_string& newDirs) {
			additionalIncludeDirs = newDirs;
		}

		void setFileFiltersCompile(const generic_string& newFilters) {
			fileFiltersCompile = newFilters;
		}

		void setFileFiltersDisasm(const generic_string& newFilters) {
			fileFiltersDisasm = newFilters;
		}

		std::vector<generic_string> getIncludeDirsV();
		std::vector<generic_string> getFileFiltersCompileV();
		std::vector<generic_string> getFileFiltersDisasmV();

		void setIncludeDirs(const std::vector<generic_string>& newFilters);
		void setFileFiltersCompile(const std::vector<generic_string>& newFilters);
		void setFileFiltersDisasm(const std::vector<generic_string>& newFilters);

		// Load values from .INI file
		void Load();
		// Save values to .INI file
		void Save();

	private:
		// Used to self-check if ini was present and correctly loaded.
		bool _bValidINI = false;
		std::string NotepadVersion;

		// Additional include dirs. Set/Get as list, save as string.
		generic_string additionalIncludeDirs;
		// File filters. Set/Get as list, save as string.
		generic_string fileFiltersCompile;
		generic_string fileFiltersDisasm;

		// Plugin config Directory (eg: %AppData%\Notepad++\plugins\config)
		std::unique_ptr<INIFile> iniFilePath;
		std::unique_ptr<INIStructure> iniFile;

		// Converts string to split list by RegEx
		std::vector<generic_string> string2VectorRegex(const generic_string& target, const jpcre2::select<TCHAR>::Regex& separator);

		// Ini Handling
		template <typename T = int>
		T GetNumber(const generic_string& section, const generic_string& key);
		bool GetBoolean(const generic_string& section, const generic_string& key);
		generic_string GetString(const generic_string& section, const generic_string& key);

		template <typename T>
		void SetNumber(const generic_string& section, const generic_string& key, T value);
		void SetBoolean(const generic_string& section, const generic_string& key, bool value);
		void SetString(const generic_string& section, const generic_string& key, const generic_string& value);
	};
}


