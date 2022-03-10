/** @file PluginMain.cpp
 * Controls the Plugin settings
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "Common.h"
#include "MiniINI.h"

using namespace mINI;

namespace NWScriptPlugin {

	struct Settings {
		explicit Settings(generic_string& sPluginConfigPath)
		{
			iniFilePath = std::make_unique<INIFile>(sPluginConfigPath);
			iniFile = std::make_unique<INIStructure>();
		}

		// Load values from .INI file
		void Load();
		// Save values to .INI file
		void Save();

		// Menu command "Use Auto Indentation" enable/disable flag
		bool bEnableAutoIndentation = false;
		// Warning about Auto-Indentation conflict was accepted by user?
		bool bAutoIndentationWarningAccepted = false;
		// Has user setup a restart hook previously?
		RestartMode iNotepadRestartMode = RestartMode::None;
		// Which function called the restart?
		RestartFunctionHook iNotepadRestartFunction = RestartFunctionHook::None;

	private:
		// Plugin config Directory (eg: %AppData%\Notepad++\plugins\config)
		std::unique_ptr<INIFile> iniFilePath;
		std::unique_ptr<INIStructure> iniFile;

		// Internal functions
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


