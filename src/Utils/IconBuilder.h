/************************************** 
* @file IconBuilder.h
* Saves a HICON to .ico file
*
* Extracted from: https://stackoverflow.com/questions/2289894/how-can-i-save-hicon-to-an-ico-file
*
**/
// Copyright (C) 2022 - Leonardo Silva 
// The License.txt file describes the conditions under which this software may be distributed.


#pragma once

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

//
// ICONS (.ICO type 1) are structured like this:
//
// ICONHEADER (just 1)
// ICONDIR [1...n] (an array, 1 for each image)
// [BITMAPINFOHEADER+COLOR_BITS+MASK_BITS] [1...n] (1 after the other, for each image)
//
// CURSORS (.ICO type 2) are identical in structure, but use
// two monochrome bitmaps (real XOR and AND masks, this time).
//

class IconBuilder {

private:

    typedef struct
    {
        WORD idReserved; // must be 0
        WORD idType; // 1 = ICON, 2 = CURSOR
        WORD idCount; // number of images (and ICONDIRs)

        // ICONDIR [1...n]
        // ICONIMAGE [1...n]

    } ICONHEADER;

    //
    // An array of ICONDIRs immediately follow the ICONHEADER
    //
    typedef struct
    {
        BYTE bWidth;
        BYTE bHeight;
        BYTE bColorCount;
        BYTE bReserved;
        WORD wPlanes; // for cursors, this field = wXHotSpot
        WORD wBitCount; // for cursors, this field = wYHotSpot
        DWORD dwBytesInRes;
        DWORD dwImageOffset; // file-offset to the start of ICONIMAGE

    } ICONDIR;

    //
    // After the ICONDIRs follow the ICONIMAGE structures -
    // consisting of a BITMAPINFOHEADER, (optional) RGBQUAD array, then
    // the color and mask bitmap bits (all packed together
    //
    typedef struct
    {
        BITMAPINFOHEADER biHeader; // header for color bitmap (no mask header)
        //RGBQUAD rgbColors[1...n];
        //BYTE bXOR[1]; // DIB bits for color bitmap
        //BYTE bAND[1]; // DIB bits for mask bitmap

    } ICONIMAGE;

    //
    // Write the ICO header to disk
    //
    static UINT WriteIconHeader(HANDLE hFile, int nImages)
    {
        ICONHEADER iconheader;
        DWORD nWritten;

        // Setup the icon header
        iconheader.idReserved = 0; // Must be 0
        iconheader.idType = 1; // Type 1 = ICON (type 2 = CURSOR)
        iconheader.idCount = nImages; // number of ICONDIRs

        // Write the header to disk
        WriteFile(hFile, &iconheader, sizeof(iconheader), &nWritten, 0);

        // following ICONHEADER is a series of ICONDIR structures (idCount of them, in fact)
        return nWritten;
    }

    //
    // Return the number of BYTES the bitmap will take ON DISK
    //
    static UINT NumBitmapBytes(BITMAP* pBitmap)
    {
        int nWidthBytes = pBitmap->bmWidthBytes;

        // bitmap scanlines MUST be a multiple of 4 bytes when stored
        // inside a bitmap resource, so round up if necessary
        if (nWidthBytes & 3)
            nWidthBytes = (nWidthBytes + 4) & ~3;

        return nWidthBytes * pBitmap->bmHeight;
    }

    //
    // Return number of bytes written
    //
    static UINT WriteIconImageHeader(HANDLE hFile, BITMAP* pbmpColor, BITMAP* pbmpMask)
    {
        BITMAPINFOHEADER biHeader;
        DWORD nWritten;
        UINT nImageBytes;

        // calculate how much space the COLOR and MASK bitmaps take
        nImageBytes = NumBitmapBytes(pbmpColor) + NumBitmapBytes(pbmpMask);

        // write the ICONIMAGE to disk (first the BITMAPINFOHEADER)
        ZeroMemory(&biHeader, sizeof(biHeader));

        // Fill in only those fields that are necessary
        biHeader.biSize = sizeof(biHeader);
        biHeader.biWidth = pbmpColor->bmWidth;
        biHeader.biHeight = pbmpColor->bmHeight * 2; // height of color+mono
        biHeader.biPlanes = pbmpColor->bmPlanes;
        biHeader.biBitCount = pbmpColor->bmBitsPixel;
        biHeader.biSizeImage = nImageBytes;

        // write the BITMAPINFOHEADER
        WriteFile(hFile, &biHeader, sizeof(biHeader), &nWritten, 0);

        // write the RGBQUAD color table (for 16 and 256 colour icons)
        if (pbmpColor->bmBitsPixel == 2 || pbmpColor->bmBitsPixel == 8)
        {

        }

        return nWritten;
    }

