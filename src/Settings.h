/** @file PluginMain.cxx
 * Controls the Plugin settings
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "Common.h"

namespace NWScriptPlugin {

	struct Settings {
		// Menu command "Use Auto Indentation" enable/disable flag
		bool bEnableAutoIndentation = false;
		// Warning about Auto-Indentation conflict was shown?
		bool bAutoIndentationWarningShown = false;
		// Warning about Auto-Indentation conflict was accepted by user?
		bool bAutoIndentationWarningAccepted = false;

		// Plugin config Directory (eg: %AppData%\Notepad++\plugins\)
		generic_string sPluginConfigPath;


		void Save() {};
		void Load() {};
	};
}