/** @file WarningDialog.cpp
 * Kind of a easter egg box...
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <Windows.h>
//#include <Commctrl.h>
//#include "Common.h"

#include "PathAccessDialog.h"
#include "PluginControlsRC.h"

using namespace NWScriptPlugin;

INT_PTR CALLBACK PathAccessDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Set bold font for warning
		HFONT hFont = ::CreateFont(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("MS Shell Dlg"));
		::SendMessage(GetDlgItem(_hSelf, IDC_LBLWARNING), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 0);

		// Set the text for accessed files.
		::SetDlgItemText(_hSelf, IDC_LBLWARNING, _sWarning.c_str());
		::SetDlgItemText(_hSelf, IDC_TXTREQUIREDFILES, _sPaths.str().c_str());
		::SetDlgItemText(_hSelf, IDC_LBLSOLUTION, _sSolution.c_str());

		// Set DIP after setting fonts.
		_dpiManager.DPIResizeControl(_hSelf);
		_dpiManager.DPIResizeChildren(_hSelf, true);

		// Set picturebox to desired icon
		int scale = _dpiManager.getDPIScalePercent();
		IconSize shieldSize = IconSize::Size64x64;
		switch (scale)
		{
		case 125:
			shieldSize = IconSize::Size96x96;
			break;
		case 150:
			shieldSize = IconSize::Size128x128;
			break;
		case 175:
			shieldSize = IconSize::Size192x192;
			break;
		case 200:
			shieldSize = IconSize::Size256x256;
			break;
		default:
			shieldSize = (IconSize)_dpiManager.ScaleIconSize(76);
		}
		HBITMAP hBitmap = getStockIconBitmap(_iconID, shieldSize);
		::SendMessage(GetDlgItem(_hSelf, IDC_SHIELDICON), STM_SETIMAGE,
			static_cast<WPARAM>(IMAGE_BITMAP),
			reinterpret_cast<LPARAM>(hBitmap));

		// Set visibility and mode for admin
		if (!_bAdminMode)
		{
			// Hide the Admin button by calling 'Show Window'.. Yeah, Windows API...
			::ShowWindow(GetDlgItem(_hSelf, IDOK), SW_HIDE);
			::SetDlgItemText(_hSelf, IDCANCEL, TEXT("Understood"));
		}
		else
		{
			// Set a shield icon also to the Run as Admin button - to preserve Windows standards
			HBITMAP hShieldSmall = getStockIconBitmap(SHSTOCKICONID::SIID_SHIELD, (IconSize)_dpiManager.ScaleIconSize((UINT)IconSize::Size16x16));
			::SendMessage(GetDlgItem(_hSelf, IDOK), BM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(hShieldSmall));
		}

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

INT_PTR PathAccessDialog::doDialog()
{
	return ShowModal(IDD_PATHACCESSDIALOG);
}

