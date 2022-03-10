/** @file PluginMain.h
 * Definitions for the plugin class
 * 
 **/
 // Copyright (C) 2022 - Leonardo Silva 
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
			const ExternalLexerAutoIndentMode langIndent;

			explicit NotepadLexer(const int _LangID, const TCHAR* _LangName, const bool _isPluginLang, const ExternalLexerAutoIndentMode _langIndent)
				: langID(_LangID), langName(_LangName), isPluginLang(_isPluginLang), langIndent(_langIndent) {}

			~NotepadLexer() { delete[] langName; }
		};

		// File checks results
		enum class FileCheckResults {
			UnknownError = -3, BlockedByApplication, ReadOnlyFiles, RequiresAdminPrivileges, CheckSuccess
		};

		enum class DarkThemeStatus {
			Uninstalled, Installed, Unsupported
		};

	public:

		// ### Class Instantiation

		Plugin(Plugin& other) = delete;
		void operator =(const Plugin &) = delete;

		// ### Initialization

		// Initializes the Plugin Instance with a DLL or EXE handle. Called from DLL Main message DLL_ATTACH
		static void PluginInit(HANDLE hModule);
		// Performs the instance cleanup: Called from DLL Main message DLL_DETACH
		static void PluginRelease();

		// Returns the Plugin Name. Notepad++ Callback function
		TCHAR* GetName() const { return pluginName.data(); }
		// Returns the Plugin Menu Items count. Notepad++ Callback function
		int GetFunctionCount() const; 
		// Returns the Plugin Menu Items pointer. Notepad++ Callback function
		FuncItem* GetFunctions() const { return pluginFunctions; }
		// Setup Notepad++ and Scintilla handles and finish initializing the
		// plugin's objects that need a Windows Handle to work
		void SetNotepadData(NppData data);

		// Retrieves the unique Plugin's Instance
		static Plugin& Instance() { return *_instance; }

		// ### Internal Objects

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
		// Returns tha handle of Notepad++ Main Menu
		HMENU GetNppMainMenu();

		// ### Message Processing

		// Processes Messages from a Notepad++ editor and the coupled Scintilla Text Editor
		void ProcessMessagesSci(SCNotification* notifyCode);
		// Processes Raw messages from a Notepad++ window (the ones not handled by editor). Currently unused by the plugin
		LRESULT ProcessMessagesNpp(UINT Message, WPARAM wParam, LPARAM lParam);
		// Setup a restart Hook. 0 to reset; 1 to restart normally; 2 to restart in admin mode
		void SetRestartHook(RestartMode type, RestartFunctionHook function = RestartFunctionHook::None) { 
			Settings().iNotepadRestartMode = type; Settings().iNotepadRestartFunction = function; 
		}

		// ### User Interfacing

		// Menu Command "Use Auto-Indentation" function handler. 
		static PLUGINCOMMAND SwitchAutoIndent();
		// Menu Command "Compile Script" function handler. 
		static PLUGINCOMMAND CompileScript();
		// Menu Command "Settings" function handler. 
		static PLUGINCOMMAND OpenSettings();
		// Menu Command "Import NWScript definitions" function handler. 
		static PLUGINCOMMAND ImportDefinitions();
		// Menu Command "Reset Editor Colors"
		static PLUGINCOMMAND ResetEditorColors();
		// Menu Command "Install Dark Theme"
		static PLUGINCOMMAND InstallDarkTheme();
		// Menu Command "About Me" function handler. 
		static PLUGINCOMMAND AboutMe();

	private:
		Plugin(HMODULE dllModule)
			: _dllHModule(dllModule), _notepadHwnd(nullptr) {}

		// Returns TRUE if the current Lexer is one of the plugin's installed lexers
		bool IsPluginLanguage() const { return _notepadCurrentLexer->isPluginLang; }

		// ### Initialization

		// Setup plugin and Notepad++ Auto-Indentation Support.
		// If it's a newer version of notepad++, we use the built-in auto-indentation, or else, we use our customized one
		void SetAutoIndentSupport();
		// Detects Dark Theme installation and setup the menu option for that action accordingly
		void DetectDarkThemeInstall();
		// Load current Notepad++ Language Lexer when language changed.
		// Called from messages: NPPN_READY, NPPN_LANGCHANGED and NPPN_BUFFERACTIVATED
		void LoadNotepadLexer();

		// ### Initialization -> Setup bitmaps
		
		// Set a plugin menu Icon to a given stock Shell Icon
		bool SetStockMenuItemIcon(int commandID, SHSTOCKICONID stockIconID, bool bSetToUncheck, bool bSetToCheck);
		// Setup Menu Icons. Some of them are dynamic shown/hidden.
		void SetupMenuIcons();

		// ### Config files management

		// Import a parsed result from NWScript file definitions into our language XML file. Function HEAVY on error handling!
		static void DoImportDefinitionsCallback(HRESULT decision);
		// Resets the Editor Colors
		void DoResetEditorColors(RestartFunctionHook whichPhase = RestartFunctionHook::None);
		// Install Dark Theme
		void DoInstallDarkTheme(RestartFunctionHook whichPhase = RestartFunctionHook::None);

		// ### User interfacing

		// Do a full file permission check and show appropriate error dialogs to the user when required. Callers can set
		// a function to be called automatically upon a possible program restart if FileCheckResults::RequiresAdminPrivileges is reached
		// and the user selected to "Run as Administrator". If function returns FileCheckResults::RequiresAdminPrivileges, the caller should
		// just quit the current procedure immediately.
		FileCheckResults FilesWritePermissionCheckup(const std::vector<generic_string>& sFiles, RestartFunctionHook iFunctionToCallIfRestart);

		// ### Internal states and variables

		// Unique plugin Instance
		static Plugin* _instance;

		// Internal states

		bool _isReady = false;
		bool _needPluginAutoIndent = false;
		DarkThemeStatus _pluginDarkThemeIs = DarkThemeStatus::Unsupported;

		// Internal classes

		std::unique_ptr<NotepadLexer> _notepadCurrentLexer;
		std::unique_ptr<PluginMessenger> _messageInstance;
		std::unique_ptr<LineIndentor> _indentor;
		std::unique_ptr<NWScriptParser::ScriptParseResults> _NWScriptParseResults;

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
		// Notepad Executable File (eg: %ProgramFiles%\Notepad++\Notepad++.exe)
		generic_string _notepadExecutableFile;
		// Notepad Plugin Installation Path (eg: %ProgramFiles%\Notepad++\plugins\)
		generic_string _notepadPluginHomePath;
		// Notepad Themes Installation Directory (eg: %ProgramFiles%\Notepad++\themes)
		generic_string _notepadThemesInstallDir;
		// Notepad Dark Theme Installation Path (eg: %ProgramFiles%\Notepad++\themes\DarkModeDefault.xml)
		generic_string _notepadDarkThemeFilePath;
		// Notepad Functions AutoComplete Directory (eg: %ProgramFiles%\Notepad++\autoCompletion)
		generic_string _notepadAutoCompleteInstallPath;
		// Notepad Pseudo-Batch to restart Application if needed (eg: %AppData%\Notepad++\plugins\~doNWScriptNotepadRestart.bat)
		generic_string _notepadPseudoBatchRestartFile;

	public:
		// Compilation-time information

		// We are defining this on code and dynamic allocated, hence made it static.
		static FuncItem pluginFunctions[];
		// We are defining this on code and dynamic allocated, hence made it static.
		static generic_string pluginName;
	};

};

