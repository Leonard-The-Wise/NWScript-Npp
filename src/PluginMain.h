/** @file PluginMain.h
 * Definitions for the plugin class
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#pragma once

#include <memory>

#include "SciLexer.h"
#include "Scintilla.h"
#include "PluginInterface.h"

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
			constexpr T SendNppMessage(const UINT msg, const WPARAM wParam = 0, const LPARAM lParam = 0) const
			{
				return static_cast<T>(::SendMessage(m_NppData._nppHandle, msg, wParam, lParam));
			}

			template<typename T = void>
			constexpr T SendSciMessage(const UINT msg, const WPARAM wParam = 0, const LPARAM lParam = 0) const
			{
				const HWND ScintillaHwnd = GetCurentScintillaHwnd();
				return static_cast<T>(::SendMessage(ScintillaHwnd, msg, wParam, lParam));
			}

			void PostNppMessages(const std::initializer_list<MessageParams> params) const;

			void PostSciMessages(const std::initializer_list<MessageParams> params) const;

		private:
			HWND GetCurentScintillaHwnd() const;
			const NppData m_NppData;
		};

		struct NotepadLanguage {
			const int langID;
			const TCHAR* langName;
			const bool isPluginLang;

			explicit NotepadLanguage(const int _LangID, const TCHAR* _LangName, const bool _isPluginLang)
				: langID(_LangID), langName(_LangName), isPluginLang(_isPluginLang) {}

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

		// Basic plugin setup;
		static void PluginInit(HANDLE hModule);
		static void PluginCleanUp();
		TCHAR* GetName() const;
		FuncItem GetFunction(int index) const;
		FuncItem* GetFunctions() const;
		int GetFunctionCount() const;
		static Plugin* Instance();
		Messenger* MessengerInst() const;
		LineIndentor* Indentor() const;
		static HMODULE DllHModule();
		void ProcessMessagesSci(SCNotification* notifyCode);
		LRESULT ProcessMessagesNpp(UINT Message, WPARAM wParam, LPARAM lParam);
		void SetNotepadData(NppData data);

		// Plugin functionality
		void IsReady(bool ready);
		bool IsReady() const;
		bool IsPluginLanguage() const;
		NotepadLanguage* GetNotepadLanguage();
		void LoadNotepadLanguage();

		// All Menu Command function pointers to export. 
		static PLUGINCOMMAND AutoIndent();
		static PLUGINCOMMAND CompileScript();
		static PLUGINCOMMAND Settings();
		static PLUGINCOMMAND GenerateDefinitions();
		static PLUGINCOMMAND AboutMe();

	private:
		Plugin() {};

		static bool m_isReady;
		static bool m_UseAutoIndent;
		static std::unique_ptr<NotepadLanguage> m_NotepadLanguage;
		static std::unique_ptr<Messenger> m_MessageInstance;
		static std::unique_ptr<LineIndentor> m_Indentor;
		static Plugin* m_instance;
		static HMODULE m_DllHModule;

		static std::unique_ptr<AboutDialog> m_AboutDialog;

		static FuncItem pluginFunctions[];
		static TCHAR pluginName[];
	};

}

