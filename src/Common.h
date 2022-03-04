/** @file Common.h
 * Definitions for common types.
 * 
 * Remarks: MUST include shlwapi.lib reference for Linker dependencies
 * in order to be able to build projects using this file.
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <assert.h>
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <stdexcept>
#include <errhandlingapi.h>
#include <sstream>
#include <tchar.h>
#include <fstream>



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


namespace {

    // Formats numeric type I into hexadecimal format
    // Extracted from here: https://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
    template <typename I> 
    std::string n2hexstr(I w, size_t hex_len = sizeof(I) << 1) {
        static const char* digits = "0123456789ABCDEF";
        std::string rc(hex_len, '0');
        for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
            rc[i] = digits[(w >> j) & 0x0f];
        return "0x" + rc;
    }

    // Formats numeric type I into hexadecimal format (TCHAR version)
    // Extracted from here: https://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
    template <typename I>
    generic_string n2hexstrg(I w, size_t hex_len = sizeof(I) << 1) {
        static const TCHAR* digits = TEXT("0123456789ABCDEF");
        generic_string rc(hex_len, TEXT('0'));
        for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
            rc[i] = digits[(w >> j) & 0x0f];
        return TEXT("0x") + rc;
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

// No need for warning of enum here
#pragma warning(push)
#pragma warning(disable : 26812)

    // Current Windows official sizes for icons
    // https://docs.microsoft.com/en-us/windows/win32/uxguide/vis-icons
    enum IconSize {
        Size8x8 = 8, Size10x10 = 10, Size14x14 = 14, Size16x16 = 16, Size20x20 = 20, Size22x22 = 22, Size24x24 = 24, 
        Size32x32 = 32, Size40x40 = 40, Size48x48 = 48, Size64x64 = 64, Size96x96 = 96, Size128x128 = 128, Size256x256 = 256
    };

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
    inline HBITMAP GetStockIconBitmap(SHSTOCKICONID stockIconID, IconSize iconSize) {
        return IconToBitmap(GetStockIcon(stockIconID, iconSize));
    }

    enum class FileWritePermission {
        FileIsReadOnly = 1, RequiresAdminPrivileges, BlockedByApplication, UndeterminedError, WriteAllowed
    };

    // Checks for a file's write permission.
    // returns false if not successful (eg: file doesn't exist), else returns true and fills outFilePermission enums
    bool CheckFileWritePermission(const generic_string& sFilePath, FileWritePermission& outFilePermission)
    {
        WIN32_FILE_ATTRIBUTE_DATA attributes = {};
        
        if (!PathFileExists(sFilePath.c_str()))
            return false;

        if (!GetFileAttributesEx(sFilePath.c_str(), GetFileExInfoStandard, &attributes))
            return false;

        if (attributes.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            outFilePermission = FileWritePermission::FileIsReadOnly;
            return true;
        }

        // We are only touching a file, hence all shared attributes may apply
        // Also, to Open a file, we call a "CreateFile" function. Lol Windows API
        HANDLE f = CreateFile(sFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, attributes.dwFileAttributes, NULL);

        // The GetLastError codes are documented here:
        // https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes
        if (f == INVALID_HANDLE_VALUE)
        {
            switch (GetLastError())
            {
            case ERROR_ACCESS_DENIED:
                outFilePermission = FileWritePermission::RequiresAdminPrivileges;
                break;
            case ERROR_SHARING_VIOLATION:
                outFilePermission = FileWritePermission::BlockedByApplication;
                break;
            default:
                outFilePermission = FileWritePermission::UndeterminedError;
            }

            return true;
        }

        CloseHandle(f);
        outFilePermission = FileWritePermission::WriteAllowed;
        return true;
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

    void runProcess(bool isElevationRequired, const generic_string& path, const generic_string& args = TEXT(""),
        const generic_string& opendir = TEXT(""))
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
        stext << "ping -n 1 -w 500 10.255.255.255 > nul" << std::endl;
        stext << "\"" << wstr2str(executePath.c_str()) << "\"" << std::endl;

        s.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!s.is_open())
            return false;

        s  << stext.str().c_str();
        s.close();
        return true;
    }


#pragma warning(pop)

}

