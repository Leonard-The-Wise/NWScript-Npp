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
    {TEXT("Settings..."), Plugin::Settings},
    {TEXT("---")},
    {TEXT("About Me"), Plugin::AboutMe},
};

#pragma region


// Static members definitions
Plugin* Plugin::m_instance(nullptr);
HMODULE Plugin::m_DllHModule(nullptr);
bool Plugin::m_isReady(false);
bool Plugin::m_UseAutoIndent(true);
std::unique_ptr<Plugin::Messenger> Plugin::m_MessageInstance(nullptr);
std::unique_ptr<Plugin::NotepadLanguage> Plugin::m_NotepadLanguage(nullptr);
std::unique_ptr<Plugin::LineIndentor> Plugin::m_Indentor(nullptr);
std::unique_ptr<AboutDialog> Plugin::m_AboutDialog(nullptr);


HWND Plugin::Messenger::GetCurentScintillaHwnd() const
{
    int currentView = -1;
    ::SendMessage(m_NppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&currentView));
    return (currentView == 0) ? m_NppData._scintillaMainHandle : m_NppData._scintillaSecondHandle;
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
    if (!m_instance)
        m_instance = new Plugin();

	Instance()->m_DllHModule = (HMODULE)hModule;
}

// Delete Singleton
void Plugin::PluginCleanUp()
{
    delete m_instance;
    m_instance = nullptr;
}

TCHAR* Plugin::GetName() const
{
    return Instance()->pluginName;
}

int Plugin::GetFunctionCount() const
{
    return (int)std::size(Instance()->pluginFunctions);
}

FuncItem Plugin::GetFunction(int index) const
{
    return Instance()->pluginFunctions[index];
}

FuncItem* Plugin::GetFunctions() const
{
    return Instance()->pluginFunctions;
}

Plugin* Plugin::Instance()
{
    return m_instance;
}

Plugin::Messenger* Plugin::MessengerInst() const
{
    return m_MessageInstance.get();
}

Plugin::LineIndentor* Plugin::Indentor() const
{
	return m_Indentor.get();
}

HMODULE Plugin::DllHModule()
{
    return m_DllHModule;
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
            if (IsReady() && IsPluginLanguage())
				Instance()->Indentor()->AutoIndentLine(static_cast<TCHAR>(notifyCode->ch));
            break;

        default:
            return;
    }
}

#pragma endregion Message Processing


#pragma region 

void Plugin::IsReady(bool Ready)
{
    Instance()->m_isReady = Ready;
}

bool Plugin::IsReady() const
{
    return Instance()->m_isReady;
}

bool Plugin::IsPluginLanguage() const
{
    return Instance()->GetNotepadLanguage()->isPluginLang;
}

Plugin::NotepadLanguage* Plugin::GetNotepadLanguage()
{
    return Instance()->m_NotepadLanguage.get();
}

void Plugin::LoadNotepadLanguage()
{
    // Get Current notepad state;
	int currLang = 0;
	Instance()->MessengerInst()->SendNppMessage<>(NPPM_GETCURRENTLANGTYPE, 0, reinterpret_cast<LPARAM>(&currLang));

    // First call: retrieve buffer size. Second call, fill up name (from Manual).
    int buffSize = Instance()->MessengerInst()->SendNppMessage<int>(NPPM_GETLANGUAGENAME, currLang, reinterpret_cast<LPARAM>(nullptr));

    TCHAR* langName = new TCHAR[buffSize+1];
	Instance()->MessengerInst()->SendNppMessage<void>(NPPM_GETLANGUAGENAME, currLang, reinterpret_cast<LPARAM>(langName));

    m_NotepadLanguage.release();
    m_NotepadLanguage = std::make_unique<NotepadLanguage>( currLang, langName, _tcscmp(langName, TEXT("NWScript"))==0);
}


#pragma endregion Plugin Funcionality







PLUGINCOMMAND Plugin::AutoIndent()
{
    MessageBox(NULL, TEXT("Not yet implemented"), TEXT("NWScript Compiler"), MB_OK);
}

PLUGINCOMMAND Plugin::CompileScript()
{
    MessageBox(NULL, TEXT("Coming soon! :)"), TEXT("NWScript Compiler"), MB_OK);
}

PLUGINCOMMAND Plugin::Settings()
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

