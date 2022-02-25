/** @file PluginMain.h
 * Definitions for the plugin class
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#pragma once

#include <memory>
#include <vector>

#include "PluginMessenger.h"
#include "LineIndentor.h"
#include "Settings.h"

#include "AboutDialog.h"
#include "WarningDialog.h"

typedef void(PLUGININTERNALS);
#define PLUGINCOMMAND PLUGININTERNALS

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
			const LangAutoIndentType langIndent;

			explicit NotepadLexer(const int _LangID, const TCHAR* _LangName, const bool _isPluginLang, const LangAutoIndentType _langIndent)
				: langID(_LangID), langName(_LangName), isPluginLang(_isPluginLang), langIndent(_langIndent) {}

			~NotepadLexer() { delete[] langName; }
		};

	public:
		Plugin(Plugin& other) = delete;
		void operator =(const Plugin &) = delete;

		// Basic plugin setup;

		// Initializes the Plugin Instance with a DLL or EXE handle. Called from DLL Main message DLL_ATTACH
		static void PluginInit(HANDLE hModule);
		// Performs the instance cleanup: Called from DLL Main message DLL_DETACH
		static void PluginCleanUp();
		// Returns the Plugin Name. Notepad++ Callback function
		TCHAR* GetName() const { return pluginName; }
		// Returns the Plugin Menu Items count. Notepad++ Callback function
		int GetFunctionCount() const; 
		// Returns the Plugin Menu Items pointer. Notepad++ Callback function
		FuncItem* GetFunctions() const { return pluginFunctions; }

		// Retrieves the unique Plugin's Instance
		static Plugin& Instance() { return *(_instance); }

		// Retrieve's Plugin's Settings Object
		Settings& Settings() { return *(_settings.get()); }
		// Retrieve's Plugin's Messenger Object
		PluginMessenger& Messenger() const { return *(_messageInstance.get()); }
		LineIndentor& Indentor() const { return *(_indentor.get()); }
		HMODULE DllHModule() { return _dllHModule; }
		void ProcessMessagesSci(SCNotification* notifyCode);
		LRESULT ProcessMessagesNpp(UINT Message, WPARAM wParam, LPARAM lParam);
		void SetNotepadData(NppData data);

		// Plugin functionality

		// Sets the Plugin Ready state (after NPPN_READY)
		void IsReady(bool ready) { _isReady = ready; }
		// Reads the current Plugin Ready state
		bool IsReady() const { return _isReady; }
		// Returns TRUE if the current Lexer is one of the plugin's installed lexers
		bool IsPluginLanguage() const { return _notepadLexer->isPluginLang; }
		// Checks if the current version of Notepad++ requires a custom indentator
		bool NeedsPluginAutoIndent() const { return _needPluginAutoIndent; }

		// Load current Notepad++ Language Lexer.
		// Called from messages: NPPN_READY, NPPN_LANGCHANGED and NPPN_BUFFERACTIVATED
		void LoadNotepadLexer();
		// Setup plugin and Notepad++ Auto-Indentation Support.
		// If it's a newer version of notepad++, we use the built-in auto-indentation, or else, we use our customized one
		void SetAutoIndentSupport();
		// Returns tha handle of Notepad++ Main Menu
		HMENU GetNppMainMenu();
		// Removes Plugin's Auto-Indentation menu command (for newer versions of Notepad++)
		void RemoveAutoIndentMenu();

		// All Menu Command function pointers to export. 
		static PLUGINCOMMAND SwitchAutoIndent();
		static PLUGINCOMMAND CompileScript();
		static PLUGINCOMMAND OpenSettings();
		static PLUGINCOMMAND GenerateDefinitions();
		static PLUGINCOMMAND AboutMe();

	private:
		Plugin()
			: _isReady(false), _needPluginAutoIndent(true), _dllHModule() {}

		static Plugin* _instance;

		bool _isReady;
		bool _needPluginAutoIndent;
		std::unique_ptr<NotepadLexer> _notepadLexer;
		std::unique_ptr<PluginMessenger> _messageInstance;
		std::unique_ptr<LineIndentor> _indentor;
		HMODULE _dllHModule;

		std::unique_ptr<NWScriptPlugin::Settings> _settings;

		std::unique_ptr<AboutDialog> _aboutDialog;
		std::unique_ptr<WarningDialog> _warningDialog;

		// We are defining this on code and dynamic allocated, hence made it static.
		static FuncItem pluginFunctions[];
		// We are defining this on code and dynamic allocated, hence made it static.
		static TCHAR pluginName[];
	};

}

