/** @file PluginMain.h
 * Definitions for the plugin class
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <vector>
#include <filesystem>

#include "Common.h"
#include "Notepad_plus_msgs.h"
#include "DockingDlgInterface.h"

#include "PluginMessenger.h"
#include "LineIndentor.h"
#include "Settings.h"
#include "NWScriptParser.h"
#include "NWScriptCompiler.h"

#include "AboutDialog.h"
#include "LoggerDialog.h"
#include "ProcessFilesDialog.h"


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
			int langID = 0;
			generic_string langName;
			bool isPluginLang = false;
			ExternalLexerAutoIndentMode langIndent = ExternalLexerAutoIndentMode::Standard;

			void SetLexer(const int _LangID, TCHAR* _LangName, const bool _isPluginLang, const ExternalLexerAutoIndentMode _langIndent) 
			{
				langID = _LangID; 
				langName = _LangName; 
				isPluginLang = _isPluginLang;
				langIndent = _langIndent;
			};
		};

		// File checks results
		enum class PathCheckResults {
			UnknownError = -3, BlockedByApplication, ReadOnly, RequiresAdminPrivileges, CheckSuccess
		};

		enum class DarkThemeStatus {
			Uninstalled, Installed, Unsupported
		};

	public:

		// ### Class Instantiation

		Plugin(Plugin& other) = delete;
		void operator =(const Plugin&) = delete;

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
		void SetNotepadData(NppData& data);

		// Retrieves the unique Plugin's Instance
		static Plugin& Instance() { return *_instance; }

		// ### Internal Objects

		// Retrieve's Plugin's Settings Object
		Settings& Settings() { return _settings; }
		// Retrieve's Plugin's Messenger Object
		PluginMessenger& Messenger() { return _messageInstance; }
		// Retrieve's Plugin's LineIndentor Object
		LineIndentor& Indentor() { return _indentor; }
		// Retrieve the Compiler Object
		NWScriptCompiler& Compiler() { return _compiler; };
		// Retrieve's Plugin's Module Handle
		HMODULE DllHModule() const { return _dllHModule; }
		// Retrieves Notepad++ HWND
		HWND NotepadHwnd() const { return _notepadHwnd; }
		// Returns the handle of Notepad++ Main Menu
		HMENU GetNppMainMenu();

		// ### Message Processing

		// Processes Messages from a Notepad++ editor and the coupled Scintilla Text Editor
		void ProcessMessagesSci(SCNotification* notifyCode);
		// Processes Raw messages from a Notepad++ window (the ones not handled by editor). Currently unused by the plugin
		LRESULT ProcessMessagesNpp(UINT Message, WPARAM wParam, LPARAM lParam);
		// Setup a restart Hook. Normal or Admin mode. Save settings immediately after.
		void SetRestartHook(RestartMode type, RestartFunctionHook function = RestartFunctionHook::None);

		// ### Dynamic behavior

		// Sets the current language to our plugin's lexer language by calling MENU commands.
		void SetNotepadToPluginLexer();
		// Gets the DPI Manager
		DPIManager& DpiManager() {
			return _dpiManager;
		}

		// ### User Interfacing

		// Menu Command "Use auto-indentation" function handler. 
		static PLUGINCOMMAND SwitchAutoIndent();
		// Menu Command "Compile script" function handler. 
		static PLUGINCOMMAND CompileScript();
		// Menu Command "Disassemble file" function handler. 
		static PLUGINCOMMAND DisassembleFile();
		// Menu Command "Compile script" function handler. 
		static PLUGINCOMMAND BatchProcessFiles();
		// Menu Command "Run last successful batch" function handler. 
		static PLUGINCOMMAND RunLastBatch();
		// Menu Command "Run last successful batch" function handler. 
		static PLUGINCOMMAND ToggleLogger();
		// Menu Command "Fetch preprocessor text" function handler. 
		static PLUGINCOMMAND FetchPreprocessorText();
		// Menu Command "View Script Dependencies" function handler. 
		static PLUGINCOMMAND ViewScriptDependencies();
		// Menu Command "Compiler settings" function handler. 
		static PLUGINCOMMAND CompilerSettings();
		// Menu Command "Compiler settings" function handler. 
		static PLUGINCOMMAND UserPreferences();
		// Menu Command "Install dark theme"
		static PLUGINCOMMAND InstallDarkTheme();
		// Menu Command "Import NWScript definitions" function handler. 
		static PLUGINCOMMAND ImportDefinitions();
		// Menu Command "Import user-defined tokens" function handler. 
		static PLUGINCOMMAND ImportUserTokens();
		// Menu Command "Reset user-defined tokens" function handler. 
		static PLUGINCOMMAND ResetUserTokens();
		// Menu Command "Reset editor colors" function handler. 
		static PLUGINCOMMAND ResetEditorColors();
		// Menu Command "Reset editor colors" function handler. 
		static PLUGINCOMMAND RepairFunctionList();
		// Menu Command "Install Plugin's XML Config Files" function handler. 
		static PLUGINCOMMAND InstallAdditionalFiles();
		// Menu Command "About Me" function handler. 
		static PLUGINCOMMAND AboutMe();

	private:
		Plugin(HMODULE dllModule)
			: _dllHModule(dllModule), _notepadHwnd(nullptr), _NWScriptParseResults(nullptr) {}

		~Plugin() {
			for (HBITMAP& b : _menuBitmaps)
				DeleteObject(b);
			for (size_t i = 0; i < std::size(_tbIcons); i++)
			{
				DeleteObject(_tbIcons[i].hToolbarBmp);
				DeleteObject(_tbIcons[i].hToolbarIcon);
				DeleteObject(_tbIcons[i].hToolbarIconDarkMode);
			}
		}

		// Returns TRUE if the current Lexer is one of the plugin's installed lexers
		bool IsPluginLanguage() const { return _notepadCurrentLexer.isPluginLang; }

		// ### Initialization

		// Make plugin's file paths
		void MakePluginFilePaths();
		// Setup plugin and Notepad++ Auto-Indentation Support.
		// If it's a newer version of notepad++, we use the built-in auto-indentation, or else, we use our customized one
		void SetAutoIndentSupport();
		// Detects Dark Theme installation and setup the menu option for that action accordingly. Can auto-restart app if necessary
		RestartMode DetectDarkThemeInstall();
		// Load current Notepad++ Language Lexer when language changed.
		// Called from messages: NPPN_READY, NPPN_LANGCHANGED and NPPN_BUFFERACTIVATED
		void LoadNotepadLexer();
		// Finds the language ID for our plugin
		int FindPluginLangID();
		// Initializes the compiler log window
		void InitCompilerLogWindow();
		// Displays/Hides the compiler window
		void DisplayCompilerLogWindow(bool toShow = true);
		// Checkup Engine Known objects file
		void CheckupPluginObjectFiles();
		// Checks dark mode usage (for legacy Notepad++ 8.3.3 and bellow)
		void CheckDarkModeLegacy();
		// Detects Dark Mode usage (for Notepad++ 8.3.4 and above)
		void RefreshDarkMode(bool ForceUseDark = false, bool UseDark = false);

		// ### Initialization -> Menu handling

		// Removes a menu item by ID or position (this last one used for commands without IDs like separators)
		void RemovePluginMenuItem(int ID, bool byPosition = false);
		// Do a scan to see which options may be removed
		void RemoveUnusedMenuItems();
		// Check Menu item enable state
		bool IsPluginMenuItemEnabled(int ID);
		// Enable/disable menu item
		void EnablePluginMenuItem(int ID, bool enabled);
		// Set plugin menu icon
		void SetPluginMenuBitmap(int commandID, HBITMAP bitmap, bool bSetToUncheck, bool bSetToCheck);
		// Setup Menu Icons. Some of them are dynamic shown/hidden.
		void SetupPluginMenuItems();
		// Lock/Unlock all of the plugin's options
		void LockPluginMenu(bool toLock);

		// ### Dialog Callback functions

		// Import engine definitions callback
		static void ImportDefinitionsCallback(HRESULT decision);
		// Import user tokens callback
		static void ImportUserTokensCallback(HRESULT decision);
		// Batch processing Dialog callback
		static void BatchProcessDialogCallback(HRESULT decision);

		// ### Compiler functionality

		// Checks the current scintilla document to see if it conforms to NWScript Language, then auto-saves the file and prepares it
		// to compilation.
		bool CheckScintillaDocument();
		// Compile/Disassemble script files, simple or batch operations.
		void DoCompileOrDisasm(generic_string filePath = TEXT(""), bool fromCurrentScintilla = false, bool batchOperations = false);
		// Reset batch processing states
		void ResetBatchStates() {
			_batchCurrentFileIndex = 0;
			_batchInterrupt = 0;
			_batchFilesToProcess.clear();
		}
		// Build the batch files list in async thread
		void BuildFilesList();

		// Some callback functions for different operations

		// Receives notifications when a "Compile" menu command ends
		static void CompileEndingCallback(HRESULT decision);
		// Receives notifications when a "Disassemble" menu command ends
		static void DisassembleEndingCallback(HRESULT decision);
		// Receives notifications for each file processed
		static void BatchProcessFilesCallback(HRESULT decision);
		// Receives notifications when a "Fetch preprocessed" menu command ends
		static void FetchPreprocessedEndingCallback(HRESULT decision);
		// Receives notifications when a "Fetch preprocessed" menu command ends
		static void ViewDependenciesEndingCallback(HRESULT decision);
		// Receives log notification messages and write to the compiler window
		static void WriteToCompilerLog(const NWScriptLogger::CompilerMessage& message);
		// Receives notifications from Compiler Window to open files and navigato to text inside it
		static void NavigateToCode(const generic_string& fileName, size_t lineNum, const generic_string& rawMessage,
			const fs::path& filePath = TEXT(""));
		// Reposition the navigation cursor assynchronously
		static void CALLBACK RunScheduledReposition(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

		// ### XML config files management

		// Import a parsed result from NWScript file definitions into our language XML file. Function HEAVY on error handling!
		void DoImportDefinitions();
		// Import a parsed result from User tokens definitions into our language XML file.
		void DoImportUserTokens();
		// Clear all user tokens
		RestartMode DoResetUserTokens(RestartFunctionHook whichPhase = RestartFunctionHook::None);
		// Resets the Editor Colors. Returns a restart mode if necessary.
		RestartMode DoResetEditorColors(RestartFunctionHook whichPhase = RestartFunctionHook::None);
		// Install Dark Theme. Returns a restart mode if necessary.
		RestartMode DoInstallDarkTheme(RestartFunctionHook whichPhase = RestartFunctionHook::None);
		// Repair OverrideMap and FunctionList files. Returns a restart mode if necessary.
		RestartMode DoRepairOverrideMap(RestartFunctionHook whichPhase = RestartFunctionHook::None);
		// Copy required XML files to directories
		RestartMode DoInstallAdditionalFiles(RestartFunctionHook whichPhase = RestartFunctionHook::None);
		// Helper to patch the Dark Theme XML Styler
		bool PatchDarkThemeXMLFile();
		// Helper to patch the Default XML Styler. This is different, since we must preserve user information.
		bool PatchDefaultThemeXMLFile();
		// Helper to merge AutoComplete file
		bool MergeAutoComplete();
		// Patch the OverrideMap XML list
		bool CheckAndPatchOverrideMapXMLFile();

		// ### Dynamic Behavior

		// Do a full file permission check and show appropriate error dialogs to the user when required. Callers can set
		// a function to be called automatically upon a possible program restart if PathCheckResults::RequiresAdminPrivileges is reached
		// and the user selected to "Run as Administrator". If function returns PathCheckResults::RequiresAdminPrivileges, the caller should
		// just quit the current procedure immediately.
		PathCheckResults WritePermissionCheckup(const std::vector<generic_string>& sPaths, RestartFunctionHook iFunctionToCallIfRestart);

		// ### Internal states and variables

		// Unique plugin Instance
		static Plugin* _instance;

		// Internal states

		// General internal states
		bool _isReady = false;
		bool _needPluginAutoIndent = false;
		bool _NppSupportDarkModeMessages = false;
		bool _OneTimeOffer = false;
		bool _OneTimeOfferAccepted = false;
		DarkThemeStatus _pluginDarkThemeIs = DarkThemeStatus::Unsupported;
		ULONGLONG _clockStart = 0;

		// Image handles
		std::vector<HBITMAP> _menuBitmaps;
		toolbarIconsWithDarkMode _tbIcons[3] = {};

		// Internal (global) classes

		NotepadLexer _notepadCurrentLexer;
		PluginMessenger _messageInstance;
		LineIndentor _indentor;
		NWScriptCompiler _compiler;
		tTbData _dockingData;			// needs persistent info for docking data
		HICON _dockingIcon;				// needs persistent info for docking data
		generic_string _dockingTitle;   // needs persistent info for docking data
		std::unique_ptr<NWScriptParser::ScriptParseResults> _NWScriptParseResults;

		// Persistent dialogs
		std::unique_ptr<LoggerDialog> _loggerWindow;
		std::unique_ptr<ProcessFilesDialog> _processingFilesDialog;
		std::unique_ptr<AboutDialog> _aboutDialog;

		// Internal handles

		HMODULE _dllHModule;
		HWND _notepadHwnd;

		// Settings instance
		NWScriptPlugin::Settings _settings;
		DPIManager _dpiManager;

		// Temporary stash to Scintilla document content to be used inside the compilation thread
		std::string _tempFileContents;

		// Batch processing flags
		std::vector<fs::path> _batchFilesToProcess;
		size_t _batchCurrentFileIndex = 0;
		std::atomic<bool> _batchInterrupt = false;

		// Meta Information about the plugin paths
		std::map<std::string, fs::path> _pluginPaths;
		// Plugin module name without extension (eg: NWScript-Npp)
		generic_string _pluginFileName;

	public:
		// Compilation-time information

		// We are defining this on code and dynamic allocated, hence made it static.
		static FuncItem pluginFunctions[];
		// We are defining this on code and dynamic allocated, hence made it static.
		static generic_string pluginName;
	};

};

