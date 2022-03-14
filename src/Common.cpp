/** @file Common.h
 * Definitions for common types.
 *
 * Remarks: MUST include shlwapi.lib reference for Linker dependencies
 * in order to be able to build projects using this file.
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <assert.h>
//#include <windows.h>
//#include <commdlg.h>
//#include <shellapi.h>
//#include <ShlObj.h>
//#include <Shlwapi.h>
//#include <stdexcept>
//#include <errhandlingapi.h>
//#include <sstream>
//#include <tchar.h>
//#include <fstream>
//#include "tinyxml2.h"

#include "Common.h"


namespace NWScriptPluginCommons {

    // Formats numeric type I into hexadecimal format
    // Extracted from here: https://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
    template <typename I>
    std::string n2hexstr(I w, size_t hex_len) {
        static const char* digits = "0123456789ABCDEF";
        std::string rc(hex_len, '0');
        for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
            rc[i] = digits[(w >> j) & 0x0f];
        return "0x" + rc;
    }

    // Formats numeric type I into hexadecimal format (TCHAR version)
    // Extracted from here: https://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
    template <typename I>
    generic_string n2hexstrg(I w, size_t hex_len) {
        static const TCHAR* digits = TEXT("0123456789ABCDEF");
        generic_string rc(hex_len, TEXT('0'));
        for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
            rc[i] = digits[(w >> j) & 0x0f];
        return TEXT("0x") + rc;
    }

    // Since codecvt is now deprecated API and no replacement is provided, we write our own.
    std::wstring str2wstr(const std::string& string)
    {
        if (string.empty())
        {
            return L"";
        }

        const auto size_needed = MultiByteToWideChar(CP_UTF8, 0, &string.at(0), (int)string.size(), nullptr, 0);
        if (size_needed < 1)
        {
            throw std::runtime_error("MultiByteToWideChar() failed: " + std::to_string(size_needed));
        }

        std::wstring result(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &string.at(0), (int)string.size(), &result.at(0), size_needed);
        return result;
    }

    // Since codecvt is now deprecated API and no replacement is provided, we write our own.
    std::string wstr2str(const std::wstring& wide_string)
    {
        if (wide_string.empty())
        {
            return "";
        }

        const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), nullptr, 0, nullptr, nullptr);
        if (size_needed < 1)
        {
            throw std::runtime_error("WideCharToMultiByte() failed: " + std::to_string(size_needed));
        }

        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), &result.at(0), size_needed, nullptr, nullptr);
        return result;
    }

    // Opens a file dialog
    bool OpenFileDialog(HWND hOwnerWnd, const TCHAR* sFilters, generic_string& outFileName)
    {

        OPENFILENAME ofn;

        TCHAR sFileOpen[MAX_PATH];
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hOwnerWnd;
        ofn.lpstrFile = sFileOpen;
        ofn.lpstrFile[0] = L'\0';
        ofn.nMaxFile = MAX_PATH * sizeof(TCHAR);
        ofn.lpstrFilter = sFilters;
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (!GetOpenFileName(&ofn))
        {
            DWORD nFail = CommDlgExtendedError();
            if (nFail > 0)
            {
                generic_stringstream sError;
                sError << TEXT("Error opening dialog box. Please report it to the plugin creator!\r\n") <<
                    TEXT("Error code: ") << n2hexstrg(nFail, 4).c_str();
                MessageBox(hOwnerWnd, sError.str().c_str(), TEXT("NWScript Plugin Error"), MB_OK | MB_ICONERROR | MB_APPLMODAL);
            }

            return false;
        }

        outFileName.append(sFileOpen);

        return true;
    }

    // Opens a folder selection dialog
    bool OpenFolderDialog(HWND hOwnerWnd, generic_string& outFolderName)
    {
        BROWSEINFO b = { 0 };
        b.hwndOwner = hOwnerWnd;
        b.lpszTitle = TEXT("Select Folder");

        LPITEMIDLIST pidl = SHBrowseForFolder(&b);
        TCHAR tszPath[MAX_PATH] = TEXT("");

        if (SHGetPathFromIDList(pidl, tszPath) == TRUE)
        {
            outFolderName = tszPath;
            return true;
        }

        return false;
    }

    // No need for warning of enum here
#pragma warning(push)
#pragma warning(disable : 26812)

    // Retrieves an HICON from the standard Windows libraries
    HICON GetStockIcon(SHSTOCKICONID stockIconID, IconSize iconSize)
    {
        UINT uiFlags = SHGSI_ICONLOCATION;

        SHSTOCKICONINFO sii;
        ZeroMemory(&sii, sizeof(sii));
        sii.cbSize = sizeof(sii);
        HRESULT hResult = SHGetStockIconInfo(stockIconID, uiFlags, &sii);

        if (!SUCCEEDED(hResult))
            return NULL;

        HMODULE hModule = LoadLibrary(sii.szPath);
        HICON hIcon = NULL;

        if (hModule)
        {
            hIcon = reinterpret_cast<HICON>(LoadImage(hModule, MAKEINTRESOURCE(-sii.iIcon),
                IMAGE_ICON, iconSize, iconSize, LR_DEFAULTCOLOR));
            FreeLibrary(hModule);
        }

        return hIcon;
    }

    // Converts a HICON to HBITMAP, preserving transparency channels
    // Extracted from example here:
    // https://cpp.hotexamples.com/pt/examples/-/-/DrawIconEx/cpp-drawiconex-function-examples.html
    HBITMAP IconToBitmap(HICON hIcon)
    {
        if (!hIcon)
            return NULL;

        ICONINFO iconInfo;
        ZeroMemory(&iconInfo, sizeof(iconInfo));
        GetIconInfo(hIcon, &iconInfo);

        HDC screenDevice = GetDC(NULL);
        HDC hdc = CreateCompatibleDC(screenDevice);
        ReleaseDC(NULL, screenDevice);

        int iconWidth = iconInfo.xHotspot * 2;
        int iconHeight = iconInfo.yHotspot * 2;

        BITMAPINFOHEADER bitmapInfoH = { sizeof(BITMAPINFOHEADER), iconWidth, iconHeight, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
        BITMAPINFO bitmapInfo = { bitmapInfoH, {} };
        HBITMAP winBitmap = nullptr;
        HGDIOBJ oldHdc = nullptr;
        DWORD* bits = nullptr;

        winBitmap = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, (VOID**)&bits, NULL, 0);
        if (winBitmap)
        {
            // Switch hdc object to point into the bitmap.
            oldHdc = SelectObject(hdc, reinterpret_cast<HGDIOBJ>(winBitmap));
            if (oldHdc)
                DrawIconEx(hdc, 0, 0, hIcon, iconWidth, iconHeight, 0, 0, DI_NORMAL);
        }

        DeleteObject(iconInfo.hbmMask);
        DeleteObject(iconInfo.hbmColor);

        // Restore previous object (state) for HDC, like recommended here: 
        // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectobject
        if (oldHdc)
            SelectObject(hdc, oldHdc);

        DeleteDC(hdc);

        return winBitmap;
    }

    // Retrieves an HICON from the standard Windows libraries and convert it to a Device Independent Bitmap
    HBITMAP GetStockIconBitmap(SHSTOCKICONID stockIconID, IconSize iconSize) {
        return IconToBitmap(GetStockIcon(stockIconID, iconSize));
    }

    generic_string GetSystemFolder(GUID folderID)
    {

        PWSTR path;
        SHGetKnownFolderPath(folderID, KF_FLAG_DEFAULT, NULL, &path);

        generic_string retVal;
        retVal = path;
        return retVal;
    }

    bool IsValidDirectory(const TCHAR* sPath)
    {
        WIN32_FILE_ATTRIBUTE_DATA attributes = {};

        if (!PathFileExists(sPath))
            return false;

        if (!GetFileAttributesEx(sPath, GetFileExInfoStandard, &attributes))
            return false;

        if (!(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            return false;

        return true;
    }

    bool IsValidDirectoryS(const generic_string& sPath)
    {
        return IsValidDirectory(sPath.c_str());
    }

    generic_string GetNwnHomePath(int CompilerVersion)
    {
        generic_string HomePath;

        if (CompilerVersion >= 174)
        {
            TCHAR DocumentsPath[MAX_PATH];

            if (!SHGetSpecialFolderPath(NULL, DocumentsPath, CSIDL_PERSONAL, TRUE))
                return HomePath;

            HomePath = DocumentsPath;
            HomePath.append(TEXT("\\Neverwinter Nights\\"));
        }
        else {
            HKEY Key;
            LONG Status;

            Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\BioWare\\NWN\\Neverwinter"), REG_OPTION_RESERVED,
#ifdef _WIN64
                KEY_QUERY_VALUE | KEY_WOW64_32KEY,
#else
                KEY_QUERY_VALUE,
#endif
                & Key);

            if (Status != NO_ERROR)
                return HomePath;

            try
            {
                char NameBuffer[MAX_PATH + 1];
                DWORD NameBufferSize;
                bool FoundIt;
                static const TCHAR* ValueNames[] =
                {
                    TEXT("Path"),     // Retail NWN2
                    TEXT("Location"), // Steam NWN2
                };
                FoundIt = false;

                for (size_t i = 0; i < _countof(ValueNames); i += 1)
                {
                    NameBufferSize = sizeof(NameBuffer) - sizeof(NameBuffer[0]);
                    Status = RegQueryValueEx(Key, ValueNames[i], NULL, NULL, (LPBYTE)NameBuffer, &NameBufferSize);

                    if (Status != NO_ERROR)
                        continue;

                    if ((NameBufferSize > 0) &&
                        (NameBuffer[NameBufferSize - 1] == '\0'))
                        NameBufferSize -= 1;

                    HomePath = str2wstr(NameBuffer);
                }
            }
            catch (...)
            {
                RegCloseKey(Key);
            }
        }

        return HomePath;
    }

    // Checks for a path's write permission.
    // returns false if not successful (eg: file/path doesn't exist), else returns true and fills outFilePermission enums
    bool CheckWritePermission(const generic_string& sPath, PathWritePermission& outPathPermission)
    {
        WIN32_FILE_ATTRIBUTE_DATA attributes = {};

        if (!PathFileExists(sPath.c_str()))
            return false;

        if (!GetFileAttributesEx(sPath.c_str(), GetFileExInfoStandard, &attributes))
            return false;

        if (attributes.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            outPathPermission = PathWritePermission::FileIsReadOnly;
            return true;
        }

        // On directories we try to write a temporary file to test permissions.
        if (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            TCHAR buffer[MAX_PATH];
            UINT retval = GetTempFileName(sPath.c_str(), TEXT("nws"), 0, buffer);
            if (retval == 0)
            {
                outPathPermission = PathWritePermission::RequiresAdminPrivileges;
                return true;
            }

            DeleteFile(buffer);
            outPathPermission = PathWritePermission::WriteAllowed;
            return true;
        }

        // We are only touching a file, hence all shared attributes may apply
        // Also, to Open a file, we call a "CreateFile" function. Lol Windows API
        HANDLE f = CreateFile(sPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, attributes.dwFileAttributes, NULL);

        // The GetLastError codes are documented here:
        // https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes
        if (f == INVALID_HANDLE_VALUE)
        {
            switch (GetLastError())
            {
            case ERROR_ACCESS_DENIED:
                outPathPermission = PathWritePermission::RequiresAdminPrivileges;
                break;
            case ERROR_SHARING_VIOLATION:
                outPathPermission = PathWritePermission::BlockedByApplication;
                break;
            default:
                outPathPermission = PathWritePermission::UndeterminedError;
            }

            return true;
        }

        CloseHandle(f);
        outPathPermission = PathWritePermission::WriteAllowed;
        return true;
    }

    void runProcess(bool isElevationRequired, const generic_string& path, const generic_string& args,
        const generic_string& opendir)
    {
        const TCHAR* opVerb = isElevationRequired ? TEXT("runas") : TEXT("open");
        ::ShellExecute(NULL, opVerb, path.c_str(), args.c_str(), opendir.c_str(), SW_HIDE);
    }

    // Writes a pseudo-batch file to store Notepad++ executable to be called by ShellExecute
    // with a delay (So it can restart seeamlessly).
    bool writePseudoBatchExecute(const generic_string& path, const generic_string& executePath)
    {
        std::ofstream s;
        std::stringstream stext;

        // Use ping trick on batch to delay execution, since it's the most compatible option to add a delay
        stext << "@echo off" << std::endl;
        stext << "ping -n 1 -w 1000 10.255.255.255 > nul" << std::endl;
        stext << "\"" << wstr2str(executePath.c_str()) << "\"" << std::endl;

        s.open(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        if (!s.is_open())
        {
            return false;
        }

        s << stext.str().c_str();
        s.close();
        return true;
    }

    // Recursively navigate on XML nodes and get rid of all comment tags.
    // Bonus: also deletes the declaration headers now, since we need to rebuild it.
    void StripXMLInfo(tinyxml2::XMLNode* node)
    {
        // All XML nodes may have children and siblings. So for each valid node, first we
        // iterate on it's (possible) children, and then we proceed to clear the node itself and jump 
        // to the next sibling
        while (node)
        {
            if (node->FirstChild() != NULL)
                StripXMLInfo(node->FirstChild());

            //Check to see if current node is a comment
            auto comment = dynamic_cast<tinyxml2::XMLComment*>(node);
            if (comment)
            {
                // If it is, we ask the parent to delete this, but first move pointer to next member so we don't get lost in a NULL reference
                node = node->NextSibling();
                comment->Parent()->DeleteChild(comment);
            }
            // We also remove XML declarations here
            else
            {
                auto declare = dynamic_cast<tinyxml2::XMLDeclaration*>(node);
                if (declare)
                {
                    node = node->NextSibling();
                    declare->Parent()->DeleteChild(declare);
                }
                else
                    node = node->NextSibling();
            }
        }
    }

    // Recursively navigate the XML structure until a determined first occurrence of an element is found.
    // (optional) Also checks if element has attribute checkAttribute with corresponding checkAttributeValue.
    // Returns NULL on a failed search.
    tinyxml2::XMLElement* SearchElement(tinyxml2::XMLElement* const from, const std::string& toName,
        const std::string checkAttribute, const std::string checkAttributeValue)
    {
        tinyxml2::XMLElement* _from = from;
        tinyxml2::XMLElement* found = NULL;

        while (_from && !found)
        {
            if (_from->Name() == toName)
            {
                if (checkAttribute.empty())
                {
                    found = _from;
                    break;
                }
                else
                {
                    if (_from->Attribute(checkAttribute.c_str(), checkAttributeValue.c_str()))
                    {
                        found = _from;
                        break;
                    }
                }
            }

            if (_from->FirstChildElement() != NULL)
                found = SearchElement(_from->FirstChildElement(), toName, checkAttribute, checkAttributeValue);

            if (!found)
                _from = _from->NextSiblingElement();
        }

        return found;

    };
#pragma warning(pop)

}
