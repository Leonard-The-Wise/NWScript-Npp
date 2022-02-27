/** @file Common.h
 * Definitions for common types
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <stdexcept>

#include <sstream>



namespace {

    typedef std::basic_string<TCHAR> generic_string;
    typedef std::basic_stringstream<TCHAR> generic_stringstream;

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

        outFileName.reserve(MAX_PATH);

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hOwnerWnd;
        ofn.lpstrFile = outFileName.data();
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
                    MessageBox(hOwnerWnd, sError.str().c_str(), TEXT("NWScript Plugin Error"), MB_OK | MB_ICONERROR);
            }

            return false;
        }

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

    //Retrieves an HICON from the standard Windows libraries
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

    inline HBITMAP GetStockIconBitmap(SHSTOCKICONID stockIconID, IconSize iconSize) {
        return IconToBitmap(GetStockIcon(stockIconID, iconSize));
    }

#pragma warning(pop)

}