    static HBITMAP createImageMask(HBITMAP bitmapHandle, const COLORREF transparencyColor) {
        // For getting information about the bitmap's height and width in this context
        BITMAP bitmap;

        // Create the device contexts for the bitmap and its mask
        HDC bitmapGraphicsDeviceContext = CreateCompatibleDC(NULL);
        HDC bitmapMaskGraphicsDeviceContext = CreateCompatibleDC(NULL);

        // For the device contexts to re-select the initial object they initialized with
        // and de-select the bitmap and mask
        HGDIOBJ bitmapDummyObject;
        HGDIOBJ bitmapMaskDummyObject;

        // The actual mask
        HBITMAP bitmapMaskHandle;

        // 1. Generate the mask.
        GetObject(bitmapHandle, sizeof(BITMAP), &bitmap);
        bitmapMaskHandle = CreateBitmap(bitmap.bmWidth, bitmap.bmHeight, 1, 1, NULL);

        // 2. Setup the device context for the mask (and the bitmap)
        //    — also get the initial selected objects in the device contexts.
        bitmapDummyObject = SelectObject(bitmapGraphicsDeviceContext, (HGDIOBJ)(HBITMAP)bitmapHandle);
        bitmapMaskDummyObject = SelectObject(bitmapMaskGraphicsDeviceContext, (HGDIOBJ)(HBITMAP)bitmapMaskHandle);

        // 3. Set the background color of the mask.
        SetBkColor(bitmapGraphicsDeviceContext, transparencyColor);

        // 4. Copy the bitmap to the mask and invert it so it blends with the background color.
        BitBlt(bitmapMaskGraphicsDeviceContext, 0, 0, bitmap.bmWidth, bitmap.bmHeight, bitmapGraphicsDeviceContext, 0, 0, SRCCOPY);
        BitBlt(bitmapGraphicsDeviceContext, 0, 0, bitmap.bmWidth, bitmap.bmHeight, bitmapMaskGraphicsDeviceContext, 0, 0, SRCINVERT);

        // 5. Select the bitmaps out before deleting the device contexts to avoid any issues.
        SelectObject(bitmapGraphicsDeviceContext, bitmapDummyObject);
        SelectObject(bitmapMaskGraphicsDeviceContext, bitmapMaskDummyObject);

        // Clean-up
        DeleteDC(bitmapGraphicsDeviceContext);
        DeleteDC(bitmapMaskGraphicsDeviceContext);

        // Voila!
        return bitmapMaskHandle;
    }

    //
    // Wrapper around GetIconInfo and GetObject(BITMAP)
    //
    static BOOL GetIconBitmapInfo(HICON hIcon, ICONINFO* pIconInfo, BITMAP* pbmpColor, BITMAP* pbmpMask)
    {
        if (!GetIconInfo(hIcon, pIconInfo))
            return FALSE;

        if (!GetObject(pIconInfo->hbmColor, sizeof(BITMAP), pbmpColor))
            return FALSE;

        if (!GetObject(pIconInfo->hbmMask, sizeof(BITMAP), pbmpMask))
            return FALSE;

        return TRUE;
    }

    //
    // Write one icon directory entry - specify the index of the image
    //
    static UINT WriteIconDirectoryEntry(HANDLE hFile, int nIdx, HICON hIcon, UINT nImageOffset)
    {
        ICONINFO iconInfo;
        ICONDIR iconDir;

        BITMAP bmpColor;
        BITMAP bmpMask;

        DWORD nWritten;
        UINT nColorCount;
        UINT nImageBytes;

        GetIconBitmapInfo(hIcon, &iconInfo, &bmpColor, &bmpMask);

        nImageBytes = NumBitmapBytes(&bmpColor) + NumBitmapBytes(&bmpMask);

        if (bmpColor.bmBitsPixel >= 8)
            nColorCount = 0;
        else
            nColorCount = 1 << (bmpColor.bmBitsPixel * bmpColor.bmPlanes);

        // Create the ICONDIR structure
        iconDir.bWidth = (BYTE)bmpColor.bmWidth;
        iconDir.bHeight = (BYTE)bmpColor.bmHeight;
        iconDir.bColorCount = nColorCount;
        iconDir.bReserved = 0;
        iconDir.wPlanes = bmpColor.bmPlanes;
        iconDir.wBitCount = bmpColor.bmBitsPixel;
        iconDir.dwBytesInRes = sizeof(BITMAPINFOHEADER) + nImageBytes;
        iconDir.dwImageOffset = nImageOffset;

        // Write to disk
        WriteFile(hFile, &iconDir, sizeof(iconDir), &nWritten, 0);

        // Free resources
        DeleteObject(iconInfo.hbmColor);
        DeleteObject(iconInfo.hbmMask);

        return nWritten;
    }

