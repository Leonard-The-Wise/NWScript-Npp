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

#include "PluginDarkMode.h"

using namespace NWScriptPlugin;

INT_PTR CALLBACK PathAccessDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{

			PluginDarkMode::autoSetupWindowAndChildren(_hSelf);

			// Set bold font for warning
			HFONT hFont = ::CreateFont(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("MS Shell Dlg"));
			::SendMessage(GetDlgItem(_hSelf, IDC_LBLWARNING), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 0);

			// Set the text for accessed files.
			::SetDlgItemText(_hSelf, IDC_LBLWARNING, _sWarning.c_str());
			::SetDlgItemText(_hSelf, IDC_TXTREQUIREDFILES, _sPaths.str().c_str());
			::SetDlgItemText(_hSelf, IDC_LBLSOLUTION, _sSolution.c_str());

			// Set DIP after setting fonts.
			_dpiManager.resizeControl(_hSelf);
			_dpiManager.resizeChildren(_hSelf, true);

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
				shieldSize = (IconSize)_dpiManager.scaleIconSize(76);
			}
			_hShield = getStockIconBitmap(_iconID, shieldSize);
			::SendMessage(GetDlgItem(_hSelf, IDC_SHIELDICON), STM_SETIMAGE,
				static_cast<WPARAM>(IMAGE_BITMAP),
				reinterpret_cast<LPARAM>(_hShield));

			// Set visibility and mode for admin
			if (!_bAdminMode)
			{
				::ShowWindow(GetDlgItem(_hSelf, IDOK), SW_HIDE);
				::SetDlgItemText(_hSelf, IDCANCEL, TEXT("Underst&ood"));
			}
			else
			{
				// Set a shield icon also to the Run as Admin button - to preserve Windows standards
				HICON _hShieldSmall = getStockIcon(SHSTOCKICONID::SIID_SHIELD, (IconSize)_dpiManager.scaleIconSize((UINT)IconSize::Size16x16));
				::SendMessage(GetDlgItem(_hSelf, IDOK), BM_SETIMAGE, static_cast<WPARAM>(IMAGE_ICON), reinterpret_cast<LPARAM>(_hShieldSmall));
			}

			// Window icon
			_hWindowIcon = loadSVGFromResourceIcon(_hInst, IDI_APPLICATIONACCESS, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16));
			::SendMessage(_hSelf, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(_hWindowIcon));

			ShowWindow(_hSelf, SW_NORMAL);

			if (_morphToCopy)
				MorphToPluginCopyMode();

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
			break;
		}
	}

	// Signals done processing messages
	return FALSE;
}

INT_PTR PathAccessDialog::doDialog()
{
	return ShowModal(IDD_PATHACCESSDIALOG);
}

void PathAccessDialog::MorphToPluginCopyMode()
{
	// Change shield logo to plugin logo
	DeleteObject(_hShield);

	// Reposition window
	RECT rcPctBox;
	GetWindowRect(GetDlgItem(_hSelf, IDC_PCTFILEACCESSLOGOBOX), &rcPctBox);
	_dpiManager.screenToClientEx(_hSelf, &rcPctBox);
	MoveWindow(GetDlgItem(_hSelf, IDC_SHIELDICON), rcPctBox.left, rcPctBox.top, rcPctBox.right - rcPctBox.left, 
		rcPctBox.bottom - rcPctBox.top, true);

	RECT pctRect;
	GetClientRect(GetDlgItem(_hSelf, IDC_PCTFILEACCESSLOGOBOX), &pctRect);
	_hShield = loadPNGFromResource(_hInst, IDB_NWSCRIPTLOGO_SIMPLIFIED, pctRect.right, pctRect.bottom);
	::SendMessage(GetDlgItem(_hSelf, IDC_SHIELDICON), STM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(_hShield));

	// Change labels
	SetWindowText(_hSelf, TEXT("Requesting permission to install additional files"));
	::SetDlgItemText(_hSelf, IDC_LBLWARNING, TEXT("The NWScript Tools plugin requires some additional files to unlock it's full potential."));
	::SetDlgItemText(_hSelf, IDC_LBLSOLUTION, TEXT("One or more of the files listed above requires elevated privileges to be written to.\r\n\
- If you wish, we can write them for you (will restart Notepad++ as an Administrator); or\r\n\
- You may manually copy the above files by yourself. See the help text on 'About Me' option for more info on where those are located."));

	::SetDlgItemText(_hSelf, IDOK, TEXT("  &Yes, please, copy them for me"));
	::SetDlgItemText(_hSelf, IDCANCEL, TEXT("I'd rather do it ma&nually"));
}