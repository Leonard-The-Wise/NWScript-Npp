// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////
// Modified to support auto-position and resizing of windows and controls
// Fixed bug in scaleRect.
//////////////////////////////////////////////////////////////////////////


#pragma once

class DPIManager
{
public:
    DPIManager() {
        init();
    };

    // Get screen DPI.
    int getDPIX() { return _dpiX; };
    int getDPIY() { return _dpiY; };
    int getDPIScalePercent() { return (int)(((float)_dpiX / (float)96.0f)*100.0f); };

    // Convert between raw pixels and relative pixels.
    int scaleX(int x) { return MulDiv(x, _dpiX, 96); };
    int scaleY(int y) { return MulDiv(y, _dpiY, 96); };
    int unscaleX(int x) { return MulDiv(x, 96, _dpiX); };
    int unscaleY(int y) { return MulDiv(y, 96, _dpiY); };
    // Scale by percent factor. Percent factor must be 100 = 1; 125 = 1.25; etc.
    int scaleByPercent(int number, int percentFactor) { return (int)((float)number * (float)percentFactor / 100.0f); }

    // Determine the screen dimensions in relative pixels.
    int scaledScreenWidth() { return scaledSystemMetricX(SM_CXSCREEN); }
    int scaledScreenHeight() { return scaledSystemMetricY(SM_CYSCREEN); }

    // Scale rectangle from raw pixels to relative pixels.
    void scaleRect(__inout RECT* pRect) 
    {
        LONG width, height;
        width = scaleX(pRect->right - pRect->left);
        height = scaleY(pRect->bottom - pRect->top);
        pRect->left = scaleX(pRect->left);
        pRect->top = scaleY(pRect->top);
        pRect->right = pRect->left + width;
        pRect->bottom = pRect->top + height;
    }

    // Scale Point from raw pixels to relative pixels.
    void scalePoint(__inout POINT* pPoint)
    {
        pPoint->x = scaleX(pPoint->x);
        pPoint->y = scaleY(pPoint->y);
    }

    // Scale Size from raw pixels to relative pixels.
    void scaleSize(__inout SIZE* pSize)
    {
        pSize->cx = scaleX(pSize->cx);
        pSize->cy = scaleY(pSize->cy);
    }

    // Determine if screen resolution meets minimum requirements in relative pixels.
    bool isResolutionAtLeast(int cxMin, int cyMin)
    {
        return (scaledScreenWidth() >= cxMin) && (scaledScreenHeight() >= cyMin);
    }

    // Convert a point size (1/72 of an inch) to raw pixels.
    int pointsToPixels(int pt) { return MulDiv(pt, _dpiY, 72); };

    // Invalidate any cached metrics.
    void Invalidate(HWND hwnd = NULL) { init(hwnd); };

    // Resize a control (or any window) based on current DPI settings
    void resizeControl(HWND hWndControl, bool resizeFont = false)
    {
        RECT windowRect;
        GetWindowRect(hWndControl, &windowRect);
        HWND parent = GetParent(hWndControl);
        screenToClientEx(parent, &windowRect);
        scaleRect(&windowRect);

        if (resizeFont)
            scaleFont(hWndControl);

        MoveWindow(hWndControl, windowRect.left, windowRect.top, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, FALSE);

    }

    // Iterates through a window and resize all children inside
    void resizeChildren(HWND hWndParent, bool resizeFont = false)
    {
        _bResizeFontAlso = resizeFont;
        std::ignore = EnumChildWindows(hWndParent, ResizeChildWindow, reinterpret_cast<LPARAM>(this));
    }

    // Scales control font according to DPI
    void scaleFont(HWND windowOrControl)
    {
        HFONT controlFont = (HFONT)SendMessage(windowOrControl, WM_GETFONT, 0, 0);
        LOGFONT fontAttributes = { 0 };
        ::GetObject(controlFont, sizeof(fontAttributes), &fontAttributes);
        fontAttributes.lfWidth = scaleX(fontAttributes.lfWidth);
        fontAttributes.lfHeight = scaleX(fontAttributes.lfHeight);
        SendMessage(windowOrControl, WM_SETFONT, (WPARAM)CreateFontIndirect(&fontAttributes), 0);
    }

