/** @file AboutDialog.cpp
 * About Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <Windows.h>
//#include <Commctrl.h>
//#include <sstream>
//
//#include "jpcre2.hpp"

//#include "PluginMain.h"
#include "PluginControlsRC.h"

// Using method to read project version straight from binaries to avoid constant 
// recompilation of this file .
// #include "ProjectVersion.h"
#include "VersionInfoEx.h"

#include "AboutDialog.h"


using namespace NWScriptPlugin;

// Anchoring and size restriction informations

constexpr const RECTSIZER mainWindowSize = { {675, 562 } };
constexpr const RECTSIZER lblPluginName = { { 251, 0 } };
constexpr const RECTSIZER lblVersion = { { 251, 0 } };
constexpr const RECTSIZER lblCopyright = { { 251, 0 } };
constexpr const RECTSIZER lblCredits = { { 251, 114 } };
constexpr const RECTSIZER lnkHomepageSize = { { 490, 0 } };
constexpr const RECTSIZER txtAboutSize = { { 607, 200 } };

BEGIN_ANCHOR_MAP(AboutDialog)
	ANCHOR_MAP_ADDGLOBALSIZERESTRICTION(mainWindowSize)
#ifdef DEBUG_ANCHORLIB
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LBLPLUGINNAME, ANF_TOP | ANF_LEFT | ANF_RIGHT, "Plugin Name Label")
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LBLVERSION, ANF_TOP | ANF_LEFT | ANF_RIGHT, "Label Version")
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LBLCOPYRIGHT, ANF_TOP | ANF_LEFT | ANF_RIGHT, "Label Copyright")
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LBLSPECIALCREDITS, ANF_TOP | ANF_LEFT | ANF_RIGHT, "Label Credits")
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LBLHOMEPAGE, ANF_TOP | ANF_LEFT | ANF_RIGHT, "Label Homepage")
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LNKHOMEPAGE, ANF_TOP | ANF_LEFT | ANF_RIGHT, "Link to Homepage")
	ANCHOR_MAP_ENTRY(_hSelf, IDC_TXTABOUT, ANF_ALL, "Editor About")
	ANCHOR_MAP_ENTRY(_hSelf, IDOK, ANF_BOTTOM, "Ok Button")
#else
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LBLPLUGINNAME, ANF_TOP | ANF_LEFT | ANF_RIGHT)
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LBLVERSION, ANF_TOP | ANF_LEFT | ANF_RIGHT)
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LBLCOPYRIGHT, ANF_TOP | ANF_LEFT | ANF_RIGHT)
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LBLSPECIALCREDITS, ANF_TOP | ANF_LEFT | ANF_RIGHT)
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LBLHOMEPAGE, ANF_TOP)
	ANCHOR_MAP_ENTRY(_hSelf, IDC_LNKHOMEPAGE, ANF_TOP)
	ANCHOR_MAP_ENTRY(_hSelf, IDC_TXTABOUT, ANF_ALL)
	ANCHOR_MAP_ENTRY(_hSelf, IDOK, ANF_BOTTOM)
#endif
	ANCHOR_MAP_ADDSIZERESTRICTION(_hSelf, IDC_LBLPLUGINNAME, lblPluginName)
	ANCHOR_MAP_ADDSIZERESTRICTION(_hSelf, IDC_LBLVERSION, lblVersion)
	ANCHOR_MAP_ADDSIZERESTRICTION(_hSelf, IDC_LBLCOPYRIGHT, lblCopyright)
	ANCHOR_MAP_ADDSIZERESTRICTION(_hSelf, IDC_LBLSPECIALCREDITS, lblCredits)
	ANCHOR_MAP_ADDSIZERESTRICTION(_hSelf, IDC_LNKHOMEPAGE, lnkHomepageSize)
	ANCHOR_MAP_ADDSIZERESTRICTION(_hSelf, IDC_TXTABOUT, txtAboutSize)
END_ANCHOR_MAP(_hSelf)


// Use one of these fonts to display text. Crescent order.
static const generic_string fontFamilies[] = {
	L"Cascadia Code", L"Cascadia Mono", L"Consolas", L"Droid Sans Mono", L"Inconsolata", L"Courier New" L"monospace", L"Monaco", L"Menlo", L"Fixedsys"
};

DWORD CALLBACK EditStreamCallback(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG cb, PLONG pcb)
{
	IStream* pstm = (IStream*)dwCookie;
	DWORD fail = FAILED(pstm->Read(lpBuff, cb, (ULONG*)pcb));
	return fail;
}

void AboutDialog::LoadAboutTextEditor()
{
	// Load resource
	auto hResource = FindResourceW(_hInst, MAKEINTRESOURCE(IDR_ABOUTDOC), L"RTF");
	size_t _size = SizeofResource(_hInst, hResource);
	auto hMemory = LoadResource(_hInst, hResource);
	LPVOID ptr = LockResource(hMemory);

	// Copy image bytes into a real hglobal memory handle
	hMemory = ::GlobalAlloc(GHND, _size);
	if (hMemory)
	{
		void* pBuffer = ::GlobalLock(hMemory);
		memcpy(pBuffer, ptr, _size);
	}

	// Make a raw string of resources
	std::string rawText;
	rawText.assign((char*)ptr, _size);

	// Replace %VARIABLE% strings from raw text
	generic_string rawTextW = replaceStringsW(str2wstr(rawText), _replaceStrings);

	// Free memory allocated for resource
	if (hMemory)
		GlobalFree(hMemory);

	// Now, redo the allocated space with the new size (with replaced texts) and copy modified text again
	hMemory = ::GlobalAlloc(GHND, rawTextW.size());
	if (hMemory)
	{
		void* pBuffer = ::GlobalLock(hMemory);
		memcpy(pBuffer, wstr2str(rawTextW).c_str(), rawTextW.size());
	}

	// Create stream on hMemory
	IStream* pStream = nullptr;
	HRESULT hr = CreateStreamOnHGlobal(hMemory, TRUE, &pStream);
	if (SUCCEEDED(hr))
	{
		// Set paramenters and load default text resource
		EDITSTREAM es = { (DWORD_PTR)pStream, 0, EditStreamCallback };
		HWND editControl = GetDlgItem(_hSelf, IDC_TXTABOUT);
		SendMessage(editControl, EM_SETOLECALLBACK, 0, (LPARAM)&_oleCallback);
		SendMessage(editControl, EM_AUTOURLDETECT, AURL_ENABLEURL, 0);
		SendMessage(editControl, EM_EXLIMITTEXT, 0, -1);
		SendMessage(editControl, EM_SETEVENTMASK, 0, ENM_LINK);
		SendMessage(editControl, EM_STREAMIN, SF_RTF, (LPARAM)&es);
		
		// Retrieve raw buffer from TXTABOUT for replace strings and later use with hyperlinks
		GETTEXTLENGTHEX tl = { GTL_NUMCHARS, 1200 };
		_aboutText.resize(SendMessage(editControl, EM_GETTEXTLENGTHEX, (WPARAM)&tl, 0) + 1);
		GETTEXTEX tex = { (DWORD)_aboutText.size() * sizeof(TCHAR), GT_RAWTEXT, 1200, NULL, NULL};
		SendMessage(editControl, EM_GETTEXTEX, (WPARAM)&tex, (LPARAM)_aboutText.data());

	}

	// Free memory allocated for resource again
	if (hMemory)
		GlobalFree(hMemory);

}

intptr_t CALLBACK AboutDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			// Get version from module's binary file
			VersionInfoEx versionInfo = VersionInfoEx::getLocalVersion();
			generic_stringstream sVersion = {};
			sVersion << "Version " << versionInfo.shortString().c_str() << " (build " << versionInfo.build() << ")";

			// Set user fonts. Try to keep same font for all.
			HFONT hTitleFont = NULL;
			size_t fontIndex = 0;
			while (fontIndex < std::size(fontFamilies) && !hTitleFont)
			{
				hTitleFont = ::CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, fontFamilies[fontIndex].c_str());
				if (!hTitleFont)
					fontIndex++;
			}

			HFONT hHomepageFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, fontFamilies[fontIndex].c_str());

			if (hTitleFont)
				::SendMessage(GetDlgItem(_hSelf, IDC_LBLPLUGINNAME), WM_SETFONT, reinterpret_cast<WPARAM>(hTitleFont), 0);
			if (hHomepageFont)
				::SendMessage(GetDlgItem(_hSelf, IDC_LBLHOMEPAGE), WM_SETFONT, reinterpret_cast<WPARAM>(hHomepageFont), 0);

			::SetDlgItemText(_hSelf, IDC_LBLVERSION, reinterpret_cast<LPCWSTR>(sVersion.str().c_str()));
			::SetDlgItemText(_hSelf, IDC_LNKHOMEPAGE, (TEXT("<a href=\"") + _homePath + TEXT("\">") + _homePath + TEXT("</a>")).c_str());

			LoadAboutTextEditor();
			//::SetDlgItemText(_hSelf, IDC_TXTABOUT, replaceStringsW(ABOUT_TEXT, _replaceStrings).c_str());

			HBITMAP hLogo = loadPNGFromResource(_hInst, IDB_NWSCRIPTLOGO);
			::SendMessage(GetDlgItem(_hSelf, IDC_PCTLOGO), STM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(hLogo));

			HICON hAbout = loadPNGFromResourceIcon(_hInst, IDI_ABOUTDIALOGWINDOWICO);
			::SendMessage(_hSelf, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hAbout));

			InitAnchors();

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
			case IDCANCEL:
			case IDOK:
				display(false);
				destroy();
				return TRUE;
			}
		}

		case WM_SIZE:
			ANCHOR_MAP_HANDLESIZERS();

		case WM_GETMINMAXINFO:
			ANCHOR_MAP_HANDLERESTRICTORS(wParam, lParam);

		case WM_NOTIFY:
		{
			switch (wParam)
			{
				case IDC_LNKHOMEPAGE:
				{
					NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
					switch (nmhdr->code)
					{
						case NM_CLICK:
						case NM_RETURN:
							PNMLINK pNMLink = (PNMLINK)lParam;
							LITEM link = pNMLink->item;

							ShellExecute(NULL, L"open", link.szUrl, NULL, NULL, SW_SHOW);
							return TRUE;
					}
					break;
				}

				case IDC_TXTABOUT:
				{
					NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
					switch (nmhdr->code)
					{
						case EN_LINK:
						{
							ENLINK* enLinkInfo = (ENLINK*)lParam;
							if (enLinkInfo->msg == WM_SETCURSOR)
							{
								SetCursor(LoadCursor(NULL, IDC_HAND));
							}

							if (enLinkInfo->msg == WM_LBUTTONUP)
							{
								LaunchHyperlink(*enLinkInfo);
							}

							return TRUE;
						}
					}
					break;
				}
			}
		}
	}

	// Signals done processing messages
	return FALSE;
}

void AboutDialog::LaunchHyperlink(const ENLINK& link)
{
	// Get text range from raw string
	generic_string url = _aboutText.substr(link.chrg.cpMin, link.chrg.cpMax - link.chrg.cpMin);
	ShellExecute(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOW);
}

void AboutDialog::doDialog()
{
	// Create from resource
	if (!isCreated())
		create(IDD_ABOUT);

	//Show and centralize
	goToCenter();
}


