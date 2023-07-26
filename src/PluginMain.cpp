/** @file PluginMain.cpp
 * Controls the Plugin functions, processes all Plugin messages
 *
 * Although this is the main object, the actual DLL Entry point is defined in PluginInterface.cpp
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

#include "menuCmdID.h"

#include "Common.h"
#include "LexerCatalogue.h"

#include "PluginMain.h"
#include "PluginControlsRC.h"

#include "NWScriptParser.h"

#include "BatchProcessingDialog.h"
#include "CompilerSettingsDialog.h"
#include "FileParseSummaryDialog.h"
#include "PathAccessDialog.h"
#include "ProcessFilesDialog.h"
#include "UsersPreferencesDialog.h"
#include "WarningDialog.h"

#include "XMLGenStrings.h"
#include "VersionInfoEx.h"

#include "ColorConvert.h"

#include "PluginDarkMode.h"

#pragma warning (disable : 6387)

//#define DEBUG_AUTO_INDENT_833      // Uncomment to test auto-indent with message
#define USE_THREADS                  // Process compilations and batchs in multi-threaded operations
#define NAGIVATECALLBACKTIMER 0x800  // Temporary timer to schedule navigations


using namespace NWScriptPlugin;
using namespace LexerInterface;

// Static members definition
generic_string Plugin::pluginName = TEXT("NWScript Tools");

// Menu functions order.
// Needs to be Sync'ed with pluginFunctions[]
#define PLUGINMENU_SWITCHAUTOINDENT 0
#define PLUGINMENU_DASH1 1
#define PLUGINMENU_COMPILESCRIPT 2
#define PLUGINMENU_DISASSEMBLESCRIPT 3
#define PLUGINMENU_BATCHPROCESSING 4
#define PLUGINMENU_RUNLASTBATCH 5
#define PLUGINMENU_DASH2 6
#define PLUGINMENU_FETCHPREPROCESSORTEXT 7
#define PLUGINMENU_VIEWSCRIPTDEPENDENCIES 8
#define PLUGINMENU_DASH3 9
#define PLUGINMENU_SHOWCONSOLE 10
#define PLUGINMENU_DASH4 11
#define PLUGINMENU_SETTINGS 12
#define PLUGINMENU_USERPREFERENCES 13
#define PLUGINMENU_DASH5 14
#define PLUGINMENU_INSTALLDARKTHEME 15
#define PLUGINMENU_IMPORTDEFINITIONS 16
#define PLUGINMENU_IMPORTUSERTOKENS 17
#define PLUGINMENU_RESETUSERTOKENS 18
#define PLUGINMENU_RESETEDITORCOLORS 19
#define PLUGINMENU_REPAIRXMLASSOCIATION 20
#define PLUGINMENU_DASH6 21
#define PLUGINMENU_INSTALLCOMPLEMENTFILES 22
#define PLUGINMENU_DASH7 23
#define PLUGINMENU_ABOUTME 24

#define PLUGIN_HOMEPATH TEXT("https://github.com/Leonard-The-Wise/NWScript-Npp")
#define PLUGIN_ONLINEHELP TEXT("https://github.com/Leonard-The-Wise/NWScript-Npp/blob/master/OnlineHelp.md")

ShortcutKey compileScriptKey = { false, false, false, VK_F9 };
ShortcutKey disassembleScriptKey = { true, false, false, VK_F9 };
ShortcutKey batchScriptKey = { true, false, true, VK_F9 };
ShortcutKey runLastBatchKey = { false, false, true, VK_F9 };
ShortcutKey toggleConsoleKey = { true, false, false,  VK_OEM_COMMA };

FuncItem Plugin::pluginFunctions[] = {
    {TEXT("Use auto-identation"), Plugin::SwitchAutoIndent, 0, false },
    {TEXT("---")},
    {TEXT("Compile NWScript"), Plugin::CompileScript, 0, false, &compileScriptKey },
    {TEXT("Disassemble NWScript file..."), Plugin::DisassembleFile, 0, false, &disassembleScriptKey },
    {TEXT("Batch Process NWScript Files..."), Plugin::BatchProcessFiles, 0, false, &batchScriptKey },
    {TEXT("Run last batch"), Plugin::RunLastBatch, 0, false, &runLastBatchKey},
    {TEXT("---")},
    {TEXT("Fetch preprocessed output"), Plugin::FetchPreprocessorText},
    {TEXT("View NWScript dependencies"), Plugin::ViewScriptDependencies},
    {TEXT("---")},
    {TEXT("Toggle NWScript Compiler Console"), Plugin::ToggleLogger, 0, false, &toggleConsoleKey},
    {TEXT("---")},
    {TEXT("Compiler settings..."), Plugin::CompilerSettings},
    {TEXT("User's preferences..."), Plugin::UserPreferences},
    {TEXT("---")},
    {TEXT("Install Dark Theme"), Plugin::InstallDarkTheme},
    {TEXT("Import NWScript definitions"), Plugin::ImportDefinitions},
    {TEXT("Import user-defined tokens"), Plugin::ImportUserTokens},
    {TEXT("Reset user-defined tokens"), Plugin::ResetUserTokens},
    {TEXT("Reset editor colors"), Plugin::ResetEditorColors},
    {TEXT("Repair Function List"), Plugin::RepairFunctionList},
    {TEXT("---")},
    {TEXT("Install Plugin's XML Config Files"), Plugin::InstallAdditionalFiles},
    {TEXT("---")},
    {TEXT("About me"), Plugin::AboutMe},
};

Plugin* Plugin::_instance(nullptr);

// Static file and directory names

// Notepad Plugin Configuration installation root directory. We made it const, since we're not retrieving this outside a plugin build 
constexpr const TCHAR NotepadPluginRootDir[] = TEXT("plugins\\");
// Notepad Plugin Configuration installation root directory. We made it const, since we're not retrieving this outside a plugin build 
constexpr const TCHAR NotepadPluginConfigRootDir[] = TEXT("plugins\\Config\\");
// Notepad Default Themes install directory. We made it const, since we're not retrieving this outside a plugin build
constexpr const TCHAR NotepadDefaultThemesRootDir[] = TEXT("themes\\");
// Notepad Default Auto Completion directory.
constexpr const TCHAR NotepadAutoCompleteRootDir[] = TEXT("autoCompletion\\");
// Notepad Default Function List directory.
constexpr const TCHAR NotepadFunctionListRootDir[] = TEXT("functionList\\");

// Notepad config file
constexpr const TCHAR NotepadConfigFile[] = TEXT("config.xml");

// OverrideMap XML file
constexpr const TCHAR OverrideMapFile[] = TEXT("overrideMap.xml");

// Notepad Default Dark Theme file.
constexpr const TCHAR NotepadDefaultDarkThemeFile[] = TEXT("DarkModeDefault.xml");

// Default pseudo-batch file to create in case we need to restart notepad++
constexpr const TCHAR PseudoBatchRestartFile[] = TEXT("~doNWScriptNotepadRestart.bat");
// NWScript known engine objects file
constexpr const TCHAR NWScriptEngineObjectsFile[] = TEXT("NWScript-Npp-EngineObjects.bin");
// NWScript known user objects file
constexpr const TCHAR NWScriptUserObjectsFile[] = TEXT("NWScript-Npp-UserObjects.bin");


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

    // The rest of metainformation is get when Notepad Messenger is set...
    // Check on Plugin::SetNotepadData

    // Load latest Richedit library and only then create the about dialog
    LoadLibrary(TEXT("Msftedit.dll"));
    Instance()._aboutDialog = std::make_unique<AboutDialog>();
    Instance()._loggerWindow = std::make_unique<LoggerDialog>();
    Instance()._processingFilesDialog = std::make_unique<ProcessFilesDialog>();
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

// Create plugin's file paths variables
void Plugin::MakePluginFilePaths()
{
    // Finish initializing the Plugin Metainformation
    TCHAR fBuffer[MAX_PATH + 1] = {0};
    generic_string sPath;

    // Get main paths for program

    Messenger().SendNppMessage(NPPM_GETNPPFULLFILEPATH, static_cast<WPARAM>(MAX_PATH), reinterpret_cast<LPARAM>(fBuffer));
    sPath = fBuffer;
    _pluginPaths.insert({ "NotepadInstallExecutablePath", fs::path(fBuffer) });
    _pluginPaths.insert({ "NotepadExecutableDir", fs::path(fBuffer).parent_path() });
    Messenger().SendNppMessage(NPPM_GETPLUGINSCONFIGDIR, static_cast<WPARAM>(MAX_PATH), reinterpret_cast<LPARAM>(fBuffer));
    sPath = fBuffer;
    _pluginPaths.insert({ "PluginConfigDir", fs::path(sPath) });
    _pluginPaths.insert({ "NotepadUserConfigDir", fs::path(sPath).parent_path().parent_path() });

    // Step 1:
    // First we get all files avaliable from Plugin Config Dir if they are present there, because
    // that's the Notepad++ priority list for loading config files

    sPath = str2wstr(_pluginPaths["NotepadUserConfigDir"].string());
    sPath.append(TEXT("\\")).append(NotepadConfigFile);
    _pluginPaths.insert({ "NotepadConfigFile", fs::path(sPath) });

    generic_string pluginLexerConfigFile = _pluginFileName;
    sPath = str2wstr(_pluginPaths["PluginConfigDir"].string());
    sPath.append(TEXT("\\")).append(pluginLexerConfigFile).append(TEXT(".xml"));
    _pluginPaths.insert({ "PluginLexerConfigFilePath", fs::path(sPath) });

    sPath = str2wstr(_pluginPaths["PluginConfigDir"].string());
    sPath.append(TEXT("\\")).append(_pluginFileName).append(TEXT(".ini"));
    _pluginPaths.insert({ "PluginSettingsFile", fs::path(sPath) });

    // These files here are the one that can be in multiple places at one time

    sPath = str2wstr(_pluginPaths["NotepadUserConfigDir"].string());
    sPath.append(TEXT("\\")).append(NotepadDefaultThemesRootDir).append(NotepadDefaultDarkThemeFile);
    _pluginPaths.insert({ "NotepadDarkThemeFilePath", fs::path(sPath) });

    sPath = str2wstr(_pluginPaths["NotepadUserConfigDir"].string());
    sPath.append(TEXT("\\")).append(NotepadAutoCompleteRootDir).append(str2wstr(LexerCatalogue::GetLexerName(0, true))).append(TEXT(".xml"));
    _pluginPaths.insert({ "PluginAutoCompleteFilePath", fs::path(sPath) });

    sPath = str2wstr(_pluginPaths["NotepadUserConfigDir"].string());
    sPath.append(TEXT("\\")).append(NotepadFunctionListRootDir).append(OverrideMapFile);
    _pluginPaths.insert({ "NotepadOverrideMapFile", fs::path(sPath) });

    sPath = str2wstr(_pluginPaths["NotepadUserConfigDir"].string());
    sPath.append(TEXT("\\")).append(NotepadFunctionListRootDir).append(str2wstr(LexerCatalogue::GetLexerName(0, true))).append(TEXT(".xml"));
    _pluginPaths.insert({ "PluginFunctionListFile", fs::path(sPath) });

    // Bellow are files that will always be on Plugin's config sPath

    // Temporary batch we write in case we need to restart Notepad++ by ourselves. Root is Plugin config sPath
    sPath = _pluginPaths["PluginConfigDir"];
    sPath.append(TEXT("\\")).append(PseudoBatchRestartFile);
    _pluginPaths.insert({ "PseudoBatchRestartFile", fs::path(sPath) });

    // Known NWScript structures
    sPath = _pluginPaths["PluginConfigDir"];
    sPath.append(TEXT("\\")).append(NWScriptEngineObjectsFile);
    _pluginPaths.insert({ "NWScriptEngineObjectsFile", fs::path(sPath) });

    sPath = _pluginPaths["PluginConfigDir"];
    sPath.append(TEXT("\\")).append(NWScriptUserObjectsFile);
    _pluginPaths.insert({ "NWScriptUserObjectsFile", fs::path(sPath) });

    // Step 2:
    // For any file not present on Plugins Config Dir, we then check on the Notepad++ executable sPath.

// Force Install paths tells the compiler to not test to files existences. This is used
// while Notepad++ do not support custom paths for the mentioned files and always use the ones inside the installation dir
#define FORCE_INSTALL_PATHS

    if (!PathFileExists(str2wstr(_pluginPaths["NotepadDarkThemeFilePath"].string()).c_str()))
    {
        sPath = _pluginPaths["NotepadExecutableDir"];
        sPath.append(TEXT("\\")).append(NotepadDefaultThemesRootDir).append(NotepadDefaultDarkThemeFile);
#ifndef FORCE_INSTALL_PATHS
        if (PathFileExists(sPath.c_str()))
#endif
            _pluginPaths["NotepadDarkThemeFilePath"] = sPath;
    }

    if (!PathFileExists(str2wstr(_pluginPaths["PluginAutoCompleteFilePath"].string()).c_str()))
    {
        sPath = _pluginPaths["NotepadExecutableDir"];
        sPath.append(TEXT("\\")).append(NotepadAutoCompleteRootDir).append(str2wstr(LexerCatalogue::GetLexerName(0, true))).append(TEXT(".xml"));
#ifndef FORCE_INSTALL_PATHS
        if (PathFileExists(sPath.c_str()))
#endif
            _pluginPaths["PluginAutoCompleteFilePath"] = sPath;
    }

    if (!PathFileExists(str2wstr(_pluginPaths["NotepadOverrideMapFile"].string()).c_str()))
    {
        sPath = _pluginPaths["NotepadExecutableDir"];
        sPath.append(TEXT("\\")).append(NotepadFunctionListRootDir).append(OverrideMapFile);
#ifndef FORCE_INSTALL_PATHS
        if (PathFileExists(sPath.c_str()))
#endif
            _pluginPaths["NotepadOverrideMapFile"] = sPath;
    }

    if (!PathFileExists(str2wstr(_pluginPaths["PluginFunctionListFile"].string()).c_str()))
    {
        sPath = _pluginPaths["NotepadExecutableDir"];
        sPath.append(TEXT("\\")).append(NotepadFunctionListRootDir).append(str2wstr(LexerCatalogue::GetLexerName(0, true))).append(TEXT(".xml"));
#ifndef FORCE_INSTALL_PATHS
        if (PathFileExists(sPath.c_str()))
#endif
            _pluginPaths["PluginFunctionListFile"] = sPath;
    }
}

// Setup Notepad++ and Scintilla handles and finish initializing the
// plugin's objects that need a Windows Handle to work
void Plugin::SetNotepadData(NppData& data)
{
    _messageInstance.SetData(data);
    _indentor.SetMessenger(_messageInstance);

    //Keep a copy of Notepad HWND to easy access
    _notepadHwnd = Messenger().GetNotepadHwnd();

    // Create the configuration files paths
    MakePluginFilePaths();

    // Create settings instance and load all values
    _settings.InitSettings(str2wstr(_pluginPaths["PluginSettingsFile"].string()));
    Settings().Load();

    // Pick current Notepad++ version and compares with Settings, to se whether user gets a new 
    // Dark Mode install attempt or not...
    VersionInfoEx currentNotepad = VersionInfoEx(_pluginPaths["NotepadInstallExecutablePath"]);
    if (Settings().notepadVersion.empty())
        Settings().notepadVersion = currentNotepad;

    if (Settings().notepadVersion < currentNotepad)
    {
        Settings().darkThemeInstallAttempt = false;
        Settings().notepadVersion = currentNotepad;
    }

    if (currentNotepad > "8.4")
        _NppSupportDarkModeMessages = true;

    // Adjust menu "Use Auto-Indentation" checked or not before creation
    pluginFunctions[PLUGINMENU_SWITCHAUTOINDENT]._init2Check = Settings().enableAutoIndentation;

    // Points the compiler to our global settings
    _compiler.appendSettings(&_settings);

    // Initializes the compiler log window
    InitCompilerLogWindow();

    // Check engine objects file
    CheckupPluginObjectFiles();

    // The rest of initialization processes on Notepad++ SCI message NPPN_READY.
}

// Initializes the compiler log window
void Plugin::InitCompilerLogWindow()
{
    _dockingData = {};

    // Register settings class to the window
    _loggerWindow->appendSettings(&_settings);

    // additional info
    _loggerWindow->init(DllHModule(), NotepadHwnd());
    _loggerWindow->create(&_dockingData);
    _dockingData.uMask = DWS_DF_CONT_BOTTOM | DWS_ICONTAB | DWS_ADDINFO;
    _dockingIcon = loadSVGFromResourceIcon(DllHModule(), IDI_NEVERWINTERAPP, false, 32, 32);
    _dockingData.hIconTab = _dockingIcon;
    _dockingData.pszModuleName = _pluginFileName.c_str();
    _dockingData.dlgID = 0;

    // Register the dialog box with Notepad++
    Messenger().SendNppMessage<void>(NPPM_DMMREGASDCKDLG, 0, (LPARAM)&_dockingData);

    if (_settings.compilerWindowVisible == false)
        DisplayCompilerLogWindow(false);

    // Allow modifications on compilerWindowVisible status (because Notepad++ internally calls show 
    // window on creation and that would change the status of compilerWindowVisible unintentionally)
    _settings.compilerWindowVisibleAllowChange = true;

    // Set the compiler log callback
    _compiler.setLoggerMessageCallback(WriteToCompilerLog);

    // Set the compiler log navigate callback (from errors list to main window)
    _loggerWindow->SetNavigateFunctionCallback(NavigateToCode);
}

// Display / Hide the compiler log window
void Plugin::DisplayCompilerLogWindow(bool toShow)
{
    Instance()._loggerWindow->display(toShow);
    SetFocus(Instance().Messenger().GetCurentScintillaHwnd());
}

// Check the files for known engine objects
void Plugin::CheckupPluginObjectFiles()
{
    if (!PathFileExists(_pluginPaths["NWScriptEngineObjectsFile"].c_str()))
    {
        // Rant message to the user if we already installed the file and then it's gone.
        if (Settings().installedEngineKnownObjects)
            MessageBox(NotepadHwnd(), (TEXT("Could not find file:\r\n\r\n") + str2wstr(_pluginPaths["NWScriptEngineObjectsFile"].string()) + TEXT(".\r\n\r\n")
                + TEXT("This file is necessary for the NWScript Tools Plugin to know which objects are already imported.\r\n\r\n")
                + TEXT("Please DO NOT delete it or you may lose some of your imported languages keywords.\r\n\r\n")
                + TEXT("The file is going to be rebuilt with the default predefinitions now.")).c_str(),
                TEXT("NWScript Tools - Warning"), MB_OK | MB_ICONEXCLAMATION);

        // Load file from resources
        auto hResource = FindResourceW(DllHModule(), MAKEINTRESOURCE(IDR_KNOWNOBJECTS), L"BIN");
        size_t _size = SizeofResource(DllHModule(), hResource);
        auto hMemory = LoadResource(DllHModule(), hResource);
        LPVOID ptr = LockResource(hMemory);

        std::string fileContents;
        if (hMemory)
        {
            fileContents.assign((char*)hMemory, _size);
            if (bufferToFile(_pluginPaths["NWScriptEngineObjectsFile"].c_str(), fileContents))
                Settings().installedEngineKnownObjects = true;
            else
                MessageBox(NotepadHwnd(), (TEXT("Could not create file: ") + str2wstr(_pluginPaths["NWScriptEngineObjectsFile"].string()) + TEXT(".\r\n")
                    + TEXT("The import definitions may not work properly. Please check for write permission on the folder and if the disk has enough space.")).c_str(),
                    TEXT("NWScript Tools - Error creating file"), MB_OK | MB_ICONERROR);
        }
        else
            MessageBox(NotepadHwnd(), TEXT("Could not allocate memory for resource buffer. Please report this error to the \
plugin creator:\r\n File: PluginMain.cpp, function 'CheckupPluginObjectFiles()'"), TEXT("NWScript Plugin - Critical Error"), MB_OK | MB_ICONERROR);

        FreeResource(hMemory);

        // Retrieve statistics
        _NWScriptParseResults = std::make_unique<NWScriptParser::ScriptParseResults>();
        _NWScriptParseResults->SerializeFromFile(_pluginPaths["NWScriptEngineObjectsFile"].c_str());
        Settings().engineStructs = _NWScriptParseResults->EngineStructuresCount;
        Settings().engineFunctionCount = _NWScriptParseResults->FunctionsCount;
        Settings().engineConstants = _NWScriptParseResults->ConstantsCount;
        _NWScriptParseResults = nullptr;
    }

    // Create or restore the plugin XML configuration file
    if (!PathFileExists(_pluginPaths["PluginLexerConfigFilePath"].c_str()))
    {
        // Create a dummy skeleton of the file from our resource string, so we don't crash...
        // (the REAL file only installs if the user wants to)

        // Set some Timestamp headers
        char timestamp[128]; time_t currTime;  struct tm currTimeP;
        time(&currTime);
        errno_t error = localtime_s(&currTimeP, &currTime);
        strftime(timestamp, 64, "Creation timestamp is: %B %d, %Y - %R", &currTimeP);
        std::string xmlHeaderComment = XMLDOCHEADER;
        xmlHeaderComment.append(timestamp).append(".\r\n");
        xmlHeaderComment = "<!--" + xmlHeaderComment + "-->\r\n";

        std::string xmlConfigFile = xmlHeaderComment + XMLPLUGINLEXERCONFIG;

        if (!bufferToFile(str2wstr(_pluginPaths["PluginLexerConfigFilePath"].string().c_str()), xmlConfigFile))
            return; // End of the line, plugin will crash...

        // Set an offer to install the plugin's definitions for the user later, when initialization finishes
        // So we can get a pretty screen up. This is a one-time offer, since next time the user will have NWScript-Npp.xml already created
        _OneTimeOffer = true;
    }
}

// Properly detects if dark mode is enabled (Notepad++ 8.3.4 and above)
void Plugin::RefreshDarkMode(bool ForceUseDark, bool UseDark)
{
    // Initialization
    if (!PluginDarkMode::isInitialized())
        PluginDarkMode::initDarkMode();

    bool isDarkModeEnabled = false;

    // Legacy support
    if (ForceUseDark)
        isDarkModeEnabled = UseDark;

    // Non-legacy support
    if (_NppSupportDarkModeMessages && !ForceUseDark)
    {
        isDarkModeEnabled = Messenger().SendNppMessage<bool>(NPPM_ISDARKMODEENABLED);
        PluginDarkMode::Colors newColors;
        bool bSuccess = Messenger().SendNppMessage<bool>(NPPM_GETDARKMODECOLORS, sizeof(newColors), reinterpret_cast<LPARAM>(&newColors));
        if (bSuccess)
        {
            PluginDarkMode::setDarkTone(PluginDarkMode::ColorTone::customizedTone);
            PluginDarkMode::changeCustomTheme(newColors);

            // We override link colors
            PluginDarkMode::setLinkTextColor(HEXRGB(0xFFC000));
        }
        else
            CheckDarkModeLegacy();
    }

    // Set Dark Mode for window/application
    PluginDarkMode::setDarkMode(isDarkModeEnabled, true);

    // Rebuild menu
    SetupPluginMenuItems();

    // Refresh persistent dialogs dark mode
    _loggerWindow->refreshDarkMode();
    _aboutDialog->refreshDarkMode();
}

// Check Dark Mode for Legacy Notepad++ versions
void Plugin::CheckDarkModeLegacy()
{
    tinyxml2::XMLDocument nppConfig;

    fs::path notepadPath = _pluginPaths["NotepadConfigFile"];
    int success = nppConfig.LoadFile(notepadPath.string().c_str());
    if (success != 0)
        return;

    tinyxml2::XMLElement* GUIConfig = searchElement(nppConfig.RootElement(), "GUIConfig", "name", "DarkMode");
    if (!GUIConfig)
        return;

    if (strcmp(GUIConfig->FindAttribute("enable")->Value(), "yes") == 0)
    {
        PluginDarkMode::initDarkMode();

        PluginDarkMode::Colors colors;
        colors.background = std::stoi(GUIConfig->FindAttribute("customColorTop")->Value());
        colors.darkerText = std::stoi(GUIConfig->FindAttribute("customColorDarkText")->Value());
        colors.disabledText = std::stoi(GUIConfig->FindAttribute("customColorDisabledText")->Value());
        colors.edge = std::stoi(GUIConfig->FindAttribute("customColorEdge")->Value());
        colors.errorBackground = std::stoi(GUIConfig->FindAttribute("customColorError")->Value());
        colors.hotBackground = std::stoi(GUIConfig->FindAttribute("customColorMenuHotTrack")->Value());
        colors.linkText = std::stoi(GUIConfig->FindAttribute("customColorLinkText")->Value());
        colors.pureBackground = std::stoi(GUIConfig->FindAttribute("customColorMain")->Value());
        colors.softerBackground = std::stoi(GUIConfig->FindAttribute("customColorActive")->Value());
        colors.text = std::stoi(GUIConfig->FindAttribute("customColorText")->Value());

        PluginDarkMode::ColorTone C = static_cast<PluginDarkMode::ColorTone>(std::stoi(GUIConfig->FindAttribute("colorTone")->Value()));

        PluginDarkMode::changeCustomTheme(colors);
        PluginDarkMode::setDarkTone(C);

        // We override link colors
        PluginDarkMode::setLinkTextColor(HEXRGB(0xFFC000));

        Instance().RefreshDarkMode(true, true);
    }
    else
        Instance().RefreshDarkMode(true, false);
}

#pragma endregion Plugin DLL Initialization

#pragma region 

// Processes Raw messages from a Notepad++ window (the ones not handled by editor).
// This function is dummy. Really all messages are from the Scintilla Editor.
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
        // Do Initialization procedures.
        // Note: The ORDER that many operations are performed here is important, so avoid changing it.

        // We do this as first step, beause we are removing dash items that cannot be made by ID, so we need
        // the proper position to be present. If we remove any other menus later, the positions might change a lot.
        RemoveUnusedMenuItems();

        // Starting from second step, these can be done on any order
        SetAutoIndentSupport();
        LoadNotepadLexer();

        // Detects Dark Theme installation and possibly setup a hook to auto-install it.
        // This step must be performed before SetupPluginMenuItems because that function depends on the detection
        // of Dark Theme to properly setup the icons for menu. If an auto-update is to happen, this function
        // causes the initialization process to cancel, so we preserve the restart hooks set in it.
        if (DetectDarkThemeInstall() == RestartMode::Admin)
            return;

        // Check OverrideMap.xml and (possibly) auto-patch it - if other plugins had overwritten it.
        if (CheckAndPatchOverrideMapXMLFile())
            RemovePluginMenuItem(PLUGINMENU_REPAIRXMLASSOCIATION);

        // Initially disable run last batch (until the user runs a batch in session)
        EnablePluginMenuItem(PLUGINMENU_RUNLASTBATCH, false);

        // Detects Dark Mode support. Can be done by messaging Notepad++ for newer versions
        // Or if the messages aren't supported (on a previous version), checks the installation on Notepad++ config.xml.
        // Note: Dark Mode is different from Dark Theme - 1st is for the plugin GUI, the second is for NWScript file lexing.
        if (_NppSupportDarkModeMessages)
            RefreshDarkMode();
        else
            CheckDarkModeLegacy();

        // Setup the rest of menu items. RefreshDarkMode checkups can build the menu icons for us by 
        // calling SetupPluginMenuItems internally when DarkMode is present for legacy version (CheckDarkModeLegacy)
        // and always for the other (RefreshDarkMode), so we check if no icons were set before to avoid double loading
        if (_menuBitmaps.size() == 0)
            SetupPluginMenuItems();

        // Checks the one-time offer to install the additional XML files when the plugin first initializes
        // (or when NWScript-Npp.xml file was not present at initialization)
        if (_OneTimeOffer)
            InstallAdditionalFiles();
        if (_OneTimeOfferAccepted)
            return;

        // Auto call a function that required restart during the previous session (because of privilege elevation)
        // Up to now...
        // 1 = ImportDefinitions
        // 2 = Fix Editor's Colors
        // 3 = Repair OverrideMap file
        // 4 = Install Additional
        // Since all functions that required restart must have returned in Admin Mode, we check this
        // to see if the user didn't cancel the UAC request.
        if (IsUserAnAdmin())
        {
            switch (Settings().notepadRestartFunction)
            {
            case RestartFunctionHook::ResetUserTokensPhase1:
                if (DoResetUserTokens(Settings().notepadRestartFunction) != RestartMode::None)
                    return;
                break;
            case RestartFunctionHook::ResetEditorColorsPhase1:
                if (DoResetEditorColors(Settings().notepadRestartFunction) != RestartMode::None)
                    return;
                break;
            case RestartFunctionHook::InstallDarkModePhase1:
                if (DoInstallDarkTheme(Settings().notepadRestartFunction) != RestartMode::None)
                    return;
                break;
            case RestartFunctionHook::RepairOverrideMapPhase1:
                if (DoRepairOverrideMap(Settings().notepadRestartFunction) != RestartMode::None)
                    return;
                break;
            case RestartFunctionHook::InstallAdditionalFilesPhase1:
                if (DoInstallAdditionalFiles(Settings().notepadRestartFunction) != RestartMode::None)
                    return;
                break;
            }
        }

        // If it makes it here, make sure to clear the hooks, temp files, etc.
        // This must always execute after processing the hooks.
        SetRestartHook(RestartMode::None, RestartFunctionHook::None);

        // Mark plugin ready to use. Last step on the initialization chain
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

        // If we have a restart hook setup, call out shell to execute it.
        if (Settings().notepadRestartMode != RestartMode::None)
        {
            runProcess(Settings().notepadRestartMode == RestartMode::Admin ? true : false,
                Instance()._pluginPaths["PseudoBatchRestartFile"].c_str());
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
    case NPPN_DARKMODECHANGED:
    {
        RefreshDarkMode();
        break;
    }
    case NPPN_TBMODIFICATION:
    {
        _tbIcons[0].hToolbarBmp = loadSVGFromResource(DllHModule(), IDI_COMPILEFILE, false, _dpiManager.scaleX(16), _dpiManager.scaleY(16));
        _tbIcons[0].hToolbarIcon = loadSVGFromResourceIcon(DllHModule(), IDI_COMPILEFILE, false, _dpiManager.scaleX(16), _dpiManager.scaleY(16));
        _tbIcons[0].hToolbarIconDarkMode = loadSVGFromResourceIcon(DllHModule(), IDI_COMPILEFILE, true, _dpiManager.scaleX(16), _dpiManager.scaleY(16));

        _tbIcons[1].hToolbarBmp = loadSVGFromResource(DllHModule(), IDI_COMPILEBATCH, false, _dpiManager.scaleX(16), _dpiManager.scaleY(16));
        _tbIcons[1].hToolbarIcon = loadSVGFromResourceIcon(DllHModule(), IDI_COMPILEBATCH, false, _dpiManager.scaleX(16), _dpiManager.scaleY(16));
        _tbIcons[1].hToolbarIconDarkMode = loadSVGFromResourceIcon(DllHModule(), IDI_COMPILEBATCH, true, _dpiManager.scaleX(16), _dpiManager.scaleY(16));

        _tbIcons[2].hToolbarBmp = loadSVGFromResource(DllHModule(), IDI_NEVERWINTERAPP, false, _dpiManager.scaleX(16), _dpiManager.scaleY(16));
        _tbIcons[2].hToolbarIcon = loadSVGFromResourceIcon(DllHModule(), IDI_NEVERWINTERAPP, false, _dpiManager.scaleX(16), _dpiManager.scaleY(16));
        _tbIcons[2].hToolbarIconDarkMode = loadSVGFromResourceIcon(DllHModule(), IDI_NEVERWINTERAPP, true, _dpiManager.scaleX(16), _dpiManager.scaleY(16));

        Messenger().SendNppMessage<void>(NPPM_ADDTOOLBARICON_FORDARKMODE,
            pluginFunctions[PLUGINMENU_COMPILESCRIPT]._cmdID, reinterpret_cast<LPARAM>(&_tbIcons[0]));
        Messenger().SendNppMessage<void>(NPPM_ADDTOOLBARICON_FORDARKMODE,
            pluginFunctions[PLUGINMENU_BATCHPROCESSING]._cmdID, reinterpret_cast<LPARAM>(&_tbIcons[1]));
        Messenger().SendNppMessage<void>(NPPM_ADDTOOLBARICON_FORDARKMODE,
            pluginFunctions[PLUGINMENU_SHOWCONSOLE]._cmdID, reinterpret_cast<LPARAM>(&_tbIcons[2]));
    }
    }
}

// Setup a restart Hook. Normal or Admin mode. This function saves settings immediately.
void Plugin::SetRestartHook(RestartMode type, RestartFunctionHook function)
{
    Settings().notepadRestartMode = type; Settings().notepadRestartFunction = function;
    if (type == RestartMode::None)
    {
        if (PathFileExists(_pluginPaths["PseudoBatchRestartFile"].c_str()))
            DeleteFile(_pluginPaths["PseudoBatchRestartFile"].c_str());
    }
    else
        writePseudoBatchExecute(_pluginPaths["PseudoBatchRestartFile"].c_str(), _pluginPaths["NotepadInstallExecutablePath"].c_str());

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

#ifndef DEBUG_AUTO_INDENT_833
        // Auto-adjust the settings
        Instance().Settings().enableAutoIndentation = false;
#endif
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
    std::unique_ptr<TCHAR[]> lexerName = std::make_unique<TCHAR[]>(buffSize + 1);
    buffSize = msg.SendNppMessage<int>(NPPM_GETLANGUAGENAME, currLang, reinterpret_cast<LPARAM>(lexerName.get()));

    // Try to get Language Auto-Indentation if it's one of the plugin installed languages
    generic_string lexerNameW;
    for (int i = 0; i < LexerCatalogue::GetLexerCount() && lexerSearch == false; i++)
    {
        lexerNameW = str2wstr(LexerCatalogue::GetLexerName(i));
        isPluginLanguage = (_tcscmp(lexerName.get(), lexerNameW.c_str()) == 0);

        if (isPluginLanguage)
        {
            lexerSearch = msg.SendNppMessage<int>(NPPM_GETEXTERNALLEXERAUTOINDENTMODE,
                reinterpret_cast<WPARAM>(lexerNameW.c_str()), reinterpret_cast<LPARAM>(&langIndent));
        }
    }

    //Update Lexer
    _notepadCurrentLexer.SetLexer(currLang, lexerName.get(), isPluginLanguage, langIndent);
}

// Detects if Dark Theme is already installed
RestartMode Plugin::DetectDarkThemeInstall()
{
    // Here we are parsing the file silently
    tinyxml2::XMLDocument darkThemeDoc;
    errno_t error = darkThemeDoc.LoadFile(wstr2str(Instance()._pluginPaths["NotepadDarkThemeFilePath"].c_str()).c_str());

    if (error)
    {
        RemovePluginMenuItem(PLUGINMENU_INSTALLDARKTHEME);
        _pluginDarkThemeIs = DarkThemeStatus::Unsupported;
        return RestartMode::None;
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

    // Auto-reinstall if a previous installation existed and support for it is enabled...
    if (_pluginDarkThemeIs == DarkThemeStatus::Uninstalled && _settings.darkThemePreviouslyInstalled && _settings.autoInstallDarkTheme)
    {
        // Check to see if we already attempted a reinstall before, so we won't loop...
        if (_settings.darkThemeInstallAttempt)
            return RestartMode::None;

        // Set to true... this will only be reset upon a successfull installation.
        _settings.darkThemeInstallAttempt = true;

        // Check files permissions (redirect result to ignore since we already know file exists)...
        PathWritePermission fPerm = PathWritePermission::UndeterminedError;
        std::ignore = checkWritePermission(_pluginPaths["NotepadDarkThemeFilePath"], fPerm);

        // Setup a restart hook...
        if (fPerm == PathWritePermission::RequiresAdminPrivileges && !IsUserAnAdmin())
        {
            SetRestartHook(RestartMode::Admin, RestartFunctionHook::InstallDarkModePhase1);
            Messenger().SendNppMessage(WM_CLOSE, 0, 0);
            return RestartMode::Admin;
        }
        else
            // Since we've got perms... call the install function right on...
            DoInstallDarkTheme(RestartFunctionHook::InstallDarkModePhase1);

        return RestartMode::None;
    }

    // Mark it was previously installed
    if (_pluginDarkThemeIs == DarkThemeStatus::Installed && !_settings.darkThemePreviouslyInstalled)
        _settings.darkThemePreviouslyInstalled = true;

    return RestartMode::None;
}

int Plugin::FindPluginLangID()
{
    TCHAR lexerName[64] = { 0 };
    for (int i = 85; i < 115; i++)  // Language names start at 85 and supports up to 30 custom langs
    {
        Messenger().SendNppMessage<void>(NPPM_GETLANGUAGENAME, i, reinterpret_cast<LPARAM>(lexerName));
        for (int j = 0; j < LexerCatalogue::GetLexerCount(); j++)
        {
            if (_tcscmp(lexerName, str2wstr(LexerCatalogue::GetLexerName(j)).c_str()) == 0)
                return i;
        }
    }

    return 0;
}

// Look for our language menu item among installed external languages and call it
void Plugin::SetNotepadToPluginLexer()
{
    MENUITEMINFO menuInfo = {};
    HMENU currentMenu = GetNppMainMenu();
    generic_string menuItemName;
    generic_string lexerName = str2wstr(LexerCatalogue::GetLexerName(0));
    TCHAR szString[256] = {};

    int commandID = -1;

    // Search for name inside given external language names
    for (int i = IDM_LANG_EXTERNAL; i < IDM_LANG_EXTERNAL_LIMIT; i++)
    {
        ZeroMemory(szString, sizeof(szString));
        menuInfo.cch = 256;
        menuInfo.fMask = MIIM_TYPE;
        menuInfo.fType = MFT_STRING;
        menuInfo.cbSize = sizeof(MENUITEMINFO);
        menuInfo.dwTypeData = szString;
        bool bSuccess = GetMenuItemInfo(currentMenu, i, MF_BYCOMMAND, &menuInfo);
        menuItemName = szString;

        if (menuItemName == lexerName)
        {
            commandID = i;
            break;
        }
    }

    // Dispatch command.
    if (commandID > -1)
        Messenger().SendNppMessage<void>(NPPM_MENUCOMMAND, 0, commandID);
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

bool Plugin::IsPluginMenuItemEnabled(int ID)
{
    HMENU hMenu = GetNppMainMenu();
    if (hMenu)
    {
        UINT state = GetMenuState(hMenu, GetFunctions()[ID]._cmdID, MF_BYCOMMAND);
        return state == MF_DISABLED || state == MF_GRAYED ? false : true;
    }

    return false;
}

void Plugin::EnablePluginMenuItem(int ID, bool enabled)
{
    HMENU hMenu = GetNppMainMenu();
    if (hMenu)
    {
        EnableMenuItem(hMenu, GetFunctions()[ID]._cmdID, (MF_BYCOMMAND | (enabled)) ? MF_ENABLED : MF_DISABLED);
    }
}

void Plugin::RemovePluginMenuItem(int ID, bool byPosition)
{
    HMENU hMenu = GetNppMainMenu();

    if (hMenu)
    {
        if (!byPosition)
            RemoveMenu(hMenu, pluginFunctions[ID]._cmdID, MF_BYCOMMAND);
        else
        {
            // Find our submenu
            HMENU hSubMenu = FindSubMenu(hMenu, pluginName);
            if (hSubMenu)
                RemoveMenu(hSubMenu, ID, MF_BYPOSITION);
        }
    }
}

void Plugin::RemoveUnusedMenuItems()
{
    // If we don't have ALL necessary files required for the plugin, we remove some functions here:
    if (!PathFileExists(_pluginPaths["PluginAutoCompleteFilePath"].c_str()) ||
        !PathFileExists(_pluginPaths["PluginFunctionListFile"].c_str()))
    {
        RemovePluginMenuItem(PLUGINMENU_IMPORTDEFINITIONS);
        RemovePluginMenuItem(PLUGINMENU_IMPORTUSERTOKENS);
        RemovePluginMenuItem(PLUGINMENU_RESETUSERTOKENS);
        RemovePluginMenuItem(PLUGINMENU_RESETEDITORCOLORS);
    }

    // If notepad don't have overrideMap or functionList, remove the option to repair it
    if (!PathFileExists(_pluginPaths["NotepadOverrideMapFile"].c_str()) ||
        !PathFileExists(_pluginPaths["PluginFunctionListFile"].c_str()))
        RemovePluginMenuItem(PLUGINMENU_REPAIRXMLASSOCIATION);

    // If all files exist, remove the option to install them
    if (PathFileExists(_pluginPaths["PluginAutoCompleteFilePath"].c_str()) &&
        PathFileExists(_pluginPaths["PluginFunctionListFile"].c_str()) &&
        PathFileExists(_pluginPaths["NotepadOverrideMapFile"].c_str()))
    {
        RemovePluginMenuItem(PLUGINMENU_DASH6, true);
        RemovePluginMenuItem(PLUGINMENU_INSTALLCOMPLEMENTFILES);
    }

#ifndef DEBUG_AUTO_INDENT_833
    if (!Instance()._needPluginAutoIndent)
    {
        // Remove the "Use Auto-Indent" menu command and the following separator.
        RemovePluginMenuItem(PLUGINMENU_SWITCHAUTOINDENT);
        // Since the dash does not have an ID, got to the previous position.
        RemovePluginMenuItem(PLUGINMENU_DASH1 - 1, true);
    }
#endif
}

void Plugin::SetPluginMenuBitmap(int commandID, HBITMAP bitmap, bool bSetToUncheck, bool bSetToCheck)
{
    HMENU hMenu = GetNppMainMenu();
    if (hMenu)
    {
        if (bSetToUncheck && bSetToCheck)
            SetMenuItemBitmaps(hMenu, GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, bitmap, bitmap);
        if (bSetToUncheck && !bSetToCheck)
            SetMenuItemBitmaps(hMenu, GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, bitmap, NULL);
        if (!bSetToUncheck && bSetToCheck)
            SetMenuItemBitmaps(hMenu, GetFunctions()[commandID]._cmdID, MF_BYCOMMAND, NULL, bitmap);
    }

    return;
}

void Plugin::SetupPluginMenuItems()
{
    // Cleanup if reloading
    if (_menuBitmaps.size() > 0)
    {
        for (HBITMAP& i : _menuBitmaps)
            DeleteObject(i);
        _menuBitmaps.clear();
    }

    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_ABOUTBOX, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_COMPILEBATCH, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_COMPILEFILE, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_DARKTHEME, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_DEPENCENCYGROUP, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_DISASSEMBLECODE, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_IMMEDIATEWINDOW, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_IMPORTSETTINGS, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_NEVERWINTERAPP, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_REPEATLASTRUN, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_REPAIR, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_REPORT, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_RESTART, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_SETTINGSGROUP, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_SHOWASSIGNEDCONFIGURATION, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_UNDOCHECKBOXLIST, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_USERBUILD, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(loadSVGFromResource(DllHModule(), IDI_USERBUILDREMOVE, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16)));
    _menuBitmaps.push_back(getStockIconBitmap(SHSTOCKICONID::SIID_SHIELD, (IconSize)_dpiManager.scaleIconSize((UINT)IconSize::Size16x16)));
    _menuBitmaps.push_back(getStockIconBitmap(SHSTOCKICONID::SIID_SOFTWARE, (IconSize)_dpiManager.scaleIconSize((UINT)IconSize::Size16x16)));

    bool bSuccessLexer = false;
    bool bSuccessDark = false;
    bool bAutoComplete = false;
    bool bOverrideMap = false;
    bool bFunctionsList = false;

    PathWritePermission fLexerPerm = PathWritePermission::UndeterminedError;
    PathWritePermission fDarkThemePerm = PathWritePermission::UndeterminedError;
    PathWritePermission fAutoCompletePerm = PathWritePermission::UndeterminedError;
    PathWritePermission fOverrideMap = PathWritePermission::UndeterminedError;
    PathWritePermission fFunctionsList = PathWritePermission::UndeterminedError;

    SetPluginMenuBitmap(PLUGINMENU_COMPILESCRIPT, _menuBitmaps[2], true, false);
    SetPluginMenuBitmap(PLUGINMENU_DISASSEMBLESCRIPT, _menuBitmaps[5], true, false);
    SetPluginMenuBitmap(PLUGINMENU_BATCHPROCESSING, _menuBitmaps[1], true, false);
    SetPluginMenuBitmap(PLUGINMENU_RUNLASTBATCH, _menuBitmaps[9], true, false);
    SetPluginMenuBitmap(PLUGINMENU_FETCHPREPROCESSORTEXT, _menuBitmaps[11], true, false);
    SetPluginMenuBitmap(PLUGINMENU_VIEWSCRIPTDEPENDENCIES, _menuBitmaps[4], true, false);
    SetPluginMenuBitmap(PLUGINMENU_SHOWCONSOLE, _menuBitmaps[6], true, true);
    SetPluginMenuBitmap(PLUGINMENU_SETTINGS, _menuBitmaps[13], true, false);
    SetPluginMenuBitmap(PLUGINMENU_USERPREFERENCES, _menuBitmaps[14], true, false);
    SetPluginMenuBitmap(PLUGINMENU_ABOUTME, _menuBitmaps[0], true, false);

    //Setup icons for menus items that can be overriden later (because of UAC permissions)
    SetPluginMenuBitmap(PLUGINMENU_INSTALLDARKTHEME, _menuBitmaps[3], true, false);
    SetPluginMenuBitmap(PLUGINMENU_IMPORTDEFINITIONS, _menuBitmaps[7], true, false);
    SetPluginMenuBitmap(PLUGINMENU_IMPORTUSERTOKENS, _menuBitmaps[16], true, false);
    SetPluginMenuBitmap(PLUGINMENU_RESETUSERTOKENS, _menuBitmaps[17], true, false);
    SetPluginMenuBitmap(PLUGINMENU_RESETEDITORCOLORS, _menuBitmaps[12], true, false);
    SetPluginMenuBitmap(PLUGINMENU_REPAIRXMLASSOCIATION, _menuBitmaps[10], true, false);
    SetPluginMenuBitmap(PLUGINMENU_INSTALLCOMPLEMENTFILES, _menuBitmaps[19], true, false);

    // Don't use the shield icons when user runs in Administrator mode
    if (!IsUserAnAdmin())
    {
        // Retrieve write permissions for _pluginLexerConfigFile and _notepadDarkThemeFilePath
        bSuccessLexer = checkWritePermission(_pluginPaths["PluginLexerConfigFilePath"], fLexerPerm);
        bSuccessDark = checkWritePermission(_pluginPaths["NotepadDarkThemeFilePath"], fDarkThemePerm);
        bAutoComplete = checkWritePermission(_pluginPaths["PluginAutoCompleteFilePath"], fAutoCompletePerm);
        bOverrideMap = checkWritePermission(_pluginPaths["NotepadOverrideMapFile"], fOverrideMap);
        bFunctionsList = checkWritePermission(_pluginPaths["PluginFunctionListFile"], fFunctionsList);

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
        {
            SetPluginMenuBitmap(PLUGINMENU_IMPORTDEFINITIONS, _menuBitmaps[18], true, false);
            SetPluginMenuBitmap(PLUGINMENU_IMPORTUSERTOKENS, _menuBitmaps[18], true, false);
            SetPluginMenuBitmap(PLUGINMENU_RESETUSERTOKENS, _menuBitmaps[18], true, false);
        }
        // For users without permission to _notepadDarkThemeFilePath, set shield on Install Dark Theme if not already installed
        if (fDarkThemePerm == PathWritePermission::RequiresAdminPrivileges && _pluginDarkThemeIs == DarkThemeStatus::Uninstalled)
            SetPluginMenuBitmap(PLUGINMENU_INSTALLDARKTHEME, _menuBitmaps[18], true, false);
        // For users without permissions to any of the files (and also only checks Dark Theme support if file is existent and supported/not corrupted)...
        if (fLexerPerm == PathWritePermission::RequiresAdminPrivileges || (fDarkThemePerm == PathWritePermission::RequiresAdminPrivileges && _pluginDarkThemeIs != DarkThemeStatus::Unsupported))
            SetPluginMenuBitmap(PLUGINMENU_RESETEDITORCOLORS, _menuBitmaps[18], true, false);
        if (fOverrideMap == PathWritePermission::RequiresAdminPrivileges || fFunctionsList == PathWritePermission::RequiresAdminPrivileges)
            SetPluginMenuBitmap(PLUGINMENU_REPAIRXMLASSOCIATION, _menuBitmaps[18], true, false);
    }

}

void Plugin::LockPluginMenu(bool toLock)
{
    static bool runLastBatchWasEnabled = false;

    // Run last batch could be enabled or disabled before locking. Save the state
    // and use when unlocking controls.
    if (toLock)
        runLastBatchWasEnabled = IsPluginMenuItemEnabled(PLUGINMENU_RUNLASTBATCH);
    else
    {
        EnablePluginMenuItem(PLUGINMENU_RUNLASTBATCH, runLastBatchWasEnabled);
        runLastBatchWasEnabled = false;
    }

    EnablePluginMenuItem(PLUGINMENU_SWITCHAUTOINDENT, !toLock);
    EnablePluginMenuItem(PLUGINMENU_COMPILESCRIPT, !toLock);
    EnablePluginMenuItem(PLUGINMENU_DISASSEMBLESCRIPT, !toLock);
    EnablePluginMenuItem(PLUGINMENU_BATCHPROCESSING, !toLock);

    EnablePluginMenuItem(PLUGINMENU_FETCHPREPROCESSORTEXT, !toLock);
    EnablePluginMenuItem(PLUGINMENU_VIEWSCRIPTDEPENDENCIES, !toLock);
    EnablePluginMenuItem(PLUGINMENU_SHOWCONSOLE, !toLock);
    EnablePluginMenuItem(PLUGINMENU_SETTINGS, !toLock);
    EnablePluginMenuItem(PLUGINMENU_USERPREFERENCES, !toLock);
    EnablePluginMenuItem(PLUGINMENU_INSTALLDARKTHEME, !toLock);
    EnablePluginMenuItem(PLUGINMENU_IMPORTDEFINITIONS, !toLock);
    EnablePluginMenuItem(PLUGINMENU_IMPORTUSERTOKENS, !toLock);
    EnablePluginMenuItem(PLUGINMENU_RESETUSERTOKENS, !toLock);
    EnablePluginMenuItem(PLUGINMENU_RESETEDITORCOLORS, !toLock);
    EnablePluginMenuItem(PLUGINMENU_ABOUTME, !toLock);
}

#pragma endregion Plugin menu handling

// Check permissions on files.
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

// Checks the current status of the in-screen Scintilla document.
bool Plugin::CheckScintillaDocument()
{
    // Check if document is empty
    if (Messenger().SendSciMessage<size_t>(SCI_GETLENGTH) == 0)
        return false;

    // Get the output filename 
    TCHAR nameBuffer[MAX_PATH] = { 0 };
    Messenger().SendNppMessage<void>(NPPM_GETFULLCURRENTPATH, std::size(nameBuffer) - sizeof(TCHAR), reinterpret_cast<LPARAM>(nameBuffer));
    fs::path scriptPath = generic_string(nameBuffer);

    // Distraction helper: see if the current file our user is trying to process is a valid NSS script...
    if (!Instance().IsPluginLanguage())
    {
        if (MessageBox(Instance().NotepadHwnd(),
            TEXT("This file is not currently set as a NWScript language file. Do you want to proceed anyway?"),
            TEXT("Confirmation required"), MB_YESNO | MB_ICONQUESTION) == IDNO)
            return false;
    }

    // Another distraction helper: check if the user is trying to compile any DISASSEMBLED generated file
    if (scriptPath.extension() == "pcode")
    {
        MessageBox(Instance().NotepadHwnd(),
            TEXT("Hey pal, are you trying to compile assemble or PCODE? It will NOT compile with this plugin! :)"),
            TEXT("Cease and desist!"), MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    // Check if document is changed or else the save call will fail.
    if (Messenger().SendSciMessage<bool>(SCI_GETMODIFY))
    {
        if (!Messenger().SendNppMessage<bool>(NPPM_SAVECURRENTFILE))
            return false;
    }

    // Check for it's existance because the user might be opening a "dirty" memory file from previous session
    // and so SCI_GETMODIFY won't detect it properly.
    if (!PathFileExists(scriptPath.c_str()))
    {
        // Call save again...
        if (!Messenger().SendNppMessage<bool>(NPPM_SAVECURRENTFILE))
            return false;
    }

    return true;
}

#pragma endregion Plugin initialization functions and dynamic behavior

#pragma region

void Plugin::DoImportDefinitions()
{
    NWScriptParser::ScriptParseResults& myResults = *_NWScriptParseResults;
    tinyxml2::XMLDocument nwscriptDoc;

    // Set some Timestamp headers
    char timestamp[128]; time_t currTime;  struct tm currTimeP;
    time(&currTime);
    errno_t error = localtime_s(&currTimeP, &currTime);
    strftime(timestamp, 64, "Creation timestamp is: %B %d, %Y - %R", &currTimeP);
    std::string xmlHeaderComment = XMLDOCHEADER;
    xmlHeaderComment.append(timestamp).append(".\r\n");

    // Since TinyXML only accepts ASCII filenames, we do a crude conversion here... hopefully we won't have any
    // intermediary directory using chinese characters here... :P
    std::string asciiFileStyler = wstr2str(_pluginPaths["PluginLexerConfigFilePath"]);
    error = nwscriptDoc.LoadFile(asciiFileStyler.c_str());
    generic_stringstream errorStream;
    if (error)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    // Call helper function to strip all comments from document, since we're merging the file, not recreating it.
    // We don't use nwscriptDoc.rootNode() here, since it will jump straight to the first ELEMENT node - ignoring
    // comments and other possible pieces of information.
    stripXMLInfo(nwscriptDoc.FirstChild());

    // Navigate to Keywords
    tinyxml2::XMLElement* notepadPlus, * languages, * language, * Keywords, * lexerStyles;
    tinyxml2::XMLNode* lexerType;
    notepadPlus = nwscriptDoc.RootElement();
    if (notepadPlus)
    {
        languages = notepadPlus->FirstChildElement("Languages");
        if (languages)
        {
            language = languages->FirstChildElement("Language");
            if (language)
            {
                Keywords = language->FirstChildElement("Keywords");
            }
        }

        lexerStyles = notepadPlus->FirstChildElement("LexerStyles");
        if (lexerStyles)
            lexerType = lexerStyles->FirstChild();
    }

    if (!notepadPlus || !languages || !language || !Keywords || !lexerStyles || !lexerType)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    // We are only supporting our default lexer here
    if (!language->Attribute("name", LexerCatalogue::GetLexerName(0).c_str()))
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("Language name for ") << str2wstr(LexerCatalogue::GetLexerName(0)) << TEXT(" not found!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    // Add new declaration and header
    nwscriptDoc.InsertFirstChild(nwscriptDoc.NewDeclaration());
    nwscriptDoc.InsertAfterChild(nwscriptDoc.FirstChild(), nwscriptDoc.NewComment(xmlHeaderComment.c_str()));
    // Add info on Languages
    languages->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINLANGUAGECOMMENT));
    // Add info on Keywords
    language->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINKEYWORDCOMMENT));
    // Add info on LexerStyles and LexerTypes
    lexerStyles->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINLEXERTYPECOMMENT));
    lexerType->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINWORDSTYLECOMMENT));

    // We use a post-check to see whether all tags where updated. Also useful to avoid processing same tag twice 
    // (only happens if user tampered) with the file.
    bool bInstre1 = false, bType1 = false, bType2 = false, bType3 = false, bType4 = false, bType6 = false;
    while (Keywords)
    {
        if (Keywords->Attribute("name", "instre1") && !bInstre1)
        {
            Keywords->SetText(fixedInstructionSet);
            bInstre1 = true;
        }

        if (Keywords->Attribute("name", "type1") && !bType1)
        {
            Keywords->SetText(fixedKeywordSet);
            bType1 = true;
        }

        if (Keywords->Attribute("name", "type2") && !bType2)
        {
            std::string generic_output = myResults.MembersAsSpacedString(NWScriptParser::MemberID::EngineStruct);
            Keywords->SetText(generic_output.c_str());
            bType2 = true;
        }

        if (Keywords->Attribute("name", "type3") && !bType3)
        {
            Keywords->SetText(fixedObjKeywordSet);
            bType3 = true;
        }

        if (Keywords->Attribute("name", "type4") && !bType4)
        {
            std::string generic_output = myResults.MembersAsSpacedString(NWScriptParser::MemberID::Constant);
            Keywords->SetText(generic_output.c_str());
            bType4 = true;
        }

        if (Keywords->Attribute("name", "type6") && !bType6)
        {
            std::string generic_output = myResults.MembersAsSpacedString(NWScriptParser::MemberID::Function);
            Keywords->SetText(generic_output.c_str());
            bType6 = true;
        }

        Keywords = Keywords->NextSiblingElement("Keywords");
    }

    // Another error handling...
    if (!bInstre1 || !bType1 || !bType2 || !bType3 || !bType4 || !bType6)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("The following nodes could not be found!\r\n");
        errorStream << TEXT("Nodes: [") << (!bType2 ? TEXT(" type2") : TEXT("")) << (!bType4 ? TEXT(" type4") : TEXT(""))
            << (!bType6 ? TEXT(" type6") : TEXT("")) << TEXT(" ]");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    if (!_NWScriptParseResults->SerializeToFile(_pluginPaths["NWScriptEngineObjectsFile"]))
    {
        errorStream << TEXT("Error while saving file: ") << _pluginPaths["NWScriptEngineObjectsFile"] << "! \r\n";
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    // Now building auto-complete file.
    // We retrieve all fixed keywords and emplace them on results, so we can sort everything out
    // because unsorted results won't work for auto-complete
    std::string kw = fixedPreProcInstructionSet;
    kw.append(" ").append(fixedInstructionSet).append(" ").append(fixedKeywordSet).append(" ").append(fixedObjKeywordSet);
    myResults.AddSpacedStringAsKeywords(kw);

    // Merge with known user objects to rebuild autoComplete file
    NWScriptParser::ScriptParseResults knownUserObjects;
    knownUserObjects.SerializeFromFile(_pluginPaths["NWScriptUserObjectsFile"]);
    _NWScriptParseResults->Members.merge(knownUserObjects.Members);

    if (!MergeAutoComplete())
        return;

    error = nwscriptDoc.SaveFile(asciiFileStyler.c_str());
    if (error)
    {
        errorStream << TEXT("Error while saving file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    // Save statistics.
    Settings().engineStructs = _NWScriptParseResults->EngineStructuresCount;
    Settings().engineFunctionCount = _NWScriptParseResults->FunctionsCount;
    Settings().engineConstants = _NWScriptParseResults->ConstantsCount;

    // Close our results (for memory cleanup) and report back.
    _NWScriptParseResults.reset();
    int mResult = MessageBox(NotepadHwnd(),
        TEXT("Notepad++ needs to be restarted for the new settings to be reflected. Do it now?"),
        TEXT("Import successful!"), MB_YESNO | MB_ICONINFORMATION);

    if (mResult == IDYES)
    {
        // Setup our hook to auto-restart notepad++ normally and not run any other function on restart.
        // Then send message for notepad to close itself.
        SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
        Messenger().SendNppMessage(WM_CLOSE, 0, 0);
    }

}

void Plugin::DoImportUserTokens()
{
    NWScriptParser::ScriptParseResults& myResults = *_NWScriptParseResults;
    tinyxml2::XMLDocument nwscriptDoc;

    // Retrieve previous results and merge
    NWScriptParser::ScriptParseResults knownUserObjects;
    knownUserObjects.SerializeFromFile(_pluginPaths["NWScriptUserObjectsFile"]);
    _NWScriptParseResults->Members.merge(knownUserObjects.Members);
    _NWScriptParseResults->RecountStructs();

    // Set some Timestamp headers
    char timestamp[128]; time_t currTime;  struct tm currTimeP;
    time(&currTime);
    errno_t error = localtime_s(&currTimeP, &currTime);
    strftime(timestamp, 64, "Creation timestamp is: %B %d, %Y - %R", &currTimeP);
    std::string xmlHeaderComment = XMLDOCHEADER;
    xmlHeaderComment.append(timestamp).append(".\r\n");

    // Since TinyXML only accepts ASCII filenames, we do a crude conversion here...
    std::string asciiFileStyler = wstr2str(_pluginPaths["PluginLexerConfigFilePath"]);
    error = nwscriptDoc.LoadFile(asciiFileStyler.c_str());
    generic_stringstream errorStream;
    if (error)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    // Navigate to Keywords
    tinyxml2::XMLElement* notepadPlus, * languages, * language, * Keywords, * lexerStyles, * lexerType, * WordsStyle;
    notepadPlus = nwscriptDoc.RootElement();
    if (notepadPlus)
    {
        languages = notepadPlus->FirstChildElement("Languages");
        if (languages)
        {
            language = languages->FirstChildElement("Language");
            if (language)
            {
                Keywords = language->FirstChildElement("Keywords");
            }
        }

        lexerStyles = notepadPlus->FirstChildElement("LexerStyles");
        if (lexerStyles)
        {
            lexerType = lexerStyles->FirstChildElement("LexerType");
            if (lexerType)
            {
                WordsStyle = lexerType->FirstChildElement("WordsStyle");
            }
        }
    }

    if (!notepadPlus || !languages || !language || !Keywords || !lexerStyles || !lexerType || !WordsStyle)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    // We are only supporting our default lexer here
    if (!lexerType->Attribute("name", LexerCatalogue::GetLexerName(0).c_str()))
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("LexerType name for ") << str2wstr(LexerCatalogue::GetLexerName(0)) << TEXT(" not found!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    // Call helper function to strip all comments from document, since we're merging the file, not recreating it.
    // We don't use nwscriptDoc.rootNode() here, since it will jump straight to the first ELEMENT node - ignoring
    // comments and other possible pieces of information.
    stripXMLInfo(nwscriptDoc.FirstChild());

    // Add new declaration and header
    nwscriptDoc.InsertFirstChild(nwscriptDoc.NewDeclaration());
    nwscriptDoc.InsertAfterChild(nwscriptDoc.FirstChild(), nwscriptDoc.NewComment(xmlHeaderComment.c_str()));
    // Add info on Languages
    languages->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINLANGUAGECOMMENT));
    // Add info on Keywords
    language->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINKEYWORDCOMMENT));
    // Add info on LexerStyles and LexerTypes
    lexerStyles->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINLEXERTYPECOMMENT));
    lexerType->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINWORDSTYLECOMMENT));

    // We use a post-check to see whether all tags where updated. Also useful to avoid processing same tag twice 
    // (only happens if user tampered) with the file.
    bool bType5 = false, bType7 = false;
    while (WordsStyle)
    {
        if (WordsStyle->Attribute("keywordClass", "type5") && !bType5)
        {
            std::string generic_output = myResults.MembersAsSpacedString(NWScriptParser::MemberID::Constant);
            WordsStyle->SetText(generic_output.c_str());
            bType5 = true;
        }

        if (WordsStyle->Attribute("keywordClass", "type7") && !bType7)
        {
            std::string generic_output = myResults.MembersAsSpacedString(NWScriptParser::MemberID::Function);
            WordsStyle->SetText(generic_output.c_str());
            bType7 = true;
        }

        WordsStyle = WordsStyle->NextSiblingElement("WordsStyle");
    }

    // Another error handling...
    if (!bType5 || !bType7)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("The following nodes could not be found!\r\n");
        errorStream << TEXT("Nodes: [") << (!bType5 ? TEXT(" type2") : TEXT("")) << (!bType7 ? TEXT(" type4") : TEXT(""));
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    if (!_NWScriptParseResults->SerializeToFile(_pluginPaths["NWScriptUserObjectsFile"]))
    {
        errorStream << TEXT("Error while saving file: ") << _pluginPaths["NWScriptUserObjectsFile"] << "! \r\n";
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    // Merge with known user engine to rebuild autoComplete file
    NWScriptParser::ScriptParseResults knownEngineObjects;
    knownEngineObjects.SerializeFromFile(_pluginPaths["NWScriptEngineObjectsFile"]);
    _NWScriptParseResults->Members.merge(knownEngineObjects.Members);

    if (!MergeAutoComplete())
        return;

    error = nwscriptDoc.SaveFile(asciiFileStyler.c_str());
    if (error)
    {
        errorStream << TEXT("Error while saving file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return;
    }

    // Save statistics.
    Settings().userFunctionCount = _NWScriptParseResults->FunctionsCount;
    Settings().userConstants = _NWScriptParseResults->ConstantsCount;

    // Close our results (for memory cleanup) and report back.
    _NWScriptParseResults.reset();
    int mResult = MessageBox(NotepadHwnd(),
        TEXT("Notepad++ needs to be restarted for the new settings to be reflected. Do it now?"),
        TEXT("Import successful!"), MB_YESNO | MB_ICONINFORMATION);

    if (mResult == IDYES)
    {
        // Setup our hook to auto-restart notepad++ normally and not run any other function on restart.
        // Then send message for notepad to close itself.
        SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
        Messenger().SendNppMessage(WM_CLOSE, 0, 0);
    }
}

RestartMode Plugin::DoResetUserTokens(RestartFunctionHook whichPhase)
{
    // For safeguard, reset all hooks, since we can be autocalling this and get another error.
    SetRestartHook(RestartMode::None, RestartFunctionHook::None);

    tinyxml2::XMLDocument nwscriptDoc;

    // Set some Timestamp headers
    char timestamp[128]; time_t currTime;  struct tm currTimeP;
    time(&currTime);
    errno_t error = localtime_s(&currTimeP, &currTime);
    strftime(timestamp, 64, "Creation timestamp is: %B %d, %Y - %R", &currTimeP);
    std::string xmlHeaderComment = XMLDOCHEADER;
    xmlHeaderComment.append(timestamp).append(".\r\n");

    // Since TinyXML only accepts ASCII filenames, we do a crude conversion here...
    std::string asciiFileStyler = wstr2str(_pluginPaths["PluginLexerConfigFilePath"]);
    error = nwscriptDoc.LoadFile(asciiFileStyler.c_str());
    generic_stringstream errorStream;
    if (error)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return RestartMode::None;
    }

    // Navigate to Keywords
    tinyxml2::XMLElement* notepadPlus, * languages, * language, * Keywords, * lexerStyles, * lexerType, * WordsStyle;
    notepadPlus = nwscriptDoc.RootElement();
    if (notepadPlus)
    {
        languages = notepadPlus->FirstChildElement("Languages");
        if (languages)
        {
            language = languages->FirstChildElement("Language");
            if (language)
            {
                Keywords = language->FirstChildElement("Keywords");
            }
        }

        lexerStyles = notepadPlus->FirstChildElement("LexerStyles");
        if (lexerStyles)
        {
            lexerType = lexerStyles->FirstChildElement("LexerType");
            if (lexerType)
            {
                WordsStyle = lexerType->FirstChildElement("WordsStyle");
            }
        }
    }

    if (!notepadPlus || !languages || !language || !Keywords || !lexerStyles || !lexerType || !WordsStyle)
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return RestartMode::None;
    }

    // We are only supporting our default lexer here
    if (!lexerType->Attribute("name", LexerCatalogue::GetLexerName(0).c_str()))
    {
        errorStream << TEXT("Error while parsing file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("LexerType name for ") << str2wstr(LexerCatalogue::GetLexerName(0)) << TEXT(" not found!\r\n");
        errorStream << TEXT("File might be corrupted!\r\n");
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return RestartMode::None;
    }

    // Call helper function to strip all comments from document, since we're merging the file, not recreating it.
    // We don't use nwscriptDoc.rootNode() here, since it will jump straight to the first ELEMENT node - ignoring
    // comments and other possible pieces of information.
    stripXMLInfo(nwscriptDoc.FirstChild());

    // Add new declaration and header
    nwscriptDoc.InsertFirstChild(nwscriptDoc.NewDeclaration());
    nwscriptDoc.InsertAfterChild(nwscriptDoc.FirstChild(), nwscriptDoc.NewComment(xmlHeaderComment.c_str()));
    // Add info on Languages
    languages->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINLANGUAGECOMMENT));
    // Add info on Keywords
    language->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINKEYWORDCOMMENT));
    // Add info on LexerStyles and LexerTypes
    lexerStyles->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINLEXERTYPECOMMENT));
    lexerType->InsertFirstChild(nwscriptDoc.NewComment(XMLPLUGINWORDSTYLECOMMENT));

    // Reset wordstyles
    while (WordsStyle)
    {
        WordsStyle->SetText("");
        WordsStyle = WordsStyle->NextSiblingElement("WordsStyle");
    }

    // Load engine objects to rebuild autoComplete file
    _NWScriptParseResults = std::make_unique<NWScriptParser::ScriptParseResults>();
    _NWScriptParseResults->SerializeFromFile(_pluginPaths["NWScriptEngineObjectsFile"]);

    if (!MergeAutoComplete())
        return RestartMode::None;

    error = nwscriptDoc.SaveFile(asciiFileStyler.c_str());
    if (error)
    {
        errorStream << TEXT("Error while saving file: ") << _pluginPaths["PluginLexerConfigFilePath"] << "! \r\n";
        errorStream << TEXT("Error ID: ") << nwscriptDoc.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return RestartMode::None;
    }

    DeleteFile(_pluginPaths["NWScriptUserObjectsFile"].c_str());

    // Save statistics.
    Settings().userFunctionCount = 0;
    Settings().userConstants = 0;

    // Close our results (for memory cleanup) and report back.
    _NWScriptParseResults.reset();

    // Do restart. If it came from a previous restart callback, do it automatically. Else ask for it.
    if (whichPhase == RestartFunctionHook::ResetUserTokensPhase1)
    {
        SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
        Messenger().SendNppMessage(WM_CLOSE, 0, 0);
        return RestartMode::Normal;
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
            return RestartMode::Admin;
        }
    }

    return RestartMode::None;
}

RestartMode Plugin::DoResetEditorColors(RestartFunctionHook whichPhase)
{
    // For safeguard, reset all hooks, since we can be autocalling this and get another error.
    SetRestartHook(RestartMode::None, RestartFunctionHook::None);

    if (!PatchDarkThemeXMLFile())
        return RestartMode::None;

    if (!PatchDefaultThemeXMLFile())
        return RestartMode::None;

    // Do restart. If it came from a previous restart callback, do it automatically. Else ask for it.
    if (whichPhase == RestartFunctionHook::ResetEditorColorsPhase1)
    {
        SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
        Messenger().SendNppMessage(WM_CLOSE, 0, 0);
        return RestartMode::Normal;
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
            return RestartMode::Admin;
        }
    }

    return RestartMode::None;
}

RestartMode Plugin::DoInstallDarkTheme(RestartFunctionHook whichPhase)
{
    // For safeguard, reset all hooks, since we can be autocalling this and get another error.
    SetRestartHook(RestartMode::None, RestartFunctionHook::None);

    if (!PatchDarkThemeXMLFile())
        return RestartMode::None;

    // Clear install attempt marks, so next Notepad++ patch this will trigger a new auto-installation again (if set)...
    _settings.darkThemeInstallAttempt = false;

    // Do restart. If it came from a callback, do it automatically. Else ask for it.
    if (whichPhase == RestartFunctionHook::InstallDarkModePhase1)
    {
        SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
        Messenger().SendNppMessage(WM_CLOSE, 0, 0);
        return RestartMode::Normal;
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
            return RestartMode::Admin;
        }
    }

    return RestartMode::None;
}

RestartMode Plugin::DoRepairOverrideMap(RestartFunctionHook whichPhase)
{
    // For safeguard, reset all hooks, since we can be autocalling this and get another error.
    SetRestartHook(RestartMode::None, RestartFunctionHook::None);

    if (!CheckAndPatchOverrideMapXMLFile())
    {
        MessageBox(NotepadHwnd(), (TEXT("Error patching \"") + str2wstr(_pluginPaths["NotepadOverrideMapFile"].string()) + TEXT("\"")).c_str(),
            TEXT("Error patching file"), MB_ICONERROR | MB_OK);
        return RestartMode::None;
    }

    // Reinstall functionList from resources

    // Set some Timestamp headers
    char timestamp[128]; time_t currTime;  struct tm currTimeP;
    time(&currTime);
    errno_t error = localtime_s(&currTimeP, &currTime);
    strftime(timestamp, 64, "Creation timestamp is: %B %d, %Y - %R", &currTimeP);
    std::string xmlHeaderComment = XMLDOCHEADER;
    xmlHeaderComment.append(timestamp).append(".\r\n");
    xmlHeaderComment = "<!--" + xmlHeaderComment + "-->\r\n";

    std::string xmlString = std::string(XMLMANIFEST) + xmlHeaderComment + XMLFUNCTIONLIST;
    if (!bufferToFile(str2wstr(_pluginPaths["PluginFunctionListFile"].string()), xmlString))
    {
        MessageBox(NotepadHwnd(), (TEXT("Error writing file \"") + str2wstr(_pluginPaths["PluginFunctionListFile"].string()) + TEXT("\"")).c_str(),
            TEXT("Error saving file"), MB_ICONERROR | MB_OK);
        return RestartMode::None;
    }

    // Do restart. If it came from a callback, do it automatically. Else ask for it.
    if (whichPhase == RestartFunctionHook::RepairOverrideMapPhase1)
    {
        SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
        Messenger().SendNppMessage(WM_CLOSE, 0, 0);
        return RestartMode::Normal;
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
            return RestartMode::Admin;
        }
    }

    return RestartMode::None;
}

RestartMode Plugin::DoInstallAdditionalFiles(RestartFunctionHook whichPhase)
{
    TCHAR fName[MAX_PATH];
    Messenger().SendNppMessage(NPPM_GETPLUGINHOMEPATH, static_cast<WPARAM>(MAX_PATH), reinterpret_cast<LPARAM>(fName));

    fName[MAX_PATH - 1] = { 0 };
    generic_string PluginHome = fName;
    PluginHome.append(TEXT("\\")).append(_pluginFileName).append(TEXT("\\"));

    const int maxFiles = 4;
    generic_string file[maxFiles] = {
        PluginHome + _pluginFileName + TEXT(".xml"),
        PluginHome + NotepadAutoCompleteRootDir + str2wstr(LexerCatalogue::GetLexerName(0, true)) + TEXT(".xml"),
        PluginHome + NotepadFunctionListRootDir + OverrideMapFile,
        PluginHome + NotepadFunctionListRootDir + str2wstr(LexerCatalogue::GetLexerName(0, true)) + TEXT(".xml")
    };

    bool pExists = true;
    for (int i = 0; i < maxFiles; i++)
        if (!PathFileExists(file[i].c_str()))
            pExists = false;

    if (!pExists)
    {
        generic_stringstream s;
        s << "One or more files bellow are missing. Did you install all of the plugin's files properly? " << "\r\n";
        for (int i = 0; i < maxFiles; i++)
            s << file[1] << "\r\n";
        MessageBox(NotepadHwnd(), s.str().c_str(), TEXT("Error while copying NWScript files"), MB_ICONERROR | MB_OK);
        return RestartMode::None;
    }

    // Copy files
    CopyFile(file[0].c_str(), str2wstr(_pluginPaths["PluginLexerConfigFilePath"].string().c_str()).c_str(), false);
    CopyFile(file[1].c_str(), str2wstr(_pluginPaths["PluginAutoCompleteFilePath"].string().c_str()).c_str(), false);
    CopyFile(file[2].c_str(), str2wstr(_pluginPaths["NotepadOverrideMapFile"].string().c_str()).c_str(), true); // Override map cannot be ovewritten
    CopyFile(file[3].c_str(), str2wstr(_pluginPaths["PluginFunctionListFile"].string().c_str()).c_str(), false);

    // To ensure overrideMap.xml patch integrity, we now force a patch.
    CheckAndPatchOverrideMapXMLFile();

    // Do restart. If it came from a previous restart callback, do it automatically. Else ask for it.
    if (whichPhase == RestartFunctionHook::InstallAdditionalFilesPhase1)
    {
        SetRestartHook(RestartMode::Normal, RestartFunctionHook::None);
        Messenger().SendNppMessage(WM_CLOSE, 0, 0);
        return RestartMode::Normal;
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
            return RestartMode::Admin;
        }
    }

    return RestartMode::None;
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
                lexerWordsStyle->SetAttribute("fgColor", patchTypeSeek->Attribute("fgColor"));
                lexerWordsStyle->SetAttribute("bgColor", patchTypeSeek->Attribute("bgColor"));
                lexerWordsStyle->SetAttribute("fontName", patchTypeSeek->Attribute("fontName"));
                lexerWordsStyle->SetAttribute("fontStyle", patchTypeSeek->Attribute("fontStyle"));
                lexerWordsStyle->SetAttribute("fontSize", patchTypeSeek->Attribute("fontSize"));
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

bool Plugin::MergeAutoComplete()
{
    NWScriptParser::ScriptParseResults& myResults = *_NWScriptParseResults;

    // Recreate the nwscript.xml (AutoComplete) file
    char timestamp[128]; time_t currTime;  struct tm currTimeP;
    time(&currTime);
    errno_t error = localtime_s(&currTimeP, &currTime);
    strftime(timestamp, 64, "Creation timestamp is: %B %d, %Y - %R", &currTimeP);
    std::string xmlHeaderComment = XMLDOCHEADER;
    xmlHeaderComment.append(timestamp).append(".\r\n");

    std::string asciiFileAutoC = wstr2str(_pluginPaths["PluginAutoCompleteFilePath"]);
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
        keyword->SetAttribute("name", m.sName.c_str());
        if (m.mID == NWScriptParser::MemberID::Function)
        {
            keyword->SetAttribute("func", "yes");
            tinyxml2::XMLElement* overload = nwscriptAutoc.NewElement("Overload");
            overload->SetAttribute("retVal", m.sType.c_str());

            for (NWScriptParser::ScriptParamMember p : m.params)
            {
                tinyxml2::XMLElement* param = nwscriptAutoc.NewElement("Param");
                std::string paramName = p.sType.c_str();
                paramName.append(" ").append(p.sName);
                if (!p.sDefaultValue.empty())
                    paramName.append("=").append(p.sDefaultValue);
                param->SetAttribute("name", paramName.c_str());

                overload->InsertEndChild(param);
            }
            keyword->InsertEndChild(overload);
        }
        autocNode->InsertEndChild(keyword);
    }

    // Finally, save files...
    generic_stringstream errorStream;
    error = nwscriptAutoc.SaveFile(asciiFileAutoC.c_str());
    if (error)
    {
        errorStream << TEXT("Error while saving file: ") << _pluginPaths["PluginAutoCompleteFilePath"] << "! \r\n";
        errorStream << TEXT("Error ID: ") << nwscriptAutoc.ErrorID();
        MessageBox(NotepadHwnd(), errorStream.str().c_str(), pluginName.c_str(), MB_OK | MB_ICONERROR);
        _NWScriptParseResults.reset();
        return false;
    }

    return true;
}

bool Plugin::CheckAndPatchOverrideMapXMLFile()
{
    // Override map depends on "functionList\nwscript.xml" existing first, or else the patch will be useless...
    if (!PathFileExists(str2wstr(_pluginPaths["PluginFunctionListFile"].string()).c_str()))
        return false;

    tinyxml2::XMLDocument overrideMapXML;

    int PluginLangID = FindPluginLangID();
    if (PluginLangID == 0)
        return false;

    std::string xmlAssociationID = LexerCatalogue::GetLexerName(0).c_str();
    std::for_each(xmlAssociationID.begin(), xmlAssociationID.end(), [](char& c) {
        c = ::tolower(c); });
    xmlAssociationID.append(".xml");

    if (overrideMapXML.LoadFile(wstr2str(_pluginPaths["NotepadOverrideMapFile"]).c_str()) != 0)
        return false;

    // File already patched
    tinyxml2::XMLElement* associationType = searchElement(overrideMapXML.RootElement(), "association", "id", xmlAssociationID);
    if (associationType)
    {
        if (associationType->Attribute("langID", std::to_string(PluginLangID).c_str()))
            return true;

        // Wrong langID association, patch...
        associationType->SetAttribute("langID", std::to_string(PluginLangID).c_str());
        return (overrideMapXML.SaveFile(wstr2str(_pluginPaths["NotepadOverrideMapFile"]).c_str()) == 0);
    }

    // Try to patch the file
    tinyxml2::XMLElement* associationMap = searchElement(overrideMapXML.RootElement(), "associationMap");
    if (!associationMap)
        return false;

    associationType = overrideMapXML.NewElement("association");
    associationType->SetAttribute("id", xmlAssociationID.c_str());
    associationType->SetAttribute("langID", std::to_string(PluginLangID).c_str());

    associationMap->InsertEndChild(associationType);

    return (overrideMapXML.SaveFile(wstr2str(_pluginPaths["NotepadOverrideMapFile"]).c_str()) == 0);
}

#pragma endregion XML Config Files Management

#pragma region

// Receives notifications from Import Definitions Dialog
void Plugin::ImportDefinitionsCallback(HRESULT decision)
{
    // Trash results for memory space in a cancel.
    if (static_cast<int>(decision) == IDCANCEL || static_cast<int>(decision) == IDNO)
    {
        Instance()._NWScriptParseResults.reset();
        return;
    }

    // Calls the actual Import Definitions method
    Instance().DoImportDefinitions();
}

void Plugin::ImportUserTokensCallback(HRESULT decision)
{
    // Trash results for memory space in a cancel.
    if (static_cast<int>(decision) == IDCANCEL || static_cast<int>(decision) == IDNO)
    {
        Instance()._NWScriptParseResults.reset();
        return;
    }

    // Calls the actual Import Definitions method
    Instance().DoImportUserTokens();
}

// Receives notifications from Batch Processing Dialog
void Plugin::BatchProcessDialogCallback(HRESULT decision)
{
    Plugin& inst = Instance();

    if ((int)decision == IDCANCEL || (int)decision == IDCLOSE)
    {
        // Disable the last batch option, since it may have changed
        // and the user didn't run to test
        inst.EnablePluginMenuItem(PLUGINMENU_RUNLASTBATCH, false);
        return;
    }

    // Start counting ticks
    Instance()._clockStart = GetTickCount64();

    if (!inst._processingFilesDialog->isCreated())
    {
        inst._processingFilesDialog->init(inst.DllHModule(), inst.NotepadHwnd());
    }

    // Setup interrupt flag
    inst._processingFilesDialog->setInterruptFlag(inst._batchInterrupt);

    // Reset batch state
    inst.ResetBatchStates();

    // Prepare compiler
    inst.Compiler().reset();
    inst.Compiler().setMode(inst._settings.batchCompileMode);
    // Set callback to batch process
    inst.Compiler().setProcessingEndCallback(BatchProcessFilesCallback);

    // Display and clear compiler log window
    inst._loggerWindow->reset();
    inst.DisplayCompilerLogWindow(true);

    // Initial status dialog
    inst._processingFilesDialog->setStatus(TEXT("Building files list..."));
    inst._processingFilesDialog->showDialog();

    // The rest processes when file filters are done in separate thread
#ifdef USE_THREADS
    std::thread tBuilder(&Plugin::BuildFilesList, &inst);
    tBuilder.detach();
#else
    inst.BuildFilesList();
#endif
}

#pragma endregion Dialog Callbacks

#pragma region

// Compile or Disassemble a file / memory stream
void Plugin::DoCompileOrDisasm(generic_string filePath, bool fromCurrentScintilla, bool batchOperations)
{
    fs::path scriptPath;
    fs::path outputDir;

    _tempFileContents.clear();

    scriptPath = filePath;

    // If compiler settings not initialized, run command to initialize them, then re-check parameters - since it's a modal window.
    if (!Settings().compilerSettingsCreated)
    {
        CompilerSettings();
        _clockStart = GetTickCount64(); // Reset clock because user had opened configurations
    }

    // If user canceled compiler settings...
    if (!Settings().compilerSettingsCreated)
    {
        if (batchOperations)
            SendMessage(_processingFilesDialog->getHSelf(), WM_COMMAND, IDCANCEL, 0);
        return;
    }

    // If from scintilla, first, save the document, so we assure it got a valid filename and at least an output sPath.
    // Then we get the script name, and then the file contents.
    if (fromCurrentScintilla)
    {
        // Gets text length and then push contents. Scintilla contents returns length + 0-terminator character bytes
        size_t size = Messenger().SendSciMessage<size_t>(SCI_GETLENGTH) + 1;
        _tempFileContents.resize(size);
        Messenger().SendSciMessage<void>(SCI_GETTEXT, size, reinterpret_cast<LPARAM>(_tempFileContents.data()));

        // And now, get the output filename
        TCHAR nameBuffer[MAX_PATH] = { 0 };
        Messenger().SendNppMessage<void>(NPPM_GETFULLCURRENTPATH, std::size(nameBuffer) - sizeof(TCHAR), reinterpret_cast<LPARAM>(nameBuffer));
        scriptPath = generic_string(nameBuffer);
    }

    // Get output sPath
    if (!batchOperations)
    {
        if (Settings().useScriptPathToCompile)
            outputDir = scriptPath.parent_path();
        else
            outputDir = properDirNameW(Settings().outputCompileDir);
    }
    else
    {
        if (Settings().useScriptPathToBatchCompile)
            outputDir = scriptPath.parent_path();
        else
            outputDir = properDirNameW(Settings().batchOutputCompileDir);
    }

    // Last check for file existence (helps debugging)
    if (!PathFileExists(scriptPath.c_str()))
    {
        MessageBox(NotepadHwnd(), TEXT("Error: file inexistent or not yet saved!"), TEXT("Preprocessor check"), MB_OK | MB_ICONERROR);
        return;
    }

    if (_compiler.isOutputDirRequired())
    {
        if (!isValidDirectory(str2wstr(outputDir.string()).c_str()))
        {
            MessageBox(NotepadHwnd(), TEXT("Error: output directory is invalid or inexistent!"), TEXT("Preprocessor check"), MB_OK | MB_ICONERROR);
            return;
        }

        _compiler.setDestinationDirectory(outputDir);
    }

    // Lock controls to compiler log window
    LockPluginMenu(true);
    _loggerWindow->LockControls(true);

    // Increment statistics
    if (_compiler.getMode() == 0 && !_compiler.isFetchPreprocessorOnly() && !_compiler.isViewDependencies())
        Settings().compileAttempts++;
    if (_compiler.getMode() == 1)
        Settings().disassembledFiles++;

    // Process script.
    _compiler.setSourceFilePath(scriptPath);
#ifdef USE_THREADS
    std::thread tProcessor(&NWScriptCompilerV2::processFile, &_compiler, fromCurrentScintilla, &_tempFileContents[0]);
    tProcessor.detach();
#else
    _compiler.processFile(fromCurrentScintilla, &_tempFileContents[0]);
#endif
}

// Builds Batch File List
void Plugin::BuildFilesList()
{
    std::vector<generic_string> fileFilters;
    if (_settings.batchCompileMode == 0)
        fileFilters = _settings.getFileFiltersCompileV();
    else
        fileFilters = _settings.getFileFiltersDisasmV();

    // Iterate on all filters
    for (const auto& entry : fileFilters)
        createFilesList(_batchFilesToProcess, _settings.startingBatchFolder, entry, _settings.recurseSubFolders, _batchInterrupt);

    // Kickstart the batch process. We may be creating a 3rd thread here, because
    // batch will process on separate threads, but
    // for now, I don't see a better way of doing this...
    BatchProcessFilesCallback(static_cast<HRESULT>(static_cast<int>(true)));
}

// Receives notifications when a "Compile" menu command ends
void Plugin::CompileEndingCallback(HRESULT decision)
{
    NWScriptCompilerV2& compiler = Instance().Compiler();

    // Clear any content of temporary stash if exists
    Instance()._tempFileContents.clear();

    // Unlock controls to compiler log window
    Instance()._loggerWindow->LockControls(false);
    Instance().LockPluginMenu(false);

    // Check if logger window need to switch to errors panel
    Instance()._loggerWindow->checkSwitchToErrors();

    if (static_cast<int>(decision) == false)
    {
        Instance().Settings().compileFails++;
        return;
    }

    Instance().Settings().compileSuccesses++;
    WriteToCompilerLog({ LogType::ConsoleMessage, TEXT("") });
    WriteToCompilerLog({ LogType::ConsoleMessage, TEXT("File compiled successfully!") });

    // Options to generate symbols and auto display must be set.
    if (Instance().Settings().autoDisplayDebugSymbols && Instance().Settings().generateSymbols)
    {
        generic_string resultPath = str2wstr(properDirNameA(compiler.getDestinationDirectory().string()) + "\\" + compiler.getSourceFilePath().stem().string() + debugSymbolsFileSuffix);
        // Points notepad++ to open that file
        std::ignore = Instance().Messenger().SendNppMessage<bool>(NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(resultPath.c_str()));
        //Sets language to NWScript (because color syntax WILL work for assembled symbols)
        Instance().SetNotepadToPluginLexer();
    }

    // Mark compilation time.
    double durationFloat = (double)(GetTickCount64() - Instance()._clockStart) / (double)1000;
    WriteToCompilerLog({ LogType::ConsoleMessage, std::format(TEXT("(total execution time: {:.2f} seconds)\n"), durationFloat) });
}

// Receives notifications when a "Disassemble" menu command ends
void Plugin::DisassembleEndingCallback(HRESULT decision)
{
    NWScriptCompilerV2& compiler = Instance().Compiler();

    // Unlock controls to compiler log window
    Instance()._loggerWindow->LockControls(false);
    Instance().LockPluginMenu(false);

    // Check if logger window need to switch to errors panel
    Instance()._loggerWindow->checkSwitchToErrors();

    if (static_cast<int>(decision) == static_cast<int>(false))
        return;

    if (Instance().Settings().autoDisplayDisassembled)
    {
        generic_string resultPath = str2wstr(properDirNameA(compiler.getDestinationDirectory().string()) + "\\" + compiler.getSourceFilePath().stem().string() + disassembledScriptSuffix);
        // Points notepad++ to open that file
        std::ignore = Instance().Messenger().SendNppMessage<bool>(NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(resultPath.c_str()));
        //Sets language to NWScript (because color syntax WILL work for assembled symbols)
        Instance().SetNotepadToPluginLexer();
    }

    // Mark compilation time.
    double durationFloat = (double)(GetTickCount64() - Instance()._clockStart) / (double)1000;
    WriteToCompilerLog({ LogType::ConsoleMessage, std::format(TEXT("(total execution time: {:.2f} seconds)\n"), durationFloat) });
}

// Receives notifications for each file processed
void Plugin::BatchProcessFilesCallback(HRESULT decision)
{
    Plugin& inst = Instance();

    // Check for failed results
    if (static_cast<int>(decision) == static_cast<int>(false) && !inst._settings.continueCompileOnFail)
    {
        WriteToCompilerLog({ LogType::ConsoleMessage, TEXT("") });
        WriteToCompilerLog({ LogType::ConsoleMessage, TEXT("Failed to process file... batch processing stopped.") });
        WriteToCompilerLog({ LogType::ConsoleMessage, TEXT("Done.") });

        inst._processingFilesDialog->display(false);
        inst._loggerWindow->LockControls(false);
        inst.LockPluginMenu(false);

        // Check if logger window need to switch to errors panel
        Instance()._loggerWindow->checkSwitchToErrors();

        return;
    }

    // Check for interruption requests.
    if (inst._batchInterrupt)
    {
        WriteToCompilerLog({ LogType::ConsoleMessage, TEXT("") });
        WriteToCompilerLog({ LogType::ConsoleMessage, TEXT("Batch processing interrupted by user's request.") });

        inst._processingFilesDialog->display(false);
        inst._loggerWindow->LockControls(false);
        inst.LockPluginMenu(false);
        return;
    }

    // Pick next (or first) file in line.
    if (inst._batchCurrentFileIndex < inst._batchFilesToProcess.size())
    {
        generic_string nextFile = inst._batchFilesToProcess[inst._batchCurrentFileIndex];
        inst._batchCurrentFileIndex++;

        // Update status
        inst._processingFilesDialog->setStatus(nextFile);

        // This function will create a thread that will callback here again for next file...
        inst.DoCompileOrDisasm(nextFile, false, true);
    }
    else
    {
        // Done processing, write messages to log, close processing dialog.
        WriteToCompilerLog({ LogType::ConsoleMessage, TEXT("") });
        WriteToCompilerLog({ LogType::ConsoleMessage, TEXT("Finished processing ") +
            std::to_wstring(inst._batchFilesToProcess.size()) + TEXT(" files successfully.") });

        inst._processingFilesDialog->display(false);

        // Enable run last batch (after unlocking controls)
        inst._loggerWindow->LockControls(false);
        inst.LockPluginMenu(false);
        Instance().EnablePluginMenuItem(PLUGINMENU_RUNLASTBATCH, true);

        // Check if logger window need to switch to errors panel
        Instance()._loggerWindow->checkSwitchToErrors();

        // Reset batch state
        inst.ResetBatchStates();

        // Mark compilation time.
        double durationFloat = (double)(GetTickCount64() - inst._clockStart) / (double)1000;
        WriteToCompilerLog({ LogType::ConsoleMessage, std::format(TEXT("(total execution time: {:.2f} seconds)\n"), durationFloat) });
    }
}

// Receives notifications when a "Fetch preprocessed" menu command ends
void Plugin::FetchPreprocessedEndingCallback(HRESULT decision)
{
    // Unlock controls to compiler log window
    Instance()._loggerWindow->LockControls(false);
    Instance().LockPluginMenu(false);

    // Check if logger window need to switch to errors panel
    Instance()._loggerWindow->checkSwitchToErrors();


    if (static_cast<int>(decision) == static_cast<int>(false))
        return;

    // Creates a new document...
    Instance().Messenger().SendNppMessage<void>(NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);
    // Sets the language to NWScript
    Instance().SetNotepadToPluginLexer();
    // Sets document data
    Instance().Messenger().SendSciMessage<void>(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(Instance().Compiler().logger().getProcessorString().c_str()));

    // Mark compilation time.
    double durationFloat = (double)(GetTickCount64() - Instance()._clockStart) / (double)1000;
    WriteToCompilerLog({ LogType::ConsoleMessage, std::format(TEXT("(total execution time: {:.2f} seconds)\n"), durationFloat) });

}

// Receives notifications when a "View Script Dependencies" menu command ends
void Plugin::ViewDependenciesEndingCallback(HRESULT decision)
{
    // Unlock controls to compiler log window
    Instance()._loggerWindow->LockControls(false);
    Instance().LockPluginMenu(false);

    // Check if logger window need to switch to errors panel
    Instance()._loggerWindow->checkSwitchToErrors();

    if (static_cast<int>(decision) == static_cast<int>(false))
        return;

    // Creates a new document...
    Instance().Messenger().SendNppMessage<void>(NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);
    // Sets document data
    Instance().Messenger().SendSciMessage<void>(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(Instance().Compiler().logger().getProcessorString().c_str()));

    // Mark compilation time.
    double durationFloat = (double)(GetTickCount64() - Instance()._clockStart) / (double)1000;
    WriteToCompilerLog({ LogType::ConsoleMessage, std::format(TEXT("(total execution time: {:.2f} seconds)\n"), durationFloat) });
}

// Write messages to the compiler window - also called back from compiler logger
void Plugin::WriteToCompilerLog(const NWScriptLogger::CompilerMessage& message)
{
    Instance()._loggerWindow->LogMessage(message, Instance()._compiler.getSourceFilePath());
}

// Receives notifications from Compiler Window to open files and navigato to text inside it
void Plugin::NavigateToCode(const generic_string& fileName, size_t lineNum, const generic_string& rawMessage,
    const fs::path& filePath)
{
    // Search for the file name.
    // First comparison: the easy way - file is the same as the one being
    // processed. Also, the compiler only returns lowercase names, so we
    // compare with case insensitive.
    generic_string finalPath;
    if (_wcsicmp(str2wstr(filePath.filename().string()).c_str(), fileName.c_str()) == 0)
        finalPath = filePath.c_str();

    // Or the hard way...
    else
    {
        // First search for filename inside the same folder as the script,
        // Then search all include paths on the compiler settings.
        // If still fails is because it's a script inside the Neverwinter
        // sPath, so we don't open.
        generic_string tempPath = str2wstr(filePath.parent_path().string()) + TEXT("\\") + fileName;
        if (PathFileExists(tempPath.c_str()))
            finalPath = tempPath;
        else
        {
            for (const generic_string& s : Instance().Settings().getIncludeDirsV())
            {
                tempPath = s + TEXT("\\") + fileName;
                if (PathFileExists(tempPath.c_str()))
                {
                    finalPath = tempPath;
                    break;
                }
            }
        }
    }

    if (finalPath.empty())
    {
        MessageBox(Instance().NotepadHwnd(), (TEXT("Cannot open file: ") + fileName +
            TEXT(". Maybe it's one of the Neverwinter original #includes that are compressed inside the Neverwinter folder.")).c_str(),
            TEXT("Inaccessible file."), MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    PluginMessenger msg = Instance().Messenger();

    // Open file into notepad. If already opened, it will get the focus.
    bool success = msg.SendNppMessage<bool>(NPPM_DOOPEN, 0, (LPARAM)finalPath.c_str());

    // Navigate to line
    // HACK: For some reason, the normal operation won't work if the file doesn't have the focus.
    // So we schedule the execution to happen assynchronously with the smallest possible time frame.
    if (success && lineNum > -1)
    {
        SetTimer(Instance().NotepadHwnd(), NAGIVATECALLBACKTIMER, USER_TIMER_MINIMUM, (TIMERPROC)RunScheduledReposition);
    }
}

// Reposition the navigation cursor assynchronously.
// This is required to fix a behavior of scintilla window not updating correctly the caret position 
// (centralized on screen) when navigating to a different document, when this function is called in the same thread.
void CALLBACK Plugin::RunScheduledReposition(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
    PluginMessenger msg = Instance().Messenger();
    int lineNum = Instance()._loggerWindow->getCurrentNavigationLine();

    int currentPosition = msg.SendSciMessage<int>(SCI_GETCURRENTPOS);
    int linePositionStart = msg.SendSciMessage<int>(SCI_POSITIONFROMLINE, (WPARAM)lineNum - 1);
    int linePositionEnd = msg.SendSciMessage<int>(SCI_GETLINEENDPOSITION, (WPARAM)lineNum - 1);

    bool isDownwards = currentPosition < linePositionEnd;

    // Make sure target lines are unfolded
    msg.SendSciMessage<void>(SCI_ENSUREVISIBLE, msg.SendSciMessage<int>(SCI_LINEFROMPOSITION, linePositionStart));
    msg.SendSciMessage<void>(SCI_ENSUREVISIBLE, msg.SendSciMessage<int>(SCI_LINEFROMPOSITION, linePositionEnd));

    // Jump-scroll to center, if current position is out of view. Repeat operation
    // to ensure visibility on recent opened documents
    msg.SendSciMessage<void>(SCI_SETVISIBLEPOLICY, CARET_JUMPS | CARET_EVEN);
    msg.SendSciMessage<void>(SCI_ENSUREVISIBLEENFORCEPOLICY,
        msg.SendSciMessage<int>(SCI_LINEFROMPOSITION, isDownwards ? linePositionEnd : linePositionStart));
    // When going downward, the end position is important, otherwise the start
    msg.SendSciMessage<void>(SCI_GOTOPOS, isDownwards ? linePositionEnd : linePositionStart);
    msg.SendSciMessage<void>(SCI_SETVISIBLEPOLICY, CARET_EVEN);
    msg.SendSciMessage<void>(SCI_ENSUREVISIBLEENFORCEPOLICY,
        msg.SendSciMessage<int>(SCI_LINEFROMPOSITION, isDownwards ? linePositionEnd : linePositionStart));

    msg.SendSciMessage<void>(SCI_SCROLLRANGE, linePositionStart, linePositionEnd);

    msg.SendSciMessage<void>(SCI_GOTOPOS, linePositionEnd);
    msg.SendSciMessage<void>(SCI_GOTOPOS, linePositionStart);

    // Update Scintilla's knowledge about what column the caret is in, so that if user
    // does up/down arrow as first navigation after the search result is selected,
    // the caret doesn't jump to an unexpected column
    Instance().Messenger().SendSciMessage<void>(SCI_CHOOSECARETX);

    KillTimer(hwnd, NAGIVATECALLBACKTIMER);
}

#pragma endregion Compiler Funcionality

#pragma region

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

#ifndef DEBUG_AUTO_INDENT_833
    // Already accepted the warning, either on this session or a previous one
    if (Instance().Settings().autoIndentationWarningAccepted)
        return;
#endif

    // Warning user of function: only once in a session (and perhaps in a lifetime if INI file doesn't change)
    if (bEnableAutoIndent && !bAutoIndentationWarningShown)
    {
        if (!warningDialog.isCreated())
            warningDialog.init(Instance().DllHModule(), Instance().NotepadHwnd());

        warningDialog.appendSettings(&Instance().Settings());

        if (!warningDialog.isVisible())
            warningDialog.doDialog();

#ifndef DEBUG_AUTO_INDENT_833
        bAutoIndentationWarningShown = true;
#endif
    }
}

//-------------------------------------------------------------

// Compiles current .NSS Script file
PLUGINCOMMAND Plugin::CompileScript()
{
    // Do a check of the current script for the user.
    if (!Instance().CheckScintillaDocument())
        return;

    // Start counting ticks
    Instance()._clockStart = GetTickCount64();

    // Display and clear compiler log window
    Instance().DisplayCompilerLogWindow(true);
    Instance()._loggerWindow->reset();

    // Reset compiler so we catch all possible dependencies editions.
    Instance().Compiler().reset();
    // Set mode to compile script
    Instance().Compiler().setMode(0);
    // Set our caller callback
    Instance().Compiler().setProcessingEndCallback(CompileEndingCallback);
    // Pass the control to core function calling compile from current document
    Instance().DoCompileOrDisasm(TEXT(""), true);
}

// Disassemble a compiled script file
PLUGINCOMMAND Plugin::DisassembleFile()
{
    std::vector<generic_string> nFileName;
    if (openFileDialog(Instance().NotepadHwnd(), nFileName,
        TEXT("NWScript Compiled Files (*.ncs)\0*.ncs\0All Files (*.*)\0*.*"),
        properDirNameW(Instance().Settings().lastOpenedDir)))
    {
        // Start counting ticks
        Instance()._clockStart = GetTickCount64();

        // Display and clear compiler log window
        Instance().DisplayCompilerLogWindow(true);
        Instance()._loggerWindow->reset();

        // Reset compiler cache and clear log so we catch all possible dependencies editions.
        Instance().Compiler().reset();
        // Set mode to disassemble script
        Instance().Compiler().setMode(1);
        // Set our caller callback
        Instance().Compiler().setProcessingEndCallback(DisassembleEndingCallback);
        // Pass the control to core function calling disassemble from file
        Instance().DoCompileOrDisasm(nFileName[0]);
    }
}

// Menu Command "Run last successful batch" function handler. 
PLUGINCOMMAND Plugin::RunLastBatch()
{
    BatchProcessDialogCallback(static_cast<HRESULT>(static_cast<int>(true)));
}

// Opens the Plugin's Batch process files dialog
PLUGINCOMMAND Plugin::BatchProcessFiles()
{
    static BatchProcessingDialog batchProcessing = {};

    if (!batchProcessing.isCreated())
    {
        batchProcessing.setOkDialogCallback(&Plugin::BatchProcessDialogCallback);
        batchProcessing.appendSettings(&Instance()._settings);
        batchProcessing.init(Instance().DllHModule(), Instance().NotepadHwnd());
    }

    if (!batchProcessing.isVisible())
        batchProcessing.doDialog();

}

//-------------------------------------------------------------

// Toggles the log console
PLUGINCOMMAND Plugin::ToggleLogger()
{
    Instance().Messenger().SendNppMessage<void>(NPPM_SETMENUITEMCHECK,
        pluginFunctions[PLUGINMENU_SHOWCONSOLE]._cmdID, !Instance()._loggerWindow->isVisible());
    Instance().DisplayCompilerLogWindow(!Instance()._loggerWindow->isVisible());
}

//-------------------------------------------------------------

// Menu Command "Fetch preprocessor text" function handler. 
PLUGINCOMMAND Plugin::FetchPreprocessorText()
{
    // Do a check of the current script for the user.
    if (!Instance().CheckScintillaDocument())
        return;

    // Start counting ticks
    Instance()._clockStart = GetTickCount64();

    // Display and clear compiler log window
    Instance().DisplayCompilerLogWindow(true);
    Instance()._loggerWindow->reset();

    // Reset compiler so we catch all possible dependencies editions.
    Instance().Compiler().reset();
    // Tells compiler to only fetch preprocessed text
    Instance().Compiler().setFetchPreprocessorOnly();
    // Set our caller callback
    Instance().Compiler().setProcessingEndCallback(FetchPreprocessedEndingCallback);
    // Pass the control to core function calling compile from current document
    Instance().DoCompileOrDisasm(TEXT(""), true);

}

// Menu Command "View Script Dependencies" function handler. 
PLUGINCOMMAND Plugin::ViewScriptDependencies()
{
    // Do a check of the current script for the user.
    if (!Instance().CheckScintillaDocument())
        return;

    // Start counting ticks
    Instance()._clockStart = GetTickCount64();

    // Display and clear compiler log window
    Instance().DisplayCompilerLogWindow(true);
    Instance()._loggerWindow->reset();

    // Reset compiler so we catch all possible dependencies editions.
    Instance().Compiler().reset();
    // Tells compiler to only fetch preprocessed text
    Instance().Compiler().setViewDependencies();
    // Set our caller callback
    Instance().Compiler().setProcessingEndCallback(ViewDependenciesEndingCallback);
    // Pass the control to core function calling compile from current document
    Instance().DoCompileOrDisasm(TEXT(""), true);
}

//-------------------------------------------------------------

// Opens the Plugin's Compiler Settings panel
PLUGINCOMMAND Plugin::CompilerSettings()
{
    static CompilerSettingsDialog compilerSettings = {};

    compilerSettings.init(Instance().DllHModule(), Instance().NotepadHwnd());
    compilerSettings.appendSettings(&Instance()._settings);
    compilerSettings.doDialog();
}

// Opens the user's preferences.
PLUGINCOMMAND Plugin::UserPreferences()
{
    static UsersPreferencesDialog userPreferences = {};

    userPreferences.init(Instance().DllHModule(), Instance().NotepadHwnd());
    userPreferences.appendSettings(&Instance()._settings);
    userPreferences.setDarkModeInstalled(Instance()._pluginDarkThemeIs == DarkThemeStatus::Installed);
    userPreferences.doDialog();
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

std::vector<generic_string> nFileName;
if (openFileDialog(Instance().NotepadHwnd(), nFileName,
    TEXT("nwscritpt.nss\0nwscript.nss\0All Files (*.*)\0*.*"),
    properDirNameW(Instance().Settings().lastOpenedDir)))
{
    // Opens the NWScript file and parse it. Keep the results for later use
    NWScriptParser nParser(Instance().NotepadHwnd());

    Instance()._NWScriptParseResults = std::make_unique<NWScriptParser::ScriptParseResults>();
    NWScriptParser::ScriptParseResults& myResults = *Instance()._NWScriptParseResults;

    bool bSuccess = nParser.ParseFile(nFileName[0], myResults);
    if (!bSuccess)
    {
        MessageBox(Instance().NotepadHwnd(), (TEXT("Error while parsing file \"") +
            nFileName[0] + TEXT("\".\r\n- File may be inaccessible.")).c_str(),
            TEXT("Error parsing file"), MB_ICONERROR | MB_OK);
        return;
    }

    // Last check for results: File empty?
    if (myResults.EngineStructuresCount == 0 && myResults.FunctionsCount == 0 && myResults.ConstantsCount == 0)
    {
        MessageBox(Instance().NotepadHwnd(), TEXT("File analysis didn't find anything to import!"), pluginName.c_str(),
            MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    // Show File Parsing Results dialog message and since we don't want it to be modal, wait for callback in ImportDefinitionsCallback.
    parseDialog.setEngineStructuresCount(myResults.EngineStructuresCount);
    parseDialog.setFunctionDefinitionsCount(myResults.FunctionsCount);
    parseDialog.setConstantsCount(myResults.ConstantsCount);

    parseDialog.setOkDialogCallback(&Plugin::ImportDefinitionsCallback);
    parseDialog.doDialog();
}
}

// Menu Command "Import user-defined tokens" function handler. 
PLUGINCOMMAND Plugin::ImportUserTokens()
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
    sFiles.push_back(Instance()._pluginPaths["PluginLexerConfigFilePath"]);
    // The auto complete file may or may not exist. If not exist, we check the directory permissions instead.
    if (PathFileExists(Instance()._pluginPaths["PluginAutoCompleteFilePath"].c_str()))
        sFiles.push_back(Instance()._pluginPaths["PluginAutoCompleteFilePath"]);
    else
        sFiles.push_back(Instance()._pluginPaths["PluginAutoCompleteFilePath"]);

    PathCheckResults fResult = Instance().WritePermissionCheckup(sFiles, RestartFunctionHook::None);
    if (static_cast<int>(fResult) < 1)
        return;

    std::vector<generic_string> nFileNames;
    if (openFileDialog(Instance().NotepadHwnd(), nFileNames,
        TEXT("NWScript Files (*.nss)\0*.nss\0All Files (*.*)\0*.*"),
        properDirNameW(Instance().Settings().lastOpenedDir), true))
    {
        // Opens the NWScript file and parse it. Keep the results for later use
        NWScriptParser nParser(Instance().NotepadHwnd());

        Instance()._NWScriptParseResults = std::make_unique<NWScriptParser::ScriptParseResults>();
        NWScriptParser::ScriptParseResults& myResults = *Instance()._NWScriptParseResults;

        bool bSuccess = nParser.ParseBatch(nFileNames, myResults);
        if (!bSuccess)
        {
            MessageBox(Instance().NotepadHwnd(), TEXT("Error while parsing file(s).\r\nOne or more of the selected files might be inaccessible."),
                TEXT("Error parsing file"), MB_ICONERROR | MB_OK);
            return;
        }

        // Last check for results: File empty?
        if (myResults.FunctionsCount == 0 && myResults.ConstantsCount == 0)
        {
            MessageBox(Instance().NotepadHwnd(), TEXT("File analysis didn't find anything to import!"), pluginName.c_str(),
                MB_ICONEXCLAMATION | MB_OK);
            return;
        }

        // Show File Parsing Results dialog message and since we don't want it to be modal, wait for callback in ImportDefinitionsCallback.
        parseDialog.setEngineStructuresCount(myResults.EngineStructuresCount);
        parseDialog.setFunctionDefinitionsCount(myResults.FunctionsCount);
        parseDialog.setConstantsCount(myResults.ConstantsCount);

        parseDialog.setOkDialogCallback(&Plugin::ImportUserTokensCallback);
        parseDialog.doDialog();
    }
}

// Menu Command "Reset user-defined tokens" function handler. 
PLUGINCOMMAND Plugin::ResetUserTokens()
{
    if (MessageBox(Instance().NotepadHwnd(), TEXT("Warning: this action cannot be undone. Continue?"), TEXT("NWScript Tools - Confirmation required"),
        MB_YESNO | MB_ICONQUESTION) == IDNO)
        return;

    // Do a file check for the necessary XML files
    std::vector<generic_string> sFiles;
    sFiles.push_back(Instance()._pluginPaths["PluginLexerConfigFilePath"]);
    // The auto complete file may or may not exist. If not exist, we check the directory permissions instead.
    if (PathFileExists(Instance()._pluginPaths["PluginAutoCompleteFilePath"].c_str()))
        sFiles.push_back(Instance()._pluginPaths["PluginAutoCompleteFilePath"]);
    else
        sFiles.push_back(Instance()._pluginPaths["PluginAutoCompleteFilePath"]);

    PathCheckResults fResult = Instance().WritePermissionCheckup(sFiles, RestartFunctionHook::ResetUserTokensPhase1);
    if (static_cast<int>(fResult) < 1)
        return;

        Instance().DoResetUserTokens();
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

// Repairs the functionList xml and OverrideMap associations
PLUGINCOMMAND Plugin::RepairFunctionList()
{
    // Do a file check for the necessary XML files
    std::vector<generic_string> sFiles;
    sFiles.push_back(Instance()._pluginPaths["NotepadOverrideMapFile"]);
    sFiles.push_back(Instance()._pluginPaths["PluginFunctionListFile"]);

    PathCheckResults fResult = Instance().WritePermissionCheckup(sFiles, RestartFunctionHook::RepairOverrideMapPhase1);
    if (static_cast<int>(fResult) < 1)
        return;

    Instance().DoRepairOverrideMap();
}

//-------------------------------------------------------------

PLUGINCOMMAND Plugin::InstallAdditionalFiles()
{
    // Pick files to get permissions
    std::vector<generic_string> sFiles;
    sFiles.push_back(Instance()._pluginPaths["PluginAutoCompleteFilePath"]);
    sFiles.push_back(Instance()._pluginPaths["PluginFunctionListFile"]);
    sFiles.push_back(Instance()._pluginPaths["NotepadOverrideMapFile"]);

    std::vector<generic_string> sDirs;
    sDirs.push_back(Instance()._pluginPaths["PluginAutoCompleteFilePath"].parent_path());
    sDirs.push_back(Instance()._pluginPaths["PluginFunctionListFile"].parent_path());

    bool Permissioned = true;
    for (const generic_string& f : sDirs)
    {
        PathWritePermission p;
        if (!checkWritePermission(f, p))
        {
            MessageBox(Instance().NotepadHwnd(), (TEXT("Directory inaccessible or inexistent: ") + f).c_str(),
                TEXT("NWScript Error"), MB_ICONERROR | MB_OK);
            return;
        }

        if (p != PathWritePermission::WriteAllowed)
            Permissioned = false;
    }

    if (Permissioned || IsUserAnAdmin())
    {
        RestartFunctionHook whichMode = Instance()._OneTimeOffer ? RestartFunctionHook::InstallAdditionalFilesPhase1 : RestartFunctionHook::None;
        Instance().DoInstallAdditionalFiles(whichMode);
        Instance()._OneTimeOfferAccepted = true; // Must set this flag here to avoid losing restart hooks
        return;
    }

    // Show File Access dialog box in Admin Mode
    PathAccessDialog ePermission = {};
    ePermission.init(Instance().DllHModule(), Instance().NotepadHwnd());
    ePermission.SetAdminMode(true);
    ePermission.SetPathsText(sFiles);
    // Morph dialog to ask the user to copy files.
    ePermission.SetMorphToCopyMode();

    INT_PTR RunAdmin = ePermission.doDialog();
    if (RunAdmin == IDOK)
    {
        // Set our hook flag 2 to re-run Notepad++ in admin mode and call iFunctionToCallIfRestart upon restart
        // Then tells notepad++ to quit.
        Instance().SetRestartHook(RestartMode::Admin, RestartFunctionHook::InstallAdditionalFilesPhase1);
        Instance().Messenger().SendNppMessage(WM_CLOSE, 0, 0);
    }

    // In case this option was called from One Time Offer
    Instance()._OneTimeOfferAccepted = (RunAdmin == IDOK);
}

//-------------------------------------------------------------

// Opens About Box
PLUGINCOMMAND Plugin::AboutMe()
{
    ::SendMessage(Instance()._loggerWindow->getHSelf(), WM_SIZE, 0, 0);

    std::vector<generic_string> darkModeLabels = { TEXT("Uninstalled"), TEXT("Installed"), TEXT("Unsupported") };

    // Initialize some user information 
    std::map<generic_string, generic_string> replaceStrings;
    replaceStrings.insert({ TEXT("%PLUGINXMLFILE%"), Instance()._pluginPaths["PluginLexerConfigFilePath"] });
    replaceStrings.insert({ TEXT("%AUTOCOMPLETEFILE%"), Instance()._pluginPaths["PluginAutoCompleteFilePath"] });
    replaceStrings.insert({ TEXT("%AUTOCOMPLETEDIR%"), Instance()._pluginPaths["PluginAutoCompleteFilePath"].parent_path() });

    DWORD bIsAutoIndentOn = Instance().Messenger().SendNppMessage<DWORD>(NPPM_ISAUTOINDENTON);

    // Diagnostic data.
    if (Instance()._needPluginAutoIndent)
    {
        generic_string autoindent = TEXT("Use the plugin's auto-indentation. Currently set to [\\b ");
        autoindent.append(Instance().Settings().enableAutoIndentation ? TEXT("ON") : TEXT("OFF")).append(TEXT("\\b0 ]"));
        replaceStrings.insert({ TEXT("%NWSCRIPTINDENT%"), autoindent });
    }
    else
    {
        generic_string autoindent = TEXT("Automatic. Currently set to [\\b ");
        autoindent.append(bIsAutoIndentOn ? TEXT("ON") : TEXT("OFF")).append(TEXT("\\b0 ]"));
        replaceStrings.insert({ TEXT("%NWSCRIPTINDENT%"), autoindent });
    }
    replaceStrings.insert({ TEXT("%DARKTHEMESUPPORT%"), darkModeLabels[static_cast<int>(Instance()._pluginDarkThemeIs)] });

    // Add user statistics
    replaceStrings.insert({ TEXT("%COMPILEATTEMPTS%"), thousandSeparatorW(Instance().Settings().compileAttempts) });
    replaceStrings.insert({ TEXT("%COMPILESUCCESSES%"), thousandSeparatorW(Instance().Settings().compileSuccesses) });
    replaceStrings.insert({ TEXT("%COMPILESFAILED%"), thousandSeparatorW(Instance().Settings().compileFails) });
    replaceStrings.insert({ TEXT("%DISASSEMBLEDFILES%"), thousandSeparatorW(Instance().Settings().disassembledFiles) });

    replaceStrings.insert({ TEXT("%engineStructures%"), thousandSeparatorW(Instance().Settings().engineStructs) });
    replaceStrings.insert({ TEXT("%engineFunctionCount%"), thousandSeparatorW(Instance().Settings().engineFunctionCount) });
    replaceStrings.insert({ TEXT("%engineConstants%"), thousandSeparatorW(Instance().Settings().engineConstants) });
    replaceStrings.insert({ TEXT("%userFunctionCount%"), thousandSeparatorW(Instance().Settings().userFunctionCount) });
    replaceStrings.insert({ TEXT("%userConstants%"), thousandSeparatorW(Instance().Settings().userConstants) });

    // Set replace strings
    Instance()._aboutDialog->setReplaceStrings(replaceStrings);
    Instance()._aboutDialog->reloadAboutDocument();

    // Set homepath dir...
    Instance()._aboutDialog->setHomePath(PLUGIN_HOMEPATH);

    // Present it
    if (!Instance()._aboutDialog->isCreated())
        Instance()._aboutDialog->init(Instance().DllHModule(), Instance().NotepadHwnd());

    if (!Instance()._aboutDialog->isVisible())
        Instance()._aboutDialog->doDialog();
}

#pragma endregion Plugin User Interfacing and Menu Commands
