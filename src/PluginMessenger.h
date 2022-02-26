/** @file PluginMessenger.h
 * Sends message to a Scintilla Text Editor window or a Notepad++ (or other generic) HWND.
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "PluginInterface.h"

namespace NWScriptPlugin {

	// Class to Send messages for Notepad++ and Scintila Text Editor windows.
	class PluginMessenger final
	{
	public:
		// Message Parameters for PostMessage types of functions
		struct MessageParams {
			UINT msg;
			WPARAM wParam;
			LPARAM lParam;
		};

		explicit PluginMessenger(const NppData& data)
			: _nppData(data) {}

		// Send a message to current Notepad++ window and return a T = typename value.
		template<typename T = void>
		constexpr T SendNppMessage(const UINT msg, WPARAM wParam = 0, LPARAM lParam = 0)
		{
			return static_cast<T>(::SendMessage(_nppData._nppHandle, msg, wParam, lParam));
		}

		// Send a message to current Scintila Text Editor window and return a T = typename value.
		template<typename T = void>
		constexpr T SendSciMessage(const UINT msg, WPARAM wParam = 0, LPARAM lParam = 0)
		{
			const HWND ScintillaHwnd = GetCurentScintillaHwnd();
			return static_cast<T>(::SendMessage(ScintillaHwnd, msg, wParam, lParam));
		}

		// Post an array of messages to current Notepad++ instance (and don't wait for return).
		void PostNppMessages(const std::initializer_list<MessageParams>& params) const;
		// Post an array of messages to current Scintilla Text Editor (and don't wait for return).
		void PostSciMessages(const std::initializer_list<MessageParams>& params) const;
		// Gets the current working Scintilla Text Editor window from Notepad++.
		HWND GetCurentScintillaHwnd() const;
		// Returns a handle for Notepad++ Windows.
		HWND GetNotepadHwnd() const;

	private:

		const NppData _nppData;
	};
}
