/** @file PluginMain.cxx
 * Controls the Plugin functions, processes all Plugin messages
 * 
 * The ACTUAL DLL Entry point is defined in PluginInterface.cxx
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include <vector>

#include "Notepad_plus_msgs.h"
#include "PluginMain.h"

using namespace NWScriptPlugin;

TCHAR Plugin::pluginName[] = TEXT("NWScript for Notepad++");

FuncItem Plugin::pluginFunctions[] = {
    {TEXT("Use Auto-Identation"), Plugin::SwitchAutoIndent},
    {TEXT("Compile Script"), Plugin::CompileScript},
    {TEXT("---")},
    {TEXT("Import NWScript definitions"), Plugin::GenerateDefinitions},
    {TEXT("Settings..."), Plugin::OpenSettings},
    {TEXT("---")},
    {TEXT("About Me"), Plugin::AboutMe},
};

#pragma region


// Static member definition
Plugin* Plugin::_instance(nullptr);


HWND Plugin::Messenger::GetCurentScintillaHwnd() const
{
    int currentView = -1;
    ::SendMessage(_NppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&currentView));
    return (currentView == 0) ? _NppData._scintillaMainHandle : _NppData._scintillaSecondHandle;
}

HWND Plugin::Messenger::GetNotepadHwnd() const
{
    return _NppData._nppHandle;
}

void Plugin::Messenger::PostSciMessages(const std::initializer_list<MessageParams> params) const
{
    const HWND ScintillaHwnd = GetCurentScintillaHwnd();
    for (const auto& Param : params)
        ::PostMessage(ScintillaHwnd, Param.msg, Param.wParam, Param.lParam);
}

void Plugin::Messenger::PostNppMessages(const std::initializer_list<MessageParams> params) const
{
    for (const auto& Param : params)
        ::PostMessage(_NppData._nppHandle, Param.msg, Param.wParam, Param.lParam);
}

// Initialize plugin Singleton
void Plugin::PluginInit(HANDLE hModule)
{
    // Instantiate the Plugin
    if (!_instance)
        _instance = new Plugin();

    // Set HMODULE
	Instance()._DllHModule = (HMODULE)hModule;

    // TODO: Load Settings from file
    Instance()._Settings = std::make_unique<NWScriptPlugin::Settings>();

    // TODO: 
    Instance().pluginFunctions[0]._init2Check = false;
}

// Delete Singleton
void Plugin::PluginCleanUp()
{
    delete _instance;
    _instance = nullptr;
}

int Plugin::GetFunctionCount() const
{
    return (int)std::size(Instance().pluginFunctions);
}

void Plugin::SetNotepadData(NppData data)
{
    // Init Notepad-Scintilla Messender
	if (!Instance()._MessageInstance)
	{
		Instance()._MessageInstance = std::make_unique<Messenger>(data);
        Instance()._Indentor = std::make_unique<LineIndentor>();
		Instance().Indentor().SetMessenger(_MessageInstance.get());
	}

    // Init Plugin Window Instances
    Instance()._AboutDialog = std::make_unique<AboutDialog>();
    Instance()._AboutDialog->init(_DllHModule, data._nppHandle);
}

#pragma endregion Plugin internal processing

#pragma region 

LRESULT Plugin::ProcessMessagesNpp(UINT Message, WPARAM wParam, LPARAM lParam)
{
    return TRUE;
}

void Plugin::ProcessMessagesSci(SCNotification* notifyCode)
{
    switch (notifyCode->nmhdr.code)
    {
        case NPPN_READY:
            IsReady(true);
            SetAutoIndentSupport();
            LoadNotepadLanguage();
            break;

        // TODO: Save Configurations file
        case NPPN_SHUTDOWN:
            break;

        case NPPN_LANGCHANGED:
            LoadNotepadLanguage();
            break;

        case NPPN_BUFFERACTIVATED:
            if (IsReady())
               LoadNotepadLanguage();
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

void Plugin::LoadNotepadLanguage()
{
    // Get Current notepad language;
    Messenger& msg = Instance().MessengerInst();
    bool langSearch = FALSE;
    int currLang = 0;
    LangAutoIndentType langIndent = LangAutoIndentType::Standard;
    msg.SendNppMessage<>(NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&currLang);

    // First call: retrieve buffer size. Second call, fill up name (from Manual).
    int buffSize = msg.SendNppMessage<int>(NPPM_GETLANGUAGENAME, currLang, reinterpret_cast<LPARAM>(nullptr));
    TCHAR* langName = new TCHAR[buffSize+1];
    msg.SendNppMessage<void>(NPPM_GETLANGUAGENAME, currLang, reinterpret_cast<LPARAM>(langName));

    // Try to get Language Auto-Indentation
    if (_tcscmp(langName, TEXT("NWScript"))==0)
        langSearch = msg.SendNppMessage<int>(NPPM_GETLANGUAGEAUTOINDENTATION,
            reinterpret_cast<WPARAM>(langName), reinterpret_cast<LPARAM>(&langIndent));

    _NotepadLanguage.release();
    _NotepadLanguage = std::make_unique<NotepadLanguage>(currLang, langName, _tcscmp(langName, TEXT("NWScript"))==0, langIndent);
}

void Plugin::SetAutoIndentSupport()
{
    Messenger& msg = Instance().MessengerInst();
    bool langSearch = false;
    LangAutoIndentType langIndent = LangAutoIndentType::Standard;

    // Try to get Language Auto-Indentation. Older versions of NPP will return FALSE (cause this message won't exist).
    langSearch = msg.SendNppMessage<int>(NPPM_GETLANGUAGEAUTOINDENTATION,
            reinterpret_cast<WPARAM>(TEXT("NWScript")), reinterpret_cast<LPARAM>(&langIndent));

    if (langSearch)
    {
        if (langIndent != LangAutoIndentType::Extended)
        {
            bool success = msg.SendNppMessage<bool>(NPPM_SETLANGUAGEAUTOINDENTATION,
                reinterpret_cast<WPARAM>(TEXT("NWScript")), (LPARAM)LangAutoIndentType::Extended);

            if (success)
            {
                Instance()._NeedPluginAutoIndent = false;

                // Remove the "Use Auto-Indent" menu command
                RemoveAutoIndentMenu();
                // Auto-adjust the settings
                Instance().Settings().bEnableAutoIndentation = false;
            }
        }        
    }
    else
    {
        Instance()._NeedPluginAutoIndent = true;
    }
}

#define NOTEPADPLUS_USER_INTERNAL  (WM_USER + 0000)
#define NPPM_INTERNAL_GETMENU      (NOTEPADPLUS_USER_INTERNAL + 14)

HMENU Plugin::GetNppMainMenu()
{
    Messenger& pMsg = Instance().MessengerInst();
    HMENU hMenu;

    // Notepad++ ver > 6.3
    hMenu = reinterpret_cast<HMENU>(pMsg.SendNppMessage<LRESULT>(NPPM_GETMENUHANDLE, 1, 0));
    if (hMenu && IsMenu(hMenu))
        return hMenu;

    // Notepad++ ver <= 6.3
    hMenu = reinterpret_cast<HMENU>(pMsg.SendNppMessage<LRESULT>(NPPM_INTERNAL_GETMENU, 0, 0));
    if (hMenu && IsMenu(hMenu))
        return hMenu;

    return ::GetMenu(Instance().MessengerInst().GetNotepadHwnd());
}

void Plugin::RemoveAutoIndentMenu()
{
    HMENU hMenu = Instance().GetNppMainMenu();
    if (hMenu)
    {
        RemoveMenu(hMenu, pluginFunctions[0]._cmdID, MF_BYCOMMAND);
    }
}

#pragma endregion Plugin Funcionality






// Command for old versions of Notepad++
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

    // Huge message box of incompatibility: Only shown ONCE.
    if (bEnableAutoIndent && !Instance().Settings().bAutoIndentationWarningShown)
        MessageBox(NULL, TEXT("Current Auto-Indentation feature has conflicts with Notepad++. \
Please disable Notepad++'s \"Use Auto-Indent\" feature on:\r\n\
Settings -> Preferences -> Auto-Completion -> Auto-Indention\r\n\
while editting NSS Script files."), TEXT("NWScript Auto-Indentor"), MB_ICONWARNING | MB_OK);

    Instance().Settings().bAutoIndentationWarningShown = true;
}

PLUGINCOMMAND Plugin::CompileScript()
{
    MessageBox(NULL, TEXT("Coming soon! :)"), TEXT("NWScript Compiler"), MB_OK | MB_ICONINFORMATION);
}

PLUGINCOMMAND Plugin::OpenSettings()
{
    MessageBox(NULL, TEXT("Not yet implemented"), TEXT("NWScript Settings"), MB_OK);
}

PLUGINCOMMAND Plugin::GenerateDefinitions()
{
    MessageBox(NULL, TEXT("Not yet implemented"), TEXT("NWScript Generation"), MB_OK);
}

PLUGINCOMMAND Plugin::AboutMe()
{
    Instance()._AboutDialog->doDialog();
}

