/** @file FileParseSummaryDialog.cpp
 * NWScript File Parsing Summary
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include "pch.h"
//#include <Windows.h>
//#include <Commctrl.h>
//#include <sstream>

//#include "PluginMain.h"

#include "PluginControlsRC.h"
#include "FileParseSummaryDialog.h"

#include "PluginDarkMode.h"

using namespace NWScriptPlugin;

intptr_t CALLBACK FileParseSummaryDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			PluginDarkMode::autoSetupWindowAndChildren(_hSelf);

			// Set the font style and colors for File Summary and Keep Results labels
			// Set bold font for warning
			HFONT hFont = ::CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("MS Shell Dlg"));
			::SendMessage(GetDlgItem(_hSelf, IDC_LBLFILESUMARY), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 0);
		
			hFont = ::CreateFont(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("MS Shell Dlg"));
			::SendMessage(GetDlgItem(_hSelf, IDC_LBLKEEPRESULTS), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 0);

			// Set the labels
			::SetDlgItemText(_hSelf, IDC_LBLENGINESTRUCTS, _engineStructuresCount.c_str());
			::SetDlgItemText(_hSelf, IDC_LBLFUNCTIONDEFINITIONS, _functionsDefinitionCount.c_str());
			::SetDlgItemText(_hSelf, IDC_LBLGLOBALCONSTANTS, _constantsCount.c_str());

			_dpiManager.resizeControl(_hSelf);
			_dpiManager.resizeChildren(_hSelf, true);

			setLogo();

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
			case IDCANCEL:
			case IDNO: 
			case IDYES:
				display(false);
				destroy();
			
				if (_okDialogCallback != nullptr)
					_okDialogCallback(static_cast<HRESULT>(wParam));
				return TRUE;
			}
			break;
		}
	}

	// Signals done processing messages
	return FALSE;
}

void FileParseSummaryDialog::doDialog()
{
	// Create from resource
	if (!isCreated())
		create(IDD_PARSERESULTS);

	//Show and centralize
	goToCenter();
}



void FileParseSummaryDialog::setLogo()
{
	// Get best image size based on DPI
	RECT pctRect;
	GetClientRect(GetDlgItem(_hSelf, IDC_PCTNWSCRIPTFILELOGOBOX), &pctRect);
	_hFile = loadPNGFromResource(_hInst, IDB_NWSCRIPTFILEPARSE, pctRect.right, pctRect.bottom);
	::SendMessage(GetDlgItem(_hSelf, IDC_PCTNWSCRIPTFILE), STM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(_hFile));
}

