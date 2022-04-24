/** @file Common.h
 * Definitions for common types.
 * 
 * Remarks: MUST include shlwapi.lib reference for Linker dependencies
 * in order to be able to build projects using this file.
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <string>
#include <tchar.h>
#include <Windows.h>
#include <shellapi.h>

#include "tinyxml2.h"

#ifndef GENERIC_STRING
#define GENERIC_STRING
typedef std::basic_string<TCHAR> generic_string;
#endif
#ifndef GENERIC_STRINGSTREAM
#define GENERIC_STRINGSTREAM
typedef std::basic_stringstream<TCHAR> generic_stringstream;
#endif
#ifndef GENERIC_FILESTREAM
#define GENERIC_FILESTREAM
typedef std::basic_ofstream<TCHAR> tofstream;
typedef std::basic_ifstream<TCHAR> tifstream;
#endif

#ifdef  UNICODE
#define to_tstring std::to_wstring;
#ifndef UNICODE_TSTAT
#define UNICODE_TSTAT
#define _tstat _wstat
struct tstat : _stat64i32 {};
#endif
#else
#define to_tstring std::to_string;
#ifndef UNICODE_TSTAT
#define _tstat stat
struct tstat : stat {};
#endif
#endif

// Constants for NWScript file names
const std::string textScriptSuffix = ".nss";
const std::string compiledScriptSuffix = ".ncs";
const std::string disassembledScriptSuffix = ".ncs.pcode";
const std::string dependencyFileSuffix = ".d";
const std::string debugSymbolsFileSuffix = ".ndb";

// Current Windows official sizes for icons
// https://docs.microsoft.com/en-us/windows/win32/uxguide/vis-icons
enum class IconSize {
    Size8x8 = 8, Size10x10 = 10, Size14x14 = 14, Size16x16 = 16, Size20x20 = 20, Size22x22 = 22, Size24x24 = 24,
    Size32x32 = 32, Size40x40 = 40, Size48x48 = 48, Size64x64 = 64, Size96x96 = 96, Size128x128 = 128, Size160x160 = 160,
    Size192x192 = 192, Size224x224 = 224, Size256x256 = 256
};

enum class PathWritePermission {
    FileIsReadOnly = 1, RequiresAdminPrivileges, BlockedByApplication, UndeterminedError, WriteAllowed
};

typedef struct tagV5BMPINFO {
    BITMAPV5HEADER bmiHeader;
    DWORD        bmiColors[3];
} V5BMPINFO;


// Some common namespace uses across modules.
namespace NWScriptPlugin {
    enum class RestartMode {
        None, Normal, Admin
    };

    enum class RestartFunctionHook {
        None, ResetUserTokensPhase1, ResetEditorColorsPhase1, InstallDarkModePhase1, RepairOverrideMapPhase1, InstallAdditionalFilesPhase1
    };
}

namespace NWScriptPluginCommons {

    // Formats numeric type I into hexadecimal format
    // Extracted from here: https://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
    template <typename I>
    std::string n2hexstr(I w, size_t hex_len = sizeof(I) << 1);

    // Formats numeric type I into hexadecimal format (TCHAR version)
    // Extracted from here: https://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
    template <typename I>
    generic_string n2hexstrg(I w, size_t hex_len = sizeof(I) << 1);

    // Create a thousand separator string
    // Extracted from: https://stackoverflow.com/questions/43482488/how-to-format-a-number-with-thousands-separator-in-c-c
    std::string thousandSeparator(int number);

    // Create a thousand separator string
    // Extracted from: https://stackoverflow.com/questions/43482488/how-to-format-a-number-with-thousands-separator-in-c-c
    generic_string thousandSeparatorW(int number);

    // Since codecvt is now deprecated API and no replacement is provided, we write our own.
    std::wstring str2wstr(const std::string& string);

    // Since codecvt is now deprecated API and no replacement is provided, we write our own.
    std::string wstr2str(const std::wstring& wide_string);

    // Returns a new string replacing all input string %VARIABLES% variables with the associated string map(%VARIABLE%, %VALUE%)
    std::string replaceStringsA(const std::string& input, std::map<std::string, std::string>& replaceStrings);

    // Returns a new string replacing all input string %VARIABLES% variables with the associated string map(%VARIABLE%, %VALUE%)
    std::wstring replaceStringsW(const std::wstring& input, std::map<std::wstring, std::wstring>& replaceStrings);

    // Opens a file dialog
    bool openFileDialog(HWND hOwnerWnd, std::vector<generic_string>& outSelectedFiles, 
        const TCHAR* sFilters = TEXT("All files (*.*)\0*.*"), 
        const generic_string& lastOpenedFolder = TEXT(""), bool multiFile = false);

    // Opens a folder selection dialog
    bool openFolderDialog(HWND hOwnerWnd, generic_string& outFolderName, const generic_string& startPath, 
        UINT flags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_NONEWFOLDERBUTTON);

    // Converts a HICON to HBITMAP, preserving transparency channels
    // Extracted from example here:
    // https://cpp.hotexamples.com/pt/examples/-/-/DrawIconEx/cpp-drawiconex-function-examples.html
    HBITMAP iconToBitmap(HICON hIcon);

    // Converts a HBITMAP to HICON.
    HICON bitmapToIcon(HBITMAP hBitmap);

    // Retrieves an HICON from the standard Windows libraries
    HICON getStockIcon(SHSTOCKICONID stockIconID, IconSize iconSize);

    // Retrieves an HICON from the standard Windows libraries and convert it to a Device Independent Bitmap
    HBITMAP getStockIconBitmap(SHSTOCKICONID stockIconID, IconSize iconSize);

    // Load a PNG file from resources. Width and Height are final sizes. 0 means Original size.
    HBITMAP loadPNGFromResource(HMODULE module, int idResource, UINT width = 0, UINT height = 0);

    // Load a PNG from resources and convert into an HICON.
    HICON loadPNGFromResourceIcon(HMODULE module, int idResource, UINT width = 0, UINT height = 0);

    // Load an SVG file from resources and render into a HBITMAP.
    HBITMAP loadSVGFromResource(HMODULE module, int idResource, bool invertLuminosity = false, UINT width = 0, UINT height = 0);

    // Load SVG file from resources and render into a HICON
    HICON loadSVGFromResourceIcon(HMODULE module, int idResource, bool invertLuminosity = false, UINT width = 0, UINT height = 0);

    // Creates an image mask for current HBITMAP
    HBITMAP createImageMask(HBITMAP bitmapHandle, const COLORREF transparencyColor);

    // Creates a bitmap mask for current HICON
    HICON createIconMask(HICON source);

    // Get one of the system or user folders by it's GUID.
    generic_string getSystemFolder(GUID folderID);

    // Converts a File Link into an actual filename
    void resolveLinkFile(generic_string& linkFilePath);

    // Checks if directory exist
    bool isValidDirectory(const TCHAR* sPath);

    // Checks if directory exist
    bool isValidDirectoryS(const generic_string& sPath);

    // Returns the proper non back-slash ended directory name (eg: C:\Windows\ -> C:\Windows)
    // Needed to canonicalize directory names, since then functions of NscLib will return names with or
    // without ending backslashes.
    std::string properDirNameA(const std::string& dirName);

    // Returns the proper non back-slash ended directory name (eg: C:\Windows\ -> C:\Windows)
    // Needed to canonicalize directory names, since then functions of NscLib will return names with or
    // without ending backslashes.
    std::wstring properDirNameW(const std::wstring& dirName);

    // Try to retrieve the Neverwinter's Home path
    generic_string getNwnHomePath(int CompilerVersion);

    // Loads a raw file into a string buffer
    bool fileToBuffer(const generic_string& filePath, std::string& sContents);

    // Saves a string buffer into a raw file 
    bool bufferToFile(const generic_string& filePath, const std::string& sContents);

    // Checks for a path's write permission.
    // returns false if not successful (eg: file/path doesn't exist), else returns true and fills outFilePermission enums
    bool checkWritePermission(const generic_string& sPath, PathWritePermission& outPathPermission);

    // Writes a pseudo-batch file to store Notepad++ executable to be called by ShellExecute
    // with a delay (So it can restart seeamlessly).
    bool writePseudoBatchExecute(const generic_string& path, const generic_string& executePath);

    // ShellExecutes a given path with elevated privileges or not.
    void runProcess(bool isElevationRequired, const generic_string& path, const generic_string& args = TEXT(""),
        const generic_string& opendir = TEXT(""));

    // Returns a temporary filename string
    bool getTemporaryFile(const generic_string& path, generic_string& outputPath);

    // Recursively navigate on XML nodes and get rid of all comment tags.
    // Bonus: also deletes the declaration headers now, since we need to rebuild it.
    void stripXMLInfo(tinyxml2::XMLNode* node);

    // Recursively navigate the XML structure until a determined first occurrence of an element is found.
    // (optional) Also checks if element has attribute checkAttribute with corresponding checkAttributeValue.
    // Returns NULL on a failed search.
    tinyxml2::XMLElement* searchElement(tinyxml2::XMLElement* const from, const std::string& toName,
        const std::string checkAttribute = "", const std::string checkAttributeValue = "");

    // Finds the Menu Handle of a submenu with name subMenuName
    HMENU FindSubMenu(HMENU baseMenu, generic_string subMenuName);

    // Returns the menu name for a given position inside a menu handle
    generic_string GetMenuItemName(HMENU menu, int position);

    // Creates a file list from a starting directory with recurse options
    // Sampled from: 
    // https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
    // This version have a cancel flag for assynchronous interruptions
    void createFilesList(std::vector<fs::path>& filesList, const generic_string& startingDir, 
        const generic_string& fileMask, bool recurse, std::atomic<bool>& cancelFlagVariable);

    // Folder browsing function callback
    // For changing default selected directory
    // Extracted from: 
    // https://stackoverflow.com/questions/6942150/why-folderbrowserdialog-dialog-does-not-scroll-to-selected-folder
    int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

    // Folder browsing child enumeration function callback
    BOOL CALLBACK EnumCallback(HWND hWndChild, LPARAM lParam);

}


using namespace NWScriptPluginCommons;

