/** @file WarningDialog.cxx
 * Kind of a easter egg box...
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include <Windows.h>
#include <Commctrl.h>
#include "Common.h"

#include "ElevateDialog.h"

#include "PluginControlsRC.h"


INT_PTR CALLBACK ElevateDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		HBITMAP hBitmap = GetStockIconBitmap(SHSTOCKICONID::SIID_SHIELD, IconSize::Size64x64);
		// Set picturebox to System Shield icon
		::SendMessage(GetDlgItem(_hSelf, IDC_SHIELDICON), STM_SETIMAGE,
			static_cast<WPARAM>(IMAGE_BITMAP),
			reinterpret_cast<LPARAM>(hBitmap));

		// Set bold font for warning
		HFONT hFont = ::CreateFont(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("MS Shell Dlg"));
		::SendMessage(GetDlgItem(_hSelf, IDC_LBLWARNING), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 0);

		// Set the text for accessed files.
		::SetDlgItemText(_hSelf, IDC_TXTREQUIREDFILES, _sFiles.str().c_str());

		return TRUE;
	}

	case WM_COMMAND:
	{
		switch (wParam)
		{
			// User is going to set preferences manually; 
		case IDCANCEL:
			// Run Notepad++ in administrator mode
		case IDOK:
			EndDialog(_hSelf, wParam);
			break;
		}
	}
	}

	// Signals done processing messages
	return FALSE;
}

INT_PTR ElevateDialog::doDialog(HINSTANCE hInst, HWND hParent)
{
	return ShowModal(hInst, hParent, IDD_REQUIREPRIVILEGE);
}

