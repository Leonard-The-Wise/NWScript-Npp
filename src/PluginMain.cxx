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
#include <ShlObj.h>

#include "Common.h"
#include "LexerCatalogue.h"

#include "PluginMain.h"

#include "AboutDialog.h"
#include "WarningDialog.h"
#include "ElevateDialog.h"


using namespace NWScriptPlugin;
using namespace LexerInterface;


// Static members definition

generic_string Plugin::pluginName = TEXT("NWScript Tools");

// Menu functions order. Needs to be Sync'ed with pluginFunctions[]
#define PLUGINMENU_SWITCHAUTOINDENT 0
#define PLUGINMENU_COMPILESCRIPT 1
#define PLUGINMENU_DASH1 2
#define PLUGINMENU_IMPORTDEFINITIONS 3
#define PLUGINMENU_SETTINGS 4
#define PLUGINMENU_DASH2 5
#define PLUGINMENU_FIXEDITORCOLORS 6
#define PLUGINMENU_ABOUTME 7

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

Plugin* Plugin::_instance(nullptr);

// Notepad Plugin Configuration installation root directory. We made it const, since we're not retrieving this outside a plugin build 
const generic_string NotepadPluginConfigRootDir = TEXT("config\\");
// Notepad Default Themes install directory. We made it const, since we're not retrieving this outside a plugin build
const generic_string NotepadDefaultThemesRootDir = TEXT("themes\\");
// Notepad Default Darkl Theme file. We made it const, since we're not retrieving this outside a plugin build
const generic_string NotepadDefaultDarkThemeFile = TEXT("DarkModeDefault.xml");

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

    // Get metainformation about this plugin
    TCHAR fTemp[MAX_PATH] = {};
    DWORD fSize = GetModuleFileName(static_cast<HMODULE>(hModule), fTemp, MAX_PATH);
    Instance()._pluginPath = std::make_unique<fs::path>(fTemp);
    Instance()._pluginFileName = Instance()._pluginPath.get()->stem();
    Instance()._pluginFileExtension = Instance()._pluginPath.get()->extension();
    Instance()._pluginLexerConfigFile = Instance()._pluginPath.get()->stem();
    Instance()._pluginLexerConfigFile.append(TEXT(".xml"));

    // The rest of metainformation is get when Notepad Messenger is set...
    // Check on Plugin::SetNotepadData
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
    // Init Notepad-Scintilla Messenger
	if (!_messageInstance)
	{
		_messageInstance = std::make_unique<PluginMessenger>(data);
        _indentor = std::make_unique<LineIndentor>(*(_messageInstance.get()));
	}

    //Keep a copy of Notepad HWND to easy access
    _notepadHwnd = Messenger().GetNotepadHwnd();

    // Finish initializing the Plugin Metainformation
    TCHAR fName[MAX_PATH] = {};
    Messenger().SendNppMessage(NPPM_GETNPPDIRECTORY, static_cast<WPARAM>(MAX_PATH), reinterpret_cast<LPARAM>(fName));
    _notepadInstallDir = fName;
    _notepadThemesInstallDir = _notepadInstallDir;
    _notepadThemesInstallDir.append(TEXT("\\"));
    _notepadThemesInstallDir.append(NotepadDefaultThemesRootDir);
    _notepadDarkThemeFilePath = _notepadThemesInstallDir;
    _notepadDarkThemeFilePath.append(NotepadDefaultDarkThemeFile);

    ZeroMemory(fName, sizeof(fName));
    Messenger().SendNppMessage(NPPM_GETPLUGINHOMEPATH, static_cast<WPARAM>(MAX_PATH), reinterpret_cast<LPARAM>(fName));
    _notepadPluginHomePath = fName;
    _pluginLexerConfigFilePath = _notepadPluginHomePath;
    _pluginLexerConfigFilePath.append(TEXT("\\"));
    _pluginLexerConfigFilePath.append(NotepadPluginConfigRootDir);
    _pluginLexerConfigFilePath.append(_pluginLexerConfigFile);

    // Settings directory is different than Plugin Install Dir.
    ZeroMemory(fName, sizeof(fName));
    Messenger().SendNppMessage(NPPM_GETPLUGINSCONFIGDIR, static_cast<WPARAM>(MAX_PATH), reinterpret_cast<LPARAM>(fName));
    generic_string sPluginConfigPath = fName;
    sPluginConfigPath.append(TEXT("\\"));
    sPluginConfigPath.append(_pluginFileName);
    sPluginConfigPath.append(TEXT(".ini"));

    // Create settings instance and load all values
    _settings = std::make_unique<NWScriptPlugin::Settings>(sPluginConfigPath);
    Settings().Load();

    // Adjust menu checked or not
    pluginFunctions[PLUGINMENU_SWITCHAUTOINDENT]._init2Check = Settings().bEnableAutoIndentation;
}

