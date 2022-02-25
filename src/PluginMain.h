/** @file PluginMain.h
 * Definitions for the plugin class
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#pragma once

#include <memory>
#include <vector>

#include "SciLexer.h"
#include "Scintilla.h"
#include "PluginInterface.h"
#include "Notepad_plus_msgs.h"

#include "Settings.h"
#include "AboutDialog.h"
#include "WarningDialog.h"


typedef void(PLUGININTERNALS);
#define PLUGINCOMMAND PLUGININTERNALS


namespace NWScriptPlugin {

	// Plugin Singleton
	class Plugin final {
	private:

		struct MessageParams {
			UINT msg;
			WPARAM wParam;
			LPARAM lParam;
		};

		class Messenger
		{
		public:
			explicit Messenger(const NppData& data)
				: _nppData(data) {}

			template<typename T = void>
			constexpr T SendNppMessage(const UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) 
			{
				return static_cast<T>(::SendMessage(_nppData._nppHandle, msg, wParam, lParam));
			}

			template<typename T = void>
			constexpr T SendSciMessage(const UINT msg, WPARAM wParam = 0, LPARAM lParam = 0)
			{
				const HWND ScintillaHwnd = GetCurentScintillaHwnd();
				return static_cast<T>(::SendMessage(ScintillaHwnd, msg, wParam, lParam));
			}

			void PostNppMessages(const std::initializer_list<MessageParams> params) const;
			void PostSciMessages(const std::initializer_list<MessageParams> params) const;
			HWND GetCurentScintillaHwnd() const;
			HWND GetNotepadHwnd() const;

		private:
			
			const NppData _nppData;
		};

		struct NotepadLexer {
			const int langID;
			const TCHAR* langName;
			const bool isPluginLang;
			const LangAutoIndentType langIndent;

			explicit NotepadLexer(const int _LangID, const TCHAR* _LangName, const bool _isPluginLang, const LangAutoIndentType _langIndent)
				: langID(_LangID), langName(_LangName), isPluginLang(_isPluginLang), langIndent(_langIndent) {}

			~NotepadLexer() { delete[] langName; }
		};

		class LineIndentor
		{
		public:
			LineIndentor() 
				:	pMsg(nullptr) {}
			void IndentLine(TCHAR ch);
			void SetMessenger(Messenger* SciMessenger);
			Messenger& Msg() { return *pMsg; }

		private:
			Messenger* pMsg;

			Sci_CharacterRange getSelection();
			bool isConditionExprLine(intptr_t lineNumber);
			intptr_t findMachedBracePos(size_t startPos, size_t endPos, char targetSymbol, char matchedSymbol);
			void setLineIndent(size_t line, size_t indent);
			intptr_t getLineLength(size_t line);
			intptr_t getLineIdent(size_t line);
		};

	public:
		Plugin(Plugin& other) = delete;
		void operator =(const Plugin &) = delete;

		// Basic plugin setup;
		static void PluginInit(HANDLE hModule);
		static void PluginCleanUp();
		TCHAR* GetName() const { return pluginName; }
		FuncItem GetFunction(int index) const { return pluginFunctions[index]; }
		FuncItem* GetFunctions() const { return pluginFunctions; }
		int GetFunctionCount() const; // { return (int)std::size(pluginFunctions); }
		static Plugin& Instance() { return *(_instance); }
		Settings& Settings() { return *(_settings.get()); }
		Messenger& MessengerInst() const { return *(_messageInstance.get()); }
		LineIndentor& Indentor() const { return *(_indentor.get()); }
		HMODULE DllHModule() { return _dllHModule; }
		void ProcessMessagesSci(SCNotification* notifyCode);
		LRESULT ProcessMessagesNpp(UINT Message, WPARAM wParam, LPARAM lParam);
		void SetNotepadData(NppData data);

		// Plugin functionality
		void IsReady(bool ready) { _isReady = ready; }
		bool IsReady() const { return _isReady; }
		bool IsPluginLanguage() const { return _notepadLexer->isPluginLang; }
		bool NeedsPluginAutoIndent() const { return _needPluginAutoIndent; }
		NotepadLexer* GetNotepadLanguage() {	return Instance()._notepadLexer.get();	}
		void LoadNotepadLanguage();
		void SetAutoIndentSupport();
		HMENU GetNppMainMenu();
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

		bool _isReady;
		bool _needPluginAutoIndent;
		std::unique_ptr<NotepadLexer> _notepadLexer;
		std::unique_ptr<Messenger> _messageInstance;
		std::unique_ptr<LineIndentor> _indentor;
		static Plugin* _instance;
		HMODULE _dllHModule;

		std::unique_ptr<NWScriptPlugin::Settings> _settings;

		std::unique_ptr<AboutDialog> _aboutDialog;
		std::unique_ptr<WarningDialog> _warningDialog;

		// We are defining this on code and dynamic allocated, hence made them static
		static FuncItem pluginFunctions[];
		static TCHAR pluginName[];
	};

}

