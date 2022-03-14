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

// Current Windows official sizes for icons
// https://docs.microsoft.com/en-us/windows/win32/uxguide/vis-icons
enum IconSize {
    Size8x8 = 8, Size10x10 = 10, Size14x14 = 14, Size16x16 = 16, Size20x20 = 20, Size22x22 = 22, Size24x24 = 24,
    Size32x32 = 32, Size40x40 = 40, Size48x48 = 48, Size64x64 = 64, Size96x96 = 96, Size128x128 = 128, Size256x256 = 256
};

enum class PathWritePermission {
    FileIsReadOnly = 1, RequiresAdminPrivileges, BlockedByApplication, UndeterminedError, WriteAllowed
};


// Some common namespace uses across modules.
namespace NWScriptPlugin {
    enum class RestartMode {
        None, Normal, Admin
    };

    enum class RestartFunctionHook {
        None, ResetEditorColorsPhase1, InstallDarkModePhase1
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

    // Since codecvt is now deprecated API and no replacement is provided, we write our own.
    std::wstring str2wstr(const std::string& string);

    // Since codecvt is now deprecated API and no replacement is provided, we write our own.
    std::string wstr2str(const std::wstring& wide_string);

    // Opens a file dialog
    bool OpenFileDialog(HWND hOwnerWnd, const TCHAR* sFilters, generic_string& outFileName);

    // Opens a folder selection dialog
    bool OpenFolderDialog(HWND hOwnerWnd, generic_string& outFolderName);

    // Retrieves an HICON from the standard Windows libraries
    HICON GetStockIcon(SHSTOCKICONID stockIconID, IconSize iconSize);

    // Converts a HICON to HBITMAP, preserving transparency channels
    // Extracted from example here:
    // https://cpp.hotexamples.com/pt/examples/-/-/DrawIconEx/cpp-drawiconex-function-examples.html
    HBITMAP IconToBitmap(HICON hIcon);

    // Retrieves an HICON from the standard Windows libraries and convert it to a Device Independent Bitmap
    HBITMAP GetStockIconBitmap(SHSTOCKICONID stockIconID, IconSize iconSize);

    generic_string GetSystemFolder(GUID folderID);

    bool IsValidDirectory(const TCHAR* sPath);

    bool IsValidDirectoryS(const generic_string& sPath);

    generic_string GetNwnHomePath(int CompilerVersion);

    // Checks for a path's write permission.
    // returns false if not successful (eg: file/path doesn't exist), else returns true and fills outFilePermission enums
    bool CheckWritePermission(const generic_string& sPath, PathWritePermission& outPathPermission);

    void runProcess(bool isElevationRequired, const generic_string& path, const generic_string& args = TEXT(""),
        const generic_string& opendir = TEXT(""));

    // Writes a pseudo-batch file to store Notepad++ executable to be called by ShellExecute
    // with a delay (So it can restart seeamlessly).
    bool writePseudoBatchExecute(const generic_string& path, const generic_string& executePath);

    // Recursively navigate on XML nodes and get rid of all comment tags.
    // Bonus: also deletes the declaration headers now, since we need to rebuild it.
    void StripXMLInfo(tinyxml2::XMLNode* node);

    // Recursively navigate the XML structure until a determined first occurrence of an element is found.
    // (optional) Also checks if element has attribute checkAttribute with corresponding checkAttributeValue.
    // Returns NULL on a failed search.
    tinyxml2::XMLElement* SearchElement(tinyxml2::XMLElement* const from, const std::string& toName,
        const std::string checkAttribute = "", const std::string checkAttributeValue = "");

}

using namespace NWScriptPluginCommons;