#pragma endregion Plugin internal processing

#pragma region 

// Processes Raw messages from a Notepad++ window (the ones not handled by editor). 
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
            _isReady = true;
            SetAutoIndentSupport();
            LoadNotepadLexer();
            SetupMenuIcons();
            break;

        // TODO: Save Configurations file
        case NPPN_SHUTDOWN:
            _isReady = false;
            Settings().Save();
            break;

        case NPPN_LANGCHANGED:
            LoadNotepadLexer();
            break;

        case NPPN_BUFFERACTIVATED:
            if (_isReady)
                LoadNotepadLexer();
            break;


        case SCN_CHARADDED:
            // Conditions to perform the Auto-Indent:
            // - Notepad is in Ready state;
            // - Current Language is set to one of the plugin supported langs
            // - Notepad version doesn't yet support Extended AutoIndent functionality
            if (_isReady && IsPluginLanguage() && _needPluginAutoIndent
                && Settings().bEnableAutoIndentation)
				    Indentor().IndentLine(static_cast<TCHAR>(notifyCode->ch));
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
    _notepadCurrentLexer = std::make_unique<NotepadLexer>(currLang, lexerName, isPluginLanguage, langIndent);
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
                    MessageBox(_notepadHwnd, reinterpret_cast<LPCWSTR>(sWarning.str().c_str()),
                           pluginName.c_str(), MB_OK | MB_ICONWARNING | MB_APPLMODAL);
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
        RemoveMenu(hMenu, pluginFunctions[PLUGINMENU_SWITCHAUTOINDENT]._cmdID, MF_BYCOMMAND);
    }
}

bool Plugin::SetStockMenuItemIcon(int commandID, SHSTOCKICONID stockIconID, bool bSetToUncheck = true, bool bSetToCheck = true)
{
    HMENU hMenu = Instance().GetNppMainMenu();
    if (hMenu)
    {
        HBITMAP hIconBmp = GetStockIconBitmap(stockIconID, IconSize::Size16x16);
        bool bSuccess = false;
        if (bSetToUncheck && bSetToCheck)
            bSuccess = SetMenuItemBitmaps(hMenu, Instance().GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, hIconBmp, hIconBmp);
        if (bSetToUncheck && !bSetToCheck)
            bSuccess = SetMenuItemBitmaps(hMenu, Instance().GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, hIconBmp, NULL);
        if (!bSetToUncheck && bSetToCheck)
            bSuccess = SetMenuItemBitmaps(hMenu, Instance().GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, NULL, hIconBmp);

        return bSuccess;
    }

    return false;
}

void Plugin::SetupMenuIcons()
{
    // Only show error messages once per session.
    static bool bErrorShown = false;

    // Only do file operations once per session.
    static bool bFilesCheched = false;
    static bool bSuccessLexer = false;
    static bool bSuccessDark = false;
    static FileWritePermission fLexerPerm = FileWritePermission::UndeterminedError;
    static FileWritePermission fDarkThemePerm = FileWritePermission::UndeterminedError;

    // Don't use the shield icons when user runs in Administrator mode
    if (IsUserAnAdmin())
        return;

    if (!bFilesCheched)
    {
        // Retrieve write permissions for _pluginLexerConfigFile and _notepadDarkThemeFilePath
        bSuccessLexer = CheckFileWritePermission(_pluginLexerConfigFilePath, fLexerPerm);
        bSuccessDark = CheckFileWritePermission(_notepadDarkThemeFilePath, fDarkThemePerm);
    }

    if ((!bSuccessLexer || !bSuccessDark) && !bErrorShown)
    {
        generic_stringstream sError;
        sError << TEXT("Unable to read information for file(s) bellow: ") << "\r\n";
        if (!bSuccessLexer)
            sError << _pluginLexerConfigFilePath << "\r\n";
        if (!bSuccessDark)
            sError << _notepadDarkThemeFilePath << "\r\n";
        sError << "\r\n";
        sError << "NWScript Plugin may not work properly, please check your installation!";

        ::MessageBox(_notepadHwnd, sError.str().c_str(), pluginName.c_str(), MB_ICONERROR | MB_APPLMODAL | MB_OK);

        bErrorShown = true;
    }

    // For users without permission to _pluginLexerConfigFilePath, set shield on Import Definitions Menu 
    if (fLexerPerm == FileWritePermission::RequiresAdminPrivileges)
        SetStockMenuItemIcon(PLUGINMENU_IMPORTDEFINITIONS, SHSTOCKICONID::SIID_SHIELD, true, false);
    // For users without permission to _pluginLexerConfigFilePath or _notepadDarkThemeFilePath, set shield on Fix Editor Colors
    if (fLexerPerm == FileWritePermission::RequiresAdminPrivileges || fDarkThemePerm == FileWritePermission::RequiresAdminPrivileges)
        SetStockMenuItemIcon(PLUGINMENU_FIXEDITORCOLORS, SHSTOCKICONID::SIID_SHIELD, true, false);
}

