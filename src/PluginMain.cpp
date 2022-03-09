/** @file PluginMain.cpp
 * Controls the Plugin functions, processes all Plugin messages
 * 
 * The ACTUAL DLL Entry point is defined in PluginInterface.cpp
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include <Windows.h>
#include <string>
#include <sstream>
#include <stdexcept>
#include <ShlObj.h>
#include <codecvt>
#include <locale>

#include "Common.h"
#include "LexerCatalogue.h"
#include "tinyxml2.h"
#include "NotepadInternalMessages.h"

#include "PluginMain.h"
#include "PluginControlsRC.h"

#include "AboutDialog.h"
#include "WarningDialog.h"
#include "FileAccessDialog.h"
#include "FileParseSummaryDialog.h"

#pragma warning (disable : 6387)

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
#define PLUGINMENU_RESETEDITORCOLORS 6
#define PLUGINMENU_INSTALLDARKTHEME 7
#define PLUGINMENU_ABOUTME 8

FuncItem Plugin::pluginFunctions[] = {
    {TEXT("Use Auto-Identation"), Plugin::SwitchAutoIndent},
    {TEXT("Compile Script"), Plugin::CompileScript},
    {TEXT("---")},
    {TEXT("Import NWScript definitions"), Plugin::ImportDefinitions},
    {TEXT("Settings..."), Plugin::OpenSettings},
    {TEXT("---")},
    {TEXT("Reset Editor Colors"), Plugin::ResetEditorColors},
    {TEXT("Install Dark Theme"), Plugin::InstallDarkTheme},
    {TEXT("About Me"), Plugin::AboutMe},
};

Plugin* Plugin::_instance(nullptr);

// Notepad Plugin Configuration installation root directory. We made it const, since we're not retrieving this outside a plugin build 
const generic_string NotepadPluginConfigRootDir = TEXT("config\\");
// Notepad Default Themes install directory. We made it const, since we're not retrieving this outside a plugin build
const generic_string NotepadDefaultThemesRootDir = TEXT("themes\\");
// Notepad Default Dark Theme file. We made it const, since we're not retrieving this outside a plugin build
const generic_string NotepadDefaultDarkThemeFile = TEXT("DarkModeDefault.xml");
// Default pseudo-batch file to create in case we need to restart notepad++
const generic_string NotepadPseudoBatchRestartFile = TEXT("~doNWScriptNotepadRestart.bat");

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
    TCHAR fName[MAX_PATH+1] = {};
    Messenger().SendNppMessage(NPPM_GETNPPDIRECTORY, static_cast<WPARAM>(MAX_PATH), reinterpret_cast<LPARAM>(fName));
    _notepadInstallDir = fName;
    _notepadThemesInstallDir = _notepadInstallDir;
    _notepadThemesInstallDir.append(TEXT("\\"));
    _notepadThemesInstallDir.append(NotepadDefaultThemesRootDir);
    _notepadDarkThemeFilePath = _notepadThemesInstallDir;
    _notepadDarkThemeFilePath.append(NotepadDefaultDarkThemeFile);

    ZeroMemory(fName, sizeof(fName));
    Messenger().SendNppMessage(NPPM_GETNPPFULLFILEPATH, static_cast<WPARAM>(MAX_PATH), reinterpret_cast<LPARAM>(fName));
    _notepadExecutableFile = fName;

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

    // Temporary batch we write in case we need to restart Notepad++ by ourselves. Root is Plugin config path
    _notepadPseudoBatchRestartFile = fName;
    _notepadPseudoBatchRestartFile.append(TEXT("\\"));
    _notepadPseudoBatchRestartFile.append(NotepadPseudoBatchRestartFile);

    // Create settings instance and load all values
    _settings = std::make_unique<NWScriptPlugin::Settings>(sPluginConfigPath);
    Settings().Load();

    // Adjust menu "Use Auto-Indentation" checked or not
    pluginFunctions[PLUGINMENU_SWITCHAUTOINDENT]._init2Check = Settings().bEnableAutoIndentation;
}

#pragma endregion Plugin DLL Initialization

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
    {
        // Do Initialization procedures
        SetAutoIndentSupport();
        DetectDarkThemeInstall();
        LoadNotepadLexer();

        // Auto call a function that required restart during the previous session (because of privilege elevation)
        // Up to now...
        // 1 = ImportDefinitions
        // 2 = Fix Editor's Colors
        // Since all functions that required restart must have returned in Admin Mode, we check this
        // to see if the user didn't cancel the UAC request.
        if (IsUserAnAdmin())
        {
            if (Settings().iNotepadRestartFunction == RestartFunctionHook::ImportDefinitions)
                ImportDefinitions();
            if (Settings().iNotepadRestartFunction == RestartFunctionHook::ResetEditorColorsPhase1 || Settings().iNotepadRestartFunction == RestartFunctionHook::ResetEditorColorsPhase2)
                DoResetEditorColors(Settings().iNotepadRestartFunction);
            if (Settings().iNotepadRestartFunction == RestartFunctionHook::InstallDarkModePhase1 || Settings().iNotepadRestartFunction == RestartFunctionHook::InstallDarkModePhase2)
                DoResetEditorColors(Settings().iNotepadRestartFunction);
        }

        // And then make sure to clear the hooks, temp files, etc. 
        // Call a instant save on settings to be avoid losing it on a session crash also.
        if (Settings().iNotepadRestartMode != RestartMode::None)
        {
            bool bIgnore = DeleteFile(_notepadPseudoBatchRestartFile.c_str());
            SetRestartHook(RestartMode::None, RestartFunctionHook::None);
            Settings().Save();
        }

        SetupMenuIcons();
        _isReady = true;

        break;
    }
    case NPPN_CANCELSHUTDOWN:
    {
        // We're clearing any attempt to hook restarts here, have they been setup or not
        SetRestartHook(RestartMode::None);
        break;
    }
    case NPPN_SHUTDOWN:
    {
        _isReady = false;
        Settings().Save();

        // If we have a restart hook setup, write batch file to re-execute notepad and call it from Windows Shell
        if (Settings().iNotepadRestartMode != RestartMode::None)
        {
            if (writePseudoBatchExecute(Instance()._notepadPseudoBatchRestartFile, Instance()._notepadExecutableFile))
                runProcess(Settings().iNotepadRestartMode == RestartMode::Admin ? true : false, _notepadPseudoBatchRestartFile);
        }

        break;
    }
    case NPPN_LANGCHANGED:
    {
        LoadNotepadLexer();
        break;
    }
    case NPPN_BUFFERACTIVATED:
    {
        if (_isReady)
            LoadNotepadLexer();
        break;
    }
    case SCN_CHARADDED:
    {
        // Conditions to perform the Auto-Indent:
        // - Notepad is in Ready state;
        // - Current Language is set to one of the plugin supported langs
        // - Notepad version doesn't yet support Extended AutoIndent functionality
        if (_isReady && IsPluginLanguage() && _needPluginAutoIndent
            && Settings().bEnableAutoIndentation)
            Indentor().IndentLine(static_cast<TCHAR>(notifyCode->ch));
        break;
    }
    default:
        return;
    }
}

#pragma endregion Plugin DLL Message Processing

#pragma region 

// Set the Auto-Indent type for all of this Plugin's installed languages
void Plugin::SetAutoIndentSupport()
{
    PluginMessenger& msg = Instance().Messenger();
    bool lexerSearch = false;
    ExternalLexerAutoIndentMode langIndent = ExternalLexerAutoIndentMode::Standard;

    // Try to set Plugin's Lexers Auto-Indentation. Older versions of NPP will return langSearch=FALSE (cause this message won't exist).
    generic_stringstream lexerNameW;
    for (int i = 0; i < LexerCatalogue::GetLexerCount(); i++)
    {
        lexerNameW = {};
        lexerNameW << LexerCatalogue::GetLexerName(i);

        lexerSearch = msg.SendNppMessage<int>(NPPM_GETEXTERNALLEXERAUTOINDENTMODE,
            reinterpret_cast<WPARAM>(lexerNameW.str().c_str()), reinterpret_cast<LPARAM>(&langIndent));

        if (lexerSearch)
        {
            if (langIndent != ExternalLexerAutoIndentMode::C_Like)
            {
                bool success = msg.SendNppMessage<bool>(NPPM_SETEXTERNALLEXERAUTOINDENTMODE,
                    reinterpret_cast<WPARAM>(lexerNameW.str().c_str()), static_cast<LPARAM>(ExternalLexerAutoIndentMode::C_Like));

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

    // lexerSearch will be TRUE if Notepad++ support NPPM_GETEXTERNALExternalLexerAutoIndentMode message
    if (lexerSearch)
    {
        Instance()._needPluginAutoIndent = false;

        // Remove the "Use Auto-Indent" menu command
        HMENU hMenu = Instance().GetNppMainMenu();
        if (hMenu)
            RemoveMenu(hMenu, pluginFunctions[PLUGINMENU_SWITCHAUTOINDENT]._cmdID, MF_BYCOMMAND);

        // Auto-adjust the settings
        Instance().Settings().bEnableAutoIndentation = false;
    }
    else
        Instance()._needPluginAutoIndent = true;
}

// Get Current notepad lexer language;
void Plugin::LoadNotepadLexer()
{
    PluginMessenger& msg = Messenger();
    bool lexerSearch = FALSE;
    bool isPluginLanguage = FALSE;
    int currLang = 0;
    ExternalLexerAutoIndentMode langIndent = ExternalLexerAutoIndentMode::Standard;
    msg.SendNppMessage<>(NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&currLang);

    // First call: retrieve buffer size. Second call, fill up name (from Manual).
    int buffSize = msg.SendNppMessage<int>(NPPM_GETLANGUAGENAME, currLang, reinterpret_cast<LPARAM>(nullptr));
    TCHAR* lexerName = new TCHAR[buffSize + 1];
    msg.SendNppMessage<void>(NPPM_GETLANGUAGENAME, currLang, reinterpret_cast<LPARAM>(lexerName));

    // Try to get Language Auto-Indentation if it's one of the plugin installed languages
    generic_stringstream lexerNameW;
    for (int i = 0; i < LexerCatalogue::GetLexerCount() && lexerSearch == false; i++)
    {
        lexerNameW = {};
        lexerNameW << LexerCatalogue::GetLexerName(i);
        isPluginLanguage = (_tcscmp(lexerName, lexerNameW.str().c_str()) == 0);

        if (isPluginLanguage)
            lexerSearch = msg.SendNppMessage<int>(NPPM_GETEXTERNALLEXERAUTOINDENTMODE,
                reinterpret_cast<WPARAM>(lexerNameW.str().c_str()), reinterpret_cast<LPARAM>(&langIndent));
    }

    // Create or Replace current lexer language.
    _notepadCurrentLexer = std::make_unique<NotepadLexer>(currLang, lexerName, isPluginLanguage, langIndent);
}

// A helper routine to remove Dark Theme menu item
void RemoveDarkThemeInstallMenu()
{
    HMENU hMenu = Plugin::Instance().GetNppMainMenu();
    if (hMenu)
    {
        RemoveMenu(hMenu, Plugin::pluginFunctions[PLUGINMENU_INSTALLDARKTHEME]._cmdID, MF_BYCOMMAND);
    }
}

// Detects if Dark Theme is already installed
void Plugin::DetectDarkThemeInstall()
{
    // Here we are parsing the file silently
    tinyxml2::XMLDocument darkThemeDoc;
    errno_t error = darkThemeDoc.LoadFile(wstr2str(Instance()._notepadDarkThemeFilePath).c_str());
    _pluginDarkThemeIs = DarkThemeStatus::Unsupported;

    // Dark theme is incompatible or file is corrupted...
    if (error)
    {
        RemoveDarkThemeInstallMenu();
        return;
    }

    // Safely navigate the XML nodes to avoid crashes
    tinyxml2::XMLElement* rootNode = darkThemeDoc.RootElement();
    tinyxml2::XMLElement* LexerStyles;
    tinyxml2::XMLElement* lexerType;

    if (rootNode)
    {
        LexerStyles = rootNode->FirstChildElement("LexerStyles");
        if (LexerStyles)
            lexerType = LexerStyles->FirstChildElement("LexerType");
        else
        {
            RemoveDarkThemeInstallMenu();
            return;
        }
    }
    else
    {
        RemoveDarkThemeInstallMenu();
        return;
    }

    //Again, if this is empty is because the file must be corrupted
    if (!lexerType)
    {
        RemoveDarkThemeInstallMenu();
        return;
    }

    // Search for lexer name inside. We're only supporting the first installed lexer here
    bool bFound = false;
    do
    {
        if (lexerType->Attribute("name", LexerCatalogue::GetLexerName(0)))
            bFound = true;
        lexerType = lexerType->NextSiblingElement("LexerType");
    } while (!bFound && lexerType);

    if (!bFound)
    {
        _pluginDarkThemeIs = DarkThemeStatus::Uninstalled;
        return;
    }

    //Dark theme installed, mark it here.
    RemoveDarkThemeInstallMenu();
    _pluginDarkThemeIs = DarkThemeStatus::Installed;
}

// Setup menu icons
#pragma region

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
    bool bSuccessLexer = false;
    bool bSuccessDark = false;
    FileWritePermission fLexerPerm = FileWritePermission::UndeterminedError;
    FileWritePermission fDarkThemePerm = FileWritePermission::UndeterminedError;

    // Don't use the shield icons when user runs in Administrator mode
    if (IsUserAnAdmin())
        return;

    // Retrieve write permissions for _pluginLexerConfigFile and _notepadDarkThemeFilePath
    bSuccessLexer = CheckFileWritePermission(_pluginLexerConfigFilePath, fLexerPerm);
    bSuccessDark = CheckFileWritePermission(_notepadDarkThemeFilePath, fDarkThemePerm);

    if (!bSuccessLexer)
    {
        generic_stringstream sError;
        sError << TEXT("Unable to read information for file bellow: ") << "\r\n";
        sError << _pluginLexerConfigFilePath << "\r\n";
        sError << "\r\n";
        sError << "NWScript Plugin may not work properly, please check your installation!";

        ::MessageBox(_notepadHwnd, sError.str().c_str(), pluginName.c_str(), MB_ICONERROR | MB_APPLMODAL | MB_OK);
    }

    // For users without permission to _pluginLexerConfigFilePath, set shield on Import Definitions
    if (fLexerPerm == FileWritePermission::RequiresAdminPrivileges)
        SetStockMenuItemIcon(PLUGINMENU_IMPORTDEFINITIONS, SHSTOCKICONID::SIID_SHIELD, true, false);
    // For users without permission to _notepadDarkThemeFilePath, set shield on Install Dark Theme if not already installed
    if (fDarkThemePerm == FileWritePermission::RequiresAdminPrivileges && _pluginDarkThemeIs == DarkThemeStatus::Uninstalled)
        SetStockMenuItemIcon(PLUGINMENU_INSTALLDARKTHEME, SHSTOCKICONID::SIID_SHIELD, true, false);
    // For users without permissions to any of the files (and also only checks Dark Theme support if file is existent and supported/not corrupted)...
    if (fLexerPerm == FileWritePermission::RequiresAdminPrivileges || (fDarkThemePerm == FileWritePermission::RequiresAdminPrivileges && _pluginDarkThemeIs != DarkThemeStatus::Unsupported))
        SetStockMenuItemIcon(PLUGINMENU_RESETEDITORCOLORS, SHSTOCKICONID::SIID_SHIELD, true, false);
}

#pragma endregion Menu icons setup

#pragma endregion Plugin initialization functions and dynamic behavior

#pragma region

void Plugin::DoImportDefinitionsCallback(HRESULT decision)
{
    // Trash results for memory space in a cancel.
    if (decision == IDCANCEL || decision == IDNO)
    {
        Instance()._NWScriptParseResults.reset();
        return;
    }

    NWScriptParser::ScriptParseResults& myResults = *Instance()._NWScriptParseResults;
    tinyxml2::XMLDocument nwscriptDoc;

    // Since TinyXML only accepts ASCII filenames, we do a crude conversion here... hopefully noone is using
    // chinese filenames.. :P
    std::string asciiFile = wstr2str(Instance()._pluginLexerConfigFilePath);
    errno_t error = nwscriptDoc.LoadFile(asciiFile.c_str());
    generic_stringstream errorStream;
    if (error)
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginLexerConfigFilePath << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    tinyxml2::XMLElement* notepadPlus = nwscriptDoc.RootElement();
    if (notepadPlus == NULL)
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginLexerConfigFilePath << "! \r\n";
        errorStream << TEXT("Cannot find root XML node 'NotepadPlus'!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    tinyxml2::XMLElement* languages = notepadPlus->FirstChildElement("Languages");
    if (languages == NULL)
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginLexerConfigFilePath << "! \r\n";
        errorStream << TEXT("Cannot find XML node 'Languages'!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    tinyxml2::XMLElement* language = languages->FirstChildElement("Language");
    if (language == NULL)
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginLexerConfigFilePath << "! \r\n";
        errorStream << TEXT("Cannot find root XML node 'Language'!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    // We are only supporting our default lexer here
    if (!language->Attribute("name", LexerCatalogue::GetLexerName(0)))
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginLexerConfigFilePath << "! \r\n";
        errorStream << TEXT("Language name for ") << LexerCatalogue::GetLexerName(0) << TEXT(" not found!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    // Now we want just the Keywords for "type2" (Engine Structures), "type4  (Engine Constants) and "type6" (Function Names)
    // Look for at least 1 keyword element
    tinyxml2::XMLElement* Keywords = language->FirstChildElement("Keywords");
    if (Keywords == NULL)
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginLexerConfigFilePath << "! \r\n";
        errorStream << TEXT("Cannot find root XML node 'Keywords'!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    // Need to convert from wchar_t to char here...
    // We use a post-check to see whether all tags where updated. Also useful to avoid processing same tag twice (only happens if user tampered)
    // with the file.
    bool bType2 = false, bType4 = false, bType6 = false;
    while (Keywords)
    {
        if (Keywords->Attribute("name", "type2") && !bType2)
        {
            generic_string generic_output = myResults.MembersAsSpacedString(NWScriptParser::MemberID::EngineStruct);
            Keywords->SetText(wstr2str(generic_output).c_str());
            bType2 = true;
        } 

        if (Keywords->Attribute("name", "type4") && !bType4)
        {
            generic_string generic_output = myResults.MembersAsSpacedString(NWScriptParser::MemberID::Constant);
            Keywords->SetText(wstr2str(generic_output).c_str());
            bType4 = true;
        }

        if (Keywords->Attribute("name", "type6") && !bType6)
        {
            generic_string generic_output = myResults.MembersAsSpacedString(NWScriptParser::MemberID::Function);
            Keywords->SetText(wstr2str(generic_output).c_str());
            bType6 = true;
        }

        Keywords = Keywords->NextSiblingElement("Keywords");
    }

    // Another error handling...
    if (!bType2 || !bType4 || !bType6)
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginLexerConfigFilePath << "! \r\n";
        errorStream << TEXT("The following nodes could not be found!\r\n");
        errorStream << TEXT("Nodes: [") << (!bType2 ? TEXT(" type2") : TEXT("")) << (!bType4 ? TEXT(" type4") : TEXT(""))
            << (!bType6 ? TEXT(" type6") : TEXT("")) << TEXT(" ]");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    // Finally, save file... and LAST error handling!
    error = nwscriptDoc.SaveFile(asciiFile.c_str());
    if (error)
    {
        errorStream << TEXT("Error while saving file: ") << Instance()._pluginLexerConfigFilePath << "! \r\n";
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
    }

    // Close our results (for memory cleanup) and report back.
    Instance()._NWScriptParseResults.reset();
    int mResult = MessageBox(Instance().NotepadHwnd(), 
            TEXT("Notepad++ needs to be restarted for the new settings to be reflected. Do it now?"), 
        TEXT("Import successful!"), MB_YESNO | MB_ICONINFORMATION);

    if (mResult == IDYES)
    {
        // Setup our hook to auto-restart notepad++ normally and not run any other function on restart.
        // Then send message for notepad to close itself.
        Instance().SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
        Instance().Messenger().SendNppMessage(WM_CLOSE, 0, 0);
    }
}

void Plugin::DoResetEditorColors(RestartFunctionHook whichPhase)
{

}

void Plugin::DoInstallDarkTheme(RestartFunctionHook whichPhase)
{

}

#pragma endregion Plugin files management


#pragma region

Plugin::FileCheckResults Plugin::FilesWritePermissionCheckup(const std::vector<generic_string>& sFiles, RestartFunctionHook iFunctionToCallIfRestart)
{
    struct stCheckedFiles {
        bool bExists;
        generic_string sFile;
        FileWritePermission fPerm;
    };

    std::vector<stCheckedFiles> checkedFiles;
    checkedFiles.reserve(sFiles.size());

    // Batch check files
    for (const generic_string& s : sFiles)
    {
        bool bExists = false;
        FileWritePermission fPerm = FileWritePermission::UndeterminedError;

        bExists = CheckFileWritePermission(s, fPerm);
        checkedFiles.emplace_back(stCheckedFiles{ bExists, s, fPerm });
    }

    // Check for Admin privileges required first
    std::vector<generic_string> sWhichFiles;
    for (stCheckedFiles& st : checkedFiles)
    {
        if (st.fPerm == FileWritePermission::RequiresAdminPrivileges)
            sWhichFiles.push_back(st.sFile);
    }
    if (sWhichFiles.size() > 0)
    {
        // Show File Access dialog box in Admin Mode
        FileAccessDialog ePermission = {};
        ePermission.init(Instance().DllHModule(), Instance().NotepadHwnd());
        ePermission.SetAdminMode(true);
        ePermission.SetIcon(SHSTOCKICONID::SIID_SHIELD);
        ePermission.SetWarning(TEXT("WARNING - this action requires write permission to the following file(s):"));
        ePermission.SetFilesText(sWhichFiles);
        ePermission.SetSolution(TEXT("To solve this, you may either : \r\n - Try to reopen Notepad++ with elevated privileges(Administrator Mode); or \r\n \
- Give write access permissions to the file(s) manually, by finding it in Windows Explorer, selecting Properties->Security Tab."));

        INT_PTR RunAdmin = ePermission.doDialog();
        if (RunAdmin == 1)
        {
            // Set our hook flag 2 to rerun Notepad++ in admin mode and call iFunctionToCallIfRestart upon restart
            // Then tells notepad++ to quit.
            SetRestartHook(RestartMode::Admin, iFunctionToCallIfRestart);
            Messenger().SendNppMessage(WM_CLOSE, 0, 0);
        }

        return FileCheckResults::RequiresAdminPrivileges;
    }

    // Check for read-only second
    sWhichFiles.clear();
    for (stCheckedFiles& st : checkedFiles)
    {
        if (st.fPerm == FileWritePermission::FileIsReadOnly)
            sWhichFiles.push_back(st.sFile);
    }
    if (sWhichFiles.size() > 0)
    {
        // Show File Access dialog box in Normal Mode
        FileAccessDialog ePermission = {};
        ePermission.init(Instance().DllHModule(), Instance().NotepadHwnd());
        ePermission.SetAdminMode(false);
        ePermission.SetIcon(SHSTOCKICONID::SIID_ERROR);
        ePermission.SetWarning(TEXT("ERROR: The following file(s) are marked as 'Read-Only' and cannot be changed:"));
        ePermission.SetFilesText(sWhichFiles);
        ePermission.SetSolution(TEXT("To solve this: \r\n  - Please, provide the necessary file permissions to use this option.\r\n \
  Find it in Windows Explorer, select Properties -> Uncheck Read-Only flag. "));
        INT_PTR ignore = ePermission.doDialog();

        return FileCheckResults::ReadOnlyFiles;
    }

    // Check for blocked by application third
    sWhichFiles.clear();
    for (stCheckedFiles& st : checkedFiles)
    {
        if (st.fPerm == FileWritePermission::BlockedByApplication)
            sWhichFiles.push_back(st.sFile);
    }
    if (sWhichFiles.size() > 0)
    {
        // Show File Access dialog box in Normal Mode
        FileAccessDialog ePermission = {};
        ePermission.init(Instance().DllHModule(), Instance().NotepadHwnd());
        ePermission.SetAdminMode(false);
        ePermission.SetIcon(SHSTOCKICONID::SIID_DOCASSOC);
        ePermission.SetWarning(TEXT("ERROR: The following file(s) are currently blocked by other applications/processes and cannot be changed:"));
        ePermission.SetFilesText(sWhichFiles);
        ePermission.SetSolution(TEXT("To solve this:\r\n  - Please, close the files in other applications before trying this action again."));
        INT_PTR ignore = ePermission.doDialog();

        return FileCheckResults::BlockedByApplication;
    }

    // Finally, check for inexistent files and/or other unknown errors
    sWhichFiles.clear();
    for (stCheckedFiles& st : checkedFiles)
    {
        if (st.bExists == false || st.fPerm == FileWritePermission::UndeterminedError)
            sWhichFiles.push_back(st.sFile);
    }
    if (sWhichFiles.size() > 0)
    {
        // Show File Access dialog box in Normal Mode
        FileAccessDialog ePermission = {};
        ePermission.init(Instance().DllHModule(), Instance().NotepadHwnd());
        ePermission.SetAdminMode(false);
        ePermission.SetIcon(SHSTOCKICONID::SIID_DELETE);
        ePermission.SetWarning(TEXT("ERROR: The file(s) bellow are inexistent or could not be accessed:"));
        ePermission.SetFilesText(sWhichFiles);
        ePermission.SetSolution(TEXT("Some reasons might happen:\r\n  - Either you are running the plugin with a Notepad++ incompatible with this function; or \
you may have accidentaly deleted the file(s).\r\nPlease try reinstalling the products."));
        INT_PTR ignore = ePermission.doDialog();

        return FileCheckResults::UnknownError;
    }

    return FileCheckResults::CheckSuccess;
}

// Support for Auto-Indentation for old versions of Notepad++
PLUGINCOMMAND Plugin::SwitchAutoIndent()
{
    static bool bAutoIndentationWarningShown = false;
    // Dialog boxes need to be static unless it's modal.
    static WarningDialog warningDialog = {};

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
        if (!warningDialog.isCreated())
            warningDialog.init(Instance().DllHModule(), Instance().NotepadHwnd());

        if (!warningDialog.isVisible())
            warningDialog.doDialog();

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
    // Initialize our results dialog
    static FileParseSummaryDialog parseDialog = {};
    if (!parseDialog.isCreated())
        parseDialog.init(Instance().DllHModule(), Instance().NotepadHwnd());

    // Someone called this twice!
    if (parseDialog.isVisible())
        return;

    // Do a file check for the necessary XML files
    std::vector<generic_string> sFiles;
    sFiles.push_back(Instance()._pluginLexerConfigFilePath);

    FileCheckResults fResult = Instance().FilesWritePermissionCheckup(sFiles, RestartFunctionHook::None);
    if (static_cast<int>(fResult) < 1)
        return;

    generic_string nFileName;
    if (OpenFileDialog(Instance().NotepadHwnd(), TEXT("nwscritpt.nss\0nwscript*.nss\0All Files (*.*)\0*.*"), nFileName))
    {
        // Opens the NWScript file and parse it. Keep the results for later use
        NWScriptParser nParser(Instance().NotepadHwnd());
        
        Instance()._NWScriptParseResults = std::make_unique<NWScriptParser::ScriptParseResults>();
        NWScriptParser::ScriptParseResults& myResults = *Instance()._NWScriptParseResults;

        bool bSuccess = nParser.ParseFile(nFileName, myResults);

        // Last check for results: File empty?
        if (myResults.EngineStructuresCount == 0 && myResults.FunctionsCount == 0 && myResults.ConstantsCount == 0)
        {
            MessageBox(Instance().NotepadHwnd(), TEXT("File analysis didn't find anything to import!"), pluginName.c_str(),
                MB_ICONEXCLAMATION | MB_OK);
            return;
        }

        // Show File Parsing Results dialog message and since we don't want it to be modal, wait for callback in DoImportDefinitionsCallback.
        parseDialog.setEngineStructuresCount(myResults.EngineStructuresCount);
        parseDialog.setFunctionDefinitionsCount(myResults.FunctionsCount);
        parseDialog.setConstantsCount(myResults.ConstantsCount);

        parseDialog.setOkDialogCallback(&Plugin::DoImportDefinitionsCallback);
        parseDialog.doDialog();
    }
}

// Fixes the language colors to default
PLUGINCOMMAND Plugin::ResetEditorColors()
{
    // Do a file check for the necessary XML files
    std::vector<generic_string> sFiles;
    sFiles.push_back(Instance()._pluginLexerConfigFilePath);

    FileCheckResults fResult = Instance().FilesWritePermissionCheckup(sFiles, RestartFunctionHook::ResetEditorColorsPhase1);
    if (static_cast<int>(fResult) < 1)
        return;

    Instance().DoResetEditorColors();
}

// Installs Dark theme
PLUGINCOMMAND Plugin::InstallDarkTheme()
{
    // Do a file check for the necessary XML files
    std::vector<generic_string> sFiles;
    sFiles.push_back(Instance()._notepadDarkThemeFilePath);

    FileCheckResults fResult = Instance().FilesWritePermissionCheckup(sFiles, RestartFunctionHook::InstallDarkModePhase1);
    if (static_cast<int>(fResult) < 1)
        return;

    Instance().DoInstallDarkTheme();
}

// Opens About Box
PLUGINCOMMAND Plugin::AboutMe()
{
    // Dialog boxes need to be static unless it's modal.
    static AboutDialog aboutDialog = {};

    if (!aboutDialog.isCreated())
        aboutDialog.init(Instance().DllHModule(), Instance().NotepadHwnd());

    if (!aboutDialog.isVisible())
        aboutDialog.doDialog();
}

#pragma endregion Plugin User Interfacing
