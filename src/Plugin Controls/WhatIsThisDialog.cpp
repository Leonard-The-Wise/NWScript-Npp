/** @file ProcessFilesDialog.cpp
 * Process Files dialog box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <Windows.h>
//#include <Commctrl.h>
//#include <sstream>

#include "PluginMain.h"
#include "WhatIsThisDialog.h"

#include "PluginControlsRC.h"
#include "PluginDarkMode.h"


using namespace NWScriptPlugin;

BEGIN_ANCHOR_MAP(WhatIsThisDialog)
	ANCHOR_MAP_ADDGLOBALSIZERESTRICTION(mainWindowSize)
	ANCHOR_MAP_ENTRY(_hSelf, IDC_TXTHELP, ANF_ALL)
	ANCHOR_MAP_ENTRY(_hSelf, IDOK, ANF_BOTTOM)
	ANCHOR_MAP_ADDSIZERESTRICTION(_hSelf, IDC_TXTHELP, txtHelpSize)
END_ANCHOR_MAP(_hSelf)



DWORD CALLBACK EditStreamCallback2(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG cb, PLONG pcb)
{
	IStream* pstm = (IStream*)dwCookie;
	DWORD fail = FAILED(pstm->Read(lpBuff, cb, (ULONG*)pcb));
	return fail;
}

void WhatIsThisDialog::LoadHelpTextEditor(int resourceID)
{
	_currentDocumentID = resourceID;

	// Load resource
	auto hResource = FindResourceW(_hInst, MAKEINTRESOURCE(_currentDocumentID), L"RTF");
	HGLOBAL hMemory = NULL;
	LPVOID ptr = NULL;
	size_t _size = 0;

	if (hResource)
	{
		_size = SizeofResource(_hInst, hResource);
		hMemory = LoadResource(_hInst, hResource);

		if (hMemory)
			ptr = LockResource(hMemory);
	}

	// Copy image bytes into a real hglobal memory handle
	hMemory = ::GlobalAlloc(GHND, _size);
	if (hMemory)
	{
		void* pBuffer = ::GlobalLock(hMemory);
		if (pBuffer && ptr)
			memcpy(pBuffer, ptr, _size);
	}

	// Make a raw string of resources
	std::string rawText;

	if (ptr)
		rawText.assign((char*)ptr, _size);

	// HACK: change Bk Color of dark mode without messages
	std::map<generic_string, generic_string> replaceStrings;
	replaceStrings.insert({ L"4210752", std::to_wstring(PluginDarkMode::getSofterBackgroundColor()) });
	generic_string rawTextW = replaceStringsW(str2wstr(rawText), replaceStrings);

	// Free memory allocated for resource
	if (hMemory)
		GlobalFree(hMemory);

	// Now, redo the allocated space with the new size (with replaced texts) and copy modified text again
	hMemory = ::GlobalAlloc(GHND, rawTextW.size());
	if (hMemory)
	{
		void* pBuffer = ::GlobalLock(hMemory);
		if (pBuffer)
			memcpy(pBuffer, wstr2str(rawTextW).c_str(), rawTextW.size());
	}

	// Create stream on hMemory
	IStream* pStream = nullptr;
	HRESULT hr = CreateStreamOnHGlobal(hMemory, TRUE, &pStream);
	if (SUCCEEDED(hr))
	{
		// Set paramenters
		EDITSTREAM es = { (DWORD_PTR)pStream, 0, EditStreamCallback2 };
		HWND editControl = GetDlgItem(_hSelf, IDC_TXTHELP);
		SendMessage(editControl, EM_EXLIMITTEXT, 0, -1);
		SendMessage(editControl, EM_SETEVENTMASK, 0, ENM_LINK);
		//SendMessage(editControl, EM_SETOLECALLBACK, 0, reinterpret_cast<LPARAM>(&_whatIsThisOleCallback));

		// Load Document 
		SendMessage(editControl, EM_SETREADONLY, 0, 0); // Set readonly off or images may not load properly
		SendMessage(editControl, EM_STREAMIN, SF_RTF, reinterpret_cast<LPARAM>(&es));
		SendMessage(editControl, EM_SETREADONLY, 1, 0);

		// Retrieve raw buffer from TXTABOUT for replace strings and later use with hyperlink clicks
		GETTEXTLENGTHEX tl = { GTL_NUMCHARS, 1200 };
		_helpText.resize(SendMessage(editControl, EM_GETTEXTLENGTHEX, reinterpret_cast<WPARAM>(&tl), 0) + 1);
		GETTEXTEX tex = { (DWORD)_helpText.size() * sizeof(TCHAR), GT_RAWTEXT, 1200, NULL, NULL };
		SendMessage(editControl, EM_GETTEXTEX, reinterpret_cast<LPARAM>(&tex), reinterpret_cast<LPARAM>(_helpText.data()));

		if (PluginDarkMode::isEnabled())
			SendMessage(editControl, EM_SETBKGNDCOLOR, 0, PluginDarkMode::getSofterBackgroundColor());
	}

	// Free memory allocated for resource again
	if (hMemory)
		GlobalFree(hMemory);

}

intptr_t CALLBACK WhatIsThisDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			_dpiManager.resizeControl(_hSelf);
			_dpiManager.resizeChildren(_hSelf, true);

			mainWindowSize = { {_dpiManager.scaleX(537), _dpiManager.scaleY(356)} };
			txtHelpSize = { { _dpiManager.scaleX(500), _dpiManager.scaleY(257) } };

			// Make window zoomable
			LONG extStyle = GetWindowLong(GetDlgItem(_hSelf, IDC_TXTHELP), GWL_EXSTYLE);
			extStyle |= ES_EX_ZOOMABLE;
			SetWindowLong(GetDlgItem(_hSelf, IDC_TXTHELP), GWL_EXSTYLE, extStyle);

			PluginDarkMode::autoSetupWindowAndChildren(_hSelf);

			LoadHelpTextEditor(PluginDarkMode::isEnabled() ? IDR_COMPILERENGINEDARK : IDR_COMPILERENGINE);

			// Setup control anchors
			InitAnchors();

			// Load PNG image for window logo
			HICON hAbout = loadSVGFromResourceIcon(_hInst, IDI_HELPTABLECONTENTS, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16));
			::SendMessage(_hSelf, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hAbout));

			return TRUE;
		}

		/*
		case WM_KEYDOWN:
		{
			WORD vkCode = LOWORD(wParam);                                 // virtual-key code
			WORD keyFlags = HIWORD(lParam);

			switch (vkCode)
			{
			case VK_RETURN:
			case VK_ESCAPE:
				EndDialog(_hSelf, wParam);
				return TRUE;
			}
		}
		*/

		case WM_COMMAND:
		{
			switch (wParam)
			{
			case IDOK:
			case IDCANCEL:
				EndDialog(_hSelf, wParam);
				break;
			}
			break;
		}

		case WM_SIZE:
			ANCHOR_MAP_HANDLESIZERS();

		case WM_GETMINMAXINFO:
			ANCHOR_MAP_HANDLERESTRICTORS(wParam, lParam);

		case WM_NOTIFY:
		{
			switch (wParam)
			{

			case IDC_TXTHELP:
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
			break;
		}


	}

	// Signals done processing messages
	return FALSE;
}

void WhatIsThisDialog::LaunchHyperlink(const ENLINK& link)
{
	// Get text range from raw string
	generic_string url = _helpText.substr(link.chrg.cpMin, link.chrg.cpMax - link.chrg.cpMin);
	ShellExecute(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOW);
}


INT_PTR WhatIsThisDialog::doDialog()
{
	return ShowModal(IDD_WHATISTHIS);
}
