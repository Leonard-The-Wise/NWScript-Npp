/** @file PluginMessenger.cpp
 * Sends message to a Scintilla Text Editor window or a Notepad++ (or other generic) HWND.
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include <vector>

#include "Notepad_plus_msgs.h"
#include "PluginMessenger.h"

using namespace NWScriptPlugin;

// Gets the current working Scintilla Text Editor window from Notepad++
HWND PluginMessenger::GetCurentScintillaHwnd() const
{
    int currentView = -1;
    ::SendMessage(_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&currentView));
    return (currentView == 0) ? _nppData._scintillaMainHandle : _nppData._scintillaSecondHandle;
}

// Returns a handle for Notepad++ Windows.
HWND PluginMessenger::GetNotepadHwnd() const
{
    return _nppData._nppHandle;
}

// Post an array of messages to current Scintilla Text Editor
void PluginMessenger::PostSciMessages(const std::initializer_list<MessageParams>& params) const
{
    const HWND ScintillaHwnd = GetCurentScintillaHwnd();
    for (const auto& Param : params)
        ::PostMessage(ScintillaHwnd, Param.msg, Param.wParam, Param.lParam);
}

// Post an array of messages to current Notepad++ instance
void PluginMessenger::PostNppMessages(const std::initializer_list<MessageParams>& params) const
{
    for (const auto& Param : params)
        ::PostMessage(_nppData._nppHandle, Param.msg, Param.wParam, Param.lParam);
}
