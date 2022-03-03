/** @file PluginMain.h
 * Definitions for the plugin class
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <memory>
#include <vector>
#include <filesystem>

#include "Common.h"

#include "Notepad_plus_msgs.h"

#include "PluginMessenger.h"
#include "LineIndentor.h"
#include "Settings.h"
#include "NWScriptParser.h"

typedef void(PLUGININTERNALS);
#define PLUGINCOMMAND PLUGININTERNALS

namespace fs = std::filesystem;

// Namespace NWScriptPlugin
// Should hold all classes relative to the plugin's functionality
namespace NWScriptPlugin {

	// The main Plugin class singleton.
	class Plugin final {
	private:

		// Holds one Notepad Lexer.
		struct NotepadLexer {
			const int langID;
			const TCHAR* langName;
			const bool isPluginLang;
			const LangAutoIndentMode langIndent;

			explicit NotepadLexer(const int _LangID, const TCHAR* _LangName, const bool _isPluginLang, const LangAutoIndentMode _langIndent)
				: langID(_LangID), langName(_LangName), isPluginLang(_isPluginLang), langIndent(_langIndent) {}

			~NotepadLexer() { delete[] langName; }
		};

		enum class FileCheckResults {
			UnknownError = -3, BlockedByApplication, ReadOnlyFiles, RequiresAdminPrivileges, CheckSuccess
		};

	public:
		Plugin(Plugin& other) = delete;
		void operator =(const Plugin &) = delete;

		// Initializes the Plugin Instance with a DLL or EXE handle. Called from DLL Main message DLL_ATTACH
		static void PluginInit(HANDLE hModule);
		// Performs the instance cleanup: Called from DLL Main message DLL_DETACH
		static void PluginRelease();

		// Setup Notepad++ and Scintilla handles and finish initializing the
		// plugin's objects that need a Windows Handle to work
		void SetNotepadData(NppData data);
		// Returns the Plugin Name. Notepad++ Callback function
		TCHAR* GetName() const { return pluginName.data(); }
		// Returns the Plugin Menu Items count. Notepad++ Callback function
		int GetFunctionCount() const; 
		// Returns the Plugin Menu Items pointer. Notepad++ Callback function
		FuncItem* GetFunctions() const { return pluginFunctions; }

		// Retrieves the unique Plugin's Instance
		static Plugin& Instance() { return *_instance; }

		// Retrieve's Plugin's Settings Object
		Settings& Settings() { return *_settings; }
		// Retrieve's Plugin's Messenger Object
		PluginMessenger& Messenger() const { return *_messageInstance; }
		// Retrieve's Plugin's LineIndentor Object
		LineIndentor& Indentor() const { return *_indentor; }
		// Retrieve's Plugin's Module Handle
		HMODULE DllHModule() const { return _dllHModule; }
		// Retrieves Notepad++ HWND
		HWND NotepadHwnd() const { return _notepadHwnd; }
		// Retrieves the Plugin Path class
		fs::path& PluginPath() const { return *_pluginPath; };
		// Processes Messages from a Notepad++ editor and the coupled Scintilla Text Editor
		void ProcessMessagesSci(SCNotification* notifyCode);
		// Processes Raw messages from a Notepad++ window (the ones not handled by editor). Currently unused by the plugin
		LRESULT ProcessMessagesNpp(UINT Message, WPARAM wParam, LPARAM lParam);

		// Opens the About dialog
		void OpenAboutDialog();
		// Opens the Warning dialog
		void OpenWarningDialog();

		// Menu Command "Use Auto-Indentation" function handler. 
		static PLUGINCOMMAND SwitchAutoIndent();
		// Menu Command "Compile Script" function handler. 
		static PLUGINCOMMAND CompileScript();
		// Menu Command "Settings" function handler. 
		static PLUGINCOMMAND OpenSettings();
		// Menu Command "Import NWScript definitions" function handler. 
		static PLUGINCOMMAND ImportDefinitions();
		// Menu Command "Fix Dark Mode"
		static PLUGINCOMMAND FixLanguageColors();
		// Menu Command "About Me" function handler. 
		static PLUGINCOMMAND AboutMe();

	private:
		Plugin(HMODULE dllModule)
			: _isReady(false), _needPluginAutoIndent(true), _dllHModule(dllModule), _notepadHwnd(nullptr) {}

		// Returns TRUE if the current Lexer is one of the plugin's installed lexers
		bool IsPluginLanguage() const { return _notepadCurrentLexer->isPluginLang; }
		// Load current Notepad++ Language Lexer.
		// Called from messages: NPPN_READY, NPPN_LANGCHANGED and NPPN_BUFFERACTIVATED
		void LoadNotepadLexer();
		// Setup plugin and Notepad++ Auto-Indentation Support.
		// If it's a newer version of notepad++, we use the built-in auto-indentation, or else, we use our customized one
		void SetAutoIndentSupport();
		// Returns tha handle of Notepad++ Main Menu
		HMENU GetNppMainMenu();
		// Setup Menu Icons. Some of them are dynamic shown/hidden.
		void SetupMenuIcons();
		// Do a full file permission check and show appropriate error dialogs to the user when required.
		FileCheckResults FilesWritePermissionCheckup(const std::vector<generic_string>& sFiles);
		// Import a parsed result from NWScript file definitions into our language XML file
		void DoImportDefinitions();

		// Removes Plugin's Auto-Indentation menu command (for newer versions of Notepad++)
		void RemoveAutoIndentMenu();
		// Set a plugin menu Icon to a given stock Shell Icon
		bool SetStockMenuItemIcon(int commandID, SHSTOCKICONID stockIconID, bool bSetToUncheck, bool bSetToCheck);

		// Unique plugin Instance
		static Plugin* _instance;

		// Internal states
		bool _isReady;
		bool _needPluginAutoIndent;

		// Internal classes
		std::unique_ptr<NotepadLexer> _notepadCurrentLexer;
		std::unique_ptr<PluginMessenger> _messageInstance;
		std::unique_ptr<LineIndentor> _indentor;
		std::unique_ptr<NWScriptParser::ScriptParseResults> _NWScriptParseResults;
		std::unique_ptr<generic_string> _test;

		// Internal handles
		HMODULE _dllHModule;
		HWND _notepadHwnd;

		// Settings instance
		std::unique_ptr<NWScriptPlugin::Settings> _settings;


		// Meta Information about the plugin

		// Current Plugin Install Path
		std::unique_ptr<fs::path> _pluginPath;

		// Plugin module name without extension (eg: NWScript-Npp)
		generic_string _pluginFileName;
		// Plugin module extension (eg: .dll)
		generic_string _pluginFileExtension;
		// Plugin Lexer config file (eg: NWScript-Npp.xml)
		generic_string _pluginLexerConfigFile;
		// Plugin Lexer config file path (eg: %ProgramFiles%\Notepad++\plugins\config\NWScript-Npp.xml)
		generic_string _pluginLexerConfigFilePath;

		// Notepad Installation Directory (eg: %ProgramFiles%\Notepad++\)
		generic_string _notepadInstallDir;
		// Notepad Plugin Installation Pathg (eg: %ProgramFiles%\Notepad++\plugins\)
		generic_string _notepadPluginHomePath;
		// Notepad Themes Installation Directory (eg: %ProgramFiles%\Notepad++\themes)
		generic_string _notepadThemesInstallDir;
		// Notepad Dark Theme Installation Path (eg: %ProgramFiles%\Notepad++\themes\DarkModeDefault.xml)
		generic_string _notepadDarkThemeFilePath;

		// Compilation-time information

		// We are defining this on code and dynamic allocated, hence made it static.
		static FuncItem pluginFunctions[];
		// We are defining this on code and dynamic allocated, hence made it static.
		static generic_string pluginName;
	};

}

