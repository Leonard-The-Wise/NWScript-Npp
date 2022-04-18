/** @file UsersPreferences.cpp
 * User's preferences dialog
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.



#include "pch.h"
//#include <Windows.h>
//#include <Commctrl.h>
//#include <sstream>

//#include "PluginMain.h"
#include "UsersPreferencesDialog.h"

#include "PluginControlsRC.h"
#include "PluginDarkMode.h"


using namespace NWScriptPlugin;

intptr_t CALLBACK UsersPreferencesDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			_dpiManager.DPIResizeControl(_hSelf);
			_dpiManager.DPIResizeChildren(_hSelf, true);

			PluginDarkMode::autoSetupWindowAndChildren(_hSelf);

			CheckDlgButton(_hSelf, IDC_CHKAUTOOPENDISASSEMBLED, _settings->autoDisplayDisassembled ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(_hSelf, IDC_CHKAUTOOPENDEBUGSYMBOLS, _settings->autoDisplayDebugSymbols ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(_hSelf, IDC_CHKAUTOINSTALLDARKTHEME, _settings->autoInstallDarkTheme ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(_hSelf, IDC_CHKUSEDARKMODE, _settings->legacyDarkModeUse ? BST_CHECKED : BST_UNCHECKED);

			if (!_UseDarkModeEnabledLegacy)
			{
				CheckDlgButton(_hSelf, IDC_CHKUSEDARKMODE, BST_INDETERMINATE);
				EnableWindow(GetDlgItem(_hSelf, IDC_CHKUSEDARKMODE), false);
			}

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
			case IDCANCEL:
			case IDOK:
				if (wParam == IDOK)
				{
					keepSettings();
					if (_UseDarkModeEnabledLegacy)
						_UseDarkModeLegacy(_settings->legacyDarkModeUse);
				}
				display(false);
				destroy();
				return TRUE;
			}
			break;
		}
	}

	// Signals done processing messages
	return FALSE;
}

void UsersPreferencesDialog::doDialog()
{
	// Create from resource
	if (!isCreated())
		create(IDD_USERSPREFERENCES);

	//Show and centralize
	goToCenter();
}


void UsersPreferencesDialog::keepSettings()
{
	_settings->autoDisplayDisassembled = IsDlgButtonChecked(_hSelf, IDC_CHKAUTOOPENDISASSEMBLED);
	_settings->autoDisplayDebugSymbols = IsDlgButtonChecked(_hSelf, IDC_CHKAUTOOPENDEBUGSYMBOLS);
	_settings->autoInstallDarkTheme = IsDlgButtonChecked(_hSelf, IDC_CHKAUTOINSTALLDARKTHEME);

	if (_UseDarkModeEnabledLegacy)
		_settings->legacyDarkModeUse = IsDlgButtonChecked(_hSelf, IDC_CHKUSEDARKMODE);
}

