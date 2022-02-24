/** @file PluginMain.cxx
 * Controls the Plugin functions
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

TCHAR Plugin::pluginName[] = TEXT("NWScript for Npp");

FuncItem Plugin::pluginFunctions[] = {
    {TEXT("Use Auto-Identation"), Plugin::AutoIndent},
    {TEXT("Compile Script"), Plugin::CompileScript},
    {TEXT("---")},
    {TEXT("Import NWScript definitions"), Plugin::GenerateDefinitions},
    {TEXT("Settings..."), Plugin::OpenSettings},
    {TEXT("---")},
    {TEXT("About Me"), Plugin::AboutMe},
};

#pragma region


// Static members definitions
Plugin* Plugin::m_instance(nullptr);
HMODULE Plugin::m_DllHModule(nullptr);
bool Plugin::m_isReady(false);
bool Plugin::m_NeedPluginAutoIndent(true);
std::unique_ptr<Plugin::Messenger> Plugin::m_MessageInstance(nullptr);
std::unique_ptr<Plugin::NotepadLanguage> Plugin::m_NotepadLanguage(nullptr);
std::unique_ptr<Plugin::LineIndentor> Plugin::m_Indentor(nullptr);
std::unique_ptr<AboutDialog> Plugin::m_AboutDialog(nullptr);
std::unique_ptr<Settings> Plugin::m_Settings(nullptr);

HWND Plugin::Messenger::GetCurentScintillaHwnd() const
{
    int currentView = -1;
    ::SendMessage(m_NppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&currentView));
    return (currentView == 0) ? m_NppData._scintillaMainHandle : m_NppData._scintillaSecondHandle;
}

HWND Plugin::Messenger::GetNotepadHwnd() const
{
    return m_NppData._nppHandle;
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
        ::PostMessage(m_NppData._nppHandle, Param.msg, Param.wParam, Param.lParam);
}

// Initialize plugin Singleton
void Plugin::PluginInit(HANDLE hModule)
{
    // Instantiate the Plugin
    if (!m_instance)
        m_instance = new Plugin();

    // Set HMODULE
	Instance()->m_DllHModule = (HMODULE)hModule;

    Instance()->m_Settings = std::make_unique<NWScriptPlugin::Settings>();
    // TODO: Load Settings from file

    // HACK: setting menu visibility outside Settings class
    pluginFunctions[0]._init2Check = false;

}

// Delete Singleton
void Plugin::PluginCleanUp()
{
    delete m_instance;
    m_instance = nullptr;
}

int Plugin::GetFunctionCount() const
{
    return (int)std::size(Instance()->pluginFunctions);
}

void Plugin::SetNotepadData(NppData data)
{
    // Init Notepad-Scintilla Messender
	if (!Instance()->m_MessageInstance)
	{
		Instance()->m_MessageInstance = std::make_unique<Messenger>(data);
		Instance()->Indentor()->SetMessenger(m_MessageInstance.get());
	}

    // Init Plugin Window Instances
    Instance()->m_AboutDialog = std::make_unique<AboutDialog>();
    Instance()->m_AboutDialog->init(m_DllHModule, data._nppHandle);
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
            if (IsReady() && IsPluginLanguage() && Instance()->NeedsPluginAutoIndent())
				Instance()->Indentor()->AutoIndentLine(static_cast<TCHAR>(notifyCode->ch));
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
    Messenger* msg = Instance()->MessengerInst();
    bool langSearch = FALSE;
    int currLang = 0;
    LangAutoIndentType langIndent = LangAutoIndentType::Standard;
    msg->SendNppMessage<>(NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&currLang);

    // First call: retrieve buffer size. Second call, fill up name (from Manual).
    int buffSize = msg->SendNppMessage<int>(NPPM_GETLANGUAGENAME, currLang, reinterpret_cast<LPARAM>(nullptr));
    TCHAR* langName = new TCHAR[buffSize+1];
    msg->SendNppMessage<void>(NPPM_GETLANGUAGENAME, currLang, reinterpret_cast<LPARAM>(langName));

    // Try to get Language Auto-Indentation
    if (_tcscmp(langName, TEXT("NWScript"))==0)
        langSearch = msg->SendNppMessage<int>(NPPM_GETLANGUAGEAUTOINDENTATION,
            reinterpret_cast<WPARAM>(langName), reinterpret_cast<LPARAM>(&langIndent));

    m_NotepadLanguage.release();
    m_NotepadLanguage = std::make_unique<NotepadLanguage>(currLang, langName, _tcscmp(langName, TEXT("NWScript"))==0, langIndent);
}

void Plugin::SetAutoIndentSupport()
{
    Messenger* msg = Instance()->MessengerInst();
    bool langSearch = false;
    LangAutoIndentType langIndent = LangAutoIndentType::Standard;

    // Try to get Language Auto-Indentation. Older versions of NPP will return FALSE (cause this message won't exist).
    langSearch = msg->SendNppMessage<int>(NPPM_GETLANGUAGEAUTOINDENTATION,
            reinterpret_cast<WPARAM>(TEXT("NWScript")), reinterpret_cast<LPARAM>(&langIndent));

    if (langSearch)
    {
        if (langIndent != LangAutoIndentType::Extended)
        {
            bool success = msg->SendNppMessage<bool>(NPPM_SETLANGUAGEAUTOINDENTATION,
                reinterpret_cast<WPARAM>(TEXT("NWScript")), (LPARAM)LangAutoIndentType::Extended);

            if (success)
            {
                Instance()->m_NeedPluginAutoIndent = false;

                // Remove the "Use Auto-Indent" menu command
                RemoveAutoIndentMenu();
                // Auto-adjust the settings
                Instance()->Settings()->bEnableAutoIndentation = false;
            }
        }        
    }
    else
    {
        Instance()->m_NeedPluginAutoIndent = true;
    }
}

#define NOTEPADPLUS_USER_INTERNAL  (WM_USER + 0000)
#define NPPM_INTERNAL_GETMENU      (NOTEPADPLUS_USER_INTERNAL + 14)

HMENU Plugin::GetNppMainMenu()
{
    Messenger* pMsg = Instance()->MessengerInst();
    HMENU hMenu;

    // Notepad++ ver > 6.3
    hMenu = reinterpret_cast<HMENU>(pMsg->SendNppMessage<LRESULT>(NPPM_GETMENUHANDLE, 1, 0));
    if (hMenu && IsMenu(hMenu))
        return hMenu;

    // Notepad++ ver <= 6.3
    hMenu = reinterpret_cast<HMENU>(pMsg->SendNppMessage<LRESULT>(NPPM_INTERNAL_GETMENU, 0, 0));
    if (hMenu && IsMenu(hMenu))
        return hMenu;

    return ::GetMenu(Instance()->MessengerInst()->GetNotepadHwnd());
}

void Plugin::RemoveAutoIndentMenu()
{
    HMENU hMenu = Instance()->GetNppMainMenu();
    if (hMenu)
    {
        RemoveMenu(hMenu, pluginFunctions[0]._cmdID, MF_BYCOMMAND);
    }
}

#pragma endregion Plugin Funcionality






// Command for old versions of Notepad++
PLUGINCOMMAND Plugin::AutoIndent()
{
    // Change settings
    Instance()->Settings()->bEnableAutoIndentation = !Instance()->Settings()->bEnableAutoIndentation;
    bool bEnableAutoIndent = Instance()->Settings()->bEnableAutoIndentation;

    HMENU hMenu = Instance()->GetNppMainMenu();
    if (hMenu)
    {
        CheckMenuItem(hMenu, pluginFunctions[0]._cmdID,
            MF_BYCOMMAND | ((bEnableAutoIndent) ? MF_CHECKED : MF_UNCHECKED));
    }

    // Huge message box of incompatibility
    if (bEnableAutoIndent)
        MessageBox(NULL, TEXT("Current Auto-Indentation feature has conflicts with Notepad++. \
Please disable Notepad++'s \"Use Auto-Indent\" feature on:\r\n\
Settings -> Preferences -> Auto-Completion -> Auto-Indention\r\n\
while editting NSS Script files."), TEXT("NWScript Auto-Indentor"), MB_ICONWARNING | MB_OK);

}

PLUGINCOMMAND Plugin::CompileScript()
{
    MessageBox(NULL, TEXT("Coming soon! :)"), TEXT("NWScript Compiler"), MB_OK);
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
    Instance()->m_AboutDialog->doDialog();
}