    static UINT WriteIconData(HANDLE hFile, HBITMAP hBitmap)
    {
        BITMAP bmp;
        int i;
        BYTE* pIconData;

        UINT nBitmapBytes;
        DWORD nWritten;

        GetObject(hBitmap, sizeof(BITMAP), &bmp);

        nBitmapBytes = NumBitmapBytes(&bmp);

        pIconData = (BYTE*)malloc(nBitmapBytes);

        GetBitmapBits(hBitmap, nBitmapBytes, pIconData);

        // bitmaps are stored inverted (vertically) when on disk..
        // so write out each line in turn, starting at the bottom + working
        // towards the top of the bitmap. Also, the bitmaps are stored in packed
        // in memory - scanlines are NOT 32bit aligned, just 1-after-the-other
        for (i = bmp.bmHeight - 1; i >= 0; i--)
        {
            // Write the bitmap scanline
            WriteFile(
                hFile,
                pIconData + (i * bmp.bmWidthBytes), // calculate offset to the line
                bmp.bmWidthBytes, // 1 line of BYTES
                &nWritten,
                0);

            // extend to a 32bit boundary (in the file) if necessary
            if (bmp.bmWidthBytes & 3)
            {
                DWORD padding = 0;
                WriteFile(hFile, &padding, 4 - bmp.bmWidthBytes, &nWritten, 0);
            }
        }

        free(pIconData);

        return nBitmapBytes;
    }

    //
    // Create a .ICO file, using the specified array of HICON images
    //
    static BOOL SaveIcon3(const TCHAR* szIconFile, const HICON hIcon[], int nNumIcons)
    {
        HANDLE hFile;
        int i;
        int* pImageOffset;

        if (hIcon == 0 || nNumIcons < 1)
            return FALSE;

        // Save icon to disk:
        hFile = CreateFile(szIconFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

        if (hFile == INVALID_HANDLE_VALUE)
            return FALSE;

        //
        // Write the iconheader first of all
        //
        WriteIconHeader(hFile, nNumIcons);

        //
        // Leave space for the IconDir entries
        //
        SetFilePointer(hFile, sizeof(ICONDIR) * nNumIcons, 0, FILE_CURRENT);

        pImageOffset = (int*)malloc(nNumIcons * sizeof(int));

        //
        // Now write the actual icon images!
        //
        for (i = 0; i < nNumIcons; i++)
        {
            ICONINFO iconInfo;
            BITMAP bmpColor, bmpMask;

            GetIconBitmapInfo(hIcon[i], &iconInfo, &bmpColor, &bmpMask);

            // HACK -> Creating a mask ourselves
            iconInfo.hbmMask = createImageMask(iconInfo.hbmColor, RGB(0, 0, 0));
            GetObject(iconInfo.hbmMask, sizeof(BITMAP), &bmpMask);
            
            // record the file-offset of the icon image for when we write the icon directories
            pImageOffset[i] = SetFilePointer(hFile, 0, 0, FILE_CURRENT);

            // bitmapinfoheader + colortable
            WriteIconImageHeader(hFile, &bmpColor, &bmpMask);

            // color and mask bitmaps
            WriteIconData(hFile, iconInfo.hbmColor);
            WriteIconData(hFile, iconInfo.hbmMask);

            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
        }

        //
        // Lastly, skip back and write the icon directories.
        //
        SetFilePointer(hFile, sizeof(ICONHEADER), 0, FILE_BEGIN);

        for (i = 0; i < nNumIcons; i++)
        {
            WriteIconDirectoryEntry(hFile, i, hIcon[i], pImageOffset[i]);
        }

        free(pImageOffset);

        // finished!
        CloseHandle(hFile);

        return TRUE;
    }

public:

    static int saveIcon(const TCHAR* filename, const HICON hIcon) {
        BOOL ret;

        ret = SaveIcon3(filename, &hIcon, 1);
        if (ret) {
            return 0;
        }
        return -1;
    }

};

