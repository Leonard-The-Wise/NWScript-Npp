/** @file PluginMain.cxx
 * Controls the Plugin functions, processes all Plugin messages
 * 
 * The ACTUAL DLL Entry point is defined in PluginInterface.cxx
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include <Windows.h>
#include <sstream>
#include <stdexcept>

#include "Common.h"
#include "LexerCatalogue.h"

#include "PluginMain.h"


using namespace NWScriptPlugin;
using namespace LexerInterface;

TCHAR Plugin::pluginName[] = TEXT("NWScript for Notepad++");

FuncItem Plugin::pluginFunctions[] = {
    {TEXT("Use Auto-Identation"), Plugin::SwitchAutoIndent},
    {TEXT("Compile Script"), Plugin::CompileScript},
    {TEXT("---")},
    {TEXT("Import NWScript definitions"), Plugin::ImportDefinitions},
    {TEXT("Settings..."), Plugin::OpenSettings},
    {TEXT("---")},
    {TEXT("Fix Editor Colors"), Plugin::FixLanguageColors},
    {TEXT("About Me"), Plugin::AboutMe},
};

// Static member definition
Plugin* Plugin::_instance(nullptr);

#pragma region

// Initializes the Plugin (called by Main DLL entry point - ATTACH)
void Plugin::PluginInit(HANDLE hModule)
{
    // Can't be called twice
    if (_instance)
    {
        throw std::runtime_error("Double initialization of a singleton class.");
        return;
    }

    // Instantiate the Plugin
    _instance = new Plugin(static_cast<HMODULE>(hModule));

    // TODO: Load Settings from file
    Instance()._settings = std::make_unique<NWScriptPlugin::Settings>();

    // TODO: 
    Instance().pluginFunctions[0]._init2Check = false;
}

// Cleanup Plugin memory upon deletion (called by Main DLL entry point - DETACH)
void Plugin::PluginRelease()
{
    if (!_instance)
    {
        throw std::runtime_error("Trying to release an uninitalized class.");
        return;
    }

    delete _instance;
    _instance = nullptr;
}

// Return the number of Menu Functions Count. Can't be inline because pluginFunctions is defined outside class
int Plugin::GetFunctionCount() const
{
    return static_cast<int>(std::size(pluginFunctions));
}

// Setup Notepad++ and Scintilla handles and finish initializing the
// plugin's objects that need a Windows Handle to work
void Plugin::SetNotepadData(NppData data)
{
    // Init Notepad-Scintilla Messender
	if (!Instance()._messageInstance)
	{
		Instance()._messageInstance = std::make_unique<PluginMessenger>(data);
        Instance()._indentor = std::make_unique<LineIndentor>(*(_messageInstance.get()));
	}

    //Keep a copy of Notepad HWND to easy access
    _notepadHwnd = Instance().Messenger().GetNotepadHwnd();
}

#pragma endregion Plugin internal processing

#pragma region 

// Processes Notepad++ specific messages. Currently unused by this Plugin
LRESULT Plugin::ProcessMessagesNpp(UINT Message, WPARAM wParam, LPARAM lParam)
{
    return TRUE;
}

// Processes all Notepad++ and Scintilla messages. Newer versions of Notepad++ will 
// send messages through this function.
void Plugin::ProcessMessagesSci(SCNotification* notifyCode)
{
    switch (notifyCode->nmhdr.code)
    {
        case NPPN_READY:
            IsReady(true);
            SetupMenuIcons();
            SetAutoIndentSupport();
            LoadNotepadLexer();
            break;

        // TODO: Save Configurations file
        case NPPN_SHUTDOWN:
            break;

        case NPPN_LANGCHANGED:
            LoadNotepadLexer();
            break;

        case NPPN_BUFFERACTIVATED:
            if (IsReady())
                LoadNotepadLexer();
            break;

        case SCN_CHARADDED:
            // Conditions to perform the Auto-Indent:
            // - Notepad is in Ready state;
            // - Current Language is set to one of the plugin supported langs
            // - Notepad version doesn't yet support Extended AutoIndent functionality
            if (IsReady() && IsPluginLanguage() && Instance().NeedsPluginAutoIndent()
                && Instance().Settings().bEnableAutoIndentation)
				    Instance().Indentor().IndentLine(static_cast<TCHAR>(notifyCode->ch));
            break;

        default:
            return;
    }
}

#pragma endregion Message Processing


#pragma region 

// Get Current notepad lexer language;
void Plugin::LoadNotepadLexer()
{
    PluginMessenger& msg = Messenger();
    bool lexerSearch = FALSE;
    bool isPluginLanguage = FALSE;
    int currLang = 0;
    LangAutoIndentType langIndent = LangAutoIndentType::Standard;
    msg.SendNppMessage<>(NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&currLang);

    // First call: retrieve buffer size. Second call, fill up name (from Manual).
    int buffSize = msg.SendNppMessage<int>(NPPM_GETLANGUAGENAME, currLang, reinterpret_cast<LPARAM>(nullptr));
    TCHAR* lexerName = new TCHAR[buffSize+1];
    msg.SendNppMessage<void>(NPPM_GETLANGUAGENAME, currLang, reinterpret_cast<LPARAM>(lexerName));

    // Try to get Language Auto-Indentation if it's one of the plugin installed languages
    generic_stringstream lexerNameW;
    for (int i = 0; i < LexerCatalogue::GetLexerCount() && lexerSearch == false; i++)
    {
        lexerNameW = {};
        lexerNameW << LexerCatalogue::GetLexerName(i);
        isPluginLanguage = (_tcscmp(lexerName, lexerNameW.str().c_str()) == 0);

        if (isPluginLanguage)
            lexerSearch = msg.SendNppMessage<int>(NPPM_GETLANGUAGEAUTOINDENTATION,
                reinterpret_cast<WPARAM>(lexerNameW.str().c_str()), reinterpret_cast<LPARAM>(&langIndent));
    }

    // Create or Replace current lexer language.
    _notepadLexer = std::make_unique<NotepadLexer>(currLang, lexerName, isPluginLanguage, langIndent);
}

// Set the Auto-Indent type for all of this Plugin's installed languages
void Plugin::SetAutoIndentSupport()
{
    PluginMessenger& msg = Instance().Messenger();
    bool lexerSearch = false;
    LangAutoIndentType langIndent = LangAutoIndentType::Standard;

    // Try to set Plugin's Lexers Auto-Indentation. Older versions of NPP will return langSearch=FALSE (cause this message won't exist).
    generic_stringstream lexerNameW;
    for (int i = 0; i < LexerCatalogue::GetLexerCount(); i++)
    {
        lexerNameW = {};
        lexerNameW << LexerCatalogue::GetLexerName(i);

        lexerSearch = msg.SendNppMessage<int>(NPPM_GETLANGUAGEAUTOINDENTATION,
            reinterpret_cast<WPARAM>(lexerNameW.str().c_str()), reinterpret_cast<LPARAM>(&langIndent));

        if (lexerSearch)
        {
            if (langIndent != LangAutoIndentType::Extended)
            {
                bool success = msg.SendNppMessage<bool>(NPPM_SETLANGUAGEAUTOINDENTATION,
                    reinterpret_cast<WPARAM>(lexerNameW.str().c_str()), static_cast<LPARAM>(LangAutoIndentType::Extended));

                // We got a problem here. Our procedure SHOULD succeed in setting this.
                if (!success)
                {
                    generic_stringstream sWarning = {};
                    sWarning << TEXT("Warning: failed to set Auto-Indentation for language [") << lexerNameW.str().c_str() << "].";
                    MessageBox(NULL, reinterpret_cast<LPCWSTR>(sWarning.str().c_str()),
                           TEXT("NWScript Plugin"), MB_OK | MB_ICONWARNING);
                    return;
                }
            }
        }
    }

    // lexerSearch will be TRUE if Notepad++ support NPPM_GETLANGUAGEAUTOINDENTATION message
    if (lexerSearch)
    {
        Instance()._needPluginAutoIndent = false;

        // Remove the "Use Auto-Indent" menu command
        RemoveAutoIndentMenu();
        // Auto-adjust the settings
        Instance().Settings().bEnableAutoIndentation = false;
    }
    else
        Instance()._needPluginAutoIndent = true;
}

#define NOTEPADPLUS_USER_INTERNAL  (WM_USER + 0000)
#define NPPM_INTERNAL_GETMENU      (NOTEPADPLUS_USER_INTERNAL + 14)

HMENU Plugin::GetNppMainMenu()
{
    PluginMessenger& pMsg = Messenger();
    HMENU hMenu;

    // Notepad++ ver > 6.3
    hMenu = reinterpret_cast<HMENU>(pMsg.SendNppMessage<LRESULT>(NPPM_GETMENUHANDLE, 1, 0));
    if (hMenu && IsMenu(hMenu))
        return hMenu;

    // Notepad++ ver <= 6.3
    hMenu = reinterpret_cast<HMENU>(pMsg.SendNppMessage<LRESULT>(NPPM_INTERNAL_GETMENU, 0, 0));
    if (hMenu && IsMenu(hMenu))
        return hMenu;

    return ::GetMenu(Instance().NotepadHwnd());
}

void Plugin::RemoveAutoIndentMenu()
{
    HMENU hMenu = Instance().GetNppMainMenu();
    if (hMenu)
    {
        RemoveMenu(hMenu, pluginFunctions[0]._cmdID, MF_BYCOMMAND);
    }
}

void Plugin::SetupMenuIcons()
{

    // TODO: Setup the shield icon ONLY when user needs privilege to access file

    HMENU hMenu = Instance().GetNppMainMenu();
    if (hMenu)
    {
        HBITMAP iShield = GetStockIconBitmap(SHSTOCKICONID::SIID_SHIELD, IconSize::Size16x16);
        MENUITEMINFO iChange;
        ZeroMemory(&iChange, sizeof(iChange));
        iChange.cbSize = sizeof(iChange);

        if (GetMenuItemInfo(hMenu, Instance().GetFunctions()[6]._cmdID, FALSE, &iChange))
        {
            iChange.fMask = MIIM_BITMAP;
            iChange.hbmpItem = iShield;
            SetMenuItemInfo(hMenu, Instance().GetFunctions()[6]._cmdID, FALSE, &iChange);
        }
    
    }

}

// Opens the About dialog
void Plugin::OpenAboutDialog()
{
    if (!Instance()._aboutDialog)
    {
        Instance()._aboutDialog = std::make_unique<AboutDialog>();
        Instance()._aboutDialog->init(Instance().DllHModule(), Instance().NotepadHwnd());
    }

    Instance()._aboutDialog->doDialog();
}

void Plugin::OpenWarningDialog()
{
    if (!Instance()._warningDialog)
    {
        Instance()._warningDialog = std::make_unique<WarningDialog>();
        Instance()._warningDialog->init(Instance().DllHModule(), Instance().NotepadHwnd());
    }

    Instance()._warningDialog->doDialog();
}

#pragma endregion Plugin Funcionality



// Support for Auto-Indentation for old versions of Notepad++
PLUGINCOMMAND Plugin::SwitchAutoIndent()
{
    // Change settings
    Instance().Settings().bEnableAutoIndentation = !Instance().Settings().bEnableAutoIndentation;
    bool bEnableAutoIndent = Instance().Settings().bEnableAutoIndentation;

    HMENU hMenu = Instance().GetNppMainMenu();
    if (hMenu)
    {
        CheckMenuItem(hMenu, pluginFunctions[0]._cmdID,
            MF_BYCOMMAND | ((bEnableAutoIndent) ? MF_CHECKED : MF_UNCHECKED));
    }

    // Already accepted the warning, either on this session or a previous one
    if (Instance().Settings().bAutoIndentationWarningAccepted)
        return;

    // Warning user of function
    if (bEnableAutoIndent && !Instance().Settings().bAutoIndentationWarningShown)
        Instance().OpenWarningDialog();

    Instance().Settings().bAutoIndentationWarningShown = true;
}

// Compiles current .NSS Script file
PLUGINCOMMAND Plugin::CompileScript()
{
    MessageBox(NULL, TEXT("Coming soon! :)"), TEXT("NWScript Compiler"), MB_OK | MB_ICONINFORMATION);
}

// Opens the Plugin's Settings panel
PLUGINCOMMAND Plugin::OpenSettings()
{
    MessageBox(NULL, TEXT("Not yet implemented"), TEXT("NWScript Settings"), MB_OK);
}

// Imports Lexer's functions and constants declarations from a NWScript.nss file
PLUGINCOMMAND Plugin::ImportDefinitions()
{
    std::unique_ptr<generic_string> nFileName = std::make_unique<generic_string>();
    if (OpenFileDialog(Instance().NotepadHwnd(), TEXT("NWScritpt.nss File\0nwscript.nss\0"), *nFileName.get()))
    {
        
    }
}

// Fixes the language colors to default
PLUGINCOMMAND Plugin::FixLanguageColors()
{

}

// Opens About Box
PLUGINCOMMAND Plugin::AboutMe()
{
    Instance().OpenAboutDialog();
}