// Opens the About dialog
void Plugin::OpenAboutDialog()
{
    // Dialog boxes need to be static unless it's modal.
    static AboutDialog aboutDialog = {};

    if (!aboutDialog.isCreated())
        aboutDialog.init(Instance().DllHModule(), Instance().NotepadHwnd());

    aboutDialog.doDialog();
}

void Plugin::OpenWarningDialog()
{
    // Dialog boxes need to be static unless it's modal.
    static WarningDialog warningDialog = {};

    if (!warningDialog.isCreated())
        warningDialog.init(Instance().DllHModule(), Instance().NotepadHwnd());

    warningDialog.doDialog();
}

#pragma endregion Plugin Funcionality



// Support for Auto-Indentation for old versions of Notepad++
PLUGINCOMMAND Plugin::SwitchAutoIndent()
{
    static bool bAutoIndentationWarningShown = false;

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

    // Warning user of function: only once in a session (and perhaps in a lifetime if INI file doesn't change)
    if (bEnableAutoIndent && !bAutoIndentationWarningShown)
    {
        Instance().OpenWarningDialog();
        bAutoIndentationWarningShown = true;
    }
}

// Compiles current .NSS Script file
PLUGINCOMMAND Plugin::CompileScript()
{
    MessageBox(Instance().NotepadHwnd(), TEXT("Coming soon! :)"), pluginName.c_str(), MB_OK | MB_ICONINFORMATION);
}

// Opens the Plugin's Settings panel
PLUGINCOMMAND Plugin::OpenSettings()
{
    MessageBox(Instance().NotepadHwnd(), TEXT("Not yet implemented"), pluginName.c_str(), MB_OK | MB_ICONINFORMATION);
}

// Imports Lexer's functions and constants declarations from a NWScript.nss file
PLUGINCOMMAND Plugin::ImportDefinitions()
{
    std::unique_ptr<generic_string> nFileName = std::make_unique<generic_string>();
    if (OpenFileDialog(Instance().NotepadHwnd(), TEXT("NWScritpt.nss File\0nwscript.nss\0"), *nFileName.get()))
    {
        MessageBox(Instance().NotepadHwnd(), TEXT("Not yet implemented"), pluginName.c_str(), MB_OK | MB_ICONINFORMATION);
    }
}

// Fixes the language colors to default
PLUGINCOMMAND Plugin::FixLanguageColors()
{
    // Get file permissions to style files
    static bool bSuccessLexer = false;
    static bool bSuccessDark = false;
    static FileWritePermission fLexerPerm = FileWritePermission::UndeterminedError;
    static FileWritePermission fDarkThemePerm = FileWritePermission::UndeterminedError;

    bSuccessLexer = CheckFileWritePermission(Instance()._pluginLexerConfigFilePath, fLexerPerm);
    bSuccessDark = CheckFileWritePermission(Instance()._notepadDarkThemeFilePath, fDarkThemePerm);

    if (fLexerPerm == FileWritePermission::RequiresAdminPrivileges || fDarkThemePerm == FileWritePermission::RequiresAdminPrivileges)
    {
        std::vector<generic_string> s = {};
        if (fLexerPerm == FileWritePermission::RequiresAdminPrivileges)
            s.push_back(Instance()._pluginLexerConfigFilePath);
        if (fDarkThemePerm == FileWritePermission::RequiresAdminPrivileges)
            s.push_back(Instance()._notepadDarkThemeFilePath);

        ElevateDialog ePermission = {};
        ePermission.SetFilesText(s);

        INT_PTR bRunAdmin = ePermission.doDialog(Instance().DllHModule(), Instance().NotepadHwnd());
        if (bRunAdmin)
        {

        }
    }
}

// Opens About Box
PLUGINCOMMAND Plugin::AboutMe()
{
    Instance().OpenAboutDialog();
}