    // Scale number to a fixed native resolution that icons can support (from 16x16 in multiples of 0.25 up to 256x256)
    UINT scaleIconSize(UINT size)
    {
        int DPIScalePercent = getDPIScalePercent();
        UINT result = (UINT)((float)size * (1.0f + ((float)(((DPIScalePercent - 100) / 25) * 25) / (float)100.0f)));
        // Do a round up or down to numbers instead of just truncating.
        lldiv_t d = div((LONGLONG)result, (LONGLONG)32);
        UINT normalResult = d.rem > 15 ? ((result / 32) + 1) * 32 : (result / 32) * 32;

        // Only supported icon sizes not multiple from 32 are these. Else return only multiples of 32, up to 256.
        if (result == 8 || result == 10 || result == 14 || result == 16 || result == 20 || result == 22 || result == 24 || 
            result == 40 || result == 48)
            return result;
        else
            return normalResult > 256 ? 256 : normalResult;
    }

    // Returns index of DPI scale (based on Microsoft's styles standards)
    BYTE currentDpiIndex() 
    {
        if (_dpiX >= 96 and _dpiX < 120)
            return 0;
        if (_dpiX >= 120 and _dpiX < 144)
            return 1;
        if (_dpiX >= 144 and _dpiX < 192)
            return 2;
        if (_dpiX >= 192 and _dpiX < 240)
            return 3;
        if (_dpiX >= 240 and _dpiX < 288)
            return 4;
        if (_dpiX >= 288 and _dpiX < 384)
            return 5;

        return 6;
    }

    bool screenToClientEx(HWND hWnd, RECT* pRect)
    {
        POINT   pt1 = {};
        POINT   pt2 = {};

        pt1.x = pRect->left;
        pt1.y = pRect->top;
        pt2.x = pRect->right;
        pt2.y = pRect->bottom;
        if (::ScreenToClient(hWnd, &pt1) == false) return(false);
        if (::ScreenToClient(hWnd, &pt2) == false) return(false);
        pRect->left = pt1.x;
        pRect->top = pt1.y;
        pRect->right = pt2.x;
        pRect->bottom = pt2.y;

        return(true);
    }


private:
    // X and Y DPI values are provided, though to date all 
    // Windows OS releases have equal X and Y scale values
    int _dpiX = 0;
    int _dpiY = 0;
    bool _bResizeFontAlso = false;

    void init(HWND hwnd = NULL) 
    {
        HDC hdc = GetDC(hwnd);
        if (hdc)
        {
            // Initialize the DPIManager member variable
            // This will correspond to the DPI setting
            // With all Windows OS's to date the X and Y DPI will be identical					
            _dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
            _dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(hwnd, hdc);
        }
    };

    // This returns a 96-DPI scaled-down equivalent value for nIndex 
    // For example, the value 120 at 120 DPI setting gets scaled down to 96		
    // X and Y versions are provided, though to date all Windows OS releases 
    // have equal X and Y scale values
    int scaledSystemMetricX(int nIndex) {
        return MulDiv(GetSystemMetrics(nIndex), 96, _dpiX);
    };

    // This returns a 96-DPI scaled-down equivalent value for nIndex 
    // For example, the value 120 at 120 DPI setting gets scaled down to 96		
    // X and Y versions are provided, though to date all Windows OS releases 
    // have equal X and Y scale values
    int scaledSystemMetricY(int nIndex)
    {
        return MulDiv(GetSystemMetrics(nIndex), 96, _dpiY);
    }

    static BOOL CALLBACK ResizeChildWindow(HWND hWnd, LPARAM lParam)
    {
        DPIManager* thisPointer = reinterpret_cast<DPIManager*>(lParam);
        thisPointer->resizeControl(hWnd, thisPointer->_bResizeFontAlso);
        return TRUE;
    }


};

