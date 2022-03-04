/** @file FileParseSummaryDialog.cxx
 * NWScript File Parsing Summary
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.



#include <Windows.h>
#include <Commctrl.h>
#include <sstream>

#include "PluginMain.h"
#include "FileParseSummaryDialog.h"

#include "PluginControlsRC.h"


using namespace NWScriptPlugin;

intptr_t CALLBACK FileParseSummaryDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Sets the dialog icon from code cause Resource Editor won't work transparencies
		HICON NWSCRIPTFILE = reinterpret_cast<HICON>(LoadImage(_hInst, MAKEINTRESOURCE(IDI_NWSCRIPTFILE96),
			IMAGE_ICON, 96, 96, LR_DEFAULTCOLOR));
		::SendMessage(GetDlgItem(_hSelf, IDC_PCTNWSCRIPTFILE), STM_SETIMAGE,
			static_cast<WPARAM>(IMAGE_BITMAP),
			reinterpret_cast<LPARAM>(IconToBitmap(NWSCRIPTFILE)));

		// Set the font style and colors for File Summary and Keep Results labels
		// Set bold font for warning
		HFONT hFont = ::CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("MS Shell Dlg"));
		::SendMessage(GetDlgItem(_hSelf, IDC_LBLFILESUMARY), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 0);
		
		hFont = ::CreateFont(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("MS Shell Dlg"));
		::SendMessage(GetDlgItem(_hSelf, IDC_LBLKEEPRESULTS), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 0);

		// Set the labels
		::SetDlgItemText(_hSelf, IDC_LBLENGINESTRUCTS, _engineStructuresCount.c_str());
		::SetDlgItemText(_hSelf, IDC_LBLFUNCTIONDEFINITIONS, _functionsDefinitionCount.c_str());
		::SetDlgItemText(_hSelf, IDC_LBLGLOBALCONSTANTS, _constantsCount.c_str());

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