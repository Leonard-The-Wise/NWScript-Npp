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


typedef void(PLUGININTERNALS);
#define PLUGINCOMMAND PLUGININTERNALS


namespace NWScriptPlugin {

	// Plugin Singleton
	class Plugin {
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
				: m_NppData(data) {}

			template<typename T = void>
			constexpr T SendNppMessage(const UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) 
			{
				return static_cast<T>(::SendMessage(m_NppData._nppHandle, msg, wParam, lParam));
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
			
			const NppData m_NppData;
		};

		struct NotepadLanguage {
			const int langID;
			const TCHAR* langName;
			const bool isPluginLang;
			const LangAutoIndentType langIndent;

			explicit NotepadLanguage(const int _LangID, const TCHAR* _LangName, const bool _isPluginLang, const LangAutoIndentType _langIndent)
				: langID(_LangID), langName(_LangName), isPluginLang(_isPluginLang), langIndent(_langIndent) {}

			~NotepadLanguage() { delete[] langName; }
		};

		class LineIndentor
		{
		public:
			static void AutoIndentLine(TCHAR ch);
			static void SetMessenger(Messenger* SciMessenger);

		private:
			LineIndentor();

			static Messenger* pMsg;

			static Sci_CharacterRange getSelection();
			static bool isConditionExprLine(intptr_t lineNumber);
			static intptr_t findMachedBracePos(size_t startPos, size_t endPos, char targetSymbol, char matchedSymbol);
			static void setLineIndent(size_t line, size_t indent);
			static intptr_t getLineLength(size_t line);
			static intptr_t getLineIdent(size_t line);
		};

	public:
		Plugin(Plugin& other) = delete;
		void operator =(const Plugin &) = delete;

		// TODO: Convert all pointer returning functions to References

		// Basic plugin setup;
		static void PluginInit(HANDLE hModule);
		static void PluginCleanUp();
		TCHAR* GetName() const { return pluginName; }
		FuncItem GetFunction(int index) const { return pluginFunctions[index]; }
		FuncItem* GetFunctions() const { return pluginFunctions; }
		int GetFunctionCount() const; // { return (int)std::size(pluginFunctions); }
		static Plugin* Instance() { return m_instance; }
		Settings* Settings() { return m_Settings.get(); }
		Messenger* MessengerInst() const { return m_MessageInstance.get(); }
		LineIndentor* Indentor() const { return m_Indentor.get(); }
		static HMODULE DllHModule() { return m_DllHModule; }
		void ProcessMessagesSci(SCNotification* notifyCode);
		LRESULT ProcessMessagesNpp(UINT Message, WPARAM wParam, LPARAM lParam);
		void SetNotepadData(NppData data);

		// Plugin functionality
		void IsReady(bool ready) { m_isReady = ready; }
		bool IsReady() const { return m_isReady; }
		bool IsPluginLanguage() const { return m_NotepadLanguage->isPluginLang; }
		bool NeedsPluginAutoIndent() const { return m_NeedPluginAutoIndent; }
		NotepadLanguage* GetNotepadLanguage() {	return Instance()->m_NotepadLanguage.get();	}
		void LoadNotepadLanguage();
		void SetAutoIndentSupport();
		HMENU GetNppMainMenu();
		void RemoveAutoIndentMenu();

		// All Menu Command function pointers to export. 
		static PLUGINCOMMAND AutoIndent();
		static PLUGINCOMMAND CompileScript();
		static PLUGINCOMMAND OpenSettings();
		static PLUGINCOMMAND GenerateDefinitions();
		static PLUGINCOMMAND AboutMe();

	private:
		Plugin() {};

		static bool m_isReady;
		static bool m_NeedPluginAutoIndent;
		static std::unique_ptr<NotepadLanguage> m_NotepadLanguage;
		static std::unique_ptr<Messenger> m_MessageInstance;
		static std::unique_ptr<LineIndentor> m_Indentor;
		static Plugin* m_instance;
		static HMODULE m_DllHModule;

		static std::unique_ptr<NWScriptPlugin::Settings> m_Settings;

		static std::unique_ptr<AboutDialog> m_AboutDialog;

		static FuncItem pluginFunctions[];
		static TCHAR pluginName[];
	};

}

