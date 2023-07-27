/** @file WarningDialog.cpp
 * Kind of a easter egg box...
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <Windows.h>
//#include <Commctrl.h>
//#include <sstream>

//#include "PluginMain.h"
#include "WarningDialog.h"

#include "PluginControlsRC.h"
#include "PluginDarkMode.h"


using namespace NWScriptPlugin;

intptr_t CALLBACK WarningDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			PluginDarkMode::autoSetupWindowAndChildren(_hSelf);

			// Warning dialog background
			_hWarning = loadPNGFromResource(_hInst, IDB_HEREBEDRAGONS);
			::SendMessage(GetDlgItem(_hSelf, IDC_HEREBEDRAGONS), STM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(_hWarning));

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
			//case IDCANCEL: disabled
				// Check settings and fall back to IDOK
			case IDC_CHKOK:
				_settings->autoIndentationWarningAccepted = true;
				[[fallthrough]];
			case IDOK:
				display(false);
				destroy();
				return TRUE;
			}
		}
	}

	// Signals done processing messages
	return FALSE;
}

void WarningDialog::doDialog()
{
	// Create from resource
	if (!isCreated())
		create(IDD_HEREBEDRAGONS);

	//Show and centralize
	goToCenter();
}