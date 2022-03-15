/** @file PluginMain.cpp
 * Controls the Plugin functions, processes all Plugin messages
 * 
 * The ACTUAL DLL Entry point is defined in PluginInterface.cpp
 * 
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"

//#include <Windows.h>
//#include <string>
//#include <sstream>
//#include <stdexcept>
//#include <ShlObj.h>
//#include <codecvt>
//#include <locale>
//#include <cwchar>
//#include <Shlwapi.h>


#include "Common.h"
#include "LexerCatalogue.h"

#include "PluginMain.h"
#include "PluginControlsRC.h"

#include "NWScriptParser.h"

#include "AboutDialog.h"
#include "BatchProcessing.h"
#include "CompilerSettings.h"
#include "FileParseSummaryDialog.h"
#include "PathAccessDialog.h"
#include "ProcessFilesDialog.h"
#include "WarningDialog.h"

#include "XMLGenStrings.h"

#pragma warning (disable : 6387)

using namespace NWScriptPlugin;
using namespace LexerInterface;

// Static members definition
generic_string Plugin::pluginName = TEXT("NWScript Tools");

// Menu functions order. Needs to be Sync'ed with pluginFunctions[]
#define PLUGINMENU_SWITCHAUTOINDENT 0
#define PLUGINMENU_DASH1 1
#define PLUGINMENU_COMPILESCRIPT 2
#define PLUGINMENU_DISASSEMBLESCRIPT 3
#define PLUGINMENU_BATCHPROCESSING 4
#define PLUGINMENU_RUNLASTBATCH 5
#define PLUGINMENU_SETTINGS 6
#define PLUGINMENU_DASH2 7
#define PLUGINMENU_INSTALLDARKTHEME 8
#define PLUGINMENU_IMPORTDEFINITIONS 9
#define PLUGINMENU_RESETEDITORCOLORS 10
#define PLUGINMENU_DASH3 11
#define PLUGINMENU_ABOUTME 12

FuncItem Plugin::pluginFunctions[] = {
    {TEXT("Use auto-identation"), Plugin::SwitchAutoIndent},
    {TEXT("---")},
    {TEXT("Compile script"), Plugin::CompileScript},
    {TEXT("Disassemble file..."), Plugin::DisassembleFile},
    {TEXT("Batch script processing..."), Plugin::BatchProcessFiles},
    {TEXT("Run last batch setup"), Plugin::RunLastBatch},
    {TEXT("Compiler settings..."), Plugin::CompilerSettings},
    {TEXT("---")},
    {TEXT("Install Dark Theme"), Plugin::InstallDarkTheme},
    {TEXT("Import NWScript definitions"), Plugin::ImportDefinitions},
    {TEXT("reset editor colors"), Plugin::ResetEditorColors},
    {TEXT("---")},
    {TEXT("About me"), Plugin::AboutMe},
};

Plugin* Plugin::_instance(nullptr);

// Notepad Plugin Configuration installation root directory. We made it const, since we're not retrieving this outside a plugin build 
const generic_string NotepadPluginConfigRootDir = TEXT("config\\");
// Notepad Default Themes install directory. We made it const, since we're not retrieving this outside a plugin build
const generic_string NotepadDefaultThemesRootDir = TEXT("themes\\");
// Notepad Default Dark Theme file. We made it const, since we're not retrieving this outside a plugin build
const generic_string NotepadDefaultDarkThemeFile = TEXT("DarkModeDefault.xml");
// Notepad Default Auto Completion directory.
const generic_string NotepadAutoCompleteRootDir = TEXT("autoCompletion\\");
// Default pseudo-batch file to create in case we need to restart notepad++
const generic_string NotepadPseudoBatchRestartFile = TEXT("~doNWScriptNotepadRestart.bat");

// Bellow is a list of FIXED keywords NWScript engine uses and since it is part of the base syntax, hardly
// this will ever change, so we made them constants here.
const generic_string fixedPreProcInstructionSet = TEXT("#define #include");
const generic_string fixedInstructionSet = TEXT("break case continue default do else FALSE for if return switch TRUE while");
const generic_string fixedKeywordSet = TEXT("action command const float int string struct vector void");
const generic_string fixedObjKeywordSet = TEXT("object OBJECT_INVALID OBJECT_SELF");

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
    Instance()._pluginPaths.insert({ "PluginPath", fs::path(fTemp) });

    Instance()._pluginFileName = Instance()._pluginPaths["PluginPath"].stem();
    Instance()._pluginLexerConfigFile = Instance()._pluginPaths["PluginPath"].stem();
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
    _messageInstance.SetData(data);
    _indentor.SetMessenger(_messageInstance);

    //Keep a copy of Notepad HWND to easy access
    _notepadHwnd = Messenger().GetNotepadHwnd();

    // Finish initializing the Plugin Metainformation
    generic_string path;

    TCHAR fName[MAX_PATH+1] = {};
    ZeroMemory(fName, sizeof(fName));
    Messenger().SendNppMessage(NPPM_GETNPPFULLFILEPATH, static_cast<WPARAM>(MAX_PATH), reinterpret_cast<LPARAM>(fName));
    path = fName;
    _pluginPaths.insert({ "NotepadExecutablePath", fs::path(fName) });

    path = _pluginPaths["NotepadExecutablePath"].parent_path();
    path.append(TEXT("\\"));
    path.append(NotepadDefaultThemesRootDir);
    path.append(NotepadDefaultDarkThemeFile);
    _pluginPaths.insert({ "NotepadDarkThemeFilePath", fs::path(path) });

    path = _pluginPaths["NotepadExecutablePath"].parent_path();
    path.append(TEXT("\\"));
    path.append(NotepadAutoCompleteRootDir);
    path.append(str2wstr(LexerCatalogue::GetLexerName(0, true)));  // Need lowercase name. Also we're only returning the first lexer
    path.append(TEXT(".xml"));
    _pluginPaths.insert({ "PluginAutoCompleteFilePath", fs::path(path) });

    ZeroMemory(fName, sizeof(fName));
    Messenger().SendNppMessage(NPPM_GETPLUGINHOMEPATH, static_cast<WPARAM>(MAX_PATH), reinterpret_cast<LPARAM>(fName));
    path = fName;
    path.append(TEXT("\\"));
    path.append(NotepadPluginConfigRootDir);
    path.append(_pluginLexerConfigFile);
    _pluginPaths.insert({ "PluginLexerConfigFilePath", fs::path(path) });

    // Settings directory is different than Plugin Install Dir.
    ZeroMemory(fName, sizeof(fName));
    Messenger().SendNppMessage(NPPM_GETPLUGINSCONFIGDIR, static_cast<WPARAM>(MAX_PATH), reinterpret_cast<LPARAM>(fName));
    generic_string sPluginConfigPath = fName;
    sPluginConfigPath.append(TEXT("\\"));
    sPluginConfigPath.append(_pluginFileName);
    sPluginConfigPath.append(TEXT(".ini"));

    // Temporary batch we write in case we need to restart Notepad++ by ourselves. Root is Plugin config path
    path = fName;
    path.append(TEXT("\\"));
    path.append(NotepadPseudoBatchRestartFile);
    _pluginPaths.insert({ "NotepadPseudoBatchRestartFile", fs::path(path) });

    // Create settings instance and load all values
    _settings.InitSettings(sPluginConfigPath);
    Settings().Load();

    // Adjust menu "Use Auto-Indentation" checked or not before creation
    pluginFunctions[PLUGINMENU_SWITCHAUTOINDENT]._init2Check = Settings().enableAutoIndentation;
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

        SetupPluginMenuItems();
        _isReady = true;

        // Auto call a function that required restart during the previous session (because of privilege elevation)
        // Up to now...
        // 1 = ImportDefinitions
        // 2 = Fix Editor's Colors
        // Since all functions that required restart must have returned in Admin Mode, we check this
        // to see if the user didn't cancel the UAC request.
        // Also break processing here, since those functions can call another restart
        if (IsUserAnAdmin())
        {
            if (Settings().notepadRestartFunction == RestartFunctionHook::ResetEditorColorsPhase1)
            {
                DoResetEditorColors(Settings().notepadRestartFunction);
                break;
            }
            if (Settings().notepadRestartFunction == RestartFunctionHook::InstallDarkModePhase1)
            {
                DoInstallDarkTheme(Settings().notepadRestartFunction);
                break;
            }
        }

        // And then make sure to clear the hooks, temp files, etc. 
        // Call as immediate to instant save on settings so we avoid losing it on a session crash also.
        SetRestartHook(RestartMode::None, RestartFunctionHook::None);

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

        // If we have a restart hook setup, call out shell to execute it.
        if (Settings().notepadRestartMode != RestartMode::None)
        {
           runProcess(Settings().notepadRestartMode == RestartMode::Admin ? true : false, 
              Instance()._pluginPaths["NotepadPseudoBatchRestartFile"].c_str());
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
            && Settings().enableAutoIndentation)
            Indentor().IndentLine(static_cast<TCHAR>(notifyCode->ch));
        break;
    }
    default:
        return;
    }
}

void Plugin::SetRestartHook(RestartMode type, RestartFunctionHook function)
{
    Settings().notepadRestartMode = type; Settings().notepadRestartFunction = function;
    if (type == RestartMode::None)
    {
        if (PathFileExists(_pluginPaths["NotepadPseudoBatchRestartFile"].c_str()))
            DeleteFile(_pluginPaths["NotepadPseudoBatchRestartFile"].c_str());
    }
    else
        writePseudoBatchExecute(_pluginPaths["NotepadPseudoBatchRestartFile"].c_str(), _pluginPaths["NotepadExecutablePath"].c_str());

    Settings().Save();
}

#pragma endregion Plugin DLL Message Processing

#pragma region 

// Set the Auto-Indent type for all of this Plugin's installed languages
void Plugin::SetAutoIndentSupport()
{
    PluginMessenger& msg = Messenger();
    bool lexerSearch = false;
    ExternalLexerAutoIndentMode langIndent = ExternalLexerAutoIndentMode::Standard;

    // Try to set Plugin's Lexers Auto-Indentation. Older versions of NPP will return langSearch=FALSE (cause this message won't exist).
    generic_stringstream lexerNameW;
    for (int i = 0; i < LexerCatalogue::GetLexerCount(); i++)
    {
        lexerNameW = {};
        lexerNameW << str2wstr(LexerCatalogue::GetLexerName(i));

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

        // Remove the "Use Auto-Indent" menu command and the following separator.
        RemovePluginMenuItem(PLUGINMENU_SWITCHAUTOINDENT);
        RemovePluginMenuItem(PLUGINMENU_DASH1);

        // Auto-adjust the settings
        Instance().Settings().enableAutoIndentation = false;
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
        lexerNameW << str2wstr(LexerCatalogue::GetLexerName(i));
        isPluginLanguage = (_tcscmp(lexerName, lexerNameW.str().c_str()) == 0);

        if (isPluginLanguage)
            lexerSearch = msg.SendNppMessage<int>(NPPM_GETEXTERNALLEXERAUTOINDENTMODE,
                reinterpret_cast<WPARAM>(lexerNameW.str().c_str()), reinterpret_cast<LPARAM>(&langIndent));
    }

    //Update Lexer
    _notepadCurrentLexer.SetLexer(currLang, lexerName, isPluginLanguage, langIndent);
}

// Detects if Dark Theme is already installed
void Plugin::DetectDarkThemeInstall()
{
    // Here we are parsing the file silently
    tinyxml2::XMLDocument darkThemeDoc;
    errno_t error = darkThemeDoc.LoadFile(wstr2str(Instance()._pluginPaths["NotepadDarkThemeFilePath"].c_str()).c_str());

    if (error)
    {
        RemovePluginMenuItem(PLUGINMENU_INSTALLDARKTHEME);
        _pluginDarkThemeIs = DarkThemeStatus::Unsupported;
        return;
    }

    // Try to navigate to the XML node corresponding to the name of the installed language (nwscript). If not found, means uninstalled.
    if (!searchElement(darkThemeDoc.RootElement(), "LexerType", "name", LexerCatalogue::GetLexerName(0)))
        _pluginDarkThemeIs = DarkThemeStatus::Uninstalled;
    else
    {    
        //Dark theme installed, mark it here.
        RemovePluginMenuItem(PLUGINMENU_INSTALLDARKTHEME);
        _pluginDarkThemeIs = DarkThemeStatus::Installed;
    }
}

// Handling functions for plugin menu
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

void Plugin::EnablePluginMenuItem(int commandID, bool enabled)
{
    HMENU hMenu = GetNppMainMenu();
    if (hMenu)
    {
        EnableMenuItem(hMenu, GetFunctions()[commandID]._cmdID, (MF_BYCOMMAND | (enabled)) ? MF_ENABLED : MF_DISABLED );
    }
}

void Plugin::RemovePluginMenuItem(int commandID)
{
    HMENU hMenu = GetNppMainMenu();
    if (hMenu)
    {
        RemoveMenu(hMenu, pluginFunctions[commandID]._cmdID, MF_BYCOMMAND);
    }
}

bool Plugin::SetPluginMenuItemIcon(int commandID, int resourceID, bool bSetToUncheck, bool bSetToCheck)
{

    HMENU hMenu = GetNppMainMenu();
    if (hMenu)
    {
        HBITMAP hIconBmp = iconToBitmap(reinterpret_cast<HICON>(LoadImage(_dllHModule, MAKEINTRESOURCE(resourceID),
            IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR)));
        bool bSuccess = false;
        if (bSetToUncheck && bSetToCheck)
            bSuccess = SetMenuItemBitmaps(hMenu, GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, hIconBmp, hIconBmp);
        if (bSetToUncheck && !bSetToCheck)
            bSuccess = SetMenuItemBitmaps(hMenu, GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, hIconBmp, NULL);
        if (!bSetToUncheck && bSetToCheck)
            bSuccess = SetMenuItemBitmaps(hMenu, GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, NULL, hIconBmp);
        return bSuccess;
    }

    return false;

}

bool Plugin::SetPluginStockMenuItemIcon(int commandID, SHSTOCKICONID stockIconID, bool bSetToUncheck = true, bool bSetToCheck = true)
{
    HMENU hMenu = GetNppMainMenu();
    if (hMenu)
    {
        HBITMAP hIconBmp = getStockIconBitmap(stockIconID, IconSize::Size16x16);
        bool bSuccess = false;
        if (bSetToUncheck && bSetToCheck)
            bSuccess = SetMenuItemBitmaps(hMenu, GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, hIconBmp, hIconBmp);
        if (bSetToUncheck && !bSetToCheck)
            bSuccess = SetMenuItemBitmaps(hMenu, GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, hIconBmp, NULL);
        if (!bSetToUncheck && bSetToCheck)
            bSuccess = SetMenuItemBitmaps(hMenu, GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, NULL, hIconBmp);

        return bSuccess;
    }

    return false;
}

void Plugin::SetupPluginMenuItems()
{
    bool bSuccessLexer = false;
    bool bSuccessDark = false;
    bool bAutoComplete = false;
    PathWritePermission fLexerPerm = PathWritePermission::UndeterminedError;
    PathWritePermission fDarkThemePerm = PathWritePermission::UndeterminedError;
    PathWritePermission fAutoCompletePerm = PathWritePermission::UndeterminedError;

    // Don't use the shield icons when user runs in Administrator mode
    if (!IsUserAnAdmin())
    {
        // Retrieve write permissions for _pluginLexerConfigFile and _notepadDarkThemeFilePath
        bSuccessLexer = checkWritePermission(_pluginPaths["PluginLexerConfigFilePath"], fLexerPerm);
        bSuccessDark = checkWritePermission(_pluginPaths["NotepadDarkThemeFilePath"], fDarkThemePerm);
        bAutoComplete = checkWritePermission(_pluginPaths["PluginAutoCompleteFilePath"], fAutoCompletePerm);

        // If this file do not exist, we test the directory instead
        if (!bAutoComplete)
            bAutoComplete = checkWritePermission(_pluginPaths["PluginAutoCompleteFilePath"].parent_path(), fAutoCompletePerm);

        if (!bSuccessLexer)
        {
            generic_stringstream sError;
            sError << TEXT("Unable to read information for file bellow: ") << "\r\n";
            sError << _pluginPaths["PluginLexerConfigFilePath"] << "\r\n";
            sError << "\r\n";
            sError << "NWScript Plugin may not work properly, please check your installation!";

            ::MessageBox(_notepadHwnd, sError.str().c_str(), pluginName.c_str(), MB_ICONERROR | MB_APPLMODAL | MB_OK);
        }

        // For users without permission to _pluginLexerConfigFilePath or _notepadAutoCompleteInstallPath, set shield on Import Definitions
        if (fLexerPerm == PathWritePermission::RequiresAdminPrivileges || fAutoCompletePerm == PathWritePermission::RequiresAdminPrivileges)
            SetPluginStockMenuItemIcon(PLUGINMENU_IMPORTDEFINITIONS, SHSTOCKICONID::SIID_SHIELD, true, false);
        // For users without permission to _notepadDarkThemeFilePath, set shield on Install Dark Theme if not already installed
        if (fDarkThemePerm == PathWritePermission::RequiresAdminPrivileges && _pluginDarkThemeIs == DarkThemeStatus::Uninstalled)
            SetPluginStockMenuItemIcon(PLUGINMENU_INSTALLDARKTHEME, SHSTOCKICONID::SIID_SHIELD, true, false);
        // For users without permissions to any of the files (and also only checks Dark Theme support if file is existent and supported/not corrupted)...
        if (fLexerPerm == PathWritePermission::RequiresAdminPrivileges || (fDarkThemePerm == PathWritePermission::RequiresAdminPrivileges && _pluginDarkThemeIs != DarkThemeStatus::Unsupported))
            SetPluginStockMenuItemIcon(PLUGINMENU_RESETEDITORCOLORS, SHSTOCKICONID::SIID_SHIELD, true, false);
    }

    // Setup icons for Compiler, settings, about me...
    SetPluginStockMenuItemIcon(PLUGINMENU_COMPILESCRIPT, SHSTOCKICONID::SIID_DOCASSOC, true, false);
    SetPluginMenuItemIcon(PLUGINMENU_SETTINGS, IDI_SETTINGS, true, false);
    SetPluginStockMenuItemIcon(PLUGINMENU_ABOUTME, SHSTOCKICONID::SIID_INFO, true, false);

    // Menu run last batch: initially disabled
    EnablePluginMenuItem(PLUGINMENU_RUNLASTBATCH, false);
}

#pragma endregion Plugin menu handling

#pragma endregion Plugin initialization functions and dynamic behavior

#pragma region

void Plugin::DoBatchProcessFiles(HRESULT decision)
{
    static ProcessFilesDialog processing;

    if ((int)decision == IDCANCEL || (int)decision == IDCLOSE)
        return;

    if (!processing.isCreated())
    {
        processing.init(Instance().DllHModule(), Instance().NotepadHwnd());
    }

    processing.doDialog();

    // Enable run last batch menu
    Instance().EnablePluginMenuItem(PLUGINMENU_RUNLASTBATCH, true);
}

void Plugin::DoImportDefinitionsCallback(HRESULT decision)
{
    // Trash results for memory space in a cancel.
    if (static_cast<int>(decision) == IDCANCEL || static_cast<int>(decision) == IDNO)
    {
        Instance()._NWScriptParseResults.reset();
        return;
    }

    NWScriptParser::ScriptParseResults& myResults = *Instance()._NWScriptParseResults;
    tinyxml2::XMLDocument nwscriptDoc;

    // We retrieve all fixed keywords and emplace them on results, so we can sort everything out
    // because unsorted results won't work for auto-complete
    generic_string kw = fixedPreProcInstructionSet;
    kw.append(TEXT(" ")).append(fixedInstructionSet).append(TEXT(" ")).append(fixedKeywordSet).append(TEXT(" ")).append(fixedObjKeywordSet);
    myResults.AddSpacedStringAsKeywords(kw);
    myResults.Sort();

    // Set some Timestamp headers
    char timestamp[128]; time_t currTime;  struct tm currTimeP;
    time(&currTime);
    errno_t error = localtime_s(&currTimeP, &currTime);
    strftime(timestamp, 64, "Creation timestamp is: %B %d, %Y - %R", &currTimeP);
    std::string xmlHeaderComment = XMLDOCHEADER;
    xmlHeaderComment.append(timestamp).append(".\n");

    // Since TinyXML only accepts ASCII filenames, we do a crude conversion here... hopefully we won't have any
    // intermediary directory using chinese characters here... :P
    std::string asciiFileStyler = wstr2str(Instance()._pluginPaths["PluginLexerConfigFilePath"]);
    error = nwscriptDoc.LoadFile(asciiFileStyler.c_str());
    generic_stringstream errorStream;
    if (error)
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        Instance()._NWScriptParseResults.reset();
        return;
    }

    // Call helper function to strip all comments from document, since we're merging the file, not recreating it.
    // We don't use nwscriptDoc.rootNode() here, since it will jump straight to the first ELEMENT node - ignoring
    // comments and other possible pieces of information.
    stripXMLInfo(nwscriptDoc.FirstChild());

    tinyxml2::XMLElement* notepadPlus = nwscriptDoc.RootElement();
    if (notepadPlus == NULL)
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("Cannot find root XML node 'NotepadPlus'!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        Instance()._NWScriptParseResults.reset();
        return;
    }

    // Add new declaration and header
    nwscriptDoc.InsertFirstChild(nwscriptDoc.NewDeclaration());
    nwscriptDoc.InsertAfterChild(nwscriptDoc.FirstChild(), nwscriptDoc.NewComment(xmlHeaderComment.c_str()));

    tinyxml2::XMLElement* languages = notepadPlus->FirstChildElement("Languages");
    if (languages == NULL)
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("Cannot find XML node 'Languages'!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        Instance()._NWScriptParseResults.reset();
        return;
    }
    // Add info on Languages
    languages->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINLANGUAGECOMMENT));

    tinyxml2::XMLElement* language = languages->FirstChildElement("Language");
    if (language == NULL)
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("Cannot find root XML node 'Language'!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        Instance()._NWScriptParseResults.reset();
        return;
    }

    // We are only supporting our default lexer here
    if (!language->Attribute("name", LexerCatalogue::GetLexerName(0).c_str()))
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("Language name for ") << str2wstr(LexerCatalogue::GetLexerName(0)) << TEXT(" not found!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        Instance()._NWScriptParseResults.reset();
        return;
    }
    // Add info on Keywords
    language->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINKEYWORDCOMMENT));

    // Now we want just the Keywords for "type2" (Engine Structures), "type4  (Engine Constants) and "type6" (Function Names)
    // Look for at least 1 keyword element
    tinyxml2::XMLElement* Keywords = language->FirstChildElement("Keywords");
    if (Keywords == NULL)
    {
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("Cannot find root XML node 'Keywords'!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        Instance()._NWScriptParseResults.reset();
        return;
    }

    // Need to convert from wchar_t to char here...
    // We use a post-check to see whether all tags where updated. Also useful to avoid processing same tag twice 
    // (only happens if user tampered) with the file.
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
        errorStream << TEXT("Error while parsing file: ") << Instance()._pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("The following nodes could not be found!\r\n");
        errorStream << TEXT("Nodes: [") << (!bType2 ? TEXT(" type2") : TEXT("")) << (!bType4 ? TEXT(" type4") : TEXT(""))
            << (!bType6 ? TEXT(" type6") : TEXT("")) << TEXT(" ]");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        Instance()._NWScriptParseResults.reset();
        return;
    }

    //Now add comment info to LexerStyles. Here we don't manage errors since its only comments
    tinyxml2::XMLElement* lexerStyles = notepadPlus->FirstChildElement("LexerStyles");
    if (lexerStyles)
    {
        tinyxml2::XMLNode* lexerType = lexerStyles->FirstChild();
        if (lexerType)
            lexerType->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINWORDSTYLECOMMENT));
        lexerStyles->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINLEXERTYPECOMMENT));
    }

    // Now we overwrite autoComplete XML. We're doing this before saving the other
    // so we do a more or less of a transactioned modification - if one fails, the other isn't saved.
    std::string asciiFileAutoC = wstr2str(Instance()._pluginPaths["PluginAutoCompleteFilePath"]);
    tinyxml2::XMLDocument nwscriptAutoc;
    nwscriptAutoc.InsertFirstChild(nwscriptAutoc.NewDeclaration());
    nwscriptAutoc.InsertEndChild(nwscriptAutoc.NewComment(xmlHeaderComment.c_str()));

    tinyxml2::XMLNode* autocRoot = nwscriptAutoc.NewElement("NotepadPlus");
    nwscriptAutoc.InsertEndChild(autocRoot);

    autocRoot->InsertEndChild(nwscriptAutoc.NewComment(XMLAUTOCLANGCOMMENT));
    tinyxml2::XMLElement* autocNode = nwscriptAutoc.NewElement("AutoComplete");
    autocNode->SetAttribute("language", LexerCatalogue::GetLexerName(0).c_str());
    autocRoot->InsertEndChild(autocNode);

    autocNode->InsertEndChild(nwscriptAutoc.NewComment(XMLAUTOCENVCOMMENT));
    tinyxml2::XMLElement* environmentNode = nwscriptAutoc.NewElement("Environment");
    environmentNode->SetAttribute("ignoreCase", "no"); environmentNode->SetAttribute("startFunc", "("); environmentNode->SetAttribute("stopFunc", ")");
    environmentNode->SetAttribute("paramSeparator", ","); environmentNode->SetAttribute("terminal", ";"); environmentNode->SetAttribute("additionalWordChar", "");
    autocNode->InsertEndChild(environmentNode);

    autocNode->InsertEndChild(nwscriptAutoc.NewComment(XMLAUTOCSORTNOTICE));

    // Now, we iterate through all parsing results and add their nodes to autocNode
    for (NWScriptParser::ScriptMember m : myResults.Members)
    {
        tinyxml2::XMLElement* keyword = nwscriptAutoc.NewElement("KeyWord");
        keyword->SetAttribute("name", wstr2str(m.sName).c_str());
        if (m.mID == NWScriptParser::MemberID::Function)
        {
            keyword->SetAttribute("func", "yes");
            tinyxml2::XMLElement* overload = nwscriptAutoc.NewElement("Overload");
            overload->SetAttribute("retVal", wstr2str(m.sType).c_str());

            for (NWScriptParser::ScriptParamMember p : m.params)
            {
                tinyxml2::XMLElement* param = nwscriptAutoc.NewElement("Param");
                generic_string paramName = p.sType.c_str();
                paramName.append(TEXT(" ")).append(p.sName);
                if (!p.sDefaultValue.empty())
                    paramName.append(TEXT("=")).append(p.sDefaultValue);
                param->SetAttribute("name", wstr2str(paramName).c_str());

                overload->InsertEndChild(param);
            }
            keyword->InsertEndChild(overload);
        }
        autocNode->InsertEndChild(keyword);
    }

    // Finally, save files...
    error = nwscriptAutoc.SaveFile(asciiFileAutoC.c_str());
    if (error)
    {
        errorStream << TEXT("Error while saving file: ") << Instance()._pluginPaths["PluginAutoCompleteFilePath"] << "! \r\n";
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        Instance()._NWScriptParseResults.reset();
        return;
    }
    error = nwscriptDoc.SaveFile(asciiFileStyler.c_str());
    if (error)
    {
        errorStream << TEXT("Error while saving file: ") << Instance()._pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(Instance().NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        Instance()._NWScriptParseResults.reset();
        return;
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
    // For safeguard, reset all hooks, since we can be autocalling this and get another error.
    SetRestartHook(RestartMode::None, RestartFunctionHook::None);

    if (!PatchDarkThemeXMLFile())
        return;

    if (!PatchDefaultThemeXMLFile())
        return;

    // Do restart. If it came from a previous restart callback, do it automatically. Else ask for it.
    if (whichPhase == RestartFunctionHook::ResetEditorColorsPhase1)
    {
        SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
        Messenger().SendNppMessage(WM_CLOSE, 0, 0);
    }
    else
    {
        int mResult = MessageBox(NotepadHwnd(),
            TEXT("Notepad++ needs to be restarted for the new settings to be reflected. Do it now?"),
            TEXT("Patch successful!"), MB_YESNO | MB_ICONINFORMATION);

        if (mResult == IDYES)
        {
            SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
            Messenger().SendNppMessage(WM_CLOSE, 0, 0);
        }
    }
}

void Plugin::DoInstallDarkTheme(RestartFunctionHook whichPhase)
{
    // For safeguard, reset all hooks, since we can be autocalling this and get another error.
    SetRestartHook(RestartMode::None, RestartFunctionHook::None);

    if (!PatchDarkThemeXMLFile())
        return;

    // Do restart. If it came from a previous restart callback, do it automatically. Else ask for it.
    if (whichPhase == RestartFunctionHook::InstallDarkModePhase1)
    {
        SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
        Messenger().SendNppMessage(WM_CLOSE, 0, 0);
    }
    else
    {
        int mResult = MessageBox(NotepadHwnd(),
            TEXT("Notepad++ needs to be restarted for the new settings to be reflected. Do it now?"),
            TEXT("Patch successful!"), MB_YESNO | MB_ICONINFORMATION);

        if (mResult == IDYES)
        {
            SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
            Messenger().SendNppMessage(WM_CLOSE, 0, 0);
        }
    }
}

bool Plugin::PatchDefaultThemeXMLFile()
{
    tinyxml2::XMLDocument defaultThemeXML;

    std::string asciiFileStyler = wstr2str(_pluginPaths["PluginLexerConfigFilePath"]);
    errno_t error = defaultThemeXML.LoadFile(asciiFileStyler.c_str());
    generic_stringstream errorStream;
    if (error)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        errorStream << TEXT("Error ID: ") << defaultThemeXML.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    // Try to navigate to LexerType named "NWScript"
    // Here we use the plugin ProperCase name.
    std::string lexerName = LexerCatalogue::GetLexerName(0);
    tinyxml2::XMLElement* lexerType = searchElement(defaultThemeXML.RootElement(), "LexerType", "name", lexerName);
    if (!lexerType)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    // Get first WordsStyle tag
    tinyxml2::XMLElement* lexerWordsStyle = lexerType->FirstChildElement("WordsStyle");
    if (!lexerWordsStyle)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    // Patch the document. Since here we are preserving any existing keywords, we want only attributes to be redefined.
    // We don't check for error for our own defined XML.
    tinyxml2::XMLDocument xmlPatch;
    xmlPatch.Parse(XMLDEFAULTSTYLER);
    tinyxml2::XMLNode* xmlPatchNode = searchElement(xmlPatch.RootElement(), "LexerType")->DeepClone(&defaultThemeXML);
    tinyxml2::XMLElement* patchTypeStart = xmlPatchNode->FirstChildElement("WordsStyle");
    tinyxml2::XMLElement* patchTypeSeek;

    bool bFound;
    while (lexerWordsStyle)
    {
        bFound = false;
        patchTypeSeek = patchTypeStart;

        // Search by ID value
        while (patchTypeSeek && !bFound)
        {
            // Convert to string to make comparisons less troublesome
            std::string first = lexerWordsStyle->Attribute("styleID");
            std::string second = patchTypeSeek->Attribute("styleID");
            if (first == second)
            {
                bFound = true;
                lexerWordsStyle->SetAttribute("fgColor",   patchTypeSeek->Attribute("fgColor"));
                lexerWordsStyle->SetAttribute("bgColor",   patchTypeSeek->Attribute("bgColor"));
                lexerWordsStyle->SetAttribute("fontName",  patchTypeSeek->Attribute("fontName"));
                lexerWordsStyle->SetAttribute("fontStyle", patchTypeSeek->Attribute("fontStyle"));
                lexerWordsStyle->SetAttribute("fontSize",  patchTypeSeek->Attribute("fontSize"));
            }
            patchTypeSeek = patchTypeSeek->NextSiblingElement("WordsStyle");
        }
        lexerWordsStyle = lexerWordsStyle->NextSiblingElement("WordsStyle");
    }

    // Save.
    error = defaultThemeXML.SaveFile(asciiFileStyler.c_str());
    if (error)
    {
        errorStream << TEXT("Error while patching file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("Error ID: ") << defaultThemeXML.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

bool Plugin::PatchDarkThemeXMLFile()
{
    tinyxml2::XMLDocument darkThemeXML;

    std::string asciiFileStyler = wstr2str(_pluginPaths["NotepadDarkThemeFilePath"]);
    errno_t error = darkThemeXML.LoadFile(asciiFileStyler.c_str());
    generic_stringstream errorStream;
    if (error)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["NotepadDarkThemeFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        errorStream << TEXT("Error ID: ") << darkThemeXML.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    // Try to navigate to LexerStyles
    tinyxml2::XMLElement* lexerStyles = searchElement(darkThemeXML.RootElement(), "LexerStyles");
    if (!lexerStyles)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["NotepadDarkThemeFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    // Make sure we are not creating our stylesheet twice (eg: from a reset Editor Colors call).
    tinyxml2::XMLElement* oldLexerType = searchElement(darkThemeXML.RootElement(), "LexerType", "name", LexerCatalogue::GetLexerName(0));
    if (oldLexerType)
        oldLexerType->Parent()->DeleteChild(oldLexerType);

    // Now, find an appropriate place to put our patch. Other language names are lowercase, but strangely
    // to external lexers, we must have a ProperCase name or else Notepad++ don't recognize it. So here
    // we search the insert place using lowercase name, but then we patch using our Proper name.
    std::string lexerName = LexerCatalogue::GetLexerName(0, true);

    tinyxml2::XMLElement* lexerType = lexerStyles->FirstChildElement("LexerType");
    bool bFound = false;
    tinyxml2::XMLElement* nextLexerType;

    while (lexerType)
    {
        // Found a possible spot?
        if (lexerName > lexerType->Attribute("name"))
        {
            // We peek ahead of this node to see if our name is properly sorted into the array.
            // Null next node mean we reached the list's end and so mark that as a usable space.
            nextLexerType = lexerType->NextSiblingElement("LexerType");
            if (nextLexerType)
            {
                if (lexerName < nextLexerType->Attribute("name"))
                    break;
            }
            else
                break;
        }
        lexerType = lexerType->NextSiblingElement("LexerType");
    }

    // Last check...
    if (!lexerType)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["NotepadDarkThemeFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    // Patch file. We don't check for error for our own defined XML.
    tinyxml2::XMLDocument xmlPatch;
    xmlPatch.Parse(XMLDARKMODEDEFAULT);
    tinyxml2::XMLNode* xmlPatchNode = searchElement(xmlPatch.RootElement(), "LexerType")->DeepClone(&darkThemeXML);
    lexerStyles->InsertAfterChild(lexerType, xmlPatchNode);

    // Save.
    error = darkThemeXML.SaveFile(asciiFileStyler.c_str());
    if (error)
    {
        errorStream << TEXT("Error while patching file: ") << _pluginPaths["NotepadDarkThemeFilePath"] << "! \r\n";
        errorStream << TEXT("Error ID: ") << darkThemeXML.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

#pragma endregion Plugin files management


#pragma region

Plugin::PathCheckResults Plugin::WritePermissionCheckup(const std::vector<generic_string>& paths, RestartFunctionHook iFunctionToCallIfRestart)
{
    struct stCheckedPaths {
        bool bExists;
        generic_string path;
        PathWritePermission fPerm;
    };

    std::vector<stCheckedPaths> checkedPaths;
    checkedPaths.reserve(paths.size());

    // Batch check files
    for (const generic_string& s : paths)
    {
        bool bExists = false;
        PathWritePermission fPerm = PathWritePermission::UndeterminedError;

        bExists = checkWritePermission(s, fPerm);
        checkedPaths.emplace_back(stCheckedPaths{ bExists, s, fPerm });
    }

    // Check for Admin privileges required first
    std::vector<generic_string> sWhichPaths;
    for (stCheckedPaths& st : checkedPaths)
    {
        if (st.fPerm == PathWritePermission::RequiresAdminPrivileges)
            sWhichPaths.push_back(st.path);
    }
    if (sWhichPaths.size() > 0)
    {
        // Show File Access dialog box in Admin Mode
        PathAccessDialog ePermission = {};
        ePermission.init(DllHModule(), NotepadHwnd());
        ePermission.SetAdminMode(true);
        ePermission.SetIcon(SHSTOCKICONID::SIID_SHIELD);
        ePermission.SetWarning(TEXT("WARNING - this action requires write permission to the following files/directories:"));
        ePermission.SetPathsText(sWhichPaths);
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

        return PathCheckResults::RequiresAdminPrivileges;
    }

    // Check for read-only second
    sWhichPaths.clear();
    for (stCheckedPaths& st : checkedPaths)
    {
        if (st.fPerm == PathWritePermission::FileIsReadOnly)
            sWhichPaths.push_back(st.path);
    }
    if (sWhichPaths.size() > 0)
    {
        // Show File Access dialog box in Normal Mode
        PathAccessDialog ePermission = {};
        ePermission.init(DllHModule(), NotepadHwnd());
        ePermission.SetAdminMode(false);
        ePermission.SetIcon(SHSTOCKICONID::SIID_ERROR);
        ePermission.SetWarning(TEXT("ERROR: The following file(s) are marked as 'Read-Only' and cannot be changed:"));
        ePermission.SetPathsText(sWhichPaths);
        ePermission.SetSolution(TEXT("To solve this: \r\n  - Please, provide the necessary permissions to use this option.\r\n \
  Find it in Windows Explorer, select Properties -> Uncheck Read-Only flag. "));
        INT_PTR ignore = ePermission.doDialog();

        return PathCheckResults::ReadOnly;
    }

    // Check for blocked by application third
    sWhichPaths.clear();
    for (stCheckedPaths& st : checkedPaths)
    {
        if (st.fPerm == PathWritePermission::BlockedByApplication)
            sWhichPaths.push_back(st.path);
    }
    if (sWhichPaths.size() > 0)
    {
        // Show File Access dialog box in Normal Mode
        PathAccessDialog ePermission = {};
        ePermission.init(DllHModule(), NotepadHwnd());
        ePermission.SetAdminMode(false);
        ePermission.SetIcon(SHSTOCKICONID::SIID_DOCASSOC);
        ePermission.SetWarning(TEXT("ERROR: The following file(s) are currently blocked by other applications/processes and cannot be changed:"));
        ePermission.SetPathsText(sWhichPaths);
        ePermission.SetSolution(TEXT("To solve this:\r\n  - Please, close the files in other applications before trying this action again."));
        INT_PTR ignore = ePermission.doDialog();

        return PathCheckResults::BlockedByApplication;
    }

    // Finally, check for inexistent files and/or other unknown errors
    sWhichPaths.clear();
    for (stCheckedPaths& st : checkedPaths)
    {
        if (st.bExists == false || st.fPerm == PathWritePermission::UndeterminedError)
            sWhichPaths.push_back(st.path);
    }
    if (sWhichPaths.size() > 0)
    {
        // Show File Access dialog box in Normal Mode
        PathAccessDialog ePermission = {};
        ePermission.init(DllHModule(), NotepadHwnd());
        ePermission.SetAdminMode(false);
        ePermission.SetIcon(SHSTOCKICONID::SIID_DELETE);
        ePermission.SetWarning(TEXT("ERROR: The file(s) bellow are inexistent or could not be accessed:"));
        ePermission.SetPathsText(sWhichPaths);
        ePermission.SetSolution(TEXT("Some reasons might happen:\r\n  - Either you are running the plugin with a Notepad++ incompatible with this function; or \
you may have accidentaly deleted the file(s).\r\nPlease try reinstalling the products."));
        INT_PTR ignore = ePermission.doDialog();

        return PathCheckResults::UnknownError;
    }

    return PathCheckResults::CheckSuccess;
}

// Support for Auto-Indentation for old versions of Notepad++
PLUGINCOMMAND Plugin::SwitchAutoIndent()
{
    static bool bAutoIndentationWarningShown = false;
    // Dialog boxes need to be static unless it's modal.
    static WarningDialog warningDialog = {};

    // Change settings
    Instance().Settings().enableAutoIndentation = !Instance().Settings().enableAutoIndentation;
    bool bEnableAutoIndent = Instance().Settings().enableAutoIndentation;

    HMENU hMenu = Instance().GetNppMainMenu();
    if (hMenu)
    {
        CheckMenuItem(hMenu, pluginFunctions[0]._cmdID,
            MF_BYCOMMAND | ((bEnableAutoIndent) ? MF_CHECKED : MF_UNCHECKED));
    }

    // Already accepted the warning, either on this session or a previous one
    if (Instance().Settings().autoIndentationWarningAccepted)
        return;

    // Warning user of function: only once in a session (and perhaps in a lifetime if INI file doesn't change)
    if (bEnableAutoIndent && !bAutoIndentationWarningShown)
    {
        if (!warningDialog.isCreated())
            warningDialog.init(Instance().DllHModule(), Instance().NotepadHwnd());

        warningDialog.appendSettings(&Instance().Settings());

        if (!warningDialog.isVisible())
            warningDialog.doDialog();

        bAutoIndentationWarningShown = true;
    }
}

//-------------------------------------------------------------

// Compiles current .NSS Script file
PLUGINCOMMAND Plugin::CompileScript()
{
    // If compiler settings not initialized, run command, re-check - since it's modal.
    if (!Instance().Settings().compilerSettingsCreated)
        CompilerSettings();

    if (!Instance().Settings().compilerSettingsCreated)
        return;

}

// Disassemble a compiled script file
PLUGINCOMMAND Plugin::DisassembleFile()
{


}

// Menu Command "Run last successful batch" function handler. 
PLUGINCOMMAND Plugin::RunLastBatch()
{

}

// Opens the Plugin's Batch process files dialog
PLUGINCOMMAND Plugin::BatchProcessFiles()
{

    static BatchProcessingDialog batchProcessing = {};

    if (!batchProcessing.isCreated())
    {
        batchProcessing.setOkDialogCallback(&Plugin::DoBatchProcessFiles);
        batchProcessing.appendSettings(&Instance()._settings);
        batchProcessing.init(Instance().DllHModule(), Instance().NotepadHwnd());
    }

    if (!batchProcessing.isVisible())
        batchProcessing.doDialog();

}

// Opens the Plugin's Compiler Settings panel
PLUGINCOMMAND Plugin::CompilerSettings()
{
    static CompilerSettingsDialog compilerSettings = {};

    compilerSettings.init(Instance().DllHModule(), Instance().NotepadHwnd());
    compilerSettings.appendSettings(&Instance()._settings);
    compilerSettings.doDialog();
}

//-------------------------------------------------------------

// Installs Dark theme
PLUGINCOMMAND Plugin::InstallDarkTheme()
{
    // Do a file check for the necessary XML files
    std::vector<generic_string> sFiles;
    sFiles.push_back(Instance()._pluginPaths["NotepadDarkThemeFilePath"]);

    PathCheckResults fResult = Instance().WritePermissionCheckup(sFiles, RestartFunctionHook::InstallDarkModePhase1);
    if (static_cast<int>(fResult) < 1)
        return;

    Instance().DoInstallDarkTheme();
}

// Imports Lexer's functions and constants declarations from a NWScript.nss file
PLUGINCOMMAND Plugin::ImportDefinitions()
{

#define WARNINGMESSAGE R"(Warning: this action will replace any nwscript.nss engine definitions tags in the plugin configuration file with new ones.

All user's definitions outside reserved spaces will be preserved (read the "USAGE" section in the about box for more details). 

You wish to Continue?)"

    // Only warns the user once in a session.
    static bool bWarnedUser = false;

    // Initialize our results dialog
    static FileParseSummaryDialog parseDialog = {};
    if (!parseDialog.isCreated())
        parseDialog.init(Instance().DllHModule(), Instance().NotepadHwnd());

    // Someone called this twice!
    if (parseDialog.isVisible())
        return;

    // Do a file check for the necessary XML files
    std::vector<generic_string> sFiles;
    sFiles.push_back(Instance()._pluginPaths["PluginLexerConfigFilePath"]);
    // The auto complete file may or may not exist. If not exist, we check the directory permissions instead.
    if (PathFileExists(Instance()._pluginPaths["PluginAutoCompleteFilePath"].c_str()))
        sFiles.push_back(Instance()._pluginPaths["PluginAutoCompleteFilePath"]);
    else
        sFiles.push_back(Instance()._pluginPaths["PluginAutoCompleteFilePath"]);

    PathCheckResults fResult = Instance().WritePermissionCheckup(sFiles, RestartFunctionHook::None);
    if (static_cast<int>(fResult) < 1)
        return;

    // A last warning to the user
    if (!bWarnedUser)
    {
        if (MessageBox(Instance().NotepadHwnd(), TEXT(WARNINGMESSAGE), TEXT("Confirmation"), MB_YESNO | MB_ICONQUESTION) == IDNO)
            return;
    }
    bWarnedUser = true;

    generic_string nFileName;
    if (openFileDialog(Instance().NotepadHwnd(), TEXT("nwscritpt.nss\0nwscript*.nss\0All Files (*.*)\0*.*"), nFileName))
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
    sFiles.push_back(Instance()._pluginPaths["PluginLexerConfigFilePath"]);
    sFiles.push_back(Instance()._pluginPaths["NotepadDarkThemeFilePath"]);

    PathCheckResults fResult = Instance().WritePermissionCheckup(sFiles, RestartFunctionHook::ResetEditorColorsPhase1);
    if (static_cast<int>(fResult) < 1)
        return;

    Instance().DoResetEditorColors();
}

//-------------------------------------------------------------

// Opens About Box
PLUGINCOMMAND Plugin::AboutMe()
{
    std::vector<generic_string> darkModeLabels = { TEXT("Uninstalled"), TEXT("Installed"), TEXT("Unsupported") };

    // Dialog boxes need to be static unless modal ones.
    static AboutDialog aboutDialog = {};

    // Initialize some user information
    std::map<generic_string, generic_string> replaceStrings;
    replaceStrings.insert({ TEXT("%PLUGINXMLFILE%"), Instance()._pluginPaths["PluginLexerConfigFilePath"] });
    replaceStrings.insert({ TEXT("%AUTOCOMPLETEFILE%"), Instance()._pluginPaths["PluginAutoCompleteFilePath"] });
    replaceStrings.insert({ TEXT("%AUTOCOMPLETEDIR%"), Instance()._pluginPaths["PluginAutoCompleteFilePath"].parent_path()});

    bool bIsAutoIndentOn = Instance().Messenger().SendSciMessage<bool>(NPPM_ISAUTOINDENTON);

    // Diagnostic data.
    if (Instance()._needPluginAutoIndent)
        replaceStrings.insert({ TEXT("%NWSCRIPTINDENT%"), TEXT("Use the built-in auto-indentation.") });
    else
    {
        generic_string autoindent = TEXT("Automatic. Currently set to ["); autoindent.append(bIsAutoIndentOn ? TEXT("ON") : TEXT("OFF")).append(TEXT("]"));
        replaceStrings.insert({ TEXT("%NWSCRIPTINDENT%"), autoindent });
    }
    replaceStrings.insert({ TEXT("%DARKTHEMESUPPORT%"), darkModeLabels[static_cast<int>(Instance()._pluginDarkThemeIs)] });

    // Set replace strings
    aboutDialog.setReplaceStrings(replaceStrings);

    // Present it
    if (!aboutDialog.isCreated())
        aboutDialog.init(Instance().DllHModule(), Instance().NotepadHwnd());

    if (!aboutDialog.isVisible())
        aboutDialog.doDialog();
}

#pragma endregion Plugin User Interfacing
